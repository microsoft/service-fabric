// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#define VERBOSE 1

#include "KtlLogShimKernel.h"

#include <bldver.h>

OverlayGlobalContext::OverlayGlobalContext(
    __in KActivityId ActivityId
    ) :
    KAsyncGlobalContext(ActivityId)
{
}

OverlayGlobalContext::OverlayGlobalContext()
{
}

OverlayGlobalContext::~OverlayGlobalContext()
{
}

NTSTATUS OverlayGlobalContext::Create(
    __in OverlayStream& OS,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out OverlayGlobalContext::SPtr& GlobalContext
    )
{
    KActivityId activityId = KLoggerUtilities::ActivityIdFromStreamId(OS.GetStreamId());
    OverlayGlobalContext::SPtr globalContext;

    GlobalContext = nullptr;
    
    globalContext = _new(AllocationTag, Allocator) OverlayGlobalContext(activityId);
    if (!globalContext)
    {
        KTraceOutOfMemory(activityId, STATUS_INSUFFICIENT_RESOURCES, NULL, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    if (!NT_SUCCESS(globalContext->Status()))
    {
        return(globalContext->Status());
    }

    GlobalContext = Ktl::Move(globalContext);
    return(STATUS_SUCCESS);
}


const ULONG OverlayStream::_ThrottleLinkageOffset = FIELD_OFFSET(OverlayStream::AsyncDestagingWriteContextOverlay,
                                                                 _ThrottleListEntry);

OverlayStream::OverlayStream(
    __in OverlayLog& OverlayLog,                             
    __in RvdLogManager& BaseLogManager,
    __in RvdLog& BaseSharedLog,
    __in const KtlLogStreamId& StreamId,
    __in const KGuid& DiskId,
    __in_opt KString const * const Path,
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in ULONG StreamMetadataSize,
    __in ThrottledKIoBufferAllocator &ThrottledAllocator
    ) :
       _OutstandingDedicatedWriteTable(OverlayStream::AsyncDestagingWriteContextOverlay::GetDedicatedLinksOffset(),
                                       KNodeTable<AsyncDestagingWriteContextOverlay>::CompareFunction(
                                                      &OverlayStream::AsyncDestagingWriteContextOverlay::Comparator)),   
       _OutstandingSharedWriteTable(OverlayStream::AsyncDestagingWriteContextOverlay::GetSharedLinksOffset(),
                                       KNodeTable<AsyncDestagingWriteContextOverlay>::CompareFunction(
                                                      &OverlayStream::AsyncDestagingWriteContextOverlay::Comparator)),   
       _OverlayLog(&OverlayLog),
       _BaseLogManager(&BaseLogManager),
       _BaseSharedLog(&BaseSharedLog),
       _StreamId(StreamId),
       _DiskId(DiskId),
       _Path(Path),
       _FailureStatus(STATUS_SUCCESS),
       _MaxRecordSize(MaxRecordSize),
       _StreamSize(StreamSize),
       _SharedLogQuota(StreamSize),
       _StreamMetadataSize(StreamMetadataSize),
       _StreamUsers(KtlLogStreamKernel::OverlayLinkageOffset),
       _WriteOnlyToDedicated(FALSE),
       _IsStreamForLogicalLog(FALSE),
#if !defined(PLATFORM_UNIX)
       _PerfCounterSetInstance(nullptr),
#endif
       _SharedWriteBytesOutstanding(0),
       _DedicatedWriteBytesOutstanding(0),
       _DedicatedWriteBytesOutstandingThreshold(KLogicalLogInformation::WriteThrottleThresholdNoLimit),
       _ThrottledWritesList(_ThrottleLinkageOffset),
       _ThrottledAllocator(&ThrottledAllocator),
       _DisableCoalescing(FALSE),
       _PeriodicFlushTimeInSec(KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSec),
       _PeriodicTimerIntervalInSec(KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSec),
#if defined(UDRIVER) || defined(UPASSTHROUGH)
       _PeriodicTimerTestDelayInSec(0),
#endif
       _StreamAllocation(0),
       _OpenServiceFSMCallout(nullptr),
       _SharedTruncationEvent(FALSE, FALSE)  // Auto reset
#if DBG
       ,_SharedTimerDelay(0),
       _DedicatedTimerDelay(0)
#endif
{
    NTSTATUS status;

    _ObjectState = Closed;
    _DedicatedContainerId = *((KtlLogContainerId*)(&StreamId));

    //
    // Establish a global context for the stream. All asyncs should use this so traces can include the
    // activity id.
    //
    KAsyncGlobalContext::SPtr globalContext;
    status = OverlayGlobalContext::Create(*this, GetThisAllocator(), GetThisAllocationTag(), _GlobalContext);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    globalContext = up_cast<KAsyncGlobalContext, OverlayGlobalContext>(_GlobalContext);
    SetGlobalContext(globalContext);

    //
    // Allocate a waiter for the base container close event
    //
    status = _DedicatedLogShutdownEvent.CreateWaitContext(GetThisAllocationTag(),
                                                          GetThisAllocator(),
                                                          _DedicatedLogShutdownWait);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return; 
    }

    OverlayManager::SPtr overlayManager = _OverlayLog->GetOverlayManager();
    if (overlayManager)
    {
        status = overlayManager->CreateLogStreamOpenGateContext(_OpenGateContext);
        if (! NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return; 
        }
        
        //
        // If DiskId not specified then map path to it
        //
        GUID nullGuid = { 0 };
        if ((_Path) && (_DiskId == nullGuid))
        {
            status = overlayManager->MapPathToDiskId(*_Path,
                                                     _DiskId);
            if (! NT_SUCCESS(status))
            {
                SetConstructorStatus(status);
                return; 
            }
        }           

        //
        // Establish periodic time values
        //
        overlayManager->GetPeriodicTimerSettings(_PeriodicFlushTimeInSec, _PeriodicTimerIntervalInSec);
    } else {
        _OpenGateContext = nullptr;
    }
        
    RtlZeroMemory(&_RecoveredLogicalLogHeader, sizeof(_RecoveredLogicalLogHeader));
}

OverlayStream::~OverlayStream()
{
    KInvariant(_OutstandingSharedWriteTable.Count() == 0);
    KInvariant(_OutstandingDedicatedWriteTable.Count() == 0);
#if !defined(PLATFORM_UNIX)
    KInvariant(! _PerfCounterSetInstance);
#endif
}

//
// WrappedServiceBase
//
NTSTATUS
OverlayStream::CreateUnderlyingService(
    __out WrappedServiceBase::SPtr& Context,    
    __in ServiceWrapper& FreeService,
    __in ServiceWrapper::Operation& OperationOpen,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    OverlayStreamFreeService* os;
    OverlayStreamFreeService::OperationOpen* op;
    OverlayStream::SPtr context;

    Context = nullptr;
    
    os = (OverlayStreamFreeService*)&FreeService;
    op = (OverlayStreamFreeService::OperationOpen*)&OperationOpen;
    
    context = _new(AllocationTag, Allocator) OverlayStream(*op->GetOverlayLog(),
                                                           *op->GetBaseLogManager(),
                                                           *op->GetBaseSharedLog(),
                                                           os->GetStreamId(),
                                                           os->GetDiskId(),
                                                           os->GetPath().RawPtr(),
                                                           os->GetMaxRecordSize(),
                                                           os->GetStreamSize(),
                                                           os->GetStreamMetadataSize(),
                                                           *op->GetOverlayLog()->GetThrottledAllocator());

    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( KLoggerUtilities::ActivityIdFromStreamId(os->GetStreamId()), status, context.RawPtr(), AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);     
}

BOOLEAN OverlayStream::IsUnderPressureToFlush()
{
    BOOLEAN isUnderMemoryPressure, isUnderSharedLogPressure;

    isUnderMemoryPressure = _ThrottledAllocator->IsUnderMemoryPressure();
    isUnderSharedLogPressure = ((_LastSharedLogStatus == STATUS_LOG_FULL) ||                                
                                (GetOverlayLog()->WaitingThrottledCount() > 0));

    return(isUnderSharedLogPressure || isUnderMemoryPressure);
}

VOID OverlayStream::DeleteTailTruncatedRecords(
    __in BOOLEAN ForSharedLog,
    __in RvdLogAsn RecordAsn
    )
{
    ULONGLONG version;
    RvdLogStream::RecordDisposition disposition;
    RvdLogAsn asn; 
    NTSTATUS status, statusDontCare;
    RvdLogStream::SPtr logStream;

    if (ForSharedLog)
    {
        logStream = _SharedLogStream;
    } else {
        logStream = _DedicatedLogStream;
    }
    
    asn = RecordAsn;
    do
    {
        status = QueryRecordAndVerifyCompleted(ForSharedLog,
                                               asn,
                                               RvdLogStream::AsyncReadContext::ReadNextRecord,
                                               &version,
                                               &disposition,
                                               NULL,
                                               NULL);
        
        if ((NT_SUCCESS(status)) && (disposition != RvdLogStream::RecordDisposition::eDispositionNone))
        {
            statusDontCare = logStream->DeleteRecord(asn, version);
        }
    } while ((disposition != RvdLogStream::RecordDisposition::eDispositionNone));
}

NTSTATUS OverlayStream::DetermineStreamTailAsn(
    __out KtlLogAsn& StreamTailAsn
    )
{
    NTSTATUS status;

    StreamTailAsn = KtlLogAsn::Null();
    
    //
    // This routine assumes that it is called after all records have
    // been moved from shared container into the dedicated container and no
    // writes are in progress. Because of the latter assumption we can count on
    // any disposition returned from QueryRecord() to be valid.
    //
    KInvariant(_State == RecoverTailTruncationAsn);
    
    KArray<RvdLogStream::RecordMetadata> dedicatedRecords(GetThisAllocator(), 0);

    status = _DedicatedLogStream->QueryRecords(KtlLogAsn::Min(),
                                               KtlLogAsn::Max(),
                                               dedicatedRecords);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(GetActivityId(), STATUS_INSUFFICIENT_RESOURCES, this, 0, 0);
        return(status);
    }

    ULONGLONG highestVersion = 0;
    
    for (ULONG i = 0; i < dedicatedRecords.Count(); i++)
    {
        if (dedicatedRecords[i].Disposition == eDispositionPersisted)
        {
            if (dedicatedRecords[i].Version > highestVersion)
            {
                highestVersion = dedicatedRecords[i].Version;
                StreamTailAsn = dedicatedRecords[i].Asn;
            } else {
                //
                // If the ASN is higher but the version is lower then the record has been truncated at the tail. In 
                // this case it should be deleted.
                //
                status = _DedicatedLogStream->DeleteRecord(dedicatedRecords[i].Asn, dedicatedRecords[i].Version);
                KInvariant(NT_SUCCESS(status));
            }
        }
    }

    return(STATUS_SUCCESS);
}

VOID
OverlayStream::DoCompleteOpen(
    __in NTSTATUS Status
    )
{
    _OpenDedicatedLog = nullptr;
    _OpenStream = nullptr;
    _DeleteStream = nullptr;
    
    if (! NT_SUCCESS(Status))
    {
        //
        // Cleanup. Note that so long as opening the Dedicated MBInfo
        // is the last operation there is no need to check for it and
        // close it on error exit. 
        //

        _ThrottledAllocator->RemoveFromLimit(_StreamAllocation);
        _StreamAllocation = 0;
        
        _SharedLogStream = nullptr;
        _DedicatedLogStream = nullptr;
        if (_DedicatedLogContainer)
        {
            _DedicatedLogContainer->SetShutdownEvent(NULL);
            _DedicatedLogContainer = nullptr;
        }
        
#if !defined(PLATFORM_UNIX)
        if (_PerfCounterSetInstance)
        {
            _PerfCounterSetInstance->Stop();
        }
        _PerfCounterSetInstance = nullptr;
#endif
        _CoalesceRecords = nullptr;
    } else {
        _ObjectState = Opened;
    }

    _CopySharedToDedicated = nullptr;
    _CopySharedToBackup = nullptr;

    _TailReadContext = nullptr;
    _LastReadContext = nullptr;
    _TailMetadataBuffer = nullptr;
    _TailIoBuffer = nullptr;
    
    if ((_OpenGateContext) && (_GateAcquired))
    {
        _OpenGateContext->ReleaseGate(*this);
    }

    CompleteOpen(Status);
}

VOID
OverlayStream::OpenServiceFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::OpenServiceCompletion);

    KDbgCheckpointWDataInformational(GetActivityId(),
                        "OverlayStream::OpenServiceFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);
            
    if (! NT_SUCCESS(Status))
    {        
        if (_State == WaitForDedicatedContainerClose)
        {
            //
            // Even if the wait for container close failed, we still want to complete with
            // the original status that caused failure
            //
            DoCompleteOpen(_FinalStatus);
            return;
        }
        
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        KDbgPrintfInformational("Failed to open log LogId %08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x \n", 
            _StreamId.Get().Data1, 
            _StreamId.Get().Data2, 
            _StreamId.Get().Data3, 
            _StreamId.Get().Data4[0],
            _StreamId.Get().Data4[1],
            _StreamId.Get().Data4[2],
            _StreamId.Get().Data4[3],
            _StreamId.Get().Data4[4],
            _StreamId.Get().Data4[5],
            _StreamId.Get().Data4[6],
            _StreamId.Get().Data4[7]);

        if ((_State == OpenDedicatedContainer) || (_State == OpenDedicatedStream))
        {
            //
            // If opening the dedicated container or dedicated stream
            // failed with specific error codes then we want to cleanup
            // the streams resources
            //
            _FinalStatus = Status;
            
            if ((Status == STATUS_OBJECT_NAME_NOT_FOUND) ||
                (Status == STATUS_OBJECT_PATH_NOT_FOUND))
            {
                AdvanceState(CleanupStreamResources);
                goto StateMachine;
            } else if (Status == K_STATUS_LOG_STRUCTURE_FAULT) {
                AdvanceState(CopySharedLogDataToBackup);
                goto StateMachine;
            }
        }

        if ((_State == CopySharedLogDataToBackup) || (_State == CleanupStreamResources))
        {
            //
            // If we failed trying to cleanup the stream resources then
            // just try to close log and move on
            //
            AdvanceState(CloseDedicatedContainer);
            goto StateMachine;
        }

        if (_State > OpenDedicatedContainer)
        {
            //
            // If the state machine has proceeded past the dedicated container being opened then
            // we need to await the dedicated container closing
            //
            AdvanceState(CloseDedicatedContainer);
            _FinalStatus = Status;
            goto StateMachine;
        }
        
        
        //
        // Anything else, just fail it
        //
        DoCompleteOpen(Status);
        return;
    }

StateMachine:
    switch (_State)
    {
        case OpenInitial:
        {
            //
            // Flag this service for deferred close behavior to ensure
            // that all operations against the container are drained
            // before close process starts
            //
            _GateAcquired = FALSE;

            _DedicatedLogContainer = nullptr;
            
            KInvariant(_ObjectState == Closed);
            
            SetDeferredCloseBehavior();

            _StreamAllocation = 0;
            
            _SharedTruncationAsn = RvdLogAsn::Null();
            _SharedTruncationVersion = 0;

            //
            // Always set this so that the first writes will be sent to
            // the dedicated. If for some reason the dedicated is in a
            // failed state then we want to fail the write back to the
            // caller immediately. Otherwise we may have stranded stuff
            // in the shared log since destaging will fail.
            //
            _IsNextRecordFirstWrite = TRUE;
            
            //
            // First thing is to allocate the resources needed
            //
            AdvanceState(AllocateResources);

            Status = OverlayStream::AsyncDestagingWriteContextOverlay::Create(_WriteTableLookupKey,
                                                                              GetThisAllocator(),
                                                                              GetThisAllocationTag(),
                                                                              GetActivityId());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteOpen(Status);
                return;
            }
            
            Status = _BaseLogManager->CreateAsyncOpenLogContext(_OpenDedicatedLog);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteOpen(Status);
                return;
            }

            Status = _BaseSharedLog->CreateAsyncOpenLogStreamContext(_OpenStream);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteOpen(Status);
                return;
            }

            //
            // Create perfcounter set and grab dedicated log backlog
            // threshold
            //
            _SharedWriteBytesOutstanding = 0;
            _DedicatedWriteBytesOutstanding = 0;

            _SharedWriteLatencyTime = 0;
            _SharedWriteLatencyTimeIntervals = 0;
            _SharedWriteLatencyTimeAverage = 0;         

            _DedicatedWriteLatencyTime = 0;
            _DedicatedWriteLatencyTimeIntervals = 0;
            _DedicatedWriteLatencyTimeAverage = 0;          
            
#if !defined(PLATFORM_UNIX)
            KInvariant(! _PerfCounterSetInstance);
            
#endif
            OverlayManager::SPtr overlayManager = _OverlayLog->GetOverlayManager();
            if (overlayManager)
            {
#if !defined(PLATFORM_UNIX)
                Status = overlayManager->CreateNewLogStreamPerfCounterSet(_StreamId, *this, _PerfCounterSetInstance);
                if (! NT_SUCCESS(Status))
                {
                    //
                    // Not having perf counters is not fatal
                    //
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                }
#endif
                _DedicatedWriteBytesOutstandingThreshold = overlayManager->GetMaximumDestagingWriteOutstanding();                   
            }

            _ContainerDedicatedBytesWritten = _OverlayLog->GetContainerDedicatedBytesWrittenPointer();
            _ContainerSharedBytesWritten = _OverlayLog->GetContainerSharedBytesWrittenPointer();
            
            //
            // Remember the max record size for the base shared log
            //
            _SharedMaximumRecordSize = _BaseSharedLog->QueryMaxRecordSize();
            
            if (_OpenGateContext)
            {
                AdvanceState(WaitForGate);

                _OpenGateContext->Reuse();
                _OpenGateContext->StartWaitForGate(_DiskId,
                                                   this,
                                                   completion);
                break;
            }

            // Fall through
        }


        case WaitForGate:
        {               
            //
            // Next open the shared log stream
            //
            _GateAcquired = TRUE;
            
            AdvanceState(OpenSharedStream);
            
            _OpenStream->StartOpenLogStream(_StreamId,
                                            _SharedLogStream,
                                            this,
                                            completion);
            break;
        }

        case OpenSharedStream:
        {
            //
            // Determine the log stream type to see if this requires
            // the special handling needed by the Logical Log
            //
            RvdLogStreamType logStreamType;
            KGuid logStreamTypeGuid;

            _SharedLogStream->SetTruncationCompletionEvent(&_SharedTruncationEvent);
            
            _SharedLogStream->QueryLogStreamType(logStreamType);

            if (logStreamType == KLogicalLogInformation::GetLogicalLogStreamType())
            {
                _IsStreamForLogicalLog = TRUE;
            }

            //
            // Get the current truncation point for this and update
            //
            RvdLogAsn truncationAsn;
            RvdLogAsn lowAsn;
            RvdLogAsn highAsn;
            Status = _SharedLogStream->QueryRecordRange(&lowAsn,
                                                        &highAsn,
                                                        &truncationAsn);
            KInvariant(NT_SUCCESS(Status));

            _SharedTruncationAsn.SetIfLarger(truncationAsn);
            
            //
            // Next open the dedicated container
            //
            AdvanceState(OpenDedicatedContainer);

            if (_Path)
            {
                _OpenDedicatedLog->StartOpenLog(*_Path,
                                                _DedicatedLogContainer,
                                                this,
                                                completion);
            } else {
                _OpenDedicatedLog->StartOpenLog(_DiskId,
                                                _DedicatedContainerId,
                                                _DedicatedLogContainer,
                                                this,
                                                completion);
            }
            break;
        }

        case OpenDedicatedContainer:
        {
            //
            // Next open the dedicated stream in the container
            //
            AdvanceState(OpenDedicatedStream);

            //
            // Set shutdown event on the log container. It will fire
            // when the container is completely shutdown in the
            // destructor.
            //
            Status = _DedicatedLogContainer->SetShutdownEvent(&_DedicatedLogShutdownEvent);            
            KInvariant(NT_SUCCESS(Status));

            _OpenStream = nullptr;
            Status = _DedicatedLogContainer->CreateAsyncOpenLogStreamContext(_OpenStream);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteOpen(Status);
                return;
            }

            _OpenStream->StartOpenLogStream(_StreamId,
                                            _DedicatedLogStream,
                                            this,
                                            completion);            
            break;
        }

        case OpenDedicatedStream:
        {
            //
            // Update the shared truncation ASN
            //
            _DedicatedLogStream->QueryLogStreamType(_LogStreamType);

            if (_IsStreamForLogicalLog)
            {
                Status = CreateCoalesceRecords(_CoalesceRecords,
                                               _MaxRecordSize,
                                               KLogicalLogInformation::FixedMetadataSize,
                                               sizeof(KtlLogVerify::KtlMetadataHeader));
                if (!NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoCompleteOpen(Status);
                    return;
                }
            }

            RvdLogAsn lowAsn;
            RvdLogAsn highAsn;
            RvdLogAsn truncationAsn;
            Status = _DedicatedLogStream->QueryRecordRange(&lowAsn,
                                                           &highAsn,
                                                           &truncationAsn);
            KInvariant(NT_SUCCESS(Status));
            _LastDedicatedAsn = highAsn;
            
            _SharedTruncationAsn.SetIfLarger(truncationAsn);
            _SharedLogStream->Truncate(_SharedTruncationAsn,
                                       _SharedTruncationAsn);


            Status = _SharedLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
            KInvariant(NT_SUCCESS(Status));
            if ((! _IsStreamForLogicalLog) || (OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn)))
            {
                //
                // Since shared stream is empty or this is not a
                // logical log, don't worry about it.
                //
                AdvanceState(BeginCopyFromSharedToDedicated);
                goto StateMachine;
            }
            _FirstSharedAsn = lowAsn;            

            //
            // Do a quick and dirty check to see if the shared log
            // records are discontiguous from the end of the dedicated
            // log.
            //
            ULONGLONG dedicatedVersion;
            ULONGLONG sharedVersion;
            RvdLogStream::RecordDisposition disposition;
            ULONG ioSize;
            ULONGLONG debugInfo1;
            ULONG maximumDataInBuffer;  
            Status = _SharedLogStream->QueryRecord(_FirstSharedAsn,
                                                     &sharedVersion,
                                                     &disposition,
                                                     &ioSize,
                                                     &debugInfo1);
            KInvariant(NT_SUCCESS(Status));
            KInvariant(disposition == RvdLogStream::RecordDisposition::eDispositionPersisted);
            Status = _DedicatedLogStream->QueryRecord(_LastDedicatedAsn,
                                                     &dedicatedVersion,
                                                     &disposition,
                                                     &ioSize,
                                                     &debugInfo1);
            KInvariant(NT_SUCCESS(Status));
            KInvariant(disposition == RvdLogStream::RecordDisposition::eDispositionPersisted);

            //
            // Make sure that the shared record is newer then the dedicated
            // record since if the shared record version is lower than
            // the dedicated record version there there was a tail
            // truncation and comparing ASNs would not be valid.
            if (sharedVersion < dedicatedVersion)
            {
                AdvanceState(BeginCopyFromSharedToDedicated);
                goto StateMachine;
            }
            
            // In the case where the shared record is newer, we check that
            // there is not a gap between the end of the last dedicated
            // record and the beginning of the first shared record.
            // Note that this is not a foolproof check since the
            // dedicated record could have padding which is not
            // accounted for in this check. So this check may pass even
            // through there is indeed a hole. In any case the
            // replicator will the entire log and if there is a hole,
            // it will be caught at that level.
            //
            maximumDataInBuffer = (KLogicalLogInformation::FixedMetadataSize -
                                   (_DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead() +
                                    sizeof(KtlLogVerify::KtlMetadataHeader) +
                                    sizeof(KLogicalLogInformation::MetadataBlockHeader) +
                                    sizeof(KLogicalLogInformation::StreamBlockHeader))) +
                                   ioSize;

            if (_FirstSharedAsn > (_LastDedicatedAsn.Get() + maximumDataInBuffer))
            {
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _FirstSharedAsn.Get(), _LastDedicatedAsn.Get());
                KTraceFailedAsyncRequest(Status, this, maximumDataInBuffer, 0);
                AdvanceState(CloseDedicatedContainer);
                _FinalStatus = Status;
                goto StateMachine;
            }

            //
            // The above check is not foolproof due to
            // padding. To make more correct, read
            // the last dedicated log record and pull out its actual
            // size and check against that.
            // 
            AdvanceState(VerifyDedicatedSharedContiguousness);
            Status = _DedicatedLogStream->CreateAsyncReadContext(_LastReadContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                AdvanceState(CloseDedicatedContainer);
                _FinalStatus = Status;
                goto StateMachine;
            }

            _LastReadContext->StartRead(_LastDedicatedAsn,
                                        &_LastVersion,
                                        _LastMetadataBuffer,
                                        _LastIoBuffer,
                                        this,
                                        completion);
            return;

            
        }

        case VerifyDedicatedSharedContiguousness:
        {
            KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
            KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
            ULONG dataSize;
            ULONG data;
            KInvariant(_IsStreamForLogicalLog);

            Status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(_LastMetadataBuffer,
                                           _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
                                           *_LastIoBuffer,
                                           sizeof(KtlLogVerify::KtlMetadataHeader),
                                           metadataHeader,
                                           streamBlockHeader,
                                           dataSize,
                                           data);
            if (! NT_SUCCESS(Status))
            {
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                AdvanceState(CloseDedicatedContainer);
                _FinalStatus = Status;
                goto StateMachine;
            }
            

            if (_FirstSharedAsn > (_LastDedicatedAsn.Get() + dataSize))
            {
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
                KTraceFailedAsyncRequest(Status, this, _FirstSharedAsn.Get(), _LastDedicatedAsn.Get());
                KTraceFailedAsyncRequest(Status, this, dataSize, 0);
                AdvanceState(CloseDedicatedContainer);
                _FinalStatus = Status;
                goto StateMachine;
            }
            

            AdvanceState(BeginCopyFromSharedToDedicated);
            goto StateMachine;
            
        }

        case BeginCopyFromSharedToDedicated:
        {
            //
            // Now copy any records left in the shared to dedicated and
            // truncate the shared
            //
            AdvanceState(CopyFromSharedToDedicated);
            Status = CreateAsyncCopySharedToDedicatedContext(_CopySharedToDedicated);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                AdvanceState(CloseDedicatedContainer);
                _FinalStatus = Status;
                goto StateMachine;
            }
            
            _CopySharedToDedicated->StartCopySharedToDedicated(this,
                                                               completion);
            break;
        }

        case CopyFromSharedToDedicated:
        {
            _TailReadContext = nullptr;
            
            if (_IsStreamForLogicalLog)
            {               
                //
                // In case of a logical log stream we need to recover
                // the tail truncation point
                //
                KtlLogAsn streamTailAsn;
                
                AdvanceState(RecoverTailTruncationAsn);
                
                Status = DetermineStreamTailAsn(streamTailAsn);
                
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    AdvanceState(CloseDedicatedContainer);
                    _FinalStatus = Status;
                    goto StateMachine;
                }

                if (!streamTailAsn.IsNull())
                {
                    Status = _DedicatedLogStream->CreateAsyncReadContext(_TailReadContext);
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        AdvanceState(CloseDedicatedContainer);
                        _FinalStatus = Status;
                        goto StateMachine;
                    }

                    _LogicalLogTailOffset = streamTailAsn;
                    _TailReadContext->StartRead(streamTailAsn,
                                                &_LogicalLogLastVersion,
                                                _TailMetadataBuffer,
                                                _TailIoBuffer,
                                                this,
                                                completion);
                    return;
                } else {
                    _LogicalLogTailOffset = KtlLogAsn::Min();
                    _LogicalLogLastVersion = 0;
                }
            }
            
            // Fall through
        }

        case RecoverTailTruncationAsn:
        {
            if (_TailReadContext)
            {
                KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
                KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
                ULONG dataSize;
                ULONG data;
                KInvariant(_IsStreamForLogicalLog);

                Status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(_TailMetadataBuffer,
                                               _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
                                               *_TailIoBuffer,
                                               sizeof(KtlLogVerify::KtlMetadataHeader),
                                               metadataHeader,
                                               streamBlockHeader,
                                               dataSize,
                                               data);
                if (! NT_SUCCESS(Status))
                {
                    Status = K_STATUS_LOG_STRUCTURE_FAULT;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    AdvanceState(CloseDedicatedContainer);
                    _FinalStatus = Status;
                    goto StateMachine;
                }

                //
                // If this last record has a flag that denotes a logical log barrier then this would be the
                // end of the logical log stream and thus the _LogicalLogTailOffset. If the barrier flag is not set
                // then read the previous record and look for the logical log barrier there. If we reach the beginning of 
                // the stream without finding a barrier then consider the stream empty.
                //

                if (metadataHeader->RecordMarkerOffsetPlusOne < streamBlockHeader->DataSize)
                {
                    //
                    // If we encounter a record that does not have a
                    // record marker or the record marker does not
                    // extend to the end of the record then its data is not considered to
                    // be valid. The next write will overwrite the asn
                    // for the stream and thus be in effect a truncate
                    // tail. Truncate tail assumes that the truncate
                    // tail record is in both shared and dedicated
                    // streams before compelting the write. This flag
                    // is set so that the next (first) write will have
                    // truncate tail semantics.
                    //
                    KDbgCheckpointWDataInformational(GetActivityId(), "Data after record marker", Status,
                                        (ULONGLONG)_LogicalLogTailOffset.Get(),
                                        (ULONGLONG)this,
                                        (ULONGLONG)metadataHeader->RecordMarkerOffsetPlusOne,
                                        (ULONGLONG)streamBlockHeader->DataSize);                                
                    
                }
                
                if (metadataHeader->RecordMarkerOffsetPlusOne != 0)
                {
                    _LogicalLogTailOffset = _LogicalLogTailOffset.Get() + (metadataHeader->RecordMarkerOffsetPlusOne -1);
                    _RecoveredLogicalLogHeader = *streamBlockHeader;
                } else {                    
                    KAsyncContextBase::CompletionCallback readPreviousCompletion(this, &OverlayStream::OpenServiceReadRecordCompletion);

                    _TailReadContext->Reuse();
                    _TailReadContext->StartRead(_LogicalLogTailOffset,
                                                RvdLogStream::AsyncReadContext::ReadType::ReadPreviousRecord,
                                                &_LogicalLogTailOffset,
                                                NULL,                   // Use version number from highest record
                                                _TailMetadataBuffer,
                                                _TailIoBuffer,
                                                this,
                                                readPreviousCompletion);
                    return;
                }
            }

            AdvanceState(FinishOpen);
            goto FinishOpen;
        }

        case RecoverTailTruncationAsnNoRecord:
        {
            //
            // We reach here when the ReadPrevious fails with STATUS_NOT_FOUND indicating that
            // no barrier record was found. In this case the tail should restart at the beginning of the
            // stream.
            //
            _LogicalLogTailOffset = KtlLogAsn::Min();

            //
            // Now access the stream metadata block
            //
            AdvanceState(FinishOpen);

            // Fall through
        }

        case FinishOpen:
        {
FinishOpen:
            //
            // Almost done
            //
#ifndef FILEFUZZ
            _OverlayLog->AccountForStreamOpen(*this);
#endif

            _StreamAllocation =_ThrottledAllocator->AddToLimit();

            DoCompleteOpen(STATUS_SUCCESS);
            return;
        }
        
        case CopySharedLogDataToBackup:
        {
            _State = CleanupStreamResources;
            Status = CreateAsyncCopySharedToBackupContext(_CopySharedToBackup);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _FinalStatus = Status;
                goto CloseDedicatedContainer;
            }
            
            _CopySharedToBackup->StartCopySharedToBackup(this,
                                                         completion);
            return;
        }

        case CleanupStreamResources:
        {
            _State = CloseDedicatedContainer;
            Status = _BaseSharedLog->CreateAsyncDeleteLogStreamContext(_DeleteStream);                  
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                goto CloseDedicatedContainer;
            }

            _SharedLogStream->SetTruncationCompletionEvent(NULL);           
            _SharedLogStream = nullptr;
            _CopySharedToDedicated = nullptr;
            _CopySharedToBackup = nullptr;
            
            //
            // Delete the log stream in the shared log to ensure that
            // the resources used by the defunct stream are cleaned up
            // in the shared log. The rest of the resources for the
            // stream (metadata, etc) will be cleaned up the next time
            // the container is opened.
            _DeleteStream->StartDeleteLogStream(_StreamId, this, completion);
                        
            return;
        }
        
        case CloseDedicatedContainer:
        {
CloseDedicatedContainer:
            AdvanceState(WaitForDedicatedContainerClose);

            _CopySharedToDedicated = nullptr;
            _CopySharedToBackup = nullptr;
            _OpenDedicatedLog = nullptr;
            _OpenStream = nullptr;
            _DeleteStream = nullptr;
            
            
            _DedicatedLogStream = nullptr;
            _TailReadContext = nullptr;
            _LastReadContext = nullptr;

            _WriteTableLookupKey = nullptr;
#if !defined(PLATFORM_UNIX)
            if (_PerfCounterSetInstance)
            {
                _PerfCounterSetInstance->Stop();
            }
            _PerfCounterSetInstance = nullptr;
#endif

            _DedicatedLogStream = nullptr;
            if (_SharedLogStream)
            {
                _SharedLogStream->SetTruncationCompletionEvent(NULL);
            }
            _SharedLogStream = nullptr;

            if (_CoalesceRecords)
            {
                _CoalesceRecords->_PeriodicFlushContext = nullptr;
                _CoalesceRecords->_CloseFlushContext = nullptr;
                _CoalesceRecords->_FlushAllRecordsForClose = nullptr;
                _CoalesceRecords = nullptr;
            }

            if (_DedicatedLogContainer != nullptr)
            {
                _DedicatedLogContainer = nullptr;
                _DedicatedLogShutdownWait->StartWaitUntilSet(this,
                                                             completion);
                break;
            }

            // Fall through....
        }

        case WaitForDedicatedContainerClose:
        {
            DoCompleteOpen(_FinalStatus);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
OverlayStream::OpenServiceReadRecordCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();
    
    //
    // Special purpose completion routine for RecoverTailTruncationAsn
    //
    KInvariant(_State == RecoverTailTruncationAsn);

    if (status == STATUS_NOT_FOUND)
    {
        //
        // This error code is if the record was not found. In this case it means that the previous record
        // doesn't exist and so there is no logical log record barrier. Move the state machine to a new state
        // and fix the status.
        //
        _State = RecoverTailTruncationAsnNoRecord;
        status = STATUS_SUCCESS;
    }

    OpenServiceFSM(status);
}

VOID
OverlayStream::OpenServiceCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    
    OpenServiceFSM(status);
}

VOID
OverlayStream::OnServiceOpen(
    )
{
    _State = OpenInitial;
    OpenServiceFSM(STATUS_SUCCESS);
}

VOID
OverlayStream::OnServiceReuse(
    )
{
    KInvariant(FALSE);    
}        

//
// Close Service
//
VOID
OverlayStream::CloseServiceFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::CloseServiceCompletion);

    KDbgCheckpointWDataInformational(GetActivityId(), "CloseServiceFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)_DedicatedLogContainer.RawPtr(),
                        (ULONGLONG)0);                                
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        CompleteClose(Status);
        return;
    }
    
    switch (_State)
    {
        case CloseInitial:
        {
            KInvariant(_ObjectState == Opened);

            _State = CloseDedicatedLog;

            _WriteTableLookupKey = nullptr;
#if !defined(PLATFORM_UNIX)
            if (_PerfCounterSetInstance)
            {
                _PerfCounterSetInstance->Stop();
            }
            _PerfCounterSetInstance = nullptr;
#endif

            //
            // Force all waiters on the shared truncation event to be
            // completed. There may be a race between OverlayStream
            // close and the OverlayLog accelerated truncation
            // functionality.
            //          
            while (! _SharedTruncationEvent.IsSignaled())
            {
                _SharedTruncationEvent.SetEvent();
            }

            _DedicatedLogStream = nullptr;
            _SharedLogStream->SetTruncationCompletionEvent(NULL);           
            _SharedLogStream = nullptr;

            if (_CoalesceRecords)
            {
                KInvariant(_CoalesceRecords->GetMetadataBufferX() == nullptr);
                KInvariant(_CoalesceRecords->GetIoBufferX() == nullptr);  
                _CoalesceRecords->SetCloseFlushContext(nullptr);
                _CoalesceRecords->SetPeriodicFlushContext(nullptr);
                _CoalesceRecords = nullptr;
            }

            KInvariant(_DedicatedLogContainer);

            _DedicatedLogContainer = nullptr;
            _DedicatedLogShutdownWait->StartWaitUntilSet(this,
                                                         completion);
            break;
        }

        case CloseDedicatedLog:
        {
#ifndef FILEFUZZ
            _OverlayLog->AccountForStreamClosed(*this);
#endif

            _ThrottledAllocator->RemoveFromLimit(_StreamAllocation);
            _StreamAllocation = 0;

            _BaseSharedLog = nullptr;
            _BaseLogManager = nullptr;
            _OverlayLog = nullptr;
                        
            _ObjectState = Closed;
            CompleteClose(Status);
            break;
        }        
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::CloseServiceCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    
    CloseServiceFSM(status);
}

VOID
OverlayStream::OnServiceClose(
    )
{
    _State = CloseInitial;
    CloseServiceFSM(STATUS_SUCCESS);
}

NTSTATUS
OverlayStream::TryAcquireRequestRef()
{
    NTSTATUS status;
    BOOLEAN acquired;

    acquired = TryAcquireServiceActivity();
    if (acquired)
    {
        status = STATUS_SUCCESS;
    } else {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
    }

    return(status);
}

VOID
OverlayStream::ReleaseRequestRef()
{
    ReleaseServiceActivity();
}

NTSTATUS
OverlayStream::StartServiceOpen(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt OpenCompletionCallback OpenCallbackPtr,
    __in_opt KAsyncGlobalContext* const
)
{
    NTSTATUS status;

    status = StartOpen(ParentAsync, OpenCallbackPtr, _GlobalContext.RawPtr());

    return(status);
}

NTSTATUS
OverlayStream::StartServiceClose(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt CloseCompletionCallback CloseCallbackPtr
)
{
    NTSTATUS status;

    status = StartClose(ParentAsync, CloseCallbackPtr);

    return(status);
}

//
// Number of operations to include in a single sample of latency
//
const ULONG OverlayStream::_LatencyOperationSampleCount = 100;

VOID OverlayStream::UpdateSharedWriteLatencyTime(
    __in LONGLONG SharedWriteLatencyTime
    )
{
    LONGLONG sharedWriteLatencyTime; 
    LONG i;

    //
    // Do not take a lock here as we tolerate races. Worst case is that
    // we miss a latency measurement.
    //
    i = InterlockedIncrement(&_SharedWriteLatencyTimeIntervals);
    sharedWriteLatencyTime = InterlockedAdd64(&_SharedWriteLatencyTime, SharedWriteLatencyTime);
    if (i == _LatencyOperationSampleCount)
    {
        _SharedWriteLatencyTimeAverage = (sharedWriteLatencyTime / i);
        _SharedWriteLatencyTime = 0;
        _SharedWriteLatencyTimeIntervals = 0;
    }
}

VOID OverlayStream::UpdateDedicatedWriteLatencyTime(
    __in LONGLONG DedicatedWriteLatencyTime
    )
{
    LONGLONG dedicatedWriteLatencyTime; 
    LONG i;

    //
    // Do not take a lock here as we tolerate races. Worst case is that
    // we miss a latency measurement.
    //
    i = InterlockedIncrement(&_DedicatedWriteLatencyTimeIntervals);
    dedicatedWriteLatencyTime = InterlockedAdd64(&_DedicatedWriteLatencyTime, DedicatedWriteLatencyTime);
    if (i == _LatencyOperationSampleCount)
    {
        _DedicatedWriteLatencyTimeAverage = (dedicatedWriteLatencyTime / i);
        _DedicatedWriteLatencyTime = 0;
        _DedicatedWriteLatencyTimeIntervals = 0;
    }
}

void ComputeLogUsage(
    __in RvdLog& log,
    __out ULONGLONG& totalSpace,
    __out ULONGLONG& freeSpace,
    __out ULONGLONG& usedSpace
    )
{
    log.QuerySpaceInformation(
        &totalSpace,
        &freeSpace);

    //
    // Core logger always reserves 2 records in case it has checkpoint records. This is not reflected in
    // the freeSpace amount returned and so must be accounted for here
    //
    freeSpace -= log.QueryReservedSpace();

    usedSpace = totalSpace - freeSpace;
}

NTSTATUS OverlayStream::ComputeLogSizeAndSpaceRemaining(
    __out ULONGLONG& LogSize,
    __out ULONGLONG& SpaceRemaining
    )
{
    NTSTATUS status;
    ULONGLONG totalSpace;
    ULONGLONG freeSpace;
    ULONGLONG usedSpace;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    ComputeLogUsage(
        *_DedicatedLogContainer,
        totalSpace,
        freeSpace,
        usedSpace);

    KDbgCheckpointWData(GetActivityId(), "ComputeLogSizeAndSpaceRemaining", STATUS_SUCCESS, totalSpace, freeSpace, usedSpace, 0);

    LogSize = usedSpace;
    SpaceRemaining = freeSpace;

    return(STATUS_SUCCESS);
}

NTSTATUS OverlayStream::ComputeLogPercentageUsed(__out ULONG& PercentUsed)
{
    NTSTATUS status;
    ULONGLONG totalSpace;
    ULONGLONG freeSpace;
    ULONGLONG usedSpace;
    ULONG percentUsed;

    PercentUsed = 0;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    ComputeLogUsage(
        *_DedicatedLogContainer,
        totalSpace,
        freeSpace,
        usedSpace);

    percentUsed = (ULONG)((usedSpace*100) / totalSpace);

#ifdef UDRIVER
    KDbgCheckpointWData(GetActivityId(), "ComputeLogPercentageUsed", STATUS_SUCCESS, totalSpace, freeSpace, usedSpace, percentUsed);
#endif
    PercentUsed = percentUsed;

    return(STATUS_SUCCESS);
}

VOID OverlayStream::CheckThresholdNotifications(
    )
{
    NTSTATUS status;
    ULONG percentUsed;

    status = ComputeLogPercentageUsed(percentUsed);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return;
    }

    K_LOCK_BLOCK(_StreamUsersLock)
    {
    
        KtlLogStreamKernel* stream = _StreamUsers.PeekHead();
        
        while (stream != NULL)
        {
            stream->CheckThresholdContexts(percentUsed);
            stream = _StreamUsers.Successor(stream);
        }
    }
}

VOID OverlayStream::AddKtlLogStream(
    __in KtlLogStreamKernel& KtlLogStream
    )
{        
    K_LOCK_BLOCK(_StreamUsersLock)
    {
        _StreamUsers.AppendTail(&KtlLogStream);
    }
}

VOID OverlayStream::RemoveKtlLogStream(
    __in KtlLogStreamKernel& KtlLogStream
    )
{
    K_LOCK_BLOCK(_StreamUsersLock)
    {
        _StreamUsers.Remove(&KtlLogStream);
    }
}

BOOLEAN OverlayStream::ThrottleWriteIfNeeded(
    __in OverlayStream::AsyncDestagingWriteContextOverlay& DestagingWriteContext
    )
{
    //
    // Determine if the write can proceed or if it needs to be throttled due to 
    // backlog on the dedicated writes or some other factor
    //

    if (GetWriteOnlyToDedicated())
    {
        //
        // If bypassing shared log there is no throttling needed
        //
        return(TRUE);
    }

    
    ULONGLONG totalSpace, freeSpace;
    BOOLEAN sharedThrottled;
    LONGLONG lowestLsn;
    LONGLONG highestLsn;
    RvdLogStreamId lowestLsnStreamId;

    //
    // First check if write needs to be throttled as a result of shared
    // log usage.
    //
    sharedThrottled = _OverlayLog->ShouldThrottleSharedLog(DestagingWriteContext,
                                                           totalSpace,
                                                           freeSpace,
                                                           lowestLsn,
                                                           highestLsn,
                                                           lowestLsnStreamId);
    if (sharedThrottled)
    {
        KDbgCheckpointWDataInformational(GetActivityId(), "Throttled write", STATUS_SUCCESS, (ULONGLONG)&DestagingWriteContext,
                            freeSpace,
                            DestagingWriteContext.GetRecordAsn().Get(),
                            totalSpace);
        KDbgCheckpointWDataInformational(GetActivityId(), "Throttled write2", STATUS_SUCCESS,
                                         lowestLsnStreamId.Get().Data1,
                                         0,
                                         lowestLsn,
                                         highestLsn);
        return(FALSE);
    }

    //
    // Next check if the shared log is getting too far ahead for this
    // particular dedicated log.
    //
    if ((GetDedicatedWriteBytesOutstandingThreshold() != KLogicalLogInformation::WriteThrottleThresholdNoLimit) &&
        (GetDedicatedWriteBytesOutstanding() >= GetDedicatedWriteBytesOutstandingThreshold()))
    {
        KDbgCheckpointWDataInformational(GetActivityId(), "Throttled write", STATUS_SUCCESS, (ULONGLONG)&DestagingWriteContext,
                            _DedicatedWriteBytesOutstanding,
                            DestagingWriteContext.GetRecordAsn().Get(),
                            GetDedicatedWriteBytesOutstandingThreshold());
        
        K_LOCK_BLOCK(_ThrottleWriteLock)
        {
            _ThrottledWritesList.AppendTail(&DestagingWriteContext);
        }
        
        return(FALSE);
    }

    return(TRUE);
}
   
VOID OverlayStream::UnthrottleWritesIfPossible(
    __in BOOLEAN ReleaseOne
    )
{
    OverlayStream::AsyncDestagingWriteContextOverlay* throttledWrite = NULL;

    BOOLEAN continueUnthrottling = TRUE;
    _OverlayLog->ShouldUnthrottleSharedLog(ReleaseOne);
    
    while (continueUnthrottling)
    {
        throttledWrite = NULL;

        K_LOCK_BLOCK(_ThrottleWriteLock)
        {
            throttledWrite = _ThrottledWritesList.PeekHead();

            if (throttledWrite == NULL)
            {
                //
                // If there is nothing on the list then nothing to
                // unthrottle
                //
                return;
            }

            if ((GetDedicatedWriteBytesOutstandingThreshold() == KLogicalLogInformation::WriteThrottleThresholdNoLimit) || 
                 (GetDedicatedWriteBytesOutstanding() < GetDedicatedWriteBytesOutstandingThreshold()))
            {
                throttledWrite = _ThrottledWritesList.RemoveHead();
            } else {
                // TODO: Remove when stabilized
                KDbgCheckpointWDataInformational(GetActivityId(), "Didnt Unthrottled write", STATUS_SUCCESS,
                                    (ULONGLONG)throttledWrite, GetDedicatedWriteBytesOutstanding(),
                                    throttledWrite->GetRecordAsn().Get(), 0);
                return;
            }            
        }

        // TODO: Remove when stabilized
        KDbgCheckpointWDataInformational(GetActivityId(), "Unthrottled write", STATUS_SUCCESS,
                            (ULONGLONG)throttledWrite, GetDedicatedWriteBytesOutstanding(),
                            throttledWrite->GetRecordAsn().Get(), 0);
        throttledWrite->OnStartAllocateBuffer();
    }
}

//
// RvdLogStream interface
//
LONGLONG
OverlayStream::QueryLogStreamUsage(
    )
{
    KInvariant(FALSE);
    return(0);
}

VOID
OverlayStream::QueryLogStreamType(
    __out RvdLogStreamType& LogStreamType
    )
{
    LogStreamType = _LogStreamType;
}

VOID
OverlayStream::QueryLogStreamId(
    __out RvdLogStreamId& LogStreamId
    )
{
    LogStreamId = _StreamId;
}

NTSTATUS
OverlayStream::QueryRecordRange(
    __out_opt RvdLogAsn* const LowestAsn,
    __out_opt RvdLogAsn* const HighestAsn,
    __out_opt RvdLogAsn* const LogTruncationAsn
    )
{
    NTSTATUS status;
    RvdLogAsn dedicatedLowest;
    RvdLogAsn dedicatedHighest;
    RvdLogAsn dedicatedLogTruncation;
    RvdLogAsn sharedLowest;
    RvdLogAsn sharedHighest;
    RvdLogAsn sharedLogTruncation;

    if (LowestAsn != NULL)
    {
        *LowestAsn = 0;
    }

    if (HighestAsn != NULL)
    {
        *HighestAsn = 0;
    }

    if (LogTruncationAsn != NULL)
    {
        *LogTruncationAsn = 0;
    }
    
    status = TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
        
    status = _DedicatedLogStream->QueryRecordRange(&dedicatedLowest,
                                                   &dedicatedHighest,
                                                   &dedicatedLogTruncation);

    if (! NT_SUCCESS(status))
    {
#ifdef UDRIVER      
        KDbgCheckpointWData(GetActivityId(), "OverlayStream::QueryRecordRange", status,
                            (ULONGLONG)this,
                            (ULONGLONG)_DedicatedLogStream.RawPtr(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);
#endif
        return(status);
    }
    
    status = _SharedLogStream->QueryRecordRange(&sharedLowest,
                                                &sharedHighest,
                                                &sharedLogTruncation);

    if (! NT_SUCCESS(status))
    {
#ifdef UDRIVER      
        KDbgCheckpointWData(GetActivityId(), "OverlayStream::QueryRecordRange", status,
                            (ULONGLONG)this,
                            (ULONGLONG)_SharedLogStream.RawPtr(),
                            (ULONGLONG)0,
                            (ULONGLONG)0);
#endif
        return(status);
    }

    if (LowestAsn != NULL)
    {
        if (sharedLowest.IsNull())
        {
            *LowestAsn = dedicatedLowest;
        } else if (dedicatedLowest.IsNull()) {
            *LowestAsn = sharedLowest;      
        } else if (dedicatedLowest < sharedLowest) {
            *LowestAsn = dedicatedLowest;       
        } else {
            *LowestAsn = sharedLowest;
        }
    }

    if (HighestAsn != NULL)
    {
        if (sharedHighest.IsNull())
        {
            *HighestAsn = dedicatedHighest;
        } else if (dedicatedHighest.IsNull()) {
            *HighestAsn = sharedHighest;        
        } else if (dedicatedHighest > sharedHighest) {
            *HighestAsn = dedicatedHighest;     
        } else {
            *HighestAsn = sharedHighest;
        }
    }

    if (LogTruncationAsn != NULL)
    {
        *LogTruncationAsn = dedicatedLogTruncation;
    }
        
    return(STATUS_SUCCESS);
}

ULONGLONG
OverlayStream::QueryCurrentReservation(
    )
{
    NTSTATUS status = TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        return(0);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    return(_DedicatedLogStream->QueryCurrentReservation());
}


//
// WriteContext
//

VOID
OverlayStream::AsyncWriteContextOverlay::CompleteRequest(
    __in NTSTATUS Status
)
{
    if (NT_SUCCESS(Status) && _LogSize != nullptr)
    {
        KAssert(_LogSpaceRemaining != nullptr);

        Status = _OverlayStream->ComputeLogSizeAndSpaceRemaining(*_LogSize, *_LogSpaceRemaining);
        if (!NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, 0, 0);
        }
    }

    Complete(Status);
}

VOID
OverlayStream::AsyncWriteContextOverlay::OnCompleted(
    )
{
    _MetaDataBuffer = nullptr;
    _IoBuffer = nullptr;
    
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }

    _DestagingWrite = nullptr;    
}

VOID
OverlayStream::AsyncWriteContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncWriteContextOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            if (! NT_SUCCESS(_OverlayStream->GetFailureStatus()))
            {
                //
                // If a dedicated log write failed then we need to fail
                // any subsequent writes
                //
                Complete(_OverlayStream->GetFailureStatus());
                return;
            }
            
            _State = WriteToDestaging;
            _DestagingWrite->StartReservedWrite(_ReserveToUse,
                                                _RecordAsn,
                                                _Version,
                                                _MetaDataBuffer,
                                                _IoBuffer,
                                                _ForceFlush,
                                                *this);
            break;
        }

        case WriteToDestaging:
        {
            Complete(STATUS_SUCCESS);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncWriteContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _DestagingWrite->Cancel();
}

VOID
OverlayStream::AsyncWriteContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncWriteContextOverlay::OnStart(
    )
{
    NTSTATUS status;

    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;

    //
    // Each AsyncWriteContextOverlay needs its own
    // AsyncDestagingWriteContext. This is because the
    // AsyncWriteContextOverlay may complete back to the caller and
    // then be reused. The AsyncDestagingWriteContext might still be in
    // use by the slower write and so a fresh
    // AsyncDestagingWriteContext is needed.
    //
    status = _OverlayStream->CreateAsyncDestagingWriteContext(_DestagingWrite);
    if (!NT_SUCCESS(status))
    {
        KTraceOutOfMemory(_OverlayStream->GetActivityId(), status, this, 0, 0);
        Complete(status);
        return;
    }

    
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncWriteContextOverlay::StartWrite(
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    _ReserveToUse = 0;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = NULL;
    _ForceFlush = FALSE;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncWriteContextOverlay::StartWrite(
    __in BOOLEAN,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    //
    // CONSIDER: Respect the LowPriorityIO flag if it is ever exposed
    //           back up through the ktlshim layer. At this point it is
    //           only used below.
    //
    _ReserveToUse = 0;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = NULL;
    _ForceFlush = FALSE;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncWriteContextOverlay::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _ReserveToUse = ReserveToUse;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = NULL;
    _ForceFlush = FALSE;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;
    
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncWriteContextOverlay::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __out ULONGLONG& LogSize,
    __out ULONGLONG& LogSpaceRemaining,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    _ReserveToUse = ReserveToUse;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = NULL;
    _ForceFlush = FALSE;
    _LogSize = &LogSize;
    _LogSpaceRemaining = &LogSpaceRemaining;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncWriteContextOverlay::StartReservedWrite(
    __in BOOLEAN,
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    //
    // CONSIDER: Respect the LowPriorityIO flag if it is ever exposed
    //           back up through the ktlshim layer. At this point it is
    //           only used below.
    //
    _ReserveToUse = ReserveToUse;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = NULL;
    _ForceFlush = FALSE;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncWriteContextOverlay::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in BOOLEAN ForceFlush,
    __out_opt BOOLEAN * const SendToShared,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    if (SendToShared != NULL)
    {
        *SendToShared = FALSE;
    }
    
    _ReserveToUse = ReserveToUse;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _SendToShared = SendToShared;
    _ForceFlush = ForceFlush;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStreamBase::AsyncWriteContext::~AsyncWriteContext()
{
}

OverlayStreamBase::AsyncWriteContext::AsyncWriteContext()
{
}

OverlayStream::AsyncWriteContextOverlay::~AsyncWriteContextOverlay()
{
}

VOID
OverlayStream::AsyncWriteContextOverlay::OnReuse(
    )
{
}


OverlayStream::AsyncWriteContextOverlay::AsyncWriteContextOverlay(
    __in OverlayStream& OStream
    )
{
    _OverlayStream = &OStream;
    _DestagingWrite = nullptr;
}


NTSTATUS
OverlayStream::CreateAsyncWriteContext(
    __out AsyncWriteContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncWriteContextOverlay::SPtr context;

    Context = nullptr;
    
    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Destaging write context
//
VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnCompleted(
    )
{
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }

    
    _MetaDataBuffer = nullptr;
    KInvariant(_SharedIoBuffer == nullptr);
    KInvariant(_DedicatedIoBuffer == nullptr);
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _DedicatedWrite->Cancel();
    _SharedWrite->Cancel();
}

VOID OverlayStream::AsyncDestagingWriteContextOverlay::ProcessWriteCompletion(
    __in BOOLEAN SharedWriteCompleted,
    __in NTSTATUS Status
    )
{
    LONG completedToCaller = 1;
    ULONG asyncsOutstanding = 99;
    ULONGLONG lsn = 0;

    if (SharedWriteCompleted)
    {
        if (NT_SUCCESS(Status))
        {
            _ContainerOperation.EndOperation(_DataSize);
            
            lsn = 0;
            _SharedLogStream->QueryRecord(_RecordAsn,
                                          RvdLogStream::AsyncReadContext::ReadExactRecord,
                                          NULL,
                                          NULL,
                                          NULL,
                                          &lsn);
        }
        
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                            "ProcessWriteCompletion enter", Status, lsn, _RecordAsn.Get(), _Version,
                            (_DataSize ) + (ULONGLONG)(0x100000000));
    }
    
    //
    // Shared and dedicated writes can be completed at the same time but only the first one should
    // complete the caller write and the last one complete this destaging write async for all writes
    // except truncate tail writes. Truncate tail writes are completed to the caller only after both
    // writes complete as moving forward from truncate tail requires both shared and dedicated logs to be
    // in sync.
    //

    KInvariant(_AsyncsOutstanding > 0);

    //
    // Determine if this completion is the first or second. Note that
    // complete this before the  The second completion can race ahead and  
    // first is done. So first completion takes a reference on this to keep it alive until it is done processing.
    //
    asyncsOutstanding = InterlockedDecrement(&_AsyncsOutstanding);
    if (asyncsOutstanding == 1)
    {
        //
        // Take ref for first completion. Released below at routine exit
        //
//        thisPtr = this;  // CONSIDER: activities should keep this alive
    }

    if (! SharedWriteCompleted)
    {
        _DedicatedCompletionStatus = Status;
    }

    if (SharedWriteCompleted && (! NT_SUCCESS(Status)) && (asyncsOutstanding == 1))
    {
        //
        // If the shared write failed for some reason then this is not a fatal situation as the write to
        // dedicated may still succeed. So if the failing shared completion is first then wait for
        // the dedicated completion before completing back to the caller
        //
        //
        // Note that we may want to force a flush on the dedicated at
        // this point since dedicated will not flush until the timer
        // happens which could be 60 seconds or more.
        goto finish;
    }

    if ((NT_SUCCESS(Status)) && (_IsTruncateTailWrite) && (asyncsOutstanding == 1))
    {
        //
        // For truncate tail requests, we only complete back to the caller when
        // both writes complete. This is because both dedicated and
        // shared logs must be in sync with respect to their ASN
        // indexes moving forward.
        //
        goto finish;
    }

    completedToCaller = InterlockedExchange(&_CompletedToCaller, 1);
    
    if (NT_SUCCESS(Status))
    {
        //
        // Success of either shared or dedicated means success for the
        // caller
        //
        if (! completedToCaller)
        {
            //
            // Update logical log tail values
            //
            if (_OverlayStream->IsStreamForLogicalLog())
            {
                if (_IsMarkedEndOfRecord)
                {
                    _OverlayStream->SetLogicalLogTailOffset(_LogicalLogTailOffset);
                } else {
                    _OverlayStream->SetIfLargerLogicalLogTailOffset(_LogicalLogTailOffset);
                }

                _OverlayStream->SetIfLargerLogicalLogLastVersion(_Version);

            }

            _OverlayStream->AddPerfCounterBytesWritten(_DataSize);

            _CallerAsync->CompleteRequest(Status);
            _CallerAsync = nullptr;
        }

        if (_OverlayStream->IsStreamForLogicalLog())
        {
            if (_IsTruncateTailWrite)
            {
                _OverlayStream->DeleteTailTruncatedRecords(TRUE, _RecordAsn);  // SharedLog
                _OverlayStream->DeleteTailTruncatedRecords(FALSE, _RecordAsn); // DedicatedLog
            }
        }
    } else {
        if (! SharedWriteCompleted)
        {
            //
            // Failure of the dedicated write means all future stream
            // writes should fail
            //
            _OverlayStream->SetFailureStatus(Status);

            KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "Failure of the dedicated write means all future stream writes should fail", Status,
                                (ULONGLONG)this,
                                (ULONGLONG)0,
                                (ULONGLONG)0,
                                (ULONGLONG)0);          
        } else {
            //
            // If the shared write failed then dedicated write
            // completion status is the one that counts. And in any
            // case flag that if the shared container has problems any
            // future write is directed to dedicated log
            //
            if (Status != STATUS_LOG_FULL)
            {
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "Failure of the shared write means all future stream writes directed to dedicated", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0,
                                    (ULONGLONG)0);                                
                _OverlayStream->SetWriteOnlyToDedicated(TRUE);
            }
            Status = _DedicatedCompletionStatus;
        }

        if (! completedToCaller)
        {
            _CallerAsync->CompleteRequest(Status);
            _CallerAsync = nullptr;
        }
    }

finish:
    //
    // Second completion should complete this, first completion release reference
    //
    if (asyncsOutstanding == 0) 
    {
        //
        // Both shared and dedicated have completed, so remove the write from the 
        // outstanding write table and then try to truncate the shared log
        //
        Status = _OverlayStream->RemoveDestagingWrite(&_OverlayStream->_OutstandingDedicatedWriteTable, *this);
        if (!NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, 0, 0);
        }

        // Do not truncate if dedicated failed

        if (NT_SUCCESS(_OverlayStream->GetFailureStatus()))
        {
            if (_TruncateOnDedicatedCompletion)
            {
                //
                // Advance the dedicated write completed asn to only what has
                // been contiguously completed. If the list is empty then
                // lowestDestagingLsn will be -1 and so we use the last
                // completed ASN instead.
                //
                _OverlayStream->TruncateSharedStreamIfPossible(_OverlayStream->GetSharedTruncationAsn(),
                                                               _OverlayStream->GetSharedTruncationVersion());
            }

            //
            // Determine if any truncation notifications need to be fired
            // for this stream
            //
            _OverlayStream->CheckThresholdNotifications();
        }
        
        Complete(STATUS_SUCCESS);
        // Do not touch this at this point as it may be gone
    }
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedWriteCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    //
    // Note: This may be running at the same time as the SharedWriteCompletion
    //
    NTSTATUS status = Async.Status();

    _OverlayStream->SubtractDedicatedWriteBytesOutstanding(_DataSize);
    
#ifdef UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "DedicatedCompletion",
                        status, (ULONGLONG)this, _DataSize, _Version, _RecordAsn.Get());
#endif
    
    KInvariant(! _CoalesceIoBuffer);
    
    if (NT_SUCCESS(status))
    {
        _OverlayStream->AddPerfCounterDedicatedBytesWritten(_DataSize);
        
        //
        // When the dedicated write completes we know we can truncate
        // from the shared log
        //
        _OverlayStream->SetIfLargerSharedTruncationAsn(_RecordAsn);

        _OverlayStream->UpdateSharedTruncationVersion(_Version);        
    } else {
        KTraceFailedAsyncRequest(status, this, _Version, _RecordAsn.Get());
    }

    //
    // See if there are any throttled writes that can be started
    //
    _OverlayStream->UnthrottleWritesIfPossible(TRUE);
    
    _CoalesceIoBuffer = nullptr;
    
    ProcessWriteCompletion(FALSE, status);

    //
    // If truncate is pending then do it now
    //
    if (_TruncationPendingAsn != 0)
    {
#ifdef UDRIVER
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "PendingTruncate",
                            status, (ULONGLONG)this, _TruncationPendingAsn.Get(), _Version, _RecordAsn.Get());
#endif
        _OverlayStream->Truncate(_TruncationPendingAsn, _TruncationPendingAsn);
        _TruncationPendingAsn = 0;
    }
    
    //
    // Release activity taken when dedicated write was started. Note
    // that this may cause this to be completed.
    //
    ReleaseActivities();
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::SharedLogFullTimerWaitCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase&
)
{
    if (! _CompletedToCaller)
    {
        KAsyncContextBase::CompletionCallback sharedCompletion(this,
                                                      &OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletion);
        _SharedWrite->Reuse();
        _SharedWrite->StartReservedWrite(FALSE,
                                        0,                 // No reservation
                                        _RecordAsn,
                                        _Version,
                                        _MetaDataBuffer,
                                        _SharedIoBuffer,
                                        this,                       // Run in independent apartment
                                        sharedCompletion);
    } else {
        SharedWriteCompletion2(STATUS_SUCCESS);
    }
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    if ((status == STATUS_LOG_FULL) && (! _CompletedToCaller))
    {
        //
        // When we hit the log full condition, retry if the dedicated
        // write has not yet completed.
        //

        //
        // Set flag that log is full to force quick flushing
        //
        _OverlayStream->SetLastSharedLogStatus(status);
        
        _SharedLogFullRetries++;
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                         "SharedLogFullRetry", status,
                                         (ULONGLONG)_RecordAsn.Get(),
                                         (ULONGLONG)_Version,
                                         (ULONGLONG)0,
                                         (ULONGLONG)_SharedLogFullRetries);

        KAsyncContextBase::CompletionCallback timerWaitCompletion(this,
                                                   &OverlayStream::AsyncDestagingWriteContextOverlay::SharedLogFullTimerWaitCompletion);
        _SharedLogFullTimer->Reuse();
        _SharedLogFullTimer->StartTimer(_SharedLogFullRetryTimerInMs, this, timerWaitCompletion);
        return;
    }

    SharedWriteCompletion2(status);
}

VOID 
OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletion2(
    __in NTSTATUS Status
    )
{
    LONGLONG sharedWriteLatencyTime = KNt::GetPerformanceTime() - _SharedWriteLatencyTimeStart;
    if (NT_SUCCESS(Status))
    {
        _OverlayStream->UpdateSharedWriteLatencyTime(sharedWriteLatencyTime);
    }

    _SharedIoBuffer = nullptr;
    
    _SharedCompletionStatus = Status;
    
    KAsyncContextBase::CompletionCallback sharedCompletion(this,
                                                           &OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletionAfterEventSet);
    
    _DedicatedCoalescedWaitContext->StartWaitUntilSet(this,
                                                      sharedCompletion);
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletionAfterEventSet(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& 
    )
{
    //
    // Note: This may be running at the same time as the DedicatedWriteCompletion
    //    
    NTSTATUS status = _SharedCompletionStatus;

    _OverlayStream->SetLastSharedLogStatus(status);
    
#ifdef UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "SharedCompletion", status,
                        (ULONGLONG)this, _DataSize, _Version, _RecordAsn.Get());
#endif
    
    _OverlayStream->RemoveDestagingWrite(&_OverlayStream->_OutstandingSharedWriteTable, *this);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    } else {
        _OverlayStream->AddPerfCounterSharedBytesWritten(_TotalNumberBytesToWrite);
    }
        
    ProcessWriteCompletion(TRUE, status);

    //
    // Release activity taken when shared write was started.  Note that
    // this may cause this to be completed and disappear
    //
    ReleaseActivities();
}

BOOLEAN
OverlayStream::AsyncDestagingWriteContextOverlay::ShouldSendToSharedStream(
    )
{
    if (_OverlayStream->GetWriteOnlyToDedicated())
    {
        //
        // If stream is configured to ignore the shared container then
        // don't send to the shared stream
        //
        return(FALSE);
    }

    if (_IsTruncateTailWrite)
    {
        //
        // TruncateTail writes need to be sent to both shared and dedicated streams since truncate tail will
        // remove records when completed. In the case where it is only sent to the dedicated, the truncate tail
        // completion could race ahead of previous shared completions. So we get into a case in the core logger where
        // the ASN index entry for the in process shared write is removed while it is pending. The core logger 
        // has an assumption that ASN indexes are only removed if they are persisted disposition.
        //
        // CONSIDER: a different fix. Rather than forcing synchronization in this way, drain all writes and stop
        //           new ones while truncate tail is in progress. This is not needed based on the usage pattern
        //           for the transactional replicator
        //
        return(TRUE);
    }

    if (_RecordAsn <= _OverlayStream->GetSharedTruncationAsn())
    {
        //
        // If the shared stream has been truncated to beyond the ASN
        // then no reason to send to the shared stream
        //
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                            "DontSendToSharedStream", STATUS_SUCCESS,
                            (ULONGLONG)_RecordAsn.Get(),
                            (ULONGLONG)_Version,
                            (ULONGLONG)_OverlayStream->GetSharedTruncationAsn().Get(),
                            (ULONGLONG)0);
        return(FALSE);
    }

    ULONG recordSize = _IoBufferSize + ((_MetaDataBuffer->QuerySize() + (0xfff)) & ~0xfff);
    if (recordSize > _OverlayStream->GetSharedMaximumRecordSize())
    {
        //
        // If the record is larger than the maximum record size of the
        // shared log then don't send it to the shared stream
        //
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                            "DontSendToSharedStream", STATUS_SUCCESS,
                            (ULONGLONG)_RecordAsn.Get(),
                            (ULONGLONG)_Version,
                            (ULONGLONG)recordSize,
                            (ULONGLONG)_OverlayStream->GetSharedMaximumRecordSize());
        return(FALSE);
    }

    //
    // No reason not to send this to the shared stream
    //
    return(TRUE);
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnStart(
)
{
    NTSTATUS status;

    _RequestRefAcquired = FALSE;
    status = _OverlayStream->TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);

        _CompletedToCaller = 1;
        _CallerAsync->CompleteRequest(status);
        _CallerAsync = nullptr;

        _SharedIoBuffer = nullptr;
        _DedicatedIoBuffer = nullptr;
        Complete(status);
        return;
    }

    _RequestRefAcquired = TRUE;

    _TruncateOnDedicatedCompletion = TRUE;
    _TruncationPendingAsn = 0;     // Assume no truncation
    
    _TotalNumberBytesToWrite = NumberBytesForWrite();
    _IoBufferSize = _SharedIoBuffer->QuerySize();

    //
    // If this is a logical log then provide validatation for the
    // incoming write
    //
    if (_OverlayStream->IsStreamForLogicalLog())
    {
        KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
        KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
        ULONG dataSize;
        ULONG dataOffset;

        status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(_MetaDataBuffer,
            _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
            *_SharedIoBuffer,
            sizeof(KtlLogVerify::KtlMetadataHeader),
            metadataBlockHeader,
            streamBlockHeader,
            dataSize,
            dataOffset);
        
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        _DataSize = dataSize;

        //
        // Enable to disallow out of order writes. CoalesceRecords is not able to handle this
        //
        if (_Version != _OverlayStream->GetLogicalLogLastVersion() + 1)
        {
            //
            // Logical log versions must only increase by 1
            //
            status = STATUS_REQUEST_OUT_OF_SEQUENCE;
            KTraceFailedAsyncRequest(status, this, _Version, _OverlayStream->GetLogicalLogLastVersion() + 1);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        if (streamBlockHeader->Signature != KLogicalLogInformation::StreamBlockHeader::Sig)
        {
            //
            // Stream block header signature must be correct
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, streamBlockHeader->Signature, 0);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        if (streamBlockHeader->StreamId.Get() != _OverlayStream->GetStreamId().Get())
        {
            //
            // Stream block header streamid must be correct
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, 0, 0);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        if (streamBlockHeader->StreamOffset != _RecordAsn.Get())
        {
            //
            // Stream block header StreamOffset must be the same as ASN
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, streamBlockHeader->StreamOffset, _RecordAsn.Get());

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }


        if (streamBlockHeader->HighestOperationId != _Version)
        {
            //
            // Stream block header HighestOperationId must be the same
            // as Version
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, streamBlockHeader->HighestOperationId, _Version);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

#if DBG
        //
        // Validate header and data CRC's
        //
        ULONGLONG crc64;

        crc64 = KLogicalLogInformation::ComputeDataBlockCRC(streamBlockHeader, *_SharedIoBuffer, dataOffset);
        if (crc64 != streamBlockHeader->DataCRC64)
        {
            //
            // Data CRC is invalid
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, streamBlockHeader->DataCRC64, crc64);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        KLogicalLogInformation::StreamBlockHeader streamBlockHeaderCopy;

        streamBlockHeaderCopy = *streamBlockHeader;

        crc64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(&streamBlockHeaderCopy);
        if (crc64 != streamBlockHeader->HeaderCRC64)
        {
            //
            // Header CRC is invalid
            //
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, streamBlockHeader->HeaderCRC64, crc64);

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }
#endif

        if (_RecordAsn.Get() > (_OverlayStream->GetLogicalLogTailOffset().Get()))
        {

            //
            // Enable to disallow out of order writes. CoalesceRecords is not able to handle this
            //
            //

            // Next ASN written must be at or before the logical log tail
            //
            status = STATUS_REQUEST_OUT_OF_SEQUENCE;
            KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _OverlayStream->_LogicalLogTailOffset.Get());

            _CompletedToCaller = 1;
            _CallerAsync->CompleteRequest(status);
            _CallerAsync = nullptr;

            _SharedIoBuffer = nullptr;
            _DedicatedIoBuffer = nullptr;
            Complete(status);
            return;
        }

        //
        // If this record is marked as ending a logical log record then map the flag into the offset into the record
        //
        _IsMarkedEndOfRecord = ((metadataBlockHeader->Flags & KLogicalLogInformation::MetadataBlockHeader::IsEndOfLogicalRecord) != 0);
        if (_IsMarkedEndOfRecord)
        {
            metadataBlockHeader->RecordMarkerOffsetPlusOne = streamBlockHeader->DataSize+1;
        }

        _IsTruncateTailWrite = FALSE;
        if (_OverlayStream->IsNextRecordFirstWrite())
        {
            //
            // _IsNextRecordFirstWrite is set for the very first write
            // to the stream. If the dedicated log has a problem then
            // we want to report that as early as possible.
            //
            _IsTruncateTailWrite = TRUE;
            _OverlayStream->ResetIsNextRecordFirstWrite();
        }

        //
        // If this is a truncate tail operation then ensure it is also marked as an end of record
        //
        if (_RecordAsn.Get() < (_OverlayStream->GetLogicalLogTailOffset().Get()))
        {
            if (_IsMarkedEndOfRecord)
            {
                _IsTruncateTailWrite = TRUE;
            } else {
                status = STATUS_INVALID_PARAMETER;
                KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _OverlayStream->GetLogicalLogTailOffset().Get());
                _CompletedToCaller = 1;
                _CallerAsync->CompleteRequest(status);
                _CallerAsync = nullptr;

                _SharedIoBuffer = nullptr;
                _DedicatedIoBuffer = nullptr;
                Complete(status);
                return;
            }
        }

        //
        // Advance the logical log tail
        //
        _LogicalLogTailOffset = _RecordAsn.Get() + dataSize;
        
    } else {
        _DataSize = 0;
        _CoalesceIoBuffer = nullptr;
    }

    //
    // Throttle point for shared log usage
    //
    BOOLEAN continueWrite = _OverlayStream->ThrottleWriteIfNeeded(*this);
    
    if (continueWrite)
    {
        OnStartAllocateBuffer();
    }
}


VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnStartAllocateBuffer(
    )
{    
    if (_OverlayStream->IsStreamForLogicalLog())
    {
        //
        // Allocate space to cache the incoming record. This is the
        // natural memory usage throttle point as waiting here will
        // keep the record from the shared and dedicated log and thus
        // throttle the caller.
        //
        KAsyncContextBase::CompletionCallback allocationCompletion(this,
                                         &OverlayStream::AsyncDestagingWriteContextOverlay::AllocateKIoBufferCompletion);

        ULONG ioBufferSizeNeeded = _CoalesceRecords->ComputeIoBufferNeeded(_DataSize);
        
        if (ioBufferSizeNeeded == 0)
        {
            _CoalesceIoBuffer = nullptr;
            AllocateKIoBufferCompletion(this, *_AllocateKIoBuffer);
            return;
        }

        _AllocateKIoBuffer->StartAllocateKIoBuffer(ioBufferSizeNeeded,
                                                   KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs,
                                                   _CoalesceIoBuffer, this, allocationCompletion);      
    } else {
        OnStartContinue();
    }
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::AllocateKIoBufferCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _AllocateKIoBuffer->GetSize());

        _CompletedToCaller = 1;
        _CallerAsync->CompleteRequest(status);
        _CallerAsync = nullptr;

        _SharedIoBuffer = nullptr;
        _DedicatedIoBuffer = nullptr;
        Complete(status);
        return;        
    }

#if UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "AllocateKIoBuffer", STATUS_SUCCESS,
        (ULONGLONG)_CoalesceIoBuffer ? _CoalesceIoBuffer->QuerySize() : 0,
        (ULONGLONG)_AppendFlushContext.RawPtr(),
        (ULONGLONG)_AllocateKIoBuffer->GetSize(),
        (ULONGLONG)_RecordAsn.Get());      
#endif
    
    OnStartContinue();
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnStartContinue(
    )
{
    ULONGLONG dedicatedReservationNeeded;

    //
    // Reserve enough space in the D log so that we can be sure the
    // write can be destaged. If caller has specified to use its
    // reservation space then there is no need to allocate additional
    //
    dedicatedReservationNeeded = _IoBufferSize + KLoggerUtilities::RoundUpTo4K(_MetaDataBuffer->QuerySize());

    if (_ReserveToUse < dedicatedReservationNeeded)
    {
        dedicatedReservationNeeded = dedicatedReservationNeeded - _ReserveToUse;

        KAsyncContextBase::CompletionCallback reservationCompletion(this,
            &OverlayStream::AsyncDestagingWriteContextOverlay::ReservationCompletion);

        _DedicatedReservation->StartUpdateReservation(dedicatedReservationNeeded,
                                                      this,
                                                      reservationCompletion);
        _ReserveToUse += dedicatedReservationNeeded;
    }
    else 
    {
        ReservationCompletionWorker(STATUS_SUCCESS);
    }
}
    
VOID
OverlayStream::AsyncDestagingWriteContextOverlay::ReservationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    ReservationCompletionWorker(Async.Status());
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::ReservationCompletionWorker(
    __in NTSTATUS Status
    )
{
    if (!NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _OverlayStream->_LogicalLogTailOffset.Get());

        _CompletedToCaller = 1;
        _CallerAsync->CompleteRequest(Status);
        _CallerAsync = nullptr;
        
        _SharedIoBuffer = nullptr;
        _DedicatedIoBuffer = nullptr;

        if (_CoalesceIoBuffer)
        {
#if UDRIVER
            KDbgCheckpointWData(_OverlayStream->GetActivityId(), "FreeKIoBuffer", STATUS_SUCCESS,
                (ULONGLONG)_RecordAsn.Get(),
                (ULONGLONG)this,
                (ULONGLONG)_CoalesceIoBuffer->QuerySize(),
                (ULONGLONG)0);
#endif
            _OverlayStream->FreeKIoBuffer(_OverlayStream->GetActivityId(), _CoalesceIoBuffer->QuerySize());
        }
        _CoalesceIoBuffer = nullptr;
        
        Complete(Status);
        return;
    }

    KAsyncContextBase::CompletionCallback dedicatedFlushCompletion(this,
        &OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedFlushCompletion);

    if ((_OverlayStream->IsStreamForLogicalLog()) && (_IsTruncateTailWrite))
    {
        //
        // Flush must occur before truncate tail write proceeds
        //
        _AppendFlushContext->Reuse();
        _AppendFlushContext->StartFlush(FALSE,   // IsClosing
                                        dedicatedFlushCompletion);
        return;
    }

    DedicatedFlushCompletion(NULL, *this);
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedFlushCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _OverlayStream->GetLogicalLogTailOffset().Get());

        _CompletedToCaller = 1;
        _CallerAsync->CompleteRequest(status);
        _CallerAsync = nullptr;
        
        _SharedIoBuffer = nullptr;
        _DedicatedIoBuffer = nullptr;

        if (_CoalesceIoBuffer)
        {
#if UDRIVER
            KDbgCheckpointWData(_OverlayStream->GetActivityId(), "FreeKIoBuffer", STATUS_SUCCESS,
                (ULONGLONG)_RecordAsn.Get(),
                (ULONGLONG)this,
                (ULONGLONG)_CoalesceIoBuffer->QuerySize(),
                (ULONGLONG)0);
#endif
            _OverlayStream->FreeKIoBuffer(_OverlayStream->GetActivityId(), _CoalesceIoBuffer->QuerySize());
        }
        _CoalesceIoBuffer = nullptr;
        
        Complete(status);
        return;
    }

    _SendToShared = ShouldSendToSharedStream();

    _CallerAsync->SetSendToShared(_SendToShared);

    //
    // Make note of the number of completions to observe
    //
    _AsyncsOutstanding = _SendToShared ? 2 : 1;

    status = _OverlayStream->AddDestagingWrite(_OverlayStream->GetOutstandingDedicatedWriteTable(), *this);
    KInvariant(NT_SUCCESS(status));
        
    if (_SendToShared)
    {
#if DBG
        if (_OverlayStream->GetSharedTimerDelay() != 0)
        {
            //
            // If delay timer for dedicated writes configured then use it
            //
            KAsyncContextBase::CompletionCallback timerWaitCompletion(this,
                                                       &OverlayStream::AsyncDestagingWriteContextOverlay::SharedTimerWaitCompletion);
            _SharedTimer->StartTimer(_OverlayStream->GetSharedTimerDelay(), this, timerWaitCompletion);
        } else {
#endif
            SharedTimerWaitCompletion(NULL, *this);
#if DBG
        }
#endif
    } else {
        _SharedIoBuffer = nullptr;
    }

#if DBG
    if (_OverlayStream->GetDedicatedTimerDelay() != 0)
    {
        //
        // If delay timer for dedicated writes configured then use it
        //
        KAsyncContextBase::CompletionCallback timerWaitCompletion(this,
                                                                  &OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedTimerWaitCompletion);
        _DedicatedTimer->StartTimer(_OverlayStream->GetDedicatedTimerDelay(), this, timerWaitCompletion);
    } else {
#endif
        DedicatedTimerWaitCompletion(NULL, *this);
#if DBG
    }
#endif
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedTimerWaitCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase&
)
{
    KAsyncContextBase::CompletionCallback dedicatedCompletion(this,
                                                   &OverlayStream::AsyncDestagingWriteContextOverlay::DedicatedWriteCompletion);

    //
    // Acquire an activity on the destaging write context to account
    // for the outstanding write to the dedicated log. It is relieved in
    // the completion routine for the dedicated log write after all
    // processing is done by the completion routine.
    //
    AcquireActivities();
    
    if (_OverlayStream->IsStreamForLogicalLog())
    {
#if UDRIVER
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "StartAppend", STATUS_SUCCESS,
                            (ULONGLONG)this, _SendToShared, _IsTruncateTailWrite, _RecordAsn.Get());
#endif

        _OverlayStream->AddDedicatedWriteBytesOutstanding(_DataSize);

        //
        // Passed off to AppendAsync. It is responsible to free the
        // buffer back to the ThrottledAllocator
        //
        KIoBuffer::SPtr coalesceIoBuffer = _CoalesceIoBuffer;
        _CoalesceIoBuffer = nullptr;
        
        _AppendFlushContext->Reuse();
        _AppendFlushContext->StartAppend(*_MetaDataBuffer,
                                         *_DedicatedIoBuffer,
                                         _ReserveToUse,
                                         (!_SendToShared) || _IsTruncateTailWrite || _ForceFlush,
                                         coalesceIoBuffer.RawPtr(),
                                         *this,
                                         dedicatedCompletion);

    }
    else 
    {
        SetDedicatedCoalescedEvent();
        
        _DedicatedWrite->StartReservedWrite(_ReserveToUse,
                                            _RecordAsn,
                                            _Version,
                                            _MetaDataBuffer,
                                            _DedicatedIoBuffer,
                                            this,
                                            dedicatedCompletion);
    }
    _DedicatedIoBuffer = nullptr;
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::SharedTimerWaitCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase&
)
{

#ifdef UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "Start shared write", STATUS_SUCCESS,
                        (ULONGLONG)this, _DataSize, _Version, _RecordAsn.Get());
#endif

    _OverlayStream->AddDestagingWrite(_OverlayStream->GetOutstandingSharedWriteTable(), *this);
    
    KAsyncContextBase::CompletionCallback sharedCompletion(this,
                                                  &OverlayStream::AsyncDestagingWriteContextOverlay::SharedWriteCompletion);
    //
    // Acquire an activity on the destaging write context to account
    // for the outstanding write to the shared log. It is relieved in
    // the completion routine for the shared log write after all
    // processing is done by the completion routine.
    //
    AcquireActivities();

    _SharedWriteLatencyTimeStart = KNt::GetPerformanceTime();

    _ContainerOperation.BeginOperation();
    _SharedWrite->StartReservedWrite(FALSE,
                                    0,                 // No reservation
                                    _RecordAsn,
                                    _Version,
                                    _MetaDataBuffer,
                                    _SharedIoBuffer,
                                    this,                       // Run in independent apartment
                                    sharedCompletion);
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::StartReservedWrite(
    __in ULONGLONG ReserveToUse,
    __in RvdLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in const KBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in BOOLEAN ForceFlush,
    __in AsyncWriteContextOverlay& ParentAsyncContext
    )
{
    _CompletedToCaller = 0;
    _CallerAsync = &ParentAsyncContext;
    
    _ReserveToUse = ReserveToUse;
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataBuffer = MetaDataBuffer;
    _SharedIoBuffer = IoBuffer;
    _DedicatedIoBuffer = IoBuffer;
    _ForceFlush = ForceFlush;

    //
    // The destaging async does keep a ref on the caller async via the SPtr
    // _CallerAsync. The destaging async will completed _CallerAsync on
    // its behalf at the appropriate time. Note that the destaging
    // async may continue to run after the caller async completes
    //
    Start(NULL, nullptr, _OverlayStream->GetGlobalContext().RawPtr());

}

OverlayStream::AsyncDestagingWriteContextOverlay::~AsyncDestagingWriteContextOverlay()
{
}

VOID
OverlayStream::AsyncDestagingWriteContextOverlay::OnReuse(
    )
{
    _DedicatedReservation->Reuse();
    _DedicatedWrite->Reuse();
    _SharedWrite->Reuse();
    _CompletedToCaller = 0;
    _IsTruncateTailWrite = FALSE;
#if DBG
    _SharedTimer->Reuse();
    _DedicatedTimer->Reuse();
#endif
    if (_CoalesceRecords)
    {
        _AppendFlushContext->Reuse();
        _AllocateKIoBuffer->Reuse();
    }
    _DedicatedCoalescedEvent.ResetEvent();
}
    

OverlayStream::AsyncDestagingWriteContextOverlay::AsyncDestagingWriteContextOverlay(
    )
{
}

OverlayStream::AsyncDestagingWriteContextOverlay::AsyncDestagingWriteContextOverlay(
    __in OverlayStream& OStream,
    __in ThrottledKIoBufferAllocator& ThrottledAllocator,
    __in RvdLogStream& SharedLogStream,
    __in RvdLog& DedicatedLogContainer,
    __in RvdLogStream& DedicatedLogStream
    ) :
   _DedicatedCoalescedEvent(FALSE, FALSE),
   _ContainerOperation(*(OStream._OverlayLog->GetInstrumentedComponent()))
{
    NTSTATUS status;
    
    _OverlayStream = &OStream;
    _ThrottledAllocator = &ThrottledAllocator;
    _SharedLogStream = &SharedLogStream;
    _DedicatedLogStream = &DedicatedLogStream;
    _DedicatedLogContainer = &DedicatedLogContainer;

    status = _DedicatedLogStream->CreateAsyncWriteContext(_DedicatedWrite);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    status = _DedicatedLogStream->CreateUpdateReservationContext(_DedicatedReservation);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    status = _SharedLogStream->CreateAsyncWriteContext(_SharedWrite);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }           

    status = _DedicatedCoalescedEvent.CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), _DedicatedCoalescedWaitContext);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }           
    
    _CoalesceRecords = _OverlayStream->_CoalesceRecords;
    if (_CoalesceRecords)
    {
        status = _CoalesceRecords->CreateAsyncAppendOrFlushContext(_AppendFlushContext);
        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }

        status = _ThrottledAllocator->CreateAsyncAllocateKIoBufferContext(_AllocateKIoBuffer);
        if (!NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }       
    }
    else
    {
        _AppendFlushContext = nullptr;
    }

    _SharedLogFullRetries = 0;
    _CompletedToCaller = 0;
    _IsTruncateTailWrite = FALSE;

    status = KTimer::Create(_SharedLogFullTimer, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
#if DBG
    //
    // By default there is no delays
    //
    status = KTimer::Create(_SharedTimer, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = KTimer::Create(_DedicatedTimer, GetThisAllocator(), GetThisAllocationTag());
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
#endif
}

NTSTATUS
OverlayStream::CreateAsyncDestagingWriteContext(
    __out AsyncDestagingWriteContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncDestagingWriteContextOverlay::SPtr context;

    Context = nullptr;
    
    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDestagingWriteContextOverlay(*this,
                                                                                                 *_ThrottledAllocator,
                                                                                                 *_SharedLogStream,
                                                                                                 *_DedicatedLogContainer,
                                                                                                 *_DedicatedLogStream);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

NTSTATUS OverlayStream::AsyncDestagingWriteContextOverlay::Create(
    __out AsyncDestagingWriteContextOverlay::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in KActivityId ActivityId
    )
{
    NTSTATUS status;
    OverlayStream::AsyncDestagingWriteContextOverlay::SPtr context;

    Context = nullptr;
    
    context = _new(AllocationTag, Allocator) AsyncDestagingWriteContextOverlay();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( ActivityId, status, 0, 0, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}


NTSTATUS
OverlayStream::AddDestagingWrite(
    __in KNodeTable<AsyncDestagingWriteContextOverlay>* WriteTable,
    __in AsyncDestagingWriteContextOverlay& DestagingWrite
)
{
    K_LOCK_BLOCK(_Lock)
    {
        if (WriteTable->Insert(DestagingWrite))
        {
            //
            // Entry added, take a refcount on the entry to keep it
            // alive while it is in the table. Ref is released when
            // removed from the table in OverlayStream::RemoveDestagingWrite()
            //
            DestagingWrite.AddRef();
            return(STATUS_SUCCESS);
        }
    }
    
    return(STATUS_OBJECT_NAME_COLLISION);
}

NTSTATUS
OverlayStream::RemoveDestagingWrite(
    __in KNodeTable<AsyncDestagingWriteContextOverlay>* WriteTable,
    __in AsyncDestagingWriteContextOverlay& DestagingWrite
)
{    
    K_LOCK_BLOCK(_Lock)
    {
        AsyncDestagingWriteContextOverlay* entry;
        entry = WriteTable->Lookup(DestagingWrite);
        
        if (entry == NULL)
        {
            return(STATUS_NOT_FOUND);
        }

        WriteTable->Remove(DestagingWrite);

        //
        // Release the reference taken when the entry was added to the
        // table in OverlayStream::AddDestagingWrite()
        //
        DestagingWrite.Release();
    }
    
    return(STATUS_SUCCESS);
}

RvdLogAsn
OverlayStream::GetLowestDestagingWriteAsn(
    __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable
    )
{
    AsyncDestagingWriteContextOverlay* destagingWrite;

    K_LOCK_BLOCK(_Lock)
    {
        destagingWrite = WriteTable.First();
        if (destagingWrite != NULL)
        {
            return(destagingWrite->GetRecordAsn());
        }
    }

    return(RvdLogAsn::Max());
}

BOOLEAN
OverlayStream::IsAsnInDestagingList(
    __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable,
    __in RvdLogAsn Asn
    )
{
    AsyncDestagingWriteContextOverlay* destagingWrite = NULL;

    K_LOCK_BLOCK(_Lock)
    {
        _WriteTableLookupKey->SetRecordAsn(Asn);
        destagingWrite = WriteTable.Lookup(*_WriteTableLookupKey);
    }

    return(destagingWrite != NULL);
}

RvdLogAsn
OverlayStream::SetTruncationPending(
    __in KNodeTable<AsyncDestagingWriteContextOverlay>& WriteTable,
    __in RvdLogAsn TruncationPendingAsn
    )
{
    AsyncDestagingWriteContextOverlay* destagingWrite = NULL;

    K_LOCK_BLOCK(_Lock)
    {
        destagingWrite = WriteTable.First();
        while (destagingWrite != NULL)
        {
            if (destagingWrite->GetRecordAsn() > TruncationPendingAsn)
            {
                destagingWrite->_TruncationPendingAsn = TruncationPendingAsn;
                return(destagingWrite->GetRecordAsn());
            }

            destagingWrite = WriteTable.Next(*destagingWrite);
        }       
    }

    return(0);
}

//
// ReadContext
//
VOID
OverlayStream::AsyncReadContextOverlay::OnCompleted(
)
{
    if (_ActualRecordAsnPtr != NULL)
    {
        *_ActualRecordAsnPtr = _ActualRecordAsnLocal;
    }

    if (_VersionPtr != NULL)
    {
        *_VersionPtr = _VersionLocal;
    }
    
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
}

NTSTATUS
OverlayStream::AsyncReadContextOverlay::DetermineRecordLocation(
    __out BOOLEAN& ReadFromDedicated,
    __out RvdLogAsn& AsnToRead,
    __out RvdLogStream::AsyncReadContext::ReadType& ReadType
)
{
    NTSTATUS status;
    ReadFromDedicated = FALSE;
    AsnToRead = 0;
    ReadType = RvdLogStream::AsyncReadContext::ReadContainingRecord;
    
    //
    // First step is to get the information about the records in the
    // dedicated and shared logs
    //
    RvdLogAsn dedicatedAsn = RvdLogAsn::Null();;
    RvdLogAsn sharedAsn = RvdLogAsn::Null();
    ULONGLONG sharedVersion = 0;
    ULONGLONG dedicatedVersion = 0;
    RvdLogStream::RecordDisposition sharedDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    RvdLogStream::RecordDisposition dedicatedDisposition = RvdLogStream::RecordDisposition::eDispositionNone;

    RvdLogAsn dedicatedExactAsn = RvdLogAsn::Null();;
    RvdLogAsn sharedExactAsn = RvdLogAsn::Null();
    ULONGLONG sharedExactVersion = 0;
    ULONGLONG dedicatedExactVersion = 0;
    RvdLogStream::RecordDisposition sharedExactDisposition = RvdLogStream::RecordDisposition::eDispositionNone;
    RvdLogStream::RecordDisposition dedicatedExactDisposition = RvdLogStream::RecordDisposition::eDispositionNone;

    KInvariant((_ReadType == RvdLogStream::AsyncReadContext::ReadContainingRecord) ||
               (_ReadType == RvdLogStream::AsyncReadContext::ReadExactRecord) ||
               (_ReadType == RvdLogStream::AsyncReadContext::ReadNextRecord) ||
               (_ReadType == RvdLogStream::AsyncReadContext::ReadPreviousRecord));

    if (_ReadType != RvdLogStream::AsyncReadContext::ReadContainingRecord)
    {
        //
        // For ReadExact, ReadNext and ReadPrev see if the passed asn
        // is in either of the streams
        //      
        sharedExactAsn = _RecordAsn;

        status = _OverlayStream->QueryRecordAndVerifyCompleted(TRUE,     // SharedLog
                                               sharedExactAsn,
                                               RvdLogStream::AsyncReadContext::ReadExactRecord,
                                               &sharedExactVersion,
                                               &sharedExactDisposition,
                                               NULL,
                                               NULL);
        KInvariant(NT_SUCCESS(status));
        
        dedicatedExactAsn = _RecordAsn;
        status = _OverlayStream->QueryRecordAndVerifyCompleted(FALSE,   // DedicatedLog
                                               dedicatedExactAsn,
                                               RvdLogStream::AsyncReadContext::ReadExactRecord,
                                               &dedicatedExactVersion,
                                               &dedicatedExactDisposition,
                                               NULL,
                                               NULL);
        KInvariant(NT_SUCCESS(status));

#ifdef VERBOSE
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "Exact",
                            (NTSTATUS)_RecordAsn.Get(), (ULONGLONG)sharedExactDisposition,
                            sharedExactVersion, (ULONGLONG)dedicatedExactDisposition, dedicatedExactVersion);
#endif
        if ((sharedExactDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted) &&
            (dedicatedExactDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted))
        {
            //
            // Specific ASN is not in either stream, return not found
            //
            
#ifdef VERBOSE
            KDbgCheckpointWData(_OverlayStream->GetActivityId(), "NotInEitherStream", STATUS_NOT_FOUND,
                                (ULONGLONG)this, _RecordAsn.Get(), 0, 0);
#endif
            return(STATUS_NOT_FOUND);
        }
    }

    if (_ReadType != RvdLogStream::AsyncReadContext::ReadExactRecord)
    {
        //
        // Perform the appropriate QueryRecord _ReadType for ReadNext,
        // ReadPrev and ReadContaining. This query is more forgiving in
        // that it tolerate ASN holes (ie, missing records) in the
        // stream.  Note that the ReadExact was done above already.
        //
        sharedAsn = _RecordAsn;
        status = _OverlayStream->QueryRecordAndVerifyCompleted(TRUE,       // SharedLog
                                               sharedAsn,
                                               _ReadType,
                                               &sharedVersion,
                                               &sharedDisposition,
                                               NULL,
                                               NULL);
        KInvariant(NT_SUCCESS(status));
        
        dedicatedAsn = _RecordAsn;
        status = _OverlayStream->QueryRecordAndVerifyCompleted(FALSE,
                                               dedicatedAsn,
                                               _ReadType,
                                               &dedicatedVersion,
                                               &dedicatedDisposition,
                                               NULL,
                                               NULL);
        KInvariant(NT_SUCCESS(status));
        
#ifdef VERBOSE
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ByReadType", _ReadType,
                            (ULONGLONG)sharedDisposition, sharedAsn.Get(), (ULONGLONG)dedicatedDisposition, dedicatedAsn.Get());
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ByReadType",
                            _ReadType, (ULONGLONG)sharedDisposition, sharedVersion, (ULONGLONG)dedicatedDisposition, dedicatedVersion);
#endif
    } else {
        sharedAsn = sharedExactAsn;
        sharedDisposition = sharedExactDisposition;
        sharedVersion = sharedExactVersion;
        dedicatedAsn = dedicatedExactAsn;
        dedicatedDisposition = dedicatedExactDisposition;
        dedicatedVersion = dedicatedExactVersion;
    }

    //
    // Determine if the record exists in one log stream or another or
    // neither
    //
    if (sharedDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted)
    {
        if (dedicatedDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted)
        {
            //
            // Check for special cases where you perform ReadNext and the previous is in the dedicated and next in the shared
            // or the case where you perform ReadPrevious and the previous is in the dedicated and next in the shared.
            //
            if ((_ReadType == RvdLogStream::AsyncReadContext::ReadNextRecord) ||
                (_ReadType == RvdLogStream::AsyncReadContext::ReadPreviousRecord))
            {
                if ((_ReadType == RvdLogStream::AsyncReadContext::ReadNextRecord) &&
                    (dedicatedExactDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted) &&
                    (sharedExactDisposition == RvdLogStream::RecordDisposition::eDispositionNone))
                {
                    //
                    // ReadNext where the specified record is in the dedicated and the next record may be in the 
                    // shared.
                    //
                    ReadFromDedicated = FALSE;
                    AsnToRead = _RecordAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadNextFromSpecificAsn;
#ifdef VERBOSE
                    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadNextFromSpecific",
                                        STATUS_SUCCESS, (ULONGLONG)AsnToRead.Get(), 0, 0, 0);
#endif
                    return(STATUS_SUCCESS);
                }

                if ((_ReadType == RvdLogStream::AsyncReadContext::ReadPreviousRecord) &&
                    (dedicatedExactDisposition == RvdLogStream::RecordDisposition::eDispositionNone) &&
                    (sharedExactDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted))
                {
                    //
                    // ReadPrevious where the specified record is in the shared and the previous record
                    // may be in the dedicated
                    //
                    ReadFromDedicated = TRUE;
                    AsnToRead = _RecordAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadPreviousFromSpecificAsn;
#ifdef VERBOSE
                    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadPreviousFromSpecific",
                                        STATUS_SUCCESS, (ULONGLONG)AsnToRead.Get(), 0, 0, 0);
#endif
                    return(STATUS_SUCCESS);
                }
            }

            //
            // Record is not in either streams, return error not found
            //
            return(STATUS_NOT_FOUND);
        } else {
            //
            // Record is in the dedicated stream but not the shared
            //
            ReadFromDedicated = TRUE;
            AsnToRead = dedicatedAsn;
            ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
#ifdef VERBOSE
            KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadFromDedicated",
                                STATUS_SUCCESS, (ULONGLONG)AsnToRead.Get(), 0, 0, 0);
#endif
            return(STATUS_SUCCESS);
        }

    } else if (dedicatedDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted) {
        //
        // Record is in the shared stream but not the dedicated
        //
        ReadFromDedicated = FALSE;
        AsnToRead = sharedAsn;
        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
#ifdef VERBOSE
        KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadFromShared", STATUS_SUCCESS,
                            (ULONGLONG)AsnToRead.Get(), 0, 0, 0);
#endif
        return(STATUS_SUCCESS);
    }

    //
    // If the record is in both log streams then there is specific
    // logic to figure the right one to pick
    //
    if (dedicatedAsn != sharedAsn)
    {
        switch (_ReadType)
        {
            case RvdLogStream::AsyncReadContext::ReadExactRecord:
            {
                //
                // dedicatedAsn and sharedAsn would always be equal
                //
                KInvariant(FALSE);
                break;
            }

            case RvdLogStream::AsyncReadContext::ReadNextRecord:
            {
                KInvariant(dedicatedAsn >= _RecordAsn);
                KInvariant(sharedAsn >= _RecordAsn);

                if (dedicatedAsn < sharedAsn)
                {
                    //
                    // The dedicated record is closer to the queried
                    // record so that is the one
                    //
                    AsnToRead = dedicatedAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                    ReadFromDedicated = TRUE;
                } else {
                    //
                    // The shared record is closer to the queried
                    // record so that is the one
                    //
                    AsnToRead = sharedAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                    ReadFromDedicated = FALSE;
                }

#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadNextRecord", STATUS_SUCCESS,
                                    (ULONGLONG)AsnToRead.Get(), (ULONGLONG)ReadFromDedicated, sharedAsn.Get(), dedicatedAsn.Get());
#endif              
                return(STATUS_SUCCESS);
            }

            case RvdLogStream::AsyncReadContext::ReadPreviousRecord:
            {
                KInvariant(dedicatedAsn <= _RecordAsn);
                KInvariant(sharedAsn <= _RecordAsn);

                if (dedicatedAsn > sharedAsn)
                {
                    //
                    // The dedicated record is closer to the queried
                    // record so that is the one
                    //
                    AsnToRead = dedicatedAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                    ReadFromDedicated = TRUE;
                } else {
                    //
                    // The shared record is closer to the queried
                    // record so that is the one
                    //
                    AsnToRead = sharedAsn;
                    ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                    ReadFromDedicated = FALSE;
                }

#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadPreviousRecord", STATUS_SUCCESS,
                                    (ULONGLONG)AsnToRead.Get(), (ULONGLONG)ReadFromDedicated, sharedAsn.Get(), dedicatedAsn.Get());
#endif
                return(STATUS_SUCCESS);
            }

            case RvdLogStream::AsyncReadContext::ReadContainingRecord:
            {
                if (sharedVersion == dedicatedVersion)
                {
                    //
                    // If the versions are the same then pick the largest
                    // one as this is the one containing the ASN
                    //
                    if (dedicatedAsn > sharedAsn)
                    {
                        //
                        // The dedicated record is closer to the queried
                        // record so that is the one
                        //
                        AsnToRead = dedicatedAsn;
                        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                        ReadFromDedicated = TRUE;
                    } else {
                        //
                        // The shared record is closer to the queried
                        // record so that is the one
                        //
                        AsnToRead = sharedAsn;
                        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                        ReadFromDedicated = FALSE;
                    }

                } else {
                    //
                    // Otherwise pick the one with the highest version.
                    // Once a version changes then every ASN after the
                    // version change is the higher version
                    //
                    if (sharedVersion > dedicatedVersion)
                    {
                        AsnToRead = sharedAsn;
                        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                        ReadFromDedicated = FALSE;
                    } else {
                        AsnToRead = dedicatedAsn;
                        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
                        ReadFromDedicated = TRUE;
                    }
                }
#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(), "ReadContainingRecord", STATUS_SUCCESS,
                                    (ULONGLONG)AsnToRead.Get(), (ULONGLONG)ReadFromDedicated, sharedAsn.Get(), dedicatedAsn.Get());
#endif
                return(STATUS_SUCCESS);
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    }

    //
    // Records have identical ASNs so whichever one has the latest
    // version wins. In the case of identical version the read is from
    // the dedicated.
    //
    if (sharedVersion > dedicatedVersion)
    {
        AsnToRead = sharedAsn;
        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
        ReadFromDedicated = FALSE;
    } else {
        AsnToRead = dedicatedAsn;
        ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
        ReadFromDedicated = TRUE;
    }

#ifdef VERBOSE
    KDbgCheckpointWData(_OverlayStream->GetActivityId(), "IdenticalAsn", STATUS_SUCCESS,
                        (ULONGLONG)AsnToRead.Get(), (ULONGLONG)ReadFromDedicated, sharedVersion, dedicatedVersion);
#endif
    return(STATUS_SUCCESS);
}

NTSTATUS OverlayStream::AsyncReadContextOverlay::TrimLogicalLogRecord(
    __out ULONGLONG& RecordStreamOffset,
    __out ULONG& RecordFullDataSize,
    __out ULONG& RecordTrimmedDataSize
    )
{
    NTSTATUS status;    
    KLogicalLogInformation::MetadataBlockHeader* metadataBlockHeader;
    KLogicalLogInformation::StreamBlockHeader* streamBlockHeader;
    ULONG dataSizeRead;
    ULONG ioDataOffset;
    ULONGLONG nextVersion;
    RvdLogAsn nextRecordAsn;
    RvdLogStream::RecordDisposition nextDisposition;
    ULONGLONG actualRecordAsn = _ActualRecordAsnLocal.Get();
    ULONGLONG endOfRecordAsn;

    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(*_MetaDataBuffer,
                                          _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
                                          *(*_IoBuffer),
                                          sizeof(KtlLogVerify::KtlMetadataHeader),
                                          metadataBlockHeader,
                                          streamBlockHeader,
                                          dataSizeRead,
                                          ioDataOffset);


    if (! NT_SUCCESS(status))
    {
        return(status);
    }
                    
    RecordFullDataSize = dataSizeRead;
    
    //
    // Get the record with the highest version that follows the one read to see if any trimming is needed
    //
    endOfRecordAsn = actualRecordAsn + dataSizeRead;

    nextRecordAsn = _ActualRecordAsnLocal;

    do
    {
        status = _DedicatedLogStream->QueryRecord(nextRecordAsn,
                                                  RvdLogStream::AsyncReadContext::ReadNextRecord,
                                                  &nextVersion,
                                                  &nextDisposition,
                                                  NULL,
                                                  NULL);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        if ((nextDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted) &&
            (nextVersion > _VersionLocal))
        {
            //
            // If there is a record with a higher version whose asn is within our record range
            // then that new record effectively truncates our record
            //
            if (endOfRecordAsn > nextRecordAsn.Get())
            {
                endOfRecordAsn = nextRecordAsn.Get();
            }
            break;
        }
    } while ((nextDisposition != RvdLogStream::RecordDisposition::eDispositionNone) &&
             (nextVersion < _VersionLocal));
    //
    // if the record read goes beyond the tail then trim the end of
    // the record
    //
    if (endOfRecordAsn > _LogicalLogTailOffset)
    {
        endOfRecordAsn = _LogicalLogTailOffset;
    }

    if ((actualRecordAsn + dataSizeRead) > endOfRecordAsn)
    {
        //
        // We need to trim the record read. 
        //
        // CONSIDER: Mute these traces once read is stabilized
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "TrimLogicalLogRecord TrimRecord", status,
                            (ULONGLONG)this, actualRecordAsn, endOfRecordAsn, dataSizeRead);
        
        KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "TrimLogicalLogRecord TrimRecord2", status,
                            (ULONGLONG)this, actualRecordAsn, endOfRecordAsn, _LogicalLogTailOffset);
        
        streamBlockHeader->DataSize -=  (ULONG)((actualRecordAsn + dataSizeRead) - endOfRecordAsn);
#if DBG
        //
        // recompute checksums on checked builds only
        //
        ULONGLONG crc64 = KLogicalLogInformation::ComputeDataBlockCRC(streamBlockHeader, *(*_IoBuffer), ioDataOffset);
        streamBlockHeader->DataCRC64 = crc64;
        
        crc64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(streamBlockHeader);
        streamBlockHeader->HeaderCRC64 = crc64;     
#endif
        if (metadataBlockHeader->OffsetToStreamHeader >= KLogicalLogInformation::FixedMetadataSize)
        {
            //
            // If the stream block header was in the data block then
            // add verification information to the metadata header
            //
            KtlLogVerify::KtlMetadataHeader* metaDataHeader =
                                     static_cast<KtlLogVerify::KtlMetadataHeader*>((*_MetaDataBuffer)->GetBuffer());

            KIoBuffer::SPtr notUsed = nullptr;
            KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                        _ActualRecordAsnLocal,
                                                        _VersionLocal,
                                                        0,
                                                        notUsed,
                                                        *_IoBuffer);
        }
    }

    RecordStreamOffset = streamBlockHeader->StreamOffset;
    RecordTrimmedDataSize = streamBlockHeader->DataSize;
    
    return(status);
}

VOID
OverlayStream::AsyncReadContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncReadContextOverlay::OperationCompletion);

#ifdef VERBOSE
    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), 
        "OverlayStream::AsyncReadContextOverlay::FSMContinue", Status, _State, (ULONGLONG)this, _RecordAsn.Get(), _ReadType);
#endif

    switch (_State)
    {
        case Initial:
        {
            BOOLEAN readFromDedicated;

            //
            // If this is a read for the logical log type then only
            // allow read containing and verify that the read is not
            // beyond the tail
            //
            if (_OverlayStream->IsStreamForLogicalLog())
            {
                ULONGLONG logicalLogTailOffset = _OverlayStream->_LogicalLogTailOffset.Get();
                _LogicalLogTailOffset = logicalLogTailOffset;

                if (_RecordAsn.Get() >= _LogicalLogTailOffset)
                {
                    Status = STATUS_NOT_FOUND;
                    KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _LogicalLogTailOffset);
                    Complete(Status);
                    return;                 
                }                
            }
            
            //
            // First step is to figure out from where to read the
            // record
            //
            RvdLogAsn asnToRead = _AsnToRead;
            RvdLogStream::AsyncReadContext::ReadType readType = _ReadType;

            Status = DetermineRecordLocation(readFromDedicated,
                                             asnToRead,
                                             readType
                                            );
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, _RecordAsn.Get());
                Complete(Status);
                return;
            }

            KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "DeterminedRecordLocation", Status,
                                             (ULONGLONG)this, asnToRead.Get(), _ReadType, (ULONGLONG)readFromDedicated);

            if (_ReadFromDedicated)
            {
                *_ReadFromDedicated = readFromDedicated;
            }

            if (! readFromDedicated)
            {
                //
                // Try to read this from the shared log
                //
               _State = ReadFromShared;

                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "StartReadFromShared", Status,
                                                 (ULONGLONG)this, asnToRead.Get(), readType, (ULONGLONG)readFromDedicated);

                _SharedRead->StartRead(asnToRead,
                                       readType,
                                       &_ActualRecordAsnLocal,
                                       &_VersionLocal,
                                       *_MetaDataBuffer,
                                       *_IoBuffer,
                                       this,
                                       completion);
            } else {
                //
                // Read from the dedicated
                //              
                _State = ReadFromDedicated;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "StartReadFromDedicated", Status,
                                                 (ULONGLONG)this, asnToRead.Get(), readType, (ULONGLONG)readFromDedicated);

                _DedicatedRead->StartRead(asnToRead,
                                          readType,
                                          &_ActualRecordAsnLocal,
                                          &_VersionLocal,
                                          *_MetaDataBuffer,
                                          *_IoBuffer,
                                          this,
                                          completion);
            }
            
            break;
        }

        case ReadFromShared:
        {
            //
            // If the read from shared failed it could be that the
            // record was truncated underneath us. In this case retry
            // from the dedicated. 
            //
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _ReadType, _AsnToRead.Get());

                RvdLogAsn asnToRead = _AsnToRead;
                RvdLogStream::AsyncReadContext::ReadType readType = _ReadType;
                BOOLEAN readFromDedicated;

                Status = DetermineRecordLocation(readFromDedicated,
                                                 asnToRead,
                                                 readType
                                                );
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, readType, _RecordAsn.Get());
                    Complete(Status);
                    return;
                }

                if (! readFromDedicated)
                {
                    //
                    // Something is strange here as the read from
                    // shared failed and so it is expected to be in the
                    // dedicated.
                    //
                    KTraceFailedAsyncRequest(STATUS_NONEXISTENT_SECTOR, this, readType, asnToRead.Get());                   
                }               
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "DeterminedRecordLocation", Status,
                                                 (ULONGLONG)this, _AsnToRead.Get(), asnToRead.Get(), readType);
                
                _State = ReadFromDedicated;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "StartReadFromDedicated", Status,
                                                 (ULONGLONG)this, asnToRead.Get(), readType, (ULONGLONG)readFromDedicated);
                _DedicatedRead->StartRead(asnToRead,
                                          readType,
                                          &_ActualRecordAsnLocal,
                                          &_VersionLocal,
                                          *_MetaDataBuffer,
                                          *_IoBuffer,
                                          this,
                                          completion);
                break;
            }

            _State = VerifyRecord;
            goto VerifyRecord;
        }
        
        case ReadFromDedicated:
        {
            //
            // pass whatever status we got from underlying read back up
            //
#ifdef VERBOSE
            KDbgCheckpointWData(_OverlayStream->GetActivityId(), "CompleteReadFromDedicated", Status,
                                (ULONGLONG)this, _AsnToRead.Get(), (ULONGLONG)0, 0);
#endif
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), 0);
                Complete(Status);
                return;
            } else {
                _State = VerifyRecord;
                // fall through
            }
        }

        case VerifyRecord:
        {
VerifyRecord:
            //
            // Validate data read
            //
            Status = KtlLogVerify::VerifyRecordCallback((UCHAR const * const)(*_MetaDataBuffer)->GetBuffer(),
                                                            (*_MetaDataBuffer)->QuerySize(),
                                                            *_IoBuffer);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, 0, 0);
                Complete(Status);
                break;
            }
            
            Complete(Status);
            break;
        }

        case InitialCoalesce:
        {           
InitialCoalesce:
            //
            // If this is a read for the logical log type then only
            // allow read containing and verify that the read is not
            // beyond the tail
            //
            KInvariant(_OverlayStream->IsStreamForLogicalLog());
            
            ULONGLONG logicalLogTailOffset = _OverlayStream->_LogicalLogTailOffset.Get();
            _LogicalLogTailOffset = logicalLogTailOffset;

            if (_RecordAsn.Get() >= _LogicalLogTailOffset)
            {
                Status = STATUS_NOT_FOUND;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "Read Past LogicalLogTailOffset", Status,
                                                 (ULONGLONG)this, _RecordAsn.Get(), _ReadType, _LogicalLogTailOffset);
                Complete(Status);
                return;                 
            }                

            if ((_ReadType == RvdLogStream::AsyncReadContext::ReadType::ReadContainingRecord) ||
                (_ReadType == RvdLogStream::AsyncReadContext::ReadType::ReadExactRecord))
            {
                RvdLogAsn lowOnDiskAsn;
                RvdLogAsn highOnDiskAsn;

                Status = _DedicatedLogStream->QueryRecordRange(&lowOnDiskAsn, &highOnDiskAsn, NULL);
                KInvariant(NT_SUCCESS(Status));

                if (_RecordAsn.Get() < lowOnDiskAsn.Get())
                {
                    Status = STATUS_NOT_FOUND;
                    KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), lowOnDiskAsn.Get());
                    Complete(Status);
                    return;                 
                }

                //
                // First look in the coalesce records
                //
                _State = ReadFromCoalesceBuffersExactContaining;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "StartReadFromCoalesceExactContaining", Status,
                                                 (ULONGLONG)this, _RecordAsn.Get(), _ReadType, (ULONGLONG)0);
                _CoalesceRead->StartRead(_RecordAsn,
                                         _ReadType,
                                         &_ActualRecordAsnLocal,
                                         &_VersionLocal,
                                         *_MetaDataBuffer,
                                         *_IoBuffer,
                                         completion);

                
            } else {
                //
                // Only ReadExact and ReadContaining are supported
                KInvariant(FALSE);
            }
            break;
        }

        case ReadFromCoalesceBuffersExactContaining:
        {
            if (! NT_SUCCESS(Status))
            {
                _State = ReadFromCoalesceDedicatedExactContaining;

                // CONSIDER: Remove when stabilized
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "StartReadFromDedicatedExactContaining", Status,
                                                 (ULONGLONG)this, _RecordAsn.Get(), _ReadType, _RecordAsn.Get());

                _DedicatedRead->StartRead(_RecordAsn,
                                          _ReadType,
                                          &_ActualRecordAsnLocal,
                                          &_VersionLocal,
                                          *_MetaDataBuffer,
                                          *_IoBuffer,
                                          this,
                                          completion);
                break;
            }

            if (_ReadFromDedicated != NULL)
            {
                *_ReadFromDedicated = FALSE;
            }
            
            //
            // Validate data read. Since this came from the coalesce
            // buffers there is no need to check for tail truncation.
            //
            Status = KtlLogVerify::VerifyRecordCallback((UCHAR const * const)(*_MetaDataBuffer)->GetBuffer(),
                                                            (*_MetaDataBuffer)->QuerySize(),
                                                            *_IoBuffer);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _ActualRecordAsnLocal.Get());
            }
            
            Complete(Status);
            break;
        }
        
        case ReadFromCoalesceDedicatedExactContaining:
        {
            if (! NT_SUCCESS(Status))
            {
                //
                // If we failed there may have been a race by the time
                // we read, it was written out.  So in this case retry up to the number of retries.
                //
                if (_TryAgainCounter++ < _TryAgainRetries)
                {
                    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "RetryRead", Status,
                                                     (ULONGLONG)this, _RecordAsn.Get(), _ReadType, _TryAgainCounter);
                    _DedicatedRead->Reuse();
                    _CoalesceRead->Reuse();
                    goto InitialCoalesce;
                }
                
                KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _LogicalLogTailOffset);
                Complete(Status);
                return;
            }

            
            if (_ReadFromDedicated)
            {
                *_ReadFromDedicated = TRUE;
            }

            //
            // Validate data read
            //
            Status = KtlLogVerify::VerifyRecordCallback((UCHAR const * const)(*_MetaDataBuffer)->GetBuffer(),
                                                            (*_MetaDataBuffer)->QuerySize(),
                                                            *_IoBuffer);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _ActualRecordAsnLocal.Get());
                Complete(Status);
                break;
            }

            ULONGLONG recordStreamOffset;
            ULONG recordFullDataSize, recordTrimmedDataSize;

            Status = TrimLogicalLogRecord(recordStreamOffset,
                                          recordFullDataSize,
                                          recordTrimmedDataSize);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _RecordAsn.Get(), _ActualRecordAsnLocal.Get());
                Status = K_STATUS_LOG_STRUCTURE_FAULT;
            }

            if (_RecordAsn.Get() >= (recordStreamOffset + (ULONGLONG)recordFullDataSize))
            {
                //
                // Determine if the read was for data beyond the end of
                // the last record. In this case we try again
                //
                Status = STATUS_NOT_FOUND;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "RetryRead: out of range", Status,
                                    (ULONGLONG)this, _RecordAsn.Get(), recordStreamOffset, (ULONGLONG)recordFullDataSize);

                if (_TryAgainCounter++ < _TryAgainRetries)
                {
                    _DedicatedRead->Reuse();
                    _CoalesceRead->Reuse();
                    goto InitialCoalesce;
                }
            } else if (_RecordAsn.Get() > (recordStreamOffset + (ULONGLONG)recordTrimmedDataSize)) {
                //
                // Determine if the record was trimmed so much that
                // there is no data being returned. If this is the case
                // then there was likely a race condition and we try
                // again. We will only try a few times as we do not
                // want to get into an infinite loop
                //
                Status = STATUS_NOT_FOUND;
                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(), "RetryRead: Trimmed out of range", Status,
                                    (ULONGLONG)this, _RecordAsn.Get(), recordStreamOffset, (ULONGLONG)recordTrimmedDataSize);
                if (_TryAgainCounter++ < _TryAgainRetries)
                {
                    _DedicatedRead->Reuse();
                    _CoalesceRead->Reuse();
                    goto InitialCoalesce;
                }
            }               

            Complete(Status);
            break;
        }
        

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncReadContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _RecordAsn.Get());
    _DedicatedRead->Cancel();
    _SharedRead->Cancel();
    if (_CoalesceRead)
    {
        _CoalesceRead->Cancel();
    }
}

VOID
OverlayStream::AsyncReadContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncReadContextOverlay::OnStart(
    )
{
    NTSTATUS status;

    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;

    _TryAgainCounter = 0;
    _State = _CoalesceRead ? InitialCoalesce : Initial;

    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncReadContextOverlay::StartRead(
    __in RvdLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    if (Version != NULL)
    {
        *Version = 0;
    }
    
    _ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
    _RecordAsn = RecordAsn;
    _ActualRecordAsnPtr = NULL;
    _VersionPtr = Version;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;
    _ReadFromDedicated = NULL;
    
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncReadContextOverlay::StartRead(
    __in RvdLogAsn RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt RvdLogAsn* const ActualRecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    if (ActualRecordAsn != NULL)
    {
        *ActualRecordAsn = RvdLogAsn::Null();
    }

    if (Version != NULL)
    {
        *Version = 0;
    }

    MetaDataBuffer = nullptr;
    IoBuffer = nullptr;
    
    _ReadType = Type;
    _RecordAsn = RecordAsn;
    _ActualRecordAsnPtr = ActualRecordAsn;
    _VersionPtr = Version;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    _ReadFromDedicated = NULL;
    
    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayStream::AsyncReadContextOverlay::StartRead(
    __in RvdLogAsn RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt RvdLogAsn* const ActualRecordAsn,
    __out_opt ULONGLONG* const Version,
    __out KBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __out_opt BOOLEAN* const ReadFromDedicated,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    if (ActualRecordAsn != NULL)
    {
        *ActualRecordAsn = RvdLogAsn::Null();
    }

    if (Version != NULL)
    {
        *Version = 0;
    }

    if (ReadFromDedicated)
    {
        *ReadFromDedicated = FALSE;
    }   
    
    MetaDataBuffer = nullptr;
    IoBuffer = nullptr;
    
    _ReadType = Type;
    _RecordAsn = RecordAsn;
    _ActualRecordAsnPtr = ActualRecordAsn;
    _VersionPtr = Version;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    _ReadFromDedicated = ReadFromDedicated;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStreamBase::AsyncReadContext::~AsyncReadContext()
{
}

OverlayStreamBase::AsyncReadContext::AsyncReadContext()
{
}

OverlayStream::AsyncReadContextOverlay::~AsyncReadContextOverlay()
{
}

VOID
OverlayStream::AsyncReadContextOverlay::OnReuse(
    )
{
    _DedicatedRead->Reuse();
    _SharedRead->Reuse();
    if (_CoalesceRead)
    {
        _CoalesceRead->Reuse();
    }
}

OverlayStream::AsyncReadContextOverlay::AsyncReadContextOverlay(
    __in OverlayStream& OStream,
    __in_opt OverlayStream::CoalesceRecords* CoalesceRecords,
    __in RvdLogStream& SharedLogStream,
    __in RvdLog& DedicatedLogContainer,
    __in RvdLogStream& DedicatedLogStream
    )
{
    NTSTATUS status;
    CoalesceRecords::SPtr coalesceRecords;
    
    _OverlayStream = &OStream;
    _CoalesceRecords = CoalesceRecords;
    _SharedLogStream = &SharedLogStream;
    _DedicatedLogContainer = &DedicatedLogContainer;
    _DedicatedLogStream = &DedicatedLogStream;
    coalesceRecords = _OverlayStream->GetCoalesceRecords();

    status = _DedicatedLogStream->CreateAsyncReadContext(_DedicatedRead);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    status = _SharedLogStream->CreateAsyncReadContext(_SharedRead);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    if (coalesceRecords)
    {
        status = coalesceRecords->CreateAsyncReadContext(_CoalesceRead);
        if (! NT_SUCCESS(status))
        {
            SetConstructorStatus(status);
            return;
        }
    }
}

NTSTATUS
OverlayStream::CreateAsyncReadContext(
    __out AsyncReadContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncReadContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadContextOverlay(*this, _CoalesceRecords.RawPtr(), *_SharedLogStream, *_DedicatedLogContainer, *_DedicatedLogStream);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// MultiRecordReadContext
//
VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::OnCompleted(
    )
{
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }

    _MetadataBuffer = nullptr;
    _IoBuffer = nullptr;
    _RecordMetadataBuffer = nullptr;
    _RecordIoBuffer = nullptr;
}

NTSTATUS
OverlayStream::AsyncMultiRecordReadContextOverlay::VerifyRecordAndGatherStreamBlock(
    __in KIoBuffer& IoBuffer,
    __in KBuffer& MetadataBuffer,
    __out KLogicalLogInformation::MetadataBlockHeader*& RecordMetadataHeader,
    __out KLogicalLogInformation::StreamBlockHeader*& RecordStreamBlockHeader,
    __out ULONG& DataSizeInRecord,
    __out ULONG& DataOffsetInRecordMetadata
    )
{
    NTSTATUS status;
    KIoBuffer::SPtr ioBuffer = &IoBuffer;
    KBuffer::SPtr metadataBuffer = &MetadataBuffer;    
    
    //
    // Copy out the data from the record just read and place into the output buffer
    //
    status = KLogicalLogInformation::FindLogicalLogHeadersWithCoreLoggerOffset(metadataBuffer,
                                                                 _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead(),
                                                                 *ioBuffer,
                                                                 sizeof(KtlLogVerify::KtlMetadataHeader),
                                                                 RecordMetadataHeader,
                                                                 RecordStreamBlockHeader,
                                                                 DataSizeInRecord,
                                                                 DataOffsetInRecordMetadata);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return(status);
    }
    
    return(STATUS_SUCCESS);
}

VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{   
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncMultiRecordReadContextOverlay::OperationCompletion);

#ifdef UDRIVER
    KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                        "OverlayStream::AsyncMultiRecordReadContextOverlay::FSMContinue", Status, _State,
                        (ULONGLONG)this, _RecordAsn.Get(), _ActualRecordAsn.Get());
#endif
    
    switch (_State)
    {
        case Initial:
        {
            if (!_OverlayStream->IsStreamForLogicalLog())
            {
                //
                // This functionality is only available for logical log streams
                //
                Status = STATUS_NOT_SUPPORTED;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            _NextRecordAsn = _RecordAsn;

            //
            // Find the record that contains the one we are looking
            // for and read it in.
            //
            _State = ReadFirstRecord;
            _UnderlyingRead->StartRead(_NextRecordAsn,
                                       RvdLogStream::AsyncReadContext::ReadContainingRecord,
                                       &_ActualRecordAsn,
                                       &_ActualRecordVersion,
                                       _RecordMetadataBuffer,
                                       _RecordIoBuffer,
                                       this,
                                       completion);

            break;
        }

        case ReadFirstRecord:
        {
            KLogicalLogInformation::MetadataBlockHeader* recordMetadataHeader = NULL;
            KLogicalLogInformation::StreamBlockHeader* recordStreamBlockHeader = NULL;
            ULONG dataSizeInRecord = 0;
            ULONG dataOffsetInRecordMetadata = 0;
            
            if (!NT_SUCCESS(Status))
            {
                if (Status == STATUS_NOT_FOUND)
                {
                    KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                        "ReadFirstRecord", Status, _State,
                                        (ULONGLONG)this, _RecordAsn.Get(), _ActualRecordAsn.Get());
                } else {                    
                    KTraceFailedAsyncRequest(Status, this, _State, _RecordAsn.Get());
                }
                Complete(Status);
                return;
            }

            //
            // If we get enough data in the first record then just
            // complete back with the record read and avoid all of the
            // extra overhead of building another record header
            //

            Status = VerifyRecordAndGatherStreamBlock(*_RecordIoBuffer,
                                                      *_RecordMetadataBuffer,
                                                      recordMetadataHeader,
                                                      recordStreamBlockHeader,
                                                      dataSizeInRecord,
                                                      dataOffsetInRecordMetadata);
            if (! NT_SUCCESS(Status))
            {
                Complete(Status);
                return;
            }

            if ((dataSizeInRecord >= _MinimumFirstReadCompleteSize) &&
                (_IoBuffer->QuerySize() >= _RecordIoBuffer->QuerySize()))
            {
                //
                // We were able to read enough data from the log and
                // there is enough space to return it.
                //
                PUCHAR recordMetadataPtr = (PUCHAR)_RecordMetadataBuffer->GetBuffer();
                PUCHAR metadataBufferPtr = (PUCHAR)_MetadataBuffer->First()->GetBuffer() + _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
                ULONG metadataBufferSize = _MetadataBuffer->First()->QuerySize() - _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
                KMemCpySafe(metadataBufferPtr, metadataBufferSize,
                            recordMetadataPtr, _RecordMetadataBuffer->QuerySize());
                
                KInvariant(_IoBuffer->QueryNumberOfIoBufferElements() < 2);

                if (_RecordIoBuffer->QuerySize() > 0)
                {
                    BOOLEAN b;
                    PUCHAR ioBufferPtr = (PUCHAR)_IoBuffer->First()->GetBuffer();

                    b = KIoBufferStream::CopyFrom(*_RecordIoBuffer, 0, _RecordIoBuffer->QuerySize(), ioBufferPtr);
                    KInvariant(b);
                }
                            
                Complete(STATUS_SUCCESS);
                return;
            }

            //
            // First read was not sufficient so we take the slow path
            // and read records until we fill up the output buffer
            //

            //
            // Prepare to write into output buffers
            //
            PUCHAR p;
            ULONG metadataBufferOffset = sizeof(KtlLogVerify::KtlMetadataHeader) + _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
            _MetadataBufferSizeLeft = KLogicalLogInformation::FixedMetadataSize - (metadataBufferOffset + sizeof(KLogicalLogInformation::MetadataBlockHeader) + 
                                                                                                          sizeof(KLogicalLogInformation::StreamBlockHeader));
            p = (PUCHAR)_MetadataBuffer->First()->GetBuffer();
            _MetadataBufferPtr = p + metadataBufferOffset + sizeof(KLogicalLogInformation::MetadataBlockHeader) +
                                                            sizeof(KLogicalLogInformation::StreamBlockHeader);

            _IoBufferSizeLeft = _IoBuffer->QuerySize();
            if (_IoBufferSizeLeft != 0)
            {
                _IoBufferPtr = (PUCHAR)_IoBuffer->First()->GetBuffer();
            }

            _TotalSize = 0;
                        
            //
            // Build part of the record header
            //
            KLogicalLogInformation::MetadataBlockHeader* metadataHeader;
            ULONG offsetToMetadataHeader = sizeof(KtlLogVerify::KtlMetadataHeader) + _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();

            metadataHeader = (KLogicalLogInformation::MetadataBlockHeader*)(p + offsetToMetadataHeader);
                                                                            
            metadataHeader->OffsetToStreamHeader = offsetToMetadataHeader + sizeof(KLogicalLogInformation::MetadataBlockHeader);
            metadataHeader->Flags = 0;
            metadataHeader->RecordMarkerOffsetPlusOne = 0;

            _StreamBlockHeader = (KLogicalLogInformation::StreamBlockHeader*)(p + metadataHeader->OffsetToStreamHeader);
            _StreamBlockHeader->Signature = KLogicalLogInformation::StreamBlockHeader::Sig;
            _StreamBlockHeader->StreamId = _OverlayStream->GetStreamId();
            _StreamBlockHeader->HighestOperationId = _ActualRecordVersion;
            _StreamBlockHeader->StreamOffset = _RecordAsn.Get();

            _OffsetToDataLocation = metadataHeader->OffsetToStreamHeader + sizeof(KLogicalLogInformation::StreamBlockHeader);

            _State = ReadSubsequentRecords;
            // fall through
        }

        case ReadSubsequentRecords:
        {
            KLogicalLogInformation::MetadataBlockHeader* recordMetadataHeader = NULL;
            KLogicalLogInformation::StreamBlockHeader* recordStreamBlockHeader = NULL;

            if (NT_SUCCESS(Status))
            {
                ULONG dataSizeInRecord = 0;
                ULONG dataOffsetInRecordMetadata = 0;
                ULONG dataOffsetInRecordIoBuffer = 0;
                ULONG bytesToCopy = 0;
                PUCHAR recordDataPtr = NULL;

                Status = VerifyRecordAndGatherStreamBlock(*_RecordIoBuffer,
                                                          *_RecordMetadataBuffer,
                                                          recordMetadataHeader,
                                                          recordStreamBlockHeader,
                                                          dataSizeInRecord,
                                                          dataOffsetInRecordMetadata);
                if (!NT_SUCCESS(Status))
                {
                    Complete(Status);
                    return;
                }
                    
                //
                // Metadata buffer is fixed up so core logger metadata isn't returned
                //
                ULONG dataSizeInRecordMetadata = 0;
                ULONG dataSizeInRecordIoBuffer = 0;

                dataOffsetInRecordMetadata -= _DedicatedLogContainer->QueryUserRecordSystemMetadataOverhead();
                dataSizeInRecordMetadata = _RecordMetadataBuffer->QuerySize() - dataOffsetInRecordMetadata;

                if (dataSizeInRecord <= dataSizeInRecordMetadata)
                {
                    dataSizeInRecordMetadata = dataSizeInRecord;
                    dataSizeInRecordIoBuffer = 0;
                }
                else 
                {
                    dataSizeInRecordIoBuffer = dataSizeInRecord - dataSizeInRecordMetadata;
                }

                //
                // We may not read a record that is directly contiguous so see if we need to trim any data before 
                // the next offset needed
                //
                KInvariant(_NextRecordAsn.Get() >= _ActualRecordAsn.Get());
                ULONGLONG amountToTrimULL = _NextRecordAsn.Get() -_ActualRecordAsn.Get();
                ULONG amountToTrim = (ULONG)amountToTrimULL;
                KInvariant((ULONGLONG)amountToTrim == amountToTrimULL);


                if (amountToTrim < dataSizeInRecord)
                {
                    //
                    // Ensure there is some data in the record that we just read for us. If there
                    // is not any data then this indicates that the we are at the end of the stream. The
                    // scenario is as follows:
                    //
                    // Dedicated Log records [ASN(version) - size]
                    //    [1(3) - beef0]
                    //
                    // Shared Log Records
                    //         [bccd1(4) - 2200]
                    //
                    // First read from dedicated puts us past the shared log record but the read for the 
                    // next record will return the shared log record. If there were a record after the one read in
                    // the dedicated then it would find that.
                    //
                    dataSizeInRecord -= amountToTrim;

                    if (amountToTrim > dataSizeInRecordMetadata)
                    {
                        amountToTrim -= dataSizeInRecordMetadata;
                        dataSizeInRecordMetadata = 0;

                        KInvariant(dataSizeInRecordIoBuffer > amountToTrim);
                        dataSizeInRecordIoBuffer -= amountToTrim;
                        dataOffsetInRecordIoBuffer += amountToTrim;
                        amountToTrim = 0;
                    }
                    else
                    {
                        dataSizeInRecordMetadata -= amountToTrim;
                        dataOffsetInRecordMetadata += amountToTrim;
                        amountToTrim = 0;
                    }


                    if (_MetadataBufferSizeLeft > 0)
                    {
                        //
                        // If there is room to write into the output metadata, start there with the record metadata
                        //
                        bytesToCopy = _MetadataBufferSizeLeft;
                        if (dataSizeInRecordMetadata < bytesToCopy)
                        {
                            bytesToCopy = dataSizeInRecordMetadata;
                        }

                        recordDataPtr = (PUCHAR)(_RecordMetadataBuffer->GetBuffer()) + dataOffsetInRecordMetadata;
                        KMemCpySafe(_MetadataBufferPtr, _MetadataBufferSizeLeft, recordDataPtr, bytesToCopy);

                        _TotalSize += bytesToCopy;
                        _NextRecordAsn = _NextRecordAsn.Get() + bytesToCopy;

                        _MetadataBufferPtr += bytesToCopy;
                        _MetadataBufferSizeLeft -= bytesToCopy;

                        dataOffsetInRecordMetadata += bytesToCopy;
                        dataSizeInRecordMetadata -= bytesToCopy;
                    }

                    if ((_MetadataBufferSizeLeft > 0) && (dataSizeInRecordIoBuffer > 0))
                    {
                        //
                        // If there is still room to write into the output metadata, try to fill with IoBuffer
                        //
                        bytesToCopy = _MetadataBufferSizeLeft;
                        if (dataSizeInRecordIoBuffer < bytesToCopy)
                        {
                            bytesToCopy = dataSizeInRecordIoBuffer;
                        }

                        KIoBufferStream recordDataStream(*_RecordIoBuffer, dataOffsetInRecordIoBuffer);
                        recordDataStream.Pull(_MetadataBufferPtr, bytesToCopy);

                        _TotalSize += bytesToCopy;
                        _NextRecordAsn = _NextRecordAsn.Get() + bytesToCopy;

                        _MetadataBufferPtr += bytesToCopy;
                        _MetadataBufferSizeLeft -= bytesToCopy;

                        dataOffsetInRecordIoBuffer += bytesToCopy;
                        dataSizeInRecordIoBuffer -= bytesToCopy;
                    }

                    if (dataSizeInRecordMetadata > 0)
                    {
                        //
                        // If there is still data in the record metadata then move into IoBuffer
                        //
                        bytesToCopy = _IoBufferSizeLeft;
                        if (dataSizeInRecordMetadata < bytesToCopy)
                        {
                            bytesToCopy = dataSizeInRecordMetadata;
                        }

                        if (bytesToCopy > 0)
                        {
                            recordDataPtr = (PUCHAR)(_RecordMetadataBuffer->GetBuffer()) + dataOffsetInRecordMetadata;
                            KMemCpySafe(_IoBufferPtr, _IoBufferSizeLeft, recordDataPtr, bytesToCopy);

                            _TotalSize += bytesToCopy;
                            _NextRecordAsn = _NextRecordAsn.Get() + bytesToCopy;

                            _IoBufferPtr += bytesToCopy;
                            _IoBufferSizeLeft -= bytesToCopy;

                            dataOffsetInRecordMetadata += bytesToCopy;
                            dataSizeInRecordMetadata -= bytesToCopy;
                        }
                    }

                    if (dataSizeInRecordIoBuffer > 0)
                    {
                        //
                        // If there is data in the IoBuffer then move it
                        //
                        bytesToCopy = _IoBufferSizeLeft;
                        if (dataSizeInRecordIoBuffer < bytesToCopy)
                        {
                            bytesToCopy = dataSizeInRecordIoBuffer;
                        }

                        KIoBufferStream recordDataStream(*_RecordIoBuffer, dataOffsetInRecordIoBuffer);
                        recordDataStream.Pull(_IoBufferPtr, bytesToCopy);

                        _TotalSize += bytesToCopy;
                        _NextRecordAsn = _NextRecordAsn.Get() + bytesToCopy;

                        _IoBufferPtr += bytesToCopy;
                        _IoBufferSizeLeft -= bytesToCopy;

                        dataSizeInRecordIoBuffer -= bytesToCopy;
                    }

                    if ((_MetadataBufferSizeLeft > 0) || (_IoBufferSizeLeft > 0))
                    {
                        //
                        // If there is still space in the output buffers then read more
                        //
                        _UnderlyingRead->Reuse();
                        _RecordMetadataBuffer = nullptr;
                        _RecordIoBuffer = nullptr;

                        _UnderlyingRead->StartRead(_NextRecordAsn.Get(),
                                                   RvdLogStream::AsyncReadContext::ReadContainingRecord,
                                                   &_ActualRecordAsn,
                                                   &_ActualRecordVersion,
                                                   _RecordMetadataBuffer,
                                                   _RecordIoBuffer,
                                                   this,
                                                   completion);

                        return;
                    }
                }
            }
            else if (Status == STATUS_NOT_FOUND) 
            {
                //
                // This indicates that we are at the end of the stream
                //
            }
            else 
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            //
            // If we have landed here then it indicates that either the out buffer is full or there are no more records
            // to read. Either way build thr record headersand return
            //

            _StreamBlockHeader->DataSize = _TotalSize;

#if DBG
            _StreamBlockHeader->DataCRC64 = KLogicalLogInformation::ComputeDataBlockCRC(_StreamBlockHeader,
                                                                                        *_IoBuffer,
                                                                                        _OffsetToDataLocation);
            _StreamBlockHeader->HeaderCRC64 = KLogicalLogInformation::ComputeStreamBlockHeaderCRC(_StreamBlockHeader);
#endif
            _State = Completed;
            Complete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _UnderlyingRead->Cancel();
}

VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;
    
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::StartRead(
    __in RvdLogAsn RecordAsn,
    __inout KIoBuffer& MetaDataBuffer,
    __inout KIoBuffer& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _RecordAsn = RecordAsn;
    _MetadataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;
    
    Start(ParentAsyncContext, CallbackPtr);
}


OverlayStream::AsyncMultiRecordReadContextOverlay::~AsyncMultiRecordReadContextOverlay()
{
}

VOID
OverlayStream::AsyncMultiRecordReadContextOverlay::OnReuse(
    )
{
    _UnderlyingRead->Reuse();
}

OverlayStream::AsyncMultiRecordReadContextOverlay::AsyncMultiRecordReadContextOverlay(
    __in OverlayStream& OStream,
    __in RvdLog& DedicatedLogContainer
    )
{
    NTSTATUS status;
    
    _OverlayStream = &OStream;
    _DedicatedLogContainer = &DedicatedLogContainer;
    
    status = _OverlayStream->CreateAsyncReadContext(_UnderlyingRead);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }           
}

NTSTATUS
OverlayStream::CreateAsyncMultiRecordReadContextOverlay(
    __out AsyncMultiRecordReadContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncMultiRecordReadContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
        
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncMultiRecordReadContextOverlay(*this, *_DedicatedLogContainer);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

NTSTATUS
OverlayStream::QueryRecordAndVerifyCompleted(
    __in BOOLEAN ForSharedLog,
    __inout RvdLogAsn& RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt ULONGLONG* const Version,
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1
    )
{
    NTSTATUS status;
    KNodeTable<AsyncDestagingWriteContextOverlay>* writeTable;
    RvdLogStream::SPtr logStream;

    if (Version != NULL)
    {
        *Version = 0;
    }

    if (Disposition != NULL)
    {
        *Disposition = RvdLogStream::RecordDisposition::eDispositionNone;
    }

    if (IoBufferSize != NULL)
    {
        *IoBufferSize = 0;
    }

    if (DebugInfo1 != NULL)
    {
        *DebugInfo1 = 0;
    }   
    
    //
    // This routine will query the shared or dedicated core logger
    // stream to see if there is a record in the ASN table for the
    // stream. There is a case where a record is in the process of
    // being written but not yet completed back from the core logger
    // and the record will be in the ASN table and have a disposition
    // of persisted. This routine will filter out ASNs that show as
    // persisted but have not yet been completed back. Note that this
    // is only available for streams that are if type logical log.
    //

    if (ForSharedLog)
    {
        logStream = _SharedLogStream;
        writeTable = &_OutstandingSharedWriteTable;
    } else {
        logStream = _DedicatedLogStream;
        writeTable = &_OutstandingDedicatedWriteTable;
    }
    
    status = logStream->QueryRecord(RecordAsn,
                                    Type,
                                    Version,
                                    Disposition,
                                    IoBufferSize,
                                    DebugInfo1);

    if ((! NT_SUCCESS(status)) ||
        (! IsStreamForLogicalLog()) ||
        (Disposition == NULL))
    {
        //
        // No checking for completion if the call failed, is not a
        // logical log or caller doesn't care about disposition
        //
        return(status);
    }

    if ((*Disposition == RvdLogStream::RecordDisposition::eDispositionPersisted) &&
        (IsAsnInDestagingList(*writeTable, RecordAsn)))
    {
        *Disposition = RvdLogStream::RecordDisposition::eDispositionPending;
        KTraceFailedAsyncRequest(status, this, RecordAsn.Get(), (ULONG)Type * 0x10 + (ULONG)ForSharedLog);
    }
    
    return(STATUS_SUCCESS);
}


NTSTATUS
OverlayStream::QueryRecord(
    __inout RvdLogAsn& RecordAsn,
    __in RvdLogStream::AsyncReadContext::ReadType Type,
    __out_opt ULONGLONG* const Version,
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1
)
{
    NTSTATUS status;
    ULONGLONG sharedVersion;
    RvdLogStream::RecordDisposition sharedDisposition;
    ULONG sharedIoBufferSize;
    ULONGLONG sharedDebugInfo1;
    ULONGLONG dedicatedVersion;
    RvdLogStream::RecordDisposition dedicatedDisposition;
    ULONG dedicatedIoBufferSize;
    ULONGLONG dedicatedDebugInfo1;
    ULONGLONG version;
    RvdLogStream::RecordDisposition disposition;
    ULONG ioBufferSize;
    ULONGLONG debugInfo1;
    RvdLogAsn dedicatedAsn, sharedAsn;

    if (Version != NULL)
    {
        *Version = 0;
    }

    if (Disposition != NULL)
    {
        *Disposition = RvdLogStream::RecordDisposition::eDispositionNone;
    }

    if (IoBufferSize != NULL)
    {
        *IoBufferSize = 0;
    }

    if (DebugInfo1 != NULL)
    {
        *DebugInfo1 = 0;
    }   
        
    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    dedicatedAsn  = RecordAsn;
    status = QueryRecordAndVerifyCompleted(FALSE,   // DedicatedLog
                                           dedicatedAsn,
                                           Type,
                                           &dedicatedVersion,
                                           &dedicatedDisposition,
                                           &dedicatedIoBufferSize,
                                           &dedicatedDebugInfo1);
    
    sharedAsn  = RecordAsn;
    status = QueryRecordAndVerifyCompleted(TRUE,    // SharedLog
                                           sharedAsn,
                                           Type,
                                           &sharedVersion,
                                           &sharedDisposition,
                                           &sharedIoBufferSize,
                                           &sharedDebugInfo1);
    
    if ((sharedDisposition == RvdLogStream::RecordDisposition::eDispositionNone) &&
        (dedicatedDisposition == RvdLogStream::RecordDisposition::eDispositionNone))
    {
        //
        // If both dispositions are None, then it doesn't matter which one is returned
        //
        RecordAsn = dedicatedAsn;
        version = dedicatedVersion;
        disposition = dedicatedDisposition;
        ioBufferSize = dedicatedIoBufferSize;
        debugInfo1 = dedicatedDebugInfo1;
    } else if ((sharedDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted) &&
               (dedicatedDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted))
    {
        if (dedicatedAsn == sharedAsn)
        {
            //
            // If both are persisted and have same Asn then return the one with the highest version
            //
            if (sharedVersion > dedicatedVersion) {
                RecordAsn = sharedAsn;
                version = sharedVersion;
                disposition = sharedDisposition;
                ioBufferSize = sharedIoBufferSize;
                debugInfo1 = sharedDebugInfo1;
            } else {
                RecordAsn = dedicatedAsn;
                version = dedicatedVersion;
                disposition = dedicatedDisposition;
                ioBufferSize = dedicatedIoBufferSize;
                debugInfo1 = dedicatedDebugInfo1;
            }
        } else {
            //
            // Otherwise pick the record with the lowest as that is
            // next
            //
            if (sharedAsn < dedicatedAsn) {
                RecordAsn = sharedAsn;
                version = sharedVersion;
                disposition = sharedDisposition;
                ioBufferSize = sharedIoBufferSize;
                debugInfo1 = sharedDebugInfo1;
            } else {
                RecordAsn = dedicatedAsn;
                version = dedicatedVersion;
                disposition = dedicatedDisposition;
                ioBufferSize = dedicatedIoBufferSize;
                debugInfo1 = dedicatedDebugInfo1;
            }           
        }
    } else if ((sharedDisposition == RvdLogStream::RecordDisposition::eDispositionPersisted) &&
               (dedicatedDisposition != RvdLogStream::RecordDisposition::eDispositionPersisted))
    {
        //
        // If shared is persisted and the dedicated is not then return shared
        //
        RecordAsn = sharedAsn;
        version = sharedVersion;
        disposition = sharedDisposition;
        ioBufferSize = sharedIoBufferSize;
        debugInfo1 = sharedDebugInfo1;
    } else {
        //
        // Otherwise they are both pending or it is just in the dedicated. So return the dedicated
        //
        RecordAsn = dedicatedAsn;
        version = dedicatedVersion;
        disposition = dedicatedDisposition;
        ioBufferSize = dedicatedIoBufferSize;
        debugInfo1 = dedicatedDebugInfo1;
    }

    if (Version != NULL)
    {
        *Version = version;
    }

    if (Disposition != NULL)
    {
        *Disposition = disposition;
    }

    if (IoBufferSize != NULL)
    {
        *IoBufferSize = ioBufferSize;
    }

    if (DebugInfo1 != NULL)
    {
        *DebugInfo1 = debugInfo1;
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS
OverlayStream::QueryRecord(
    __in RvdLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo1)
{
    NTSTATUS status;

    if (Version != NULL)
    {
        *Version = 0;
    }

    if (Disposition != NULL)
    {
        *Disposition = RvdLogStream::RecordDisposition::eDispositionNone;
    }

    if (IoBufferSize != NULL)
    {
        *IoBufferSize = 0;
    }

    if (DebugInfo1 != NULL)
    {
        *DebugInfo1 = 0;
    }   
    
    status = QueryRecord(RecordAsn,
                         RvdLogStream::AsyncReadContext::ReadExactRecord,
                         Version,
                         Disposition,
                         IoBufferSize,
                         DebugInfo1);

    return(status);
}

NTSTATUS
OverlayStream::QueryRecords(
__in RvdLogAsn LowestAsn,
__in RvdLogAsn HighestAsn,
__in KArray<RvdLogStream::RecordMetadata>& ResultsArray)
{
    NTSTATUS status;
    KArray<RvdLogStream::RecordMetadata> dedicatedResultsArray(GetThisAllocator());
    KArray<RvdLogStream::RecordMetadata> sharedResultsArray(GetThisAllocator());
    ULONG sharedIndex;
    KtlLogAsn sharedAsn;
    ULONG dedicatedIndex;
    KtlLogAsn dedicatedAsn;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    status = _SharedLogStream->QueryRecords(LowestAsn,
        HighestAsn,
        sharedResultsArray);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    if (sharedResultsArray.Count() == 0)
    {
        //
        // If the shared log is empty then return back the contents
        // of the dedicated log. No merge required.
        //
        status = _DedicatedLogStream->QueryRecords(LowestAsn,
            HighestAsn,
            ResultsArray);
        return(status);
    }

    status = _DedicatedLogStream->QueryRecords(LowestAsn,
        HighestAsn,
        dedicatedResultsArray);
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    ResultsArray.Clear();

    if (dedicatedResultsArray.Count() == 0)
    {
        //
        // And if the dedicated log is empty then return back the contents
        // of the shared log. A copy but no merge is required.
        //
        for (ULONG i = 0; i < sharedResultsArray.Count(); i++)
        {
            ResultsArray.Append(sharedResultsArray[i]);
        }

        return(STATUS_SUCCESS);
    }

    //
    // Merge is required.
    //
    // Shared log should not have any records whose ASN is less than
    // the lowest ASN of the dedicated log
    //
    KInvariant(sharedResultsArray[0].Asn >= dedicatedResultsArray[0].Asn);

    dedicatedIndex = 0;
    sharedIndex = 0;

    while ((sharedIndex < sharedResultsArray.Count()) || (dedicatedIndex < dedicatedResultsArray.Count()))
    {
        if (sharedIndex < sharedResultsArray.Count())
        {
            sharedAsn = sharedResultsArray[sharedIndex].Asn;
        } else {
            sharedAsn = RvdLogAsn::Max();
        }

        if (dedicatedIndex < dedicatedResultsArray.Count())
        {
            dedicatedAsn = dedicatedResultsArray[dedicatedIndex].Asn;
        } else {
            dedicatedAsn = RvdLogAsn::Max();
        }

        if (sharedAsn < dedicatedAsn)
        {
            //
            // If shared is below dedicated then add the shared to the list and move shared forward
            //
#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(), "Append Shared", STATUS_SUCCESS,
                                sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
            status = ResultsArray.Append(sharedResultsArray[sharedIndex]);
            if (!NT_SUCCESS(status))
            {
                return(status);
            }
            sharedIndex++;
        } else if (dedicatedAsn < sharedAsn)
        {
            //
            // If shared is below dedicated then add the shared to the list and move shared forward
            //
#ifdef UDRIVER
            KDbgCheckpointWData(GetActivityId(), "Append Dedicated", STATUS_SUCCESS,
                                sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
            status = ResultsArray.Append(dedicatedResultsArray[dedicatedIndex]);
            if (!NT_SUCCESS(status))
            {
                return(status);
            }
            dedicatedIndex++;
        } else {
            //
            // If the record is in both the dedicated and the shared
            // then pick the one with the highest version and if the
            // same version pick the one that is persisted and not
            // pending
            //
            if (sharedResultsArray[sharedIndex].Version == dedicatedResultsArray[dedicatedIndex].Version)
            {
                if ((sharedResultsArray[sharedIndex].Disposition == RvdLogStream::eDispositionPersisted))
                {
#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(), "Append Shared 3", STATUS_SUCCESS,
                                        sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
                    status = ResultsArray.Append(sharedResultsArray[sharedIndex]);
                    if (!NT_SUCCESS(status))
                    {
                        return(status);
                    }
                } else if (dedicatedResultsArray[dedicatedIndex].Disposition == RvdLogStream::eDispositionPersisted) {
#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(), "Append Dedicated 4", STATUS_SUCCESS,
                                        sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
                    status = ResultsArray.Append(dedicatedResultsArray[dedicatedIndex]);
                    if (!NT_SUCCESS(status))
                    {
                        return(status);
                    }
                } else {
#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(), "Append Dedicated 5", STATUS_SUCCESS,
                                        sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
                    status = ResultsArray.Append(dedicatedResultsArray[dedicatedIndex]);
                    if (!NT_SUCCESS(status))
                    {
                        return(status);
                    }
                }
            } else {
                if (sharedResultsArray[sharedIndex].Version > dedicatedResultsArray[dedicatedIndex].Version)
                {
#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(), "Append Shared 6", STATUS_SUCCESS,
                                        sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
                    status = ResultsArray.Append(sharedResultsArray[sharedIndex]);
                    if (!NT_SUCCESS(status))
                    {
                        return(status);
                    }
                } else {
#ifdef UDRIVER
                    KDbgCheckpointWData(GetActivityId(), "Append Dedicated 7", STATUS_SUCCESS,
                                        sharedAsn.Get(), dedicatedAsn.Get(), sharedIndex, dedicatedIndex);
#endif
                    status = ResultsArray.Append(dedicatedResultsArray[dedicatedIndex]);
                    if (!NT_SUCCESS(status))
                    {
                        return(status);
                    }
                }
            }

            sharedIndex++;
            dedicatedIndex++;
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS
OverlayStream::DeleteRecord(
    __in RvdLogAsn,
    __in ULONGLONG)
{
    KInvariant(FALSE);
    return(STATUS_NOT_IMPLEMENTED);
}



VOID
OverlayStream::TruncateSharedStreamIfPossible(
    __in RvdLogAsn DesiredTruncationPoint,
    __in ULONGLONG Version
    )
{
    //
    // If the shared log stream has returned an error from write then
    // it is not safe to truncate.
    //
    if (_LastSharedLogStatus == K_STATUS_LOG_STRUCTURE_FAULT)
    {
        KTraceFailedAsyncRequest(K_STATUS_LOG_STRUCTURE_FAULT,
                                 this, DesiredTruncationPoint.Get(), _LogicalLogTailOffset.Get());
        return;
    }
    
    //
    // Only truncate up to what has completed in the shared log.
    // There is an issue where if a truncation occurs on a stream
    // while that ASN is in process of being written the core
    // logger will assert. By limiting our truncations to completed
    // ASNs we avoid this
    //
    RvdLogAsn mustTruncationBelowAsn = GetLowestDestagingWriteAsn(_OutstandingDedicatedWriteTable);
    RvdLogAsn truncationAsn = RvdLogAsn::Null();

    if (DesiredTruncationPoint < mustTruncationBelowAsn)
    {
        //
        // We may possibly be able to truncate since the completion
        // is below the highest outstanding write
        //
        truncationAsn.SetIfLarger(DesiredTruncationPoint);

        if (truncationAsn <= _SharedTruncationAsn)
        {
            //
            // We are allowed to truncate up to the shared
            // truncation ASN
            //
            _SharedLogStream->TruncateBelowVersion(truncationAsn,
                                                   Version);

            LONGLONG outstanding = _LogicalLogTailOffset.Get() - truncationAsn.Get();
            SetSharedWriteBytesOutstanding(outstanding);
            
#ifdef UDRIVER
            KDbgCheckpointWDataInformational(GetActivityId(), "Shared outstanding", STATUS_SUCCESS,
                               (ULONGLONG)this, truncationAsn.Get(), Version, outstanding);
#endif
        }

    } else {
        //
        // Since there is an outstanding write with an ASN less
        // than the just completed ASN, it is not safe to truncate.
        // 
        // TODO: Remove me when code is fully stabilized
        KDbgCheckpointWDataInformational(GetActivityId(), "TRUNC** outstanding write with an ASN less", STATUS_SUCCESS,
                            (ULONGLONG)this,
                            (ULONGLONG)DesiredTruncationPoint.Get(),
                            (ULONGLONG)mustTruncationBelowAsn.Get(),
                            (ULONGLONG)_SharedTruncationAsn.Get());
    }
}

VOID
OverlayStream::TruncateBelowVersion(__in RvdLogAsn, __in ULONGLONG)
{
    // This is not exposed up through the KTLShim layer
    KInvariant(FALSE);
}

VOID
OverlayStream::Truncate(__in RvdLogAsn TruncationPoint, __in RvdLogAsn PreferredTruncationPoint)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        return;
    }
    KFinally([&] { ReleaseRequestRef(); });

    //
    // If the dedicated log stream has returned an error from write then
    // it is not safe to truncate.
    //
    if (! NT_SUCCESS(GetFailureStatus()))
    {
        KTraceFailedAsyncRequest(GetFailureStatus(),
                                 this, TruncationPoint.Get(), _LogicalLogTailOffset.Get());
        return;
    }
    
    //
    // If this is a logical log then truncation may occur within a
    // record. In this case we do not want to truncate the record that
    // contains the ASN as that would also truncate the data from that
    // ASN onward to the end of the record. So for a logical log we
    // need to determine the highest record that can be truncated and
    // still keep the data beyond the truncation point. Also note that
    // for correct recovery, there must be at least one record remaining
    // so even if the last record can be truncated, it shouldn't.
    //
    if (IsStreamForLogicalLog())
    {
        RvdLogAsn containingAsn;
        ULONGLONG version;
        ULONGLONG containingVersion;
        RvdLogStream::RecordDisposition disposition;
        ULONG ioBufferSize;
        ULONGLONG debugInfo;

        //
        // Do not allow truncation of the head past the tail
        //
        if (_LogicalLogTailOffset.Get() < TruncationPoint.Get())
        {
            KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER,
                                     this, TruncationPoint.Get(), _LogicalLogTailOffset.Get());
            return;
        }

        //
        // Find the record that contains this ASN and then find the
        // record immediately before it and that was written before it.
        // If that exists then it is the highest ASN that can be
        // truncated. In the case of an overwrite - where a record with
        // a higher version but lower ASN is encountered, there can be
        // no assumptions about the data in the record. So the code
        // will go back to the specific record before the record that
        // should be saved.
        //
        containingAsn = TruncationPoint;
        status = QueryRecordAndVerifyCompleted(FALSE,         // DedicatedLog
                                               containingAsn,
                                               RvdLogStream::AsyncReadContext::ReadContainingRecord,
                                               &containingVersion,
                                               &disposition,
                                               &ioBufferSize,
                                               &debugInfo);
        if ((! NT_SUCCESS(status)) || (disposition == eDispositionNone))
        {
            KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER,
                                     this, TruncationPoint.Get(), _LogicalLogTailOffset.Get());

            //
            // If the record hasn't been destaged yet to the dedicated
            // log then remember that a truncate is pending by
            // recording it in the appropriate DestagedWriteContext so
            // that when that DestagedWriteContext completes the
            // truncate happens.
            //

            RvdLogAsn setInList;
            setInList = SetTruncationPending(_OutstandingDedicatedWriteTable, TruncationPoint);
            if (setInList != 0)
            {
                KDbgCheckpointWDataInformational(GetActivityId(), "Pend Truncate", STATUS_SUCCESS,
                                                 setInList.Get(), TruncationPoint.Get(), 0, 0);
            }

            return;
        }

        do
        {
            status = QueryRecordAndVerifyCompleted(FALSE, // DedicatedLog
                                                   containingAsn,
                                                   RvdLogStream::AsyncReadContext::ReadPreviousRecord,
                                                   &version,
                                                   &disposition,
                                                   &ioBufferSize,
                                                   &debugInfo);
            if ((! NT_SUCCESS(status)) || (disposition == eDispositionNone))
            {
                KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER,
                                         this, TruncationPoint.Get(), containingVersion);
                return;
            }
        } while (containingVersion < version);
        
        TruncationPoint = containingAsn;
        PreferredTruncationPoint = containingAsn;       
    }

    //
    // Do not allow truncation beyond the last record written to both logs
    // There is a case where the shared will complete and the app will immediately
    // truncate. In this case the dedicated record is DispositionPending and thus will
    // assert.
    //
    RvdLogAsn mustTruncationBelowAsn = GetLowestDestagingWriteAsn(_OutstandingDedicatedWriteTable);

    if (TruncationPoint.Get() >= mustTruncationBelowAsn.Get())
    {
        KTraceFailedAsyncRequest(STATUS_INVALID_PARAMETER,
                                 this, (ULONGLONG)TruncationPoint.Get(), (ULONGLONG)mustTruncationBelowAsn.Get());
        return;
    }

    _SharedTruncationAsn.SetIfLarger(TruncationPoint);

#ifdef UDRIVER
    KDbgCheckpointWData(GetActivityId(),
                        "TruncateDedicated", STATUS_SUCCESS,
                        (ULONGLONG)this, TruncationPoint.Get(), mustTruncationBelowAsn.Get(), 0);
#endif

    _DedicatedLogStream->Truncate(TruncationPoint,
                                  PreferredTruncationPoint);    

    TruncateSharedStreamIfPossible(TruncationPoint, MAXULONGLONG);
}

//
// ReservationContext
//
VOID
OverlayStream::AsyncReservationContextOverlay::OnCompleted(
    )
{
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
}

VOID
OverlayStream::AsyncReservationContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncReservationContextOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = ReserveFromDedicated;
            _DedicatedReservation->StartUpdateReservation(_Delta,
                                                          this,
                                                          completion);
            break;
        }

        case ReserveFromDedicated:
        {
            Complete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncReservationContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::AsyncReservationContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncReservationContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    
    _RequestRefAcquired = TRUE;
    
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncReservationContextOverlay::StartUpdateReservation(
    __in LONGLONG Delta,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _Delta = Delta;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStreamBase::AsyncReservationContext::~AsyncReservationContext()
{
}

OverlayStreamBase::AsyncReservationContext::AsyncReservationContext()
{
}

OverlayStream::AsyncReservationContextOverlay::~AsyncReservationContextOverlay()
{
}

VOID
OverlayStream::AsyncReservationContextOverlay::OnReuse(
    )
{
    _DedicatedReservation->Reuse();
}

OverlayStream::AsyncReservationContextOverlay::AsyncReservationContextOverlay(
    __in OverlayStream& OStream
    )
{
    NTSTATUS status;
    
    _OverlayStream = &OStream;
    
    status = _OverlayStream->_DedicatedLogStream->CreateUpdateReservationContext(_DedicatedReservation);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
            
}

NTSTATUS
OverlayStream::CreateUpdateReservationContext(
    __out AsyncReservationContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncReservationContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReservationContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// WriteMetadata
//
VOID
OverlayStream::AsyncWriteMetadataContextOverlay::OnCompleted(
    )
{
    _IoBuffer = nullptr;
    _WriteMetadata = nullptr;
    _DedMBInfo = nullptr;

    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
    
}

VOID
OverlayStream::AsyncWriteMetadataContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncWriteMetadataContextOverlay::OperationCompletion);
        
    switch (_State)
    {
        case Initial:
        {
            Status = DedicatedLCMBInfoAccess::Create(_DedMBInfo,
                                                     _OverlayStream->GetDiskId(),
                                                     _OverlayStream->GetPath(),
                                                     _OverlayStream->GetDedicatedContainerId(),
                                                     _OverlayStream->QueryMaxMetadataLength(),
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            _State = OpenDedMBInfo;
            _DedMBInfo->StartOpenMetadataBlock(this,
                                               completion);
            break;
        }

        case OpenDedMBInfo:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }
    
            //
            // Send it to the metadata location
            //
            Status = _DedMBInfo->CreateAsyncWriteMetadataContext(_WriteMetadata);
            if (NT_SUCCESS(Status))
            {
                _State = WriteMetadata;
                _WriteMetadata->StartWriteMetadata(_IoBuffer,
                                                   this,
                                                   completion);
                break;
            } else {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = WriteMetadata;
                // Fall through
            }           
        }

        case WriteMetadata:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _LastStatus = Status;
            } else {
                _LastStatus = STATUS_SUCCESS;
            }
            
            _State = CloseDedMBInfo;
            _DedMBInfo->Reuse();
            _DedMBInfo->StartCloseMetadataBlock(this,
                                                completion);
            break;
        }

        case CloseDedMBInfo:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                if (NT_SUCCESS(_LastStatus))
                {
                    _LastStatus = Status;
                }
            }
            
            Complete(_LastStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncWriteMetadataContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _WriteMetadata->Cancel();
}

VOID
OverlayStream::AsyncWriteMetadataContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncWriteMetadataContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;
    
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncWriteMetadataContextOverlay::StartWriteMetadata(
    __in const KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _IoBuffer = IoBuffer;

    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStream::AsyncWriteMetadataContextOverlay::~AsyncWriteMetadataContextOverlay()
{
}

VOID
OverlayStream::AsyncWriteMetadataContextOverlay::OnReuse(
    )
{
    _WriteMetadata->Reuse();
}


OverlayStream::AsyncWriteMetadataContextOverlay::AsyncWriteMetadataContextOverlay(
    __in OverlayStream& OStream
    )
{
    _OverlayStream = &OStream;
}


NTSTATUS
OverlayStream::CreateAsyncWriteMetadataContext(
    __out AsyncWriteMetadataContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncWriteMetadataContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
        
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteMetadataContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}


//
// ReadMetadata
//
VOID
OverlayStream::AsyncReadMetadataContextOverlay::OnCompleted(
    )
{
    _IoBuffer = nullptr;
    _ReadMetadata = nullptr;
    _DedMBInfo = nullptr;
    
    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }
    
}

VOID
OverlayStream::AsyncReadMetadataContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncReadMetadataContextOverlay::OperationCompletion);
    
    switch (_State)
    {
        case Initial:
        {
            Status = DedicatedLCMBInfoAccess::Create(_DedMBInfo,
                                                     _OverlayStream->GetDiskId(),
                                                     _OverlayStream->GetPath(),
                                                     _OverlayStream->GetDedicatedContainerId(),
                                                     _OverlayStream->QueryMaxMetadataLength(),
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            _State = OpenDedMBInfo;
            _DedMBInfo->StartOpenMetadataBlock(this,
                                               completion);
            break;
        }

        case OpenDedMBInfo:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }
    
            //
            // Send it to the metadata location
            //
            Status = _DedMBInfo->CreateAsyncReadMetadataContext(_ReadMetadata);
            if (NT_SUCCESS(Status))
            {
                _State = ReadMetadata;
                _ReadMetadata->StartReadMetadata(_IoBuffer,
                                                 this,
                                                 completion);
                break;
            } else {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = ReadMetadata;
                // Fall through
            }
        }

        case ReadMetadata:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _LastStatus = Status;
            } else {
                _LastStatus = STATUS_SUCCESS;
            }
            
            _State = CloseDedMBInfo;
            _DedMBInfo->Reuse();
            _DedMBInfo->StartCloseMetadataBlock(this,
                                                completion);
            break;
        }

        case CloseDedMBInfo:
        {
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                if (NT_SUCCESS(_LastStatus))
                {
                    _LastStatus = Status;
                }
            }

            if (NT_SUCCESS(_LastStatus))
            {
                *_IoBufferPtr = _IoBuffer;
            }
            
            Complete(_LastStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }        
}

VOID
OverlayStream::AsyncReadMetadataContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _ReadMetadata->Cancel();
}

VOID
OverlayStream::AsyncReadMetadataContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{   
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncReadMetadataContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    _RequestRefAcquired = TRUE;
    
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncReadMetadataContextOverlay::StartReadMetadata(
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    IoBuffer = nullptr;
    
    _IoBufferPtr = &IoBuffer;

    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStream::AsyncReadMetadataContextOverlay::~AsyncReadMetadataContextOverlay()
{
}

VOID
OverlayStream::AsyncReadMetadataContextOverlay::OnReuse(
    )
{
    _ReadMetadata->Reuse();
}


OverlayStream::AsyncReadMetadataContextOverlay::AsyncReadMetadataContextOverlay(
    __in OverlayStream& OStream
    )
{
    _OverlayStream = &OStream;    
}


NTSTATUS
OverlayStream::CreateAsyncReadMetadataContext(
    __out AsyncReadMetadataContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncReadMetadataContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
        
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadMetadataContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}


//
// Ioctl
//
VOID
OverlayStream::AsyncIoctlContextOverlay::OnCompleted(
    )
{
    _InBuffer = nullptr;

    if (_RequestRefAcquired)
    {
        _OverlayStream->ReleaseRequestRef();
        _RequestRefAcquired = FALSE;
    }    
}

VOID
OverlayStream::AsyncIoctlContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncIoctlContextOverlay::OperationCompletion);
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                "OverlayStream::AsyncIoctlContextOverlay::FSMContinue", Status,
                                (ULONGLONG)_State,
                                (ULONGLONG)this,
                                (ULONGLONG)_ControlCode,
                                (ULONGLONG)0);

            //
            // assume that the control is not valid
            //
            Status = STATUS_INVALID_PARAMETER;
            
            switch (_ControlCode)
            {
                case KLogicalLogInformation::QueryLogicalLogTailAsnAndHighestOperationControl:
                {
                    if (_OverlayStream->_IsStreamForLogicalLog)
                    {
                        KBuffer::SPtr outBuffer;
                        KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation* outStruct;

                        Status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation),
                                                    outBuffer,
                                                    GetThisAllocator());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);

                            _State = CompletedWithError;
                            Complete(Status);
                            return;
                        }

                        outStruct = (KLogicalLogInformation::LogicalLogTailAsnAndHighestOperation*)outBuffer->GetBuffer();
                        outStruct->HighestOperationCount = _OverlayStream->GetLogicalLogLastVersion();
                        outStruct->TailAsn = _OverlayStream->GetLogicalLogTailOffset().Get();

                        outStruct->MaximumBlockSize = _DedicatedLogContainer->QueryMaxUserRecordSize();
                        outStruct->RecoveredLogicalLogHeader = *_OverlayStream->GetRecoveredLogicalLogHeader();

                        *_OutBuffer = outBuffer;
                    }
                    break;
                }

                case KLogicalLogInformation::WriteOnlyToDedicatedLog:
                {
                    _OverlayStream->SetWriteOnlyToDedicated(TRUE);
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KLogicalLogInformation::WriteToSharedAndDedicatedLog:
                {
                    _OverlayStream->SetWriteOnlyToDedicated(FALSE);
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KLogicalLogInformation::QueryCurrentWriteInformation:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::CurrentWriteInformation* outStruct;

                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::CurrentWriteInformation),
                                             outBuffer,
                                             GetThisAllocator());
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::CurrentWriteInformation*)outBuffer->GetBuffer();
                    outStruct->MaxAsnWrittenToBothLogs = _OverlayStream->GetLowestDestagingWriteAsn(*_OverlayStream->GetOutstandingDedicatedWriteTable()).Get();

                    *_OutBuffer = outBuffer;
                    break;
                }

                case KLogicalLogInformation::QueryCurrentBuildInformation:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::CurrentBuildInformation* outStruct;

                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::CurrentBuildInformation),
                                             outBuffer,
                                             GetThisAllocator());
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::CurrentBuildInformation*)outBuffer->GetBuffer();
                    outStruct->BuildNumber = VER_PRODUCTBUILD;
#if DBG
                    outStruct->IsFreeBuild = FALSE;
#else
                    outStruct->IsFreeBuild = TRUE;
#endif
                    *_OutBuffer = outBuffer;
                    break;
                }

                case KLogicalLogInformation::QueryCurrentLogUsageInformation:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::CurrentLogUsageInformation* outStruct;

                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::CurrentLogUsageInformation),
                                             outBuffer,
                                             GetThisAllocator());
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::CurrentLogUsageInformation*)outBuffer->GetBuffer();
                    
                    Status = _OverlayStream->ComputeLogPercentageUsed(outStruct->PercentageLogUsage);
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    *_OutBuffer = outBuffer;
                    break;
                }

                case KLogicalLogInformation::QueryLogSizeAndSpaceRemaining:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::LogSizeAndSpaceRemaining* outStruct;

                    Status = KBuffer::Create(
                        sizeof(KLogicalLogInformation::LogSizeAndSpaceRemaining),
                        outBuffer,
                        GetThisAllocator());
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, sizeof(KLogicalLogInformation::LogSizeAndSpaceRemaining));

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::LogSizeAndSpaceRemaining*)outBuffer->GetBuffer();

                    Status = _OverlayStream->ComputeLogSizeAndSpaceRemaining(outStruct->LogSize, outStruct->SpaceRemaining);
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    *_OutBuffer = outBuffer;
                    break;
                }
                
                case KLogicalLogInformation::SetWriteThrottleThreshold:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::WriteThrottleThreshold* outStruct;

                    //
                    // Return the old threshold value
                    //
                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::WriteThrottleThreshold),
                                            outBuffer,
                                            GetThisAllocator());
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::WriteThrottleThreshold*)outBuffer->GetBuffer();
                    outStruct->MaximumOutstandingBytes = _OverlayStream->GetDedicatedWriteBytesOutstandingThreshold();
                                                                          
                    //
                    // Establish the new write throttle threshold
                    //
                    KLogicalLogInformation::WriteThrottleThreshold* writeThrottleThreshold;
                    LONGLONG threshold;

                    if (! _InBuffer || (_InBuffer->QuerySize() < sizeof(KLogicalLogInformation::WriteThrottleThreshold)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    writeThrottleThreshold = (KLogicalLogInformation::WriteThrottleThreshold*)_InBuffer->GetBuffer();
                    threshold = writeThrottleThreshold->MaximumOutstandingBytes;

                    if ((threshold != KLogicalLogInformation::WriteThrottleThresholdNoLimit) &&
                        (threshold < KLogicalLogInformation::WriteThrottleThresholdMinimum))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, threshold);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    _OverlayStream->SetDedicatedWriteBytesOutstandingThreshold(threshold);
                    *_OutBuffer = outBuffer;
                    Status = STATUS_SUCCESS;
                    break;
                }

#if DBG
                case KLogicalLogInformation::DelaySharedWrites:
                {
                    _OverlayStream->SetSharedTimerDelay(OverlayStream::DebugDelayTimeInMs);
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KLogicalLogInformation::DelayDedicatedWrites:
                {
                    _OverlayStream->SetDedicatedTimerDelay(OverlayStream::DebugDelayTimeInMs);
                    Status = STATUS_SUCCESS;
                    break;
                }
#endif

                case KLogicalLogInformation::DisableCoalescingWrites:
                {
                    _OverlayStream->SetDisableCoalescing(TRUE);
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KLogicalLogInformation::EnableCoalescingWrites:
                {
                    _OverlayStream->SetDisableCoalescing(FALSE);
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KLogicalLogInformation::SetSharedTruncationAsn:
                {
                    KBuffer::SPtr outBuffer;
                    KLogicalLogInformation::SharedTruncationAsnStruct* outStruct;

                    //
                    // Return the old threshold value
                    //
                    Status = KBuffer::Create(sizeof(KLogicalLogInformation::SharedTruncationAsnStruct),
                                            outBuffer,
                                            GetThisAllocator());
                    if (!NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    outStruct = (KLogicalLogInformation::SharedTruncationAsnStruct*)outBuffer->GetBuffer();
                    outStruct->SharedTruncationAsn = _OverlayStream->GetSharedTruncationAsn();
                                                                          
                    //
                    // Establish the new shared truncation ASN. This is
                    // useful for testing purposes only
                    //
                    KLogicalLogInformation::SharedTruncationAsnStruct* inStruct;

                    if (! _InBuffer || (_InBuffer->QuerySize() < sizeof(KLogicalLogInformation::SharedTruncationAsnStruct)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        _State = CompletedWithError;
                        Complete(Status);
                        return;
                    }

                    inStruct = (KLogicalLogInformation::SharedTruncationAsnStruct*)_InBuffer->GetBuffer();

                    _OverlayStream->SetSharedTruncationAsn(inStruct->SharedTruncationAsn);
                    *_OutBuffer = outBuffer;
                    Status = STATUS_SUCCESS;
                    break;
                }
                
                case KLogicalLogInformation::QueryLogicalLogReadInformation:
                {
                    if (_OverlayStream->_IsStreamForLogicalLog)
                    {
                        KBuffer::SPtr outBuffer;
                        KLogicalLogInformation::LogicalLogReadInformation* outStruct;

                        Status = KBuffer::Create(sizeof(KLogicalLogInformation::LogicalLogReadInformation),
                                                    outBuffer,
                                                    GetThisAllocator());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);

                            _State = CompletedWithError;
                            Complete(Status);
                            return;
                        }

                        outStruct = (KLogicalLogInformation::LogicalLogReadInformation*)outBuffer->GetBuffer();
                        outStruct->MaximumReadRecordSize = _DedicatedLogContainer->QueryMaxRecordSize();

                        *_OutBuffer = outBuffer;
                    }
                    break;
                }

                case KLogicalLogInformation::QueryTelemetryStatistics:
                {
                    if (_OverlayStream->_IsStreamForLogicalLog)
                    {
                        KBuffer::SPtr outBuffer;
                        KLogicalLogInformation::TelemetryStatistics* outStruct;

                        Status = KBuffer::Create(sizeof(KLogicalLogInformation::TelemetryStatistics),
                                                    outBuffer,
                                                    GetThisAllocator());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);

                            _State = CompletedWithError;
                            Complete(Status);
                            return;
                        }

                        outStruct = (KLogicalLogInformation::TelemetryStatistics*)outBuffer->GetBuffer();
                        outStruct->DedicatedWriteBytesOutstanding = _OverlayStream->_DedicatedWriteBytesOutstanding;
                        outStruct->DedicatedWriteBytesOutstandingThreshold = _OverlayStream->_DedicatedWriteBytesOutstandingThreshold;
                        outStruct->ApplicationBytesWritten = _OverlayStream->_ApplicationBytesWritten;
                        outStruct->SharedBytesWritten = _OverlayStream->_SharedBytesWritten;
                        outStruct->DedicatedBytesWritten = _OverlayStream->_DedicatedBytesWritten;

                        *_OutBuffer = outBuffer;
                    }
                    break;
                }

                case KLogicalLogInformation::QueryAcceleratedFlushMode:
                {
                    if (_OverlayStream->_IsStreamForLogicalLog)
                    {
                        KBuffer::SPtr outBuffer;
                        KLogicalLogInformation::AcceleratedFlushMode* outStruct;

                        Status = KBuffer::Create(sizeof(KLogicalLogInformation::AcceleratedFlushMode),
                                                    outBuffer,
                                                    GetThisAllocator());
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this, _State, 0);

                            _State = CompletedWithError;
                            Complete(Status);
                            return;
                        }

                        outStruct = (KLogicalLogInformation::AcceleratedFlushMode*)outBuffer->GetBuffer();
                        outStruct->IsAccelerateInActiveMode = _OverlayStream->GetOverlayLog()->IsAccelerateInActiveMode();

                        *_OutBuffer = outBuffer;
                    }
                    break;
                }

                
                default:
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _ControlCode);
                    break;
                }
            }
            
            _State = Completed;
            Complete(Status);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStream::AsyncIoctlContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::AsyncIoctlContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncIoctlContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    _RequestRefAcquired = FALSE;
    
    NTSTATUS status = _OverlayStream->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    _RequestRefAcquired = TRUE;
    FSMContinue(STATUS_SUCCESS);   
}


VOID
OverlayStream::AsyncIoctlContextOverlay::StartIoctl(
    __in ULONG ControlCode,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;

    Result = (ULONG)STATUS_NOT_SUPPORTED;
    OutBuffer = nullptr;
    
    _ControlCode = ControlCode;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;

    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStream::AsyncIoctlContextOverlay::~AsyncIoctlContextOverlay()
{
}

VOID
OverlayStream::AsyncIoctlContextOverlay::OnReuse(
    )
{
}


OverlayStream::AsyncIoctlContextOverlay::AsyncIoctlContextOverlay(
    __in OverlayStream& OStream,
    __in RvdLog& DedicatedLogContainer
    )
{
    _OverlayStream = &OStream;
    _DedicatedLogContainer = &DedicatedLogContainer;
}


NTSTATUS
OverlayStream::CreateAsyncIoctlContext(
    __out AsyncIoctlContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncIoctlContextOverlay::SPtr context;

    Context = nullptr;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
        
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncIoctlContextOverlay(*this, *_DedicatedLogContainer);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

   

//
// CopySharedToDedicated
//
VOID
OverlayStream::AsyncCopySharedToDedicatedContext::OnCompleted(
    )
{
    _CopyMetadataBuffer = nullptr;
    _CopyIoBuffer = nullptr;    
}

VOID
OverlayStream::AsyncCopySharedToDedicatedContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncCopySharedToDedicatedContext::OperationCompletion);

    //
    // CONSIDER: Add coalescing of records from shared into dedicated
    //
    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                        "OverlayStream::AsyncCopySharedToDedicatedContext::FSMContinue", Status,
                    (ULONGLONG)_State,
                    (ULONGLONG)this,
                    (ULONGLONG)_CopyAsn.Get(),
                    (ULONGLONG)0);                                

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
    }

    // TODO: How to handle errors ?
    
#pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        switch (_State)
        {
            case Initial:
            {
                RvdLogAsn lowAsn;
                RvdLogAsn highAsn;
                RvdLogAsn truncationAsn;

                _State = ReadRecord;

                _CopyMetadataBuffer = nullptr;
                _CopyIoBuffer = nullptr;
                _SharedRead->Reuse();
                
                Status = _SharedLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
                KInvariant(NT_SUCCESS(Status));

                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                    "SharedStreamRecordRange", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)lowAsn.Get(),
                                    (ULONGLONG)highAsn.Get(),
                                    (ULONGLONG)truncationAsn.Get());
                
                if (OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn))
                {
                    //
                    // No more records to copy
                    //
                    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                        "Copy Complete", Status,
                                        (ULONGLONG)this,
                                        (ULONGLONG)_State,
                                        (ULONGLONG)_CopyAsn.Get(),
                                        (ULONGLONG)0);                                
                    Complete(STATUS_SUCCESS);
                    return;                                             
                } else {

                    _CopyAsn = lowAsn;                    
#ifdef VERBOSE
                    KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                        "StartRead", Status,
                                        (ULONGLONG)this,
                                        (ULONGLONG)_State,
                                        (ULONGLONG)_CopyAsn.Get(),
                                        (ULONGLONG)0);
#endif
                    _SharedRead->StartRead(_CopyAsn,
                                             RvdLogStream::AsyncReadContext::ReadExactRecord,
                                             NULL,                  // ActualAsn
                                             &_CopyVersion,
                                             _CopyMetadataBuffer,
                                             _CopyIoBuffer,
                                             this,
                                             completion);
                }
                return;
            }

            case ReadRecord:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
                    Complete(Status);
                    return;
                }
                
                _State = WriteRecord;
                _DedicatedWrite->Reuse();

#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                    "StartWrite", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)_State,
                                    (ULONGLONG)_CopyAsn.Get(),
                                    (ULONGLONG)0);
#endif
                _DedicatedWrite->StartReservedWrite(0,            // Reservation
                                                    _CopyAsn,
                                                    _CopyVersion,
                                                    _CopyMetadataBuffer,
                                                    _CopyIoBuffer,
                                                    this,
                                                    completion);
                return;
            }

            case WriteRecord:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
                    if (Status != STATUS_OBJECT_NAME_COLLISION)
                    {
                        //
                        // STATUS_OBJECT_NAME_COLLISION is ok as it
                        // means the record is also in the dedicated
                        // log
                        Complete(Status);
                        return;                     
                    }
                }

                RvdLogStream::RecordDisposition disposition;
                ULONGLONG debugInfo1;
#if DBG
                //
                // This is safe since no writes should be outstanding
                //
                Status = _DedicatedLogStream->QueryRecord(
                    _CopyAsn,
                    NULL,
                    &disposition,
                    NULL,
                    &debugInfo1);
                KInvariant(disposition == RvdLogStream::eDispositionPersisted);
#else
                disposition = RvdLogStream::eDispositionPersisted;
                debugInfo1 = (ULONGLONG)-1;
#endif
#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                    "Record Copied", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)debugInfo1,
                                    (ULONGLONG)_CopyAsn.Get(),
                                    (ULONGLONG)disposition);
#endif
                
                _SharedLogStream->Truncate(_CopyAsn, _CopyAsn);
                _State = Initial;
                continue;
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    }    
}

VOID
OverlayStream::AsyncCopySharedToDedicatedContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::AsyncCopySharedToDedicatedContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncCopySharedToDedicatedContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);
}


VOID
OverlayStream::AsyncCopySharedToDedicatedContext::StartCopySharedToDedicated(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStream::AsyncCopySharedToDedicatedContext::~AsyncCopySharedToDedicatedContext()
{
}

VOID
OverlayStream::AsyncCopySharedToDedicatedContext::OnReuse(
    )
{
}


OverlayStream::AsyncCopySharedToDedicatedContext::AsyncCopySharedToDedicatedContext(
    __in OverlayStream& OStream,
    __in RvdLogStream& SharedLogStream,
    __in RvdLogStream& DedicatedLogStream
    ) : 
    _OverlayStream(&OStream),
    _SharedLogStream(&SharedLogStream),
    _DedicatedLogStream(&DedicatedLogStream)
{
    NTSTATUS status;
    RvdLogStream::AsyncReadContext::SPtr sharedRead;
    RvdLogStream::AsyncWriteContext::SPtr dedicatedWrite;
    
    status = _SharedLogStream->CreateAsyncReadContext(sharedRead);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    status = _DedicatedLogStream->CreateAsyncWriteContext(dedicatedWrite);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    _SharedRead = Ktl::Move(sharedRead);
    _DedicatedWrite = Ktl::Move(dedicatedWrite);
}


NTSTATUS
OverlayStream::CreateAsyncCopySharedToDedicatedContext(
    __out AsyncCopySharedToDedicatedContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncCopySharedToDedicatedContext::SPtr context;

    Context = nullptr;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCopySharedToDedicatedContext(*this, *_SharedLogStream, *_DedicatedLogStream);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}


//
// CopySharedToBackup
//
VOID
OverlayStream::AsyncCopySharedToBackupContext::OnCompleted(
    )
{
    _CopyMetadataBuffer = nullptr;
    _CopyIoBuffer = nullptr;
    _BackupLogContainer = nullptr;
    _BackupLogStream = nullptr;
    _SharedLogStream = nullptr;
    _SharedRead = nullptr;
    _BackupWrite = nullptr;
    _OpenLog = nullptr;
    _CreateLog = nullptr;
    _OpenStream = nullptr;
    _CreateStream = nullptr;
}

VOID
OverlayStream::AsyncCopySharedToBackupContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStream::AsyncCopySharedToBackupContext::OperationCompletion);

    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                    "OverlayStream::AsyncCopySharedToBackupContext::FSMContinue", Status,
                    (ULONGLONG)_State,
                    (ULONGLONG)this,
                    (ULONGLONG)_CopyAsn.Get(),
                    (ULONGLONG)0);                                

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
    }

#pragma warning(disable:4127)   // C4127: conditional expression is constant
    while (TRUE)
    {
        switch (_State)
        {
            case Initial:
            {
                RvdLogAsn lowAsn, highAsn, truncationAsn;
                
                Status = _SharedLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
                KInvariant(NT_SUCCESS(Status));

                if (OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn))
                {
                    //
                    // If there are no records in the shared then there
                    // is nothing to do.
                    //
                    Complete(STATUS_SUCCESS);
                    return;
                }

                //
                // We have data to save from the shared log
                //
                if (_OverlayStream->GetPath())
                {
                    BOOLEAN b;

                    //
                    // Create the log container with a temp filename first,
                    // it will be renamed to the actual filename once all
                    // initialization is complete.
                    //
                    Status = KString::Create(_Path,
                                             GetThisAllocator(),
                                             (LPCWSTR)(*(_OverlayStream->GetPath())));
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;
                    }

                    KStringView tempString(L".Backup");
                    b = _Path->Concat(tempString);
                    if (! b)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        KTraceFailedAsyncRequest(Status, this, _State, 0);
                        Complete(Status);
                        return;                 
                    }
                } else {
                    //
                    // No path, too bad
                    //
                    Status = STATUS_NOT_SUPPORTED;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

                Status = _LogManager->CreateAsyncCreateLogContext(_CreateLog);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

                //
                // Compute the size of the backup log
                //
                KArray<RvdLogStream::RecordMetadata> records(GetThisAllocator());

                Status = records.Status();
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;                     
                }

                Status = _SharedLogStream->QueryRecords(RvdLogAsn::Min(), RvdLogAsn::Max(), records);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;                     
                }

                ULONGLONG logSize = 0;
                const ULONG metadataSize = 4096;
                const ULONGLONG oneMB = 1024 * 1024;
                const ULONGLONG minLogSize = (256 * oneMB);
                for (ULONG i = 0; i < records.Count(); i++)
                {
                    logSize += (metadataSize + records[i].Size);
                }

                logSize += (logSize / 2);

                logSize = (logSize + (minLogSize-1)) & ~(minLogSize-1);

                _State = CreateBackupLog;
                _CreateLog->StartCreateLog((KStringView)*_Path,
                                           (RvdLogId)((_OverlayStream->GetStreamId()).Get()),
                                           _LogType,
                                           logSize,
                                           16,    // MaxAllowedStreams
                                           0,
                                           1,     // Sparse
                                           _BackupLogContainer,
                                           this,
                                           completion);
                return;
            }
            
            case CreateBackupLog:
            {
                if (! NT_SUCCESS(Status))
                {
                    Complete(Status);
                    return;
                }

                _State = CreateBackupStream;
                Status = _BackupLogContainer->CreateAsyncCreateLogStreamContext(_CreateStream);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }

                RvdLogStreamType logType;

                _SharedLogStream->QueryLogStreamType(logType);
                _CreateStream->StartCreateLogStream(_OverlayStream->GetStreamId(),
                                                    logType,
                                                    _BackupLogStream,
                                                    this,
                                                    completion);
                return;
            }

            case CreateBackupStream:
            {
                if (! NT_SUCCESS(Status))
                {
                    Complete(Status);
                    return;
                }

                Status = _BackupLogStream->CreateAsyncWriteContext(_BackupWrite);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
                    Complete(Status);
                    return;
                }
                
                _State = CopyRecords;
                break;
            }
            
            case CopyRecords:
            {
                RvdLogAsn lowAsn;
                RvdLogAsn highAsn;
                RvdLogAsn truncationAsn;

                _State = ReadRecord;

                _CopyMetadataBuffer = nullptr;
                _CopyIoBuffer = nullptr;
                _SharedRead->Reuse();
                
                Status = _SharedLogStream->QueryRecordRange(&lowAsn, &highAsn, &truncationAsn);
                KInvariant(NT_SUCCESS(Status));

                KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                    "SharedStreamRecordRange", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)lowAsn.Get(),
                                    (ULONGLONG)highAsn.Get(),
                                    (ULONGLONG)truncationAsn.Get());
                
                if (OverlayStream::IsStreamEmpty(lowAsn, highAsn, truncationAsn))
                {
                    //
                    // No more records to copy
                    //
                    KDbgCheckpointWDataInformational(_OverlayStream->GetActivityId(),
                                        "Copy Complete", Status,
                                        (ULONGLONG)this,
                                        (ULONGLONG)_State,
                                        (ULONGLONG)_CopyAsn.Get(),
                                        (ULONGLONG)0);                                
                    Complete(STATUS_SUCCESS);
                    return;                                             
                } else {

                    _CopyAsn = lowAsn;                    
#ifdef VERBOSE
                    KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                        "StartRead", Status,
                                        (ULONGLONG)this,
                                        (ULONGLONG)_State,
                                        (ULONGLONG)_CopyAsn.Get(),
                                        (ULONGLONG)0);
#endif
                    _SharedRead->StartRead(_CopyAsn,
                                             RvdLogStream::AsyncReadContext::ReadExactRecord,
                                             NULL,                  // ActualAsn
                                             &_CopyVersion,
                                             _CopyMetadataBuffer,
                                             _CopyIoBuffer,
                                             this,
                                             completion);
                }
                return;
            }

            case ReadRecord:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
                    Complete(Status);
                    return;
                }
                
                _State = WriteRecord;
                _BackupWrite->Reuse();

#ifdef VERBOSE
                KDbgCheckpointWData(_OverlayStream->GetActivityId(),
                                    "StartWrite", Status,
                                    (ULONGLONG)this,
                                    (ULONGLONG)_State,
                                    (ULONGLONG)_CopyAsn.Get(),
                                    (ULONGLONG)0);
#endif
                _BackupWrite->StartReservedWrite(0,            // Reservation
                                                    _CopyAsn,
                                                    _CopyVersion,
                                                    _CopyMetadataBuffer,
                                                    _CopyIoBuffer,
                                                    this,
                                                    completion);
                return;
            }

            case WriteRecord:
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, _CopyAsn.Get());
                    if (Status != STATUS_OBJECT_NAME_COLLISION)
                    {
                        //
                        // STATUS_OBJECT_NAME_COLLISION is ok as it
                        // means the record is also in the dedicated
                        // log
                        Complete(Status);
                        return;                     
                    }
                }
                
                _SharedLogStream->Truncate(_CopyAsn, _CopyAsn);
                _State = CopyRecords;
                continue;
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    }    
}

VOID
OverlayStream::AsyncCopySharedToBackupContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStream::AsyncCopySharedToBackupContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStream::AsyncCopySharedToBackupContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);
}


VOID
OverlayStream::AsyncCopySharedToBackupContext::StartCopySharedToBackup(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStream::AsyncCopySharedToBackupContext::~AsyncCopySharedToBackupContext()
{
}

VOID
OverlayStream::AsyncCopySharedToBackupContext::OnReuse(
    )
{
}


OverlayStream::AsyncCopySharedToBackupContext::AsyncCopySharedToBackupContext(
    __in OverlayStream& OStream,
    __in RvdLogManager& LogManager, 
    __in RvdLogStream& SharedLogStream
    ) : 
    _OverlayStream(&OStream),
    _LogManager(&LogManager),
    _SharedLogStream(&SharedLogStream),
    _LogType(GetThisAllocator())
{
    NTSTATUS status;
    RvdLogStream::AsyncReadContext::SPtr sharedRead;

    _LogType =  L"Winfab Backup Logical Log";
    
    status = _LogType.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    status = _SharedLogStream->CreateAsyncReadContext(sharedRead);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    _SharedRead = Ktl::Move(sharedRead);
}


NTSTATUS
OverlayStream::CreateAsyncCopySharedToBackupContext(
    __out AsyncCopySharedToBackupContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStream::AsyncCopySharedToBackupContext::SPtr context;

    Context = nullptr;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCopySharedToBackupContext(*this,
                                                                                              *_BaseLogManager,
                                                                                              *_SharedLogStream);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( GetActivityId(), status, _DedicatedLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

VOID OverlayStream::FreeKIoBuffer(
    __in KActivityId ActivityId,
    __in ULONG Size
    )
{
    _ThrottledAllocator->FreeKIoBuffer(ActivityId, Size);
}


NTSTATUS OverlayStream::CreateTruncateCompletedWaiter(
    __out KAsyncEvent::WaitContext::SPtr& WaitContext
)
{
    NTSTATUS status;
    
    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    status = _SharedTruncationEvent.CreateWaitContext(GetThisAllocationTag(),
                                                      GetThisAllocator(),
                                                      WaitContext);

    return(status);
}

VOID OverlayStream::ResetTruncateCompletedEvent()
{
    _SharedTruncationEvent.ResetEvent();
}
