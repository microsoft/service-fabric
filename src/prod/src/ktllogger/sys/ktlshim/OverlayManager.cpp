// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

#define VERBOSE 1

//
// Overlay Manager implementation
//
OverlayManager::~OverlayManager()
{
    KInvariant(GetPinnedIoBufferUsage() == 0);
}

OverlayManager::OverlayManager(
    __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
    __in KtlLogManager::SharedLogContainerSettings& SharedLogContainerSettings,
    __in KtlLogManager::AcceleratedFlushLimits& AcceleratedFlushLimits
      ) :
   _MemoryThrottleLimits(MemoryThrottleLimits),
   _AcceleratedFlushLimits(AcceleratedFlushLimits),
   _SharedLogContainerSettings(SharedLogContainerSettings),
   _ContainersTable(*this,
                    GetThisAllocator(),
                    GetThisAllocationTag()),
   _GateList(FIELD_OFFSET(GateTableEntry, Link)),
#if !defined(PLATFORM_UNIX)
   _VolumeList(GetThisAllocator()),
#endif
   _ActiveCount(0),
   _PinnedIoBufferUsage(0),
#if DBG
   _PinMemoryFailureCount(0),
#endif
#if defined(UDRIVER) || defined(UPASSTHROUGH)
    _UsageCount(0),
#endif
#if defined(UPASSTHROUGH)
    _OpenWaitEvent(TRUE, FALSE),
#endif  
   _ObjectState(Closed),
   _State(Created)
{
    NTSTATUS status;
    
    //
    // Create the base log manager to which calls will be forwarded
    //
    status = RvdLogManager::Create(GetThisAllocationTag(),
                                   GetThisAllocator(),
                                   _BaseLogManager);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _ContainersTable.SetBaseLogManager(*_BaseLogManager);
}

NTSTATUS
OverlayManager::Create(
    __out OverlayManager::SPtr& Context,    
    __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
    __in KtlLogManager::SharedLogContainerSettings& SharedLogContainerSettings,
    __in KtlLogManager::AcceleratedFlushLimits& AcceleratedFlushLimits,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    OverlayManager::SPtr context;

    context = _new(AllocationTag, Allocator) OverlayManager(MemoryThrottleLimits, SharedLogContainerSettings, AcceleratedFlushLimits);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
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

NTSTATUS
OverlayManager::StartServiceOpen(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt KAsyncServiceBase::OpenCompletionCallback OpenCallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
)
{
    NTSTATUS status;

    status = StartOpen(ParentAsync, OpenCallbackPtr, GlobalContextOverride);

    return(status);
}

NTSTATUS
OverlayManager::StartServiceClose(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt KAsyncServiceBase::CloseCompletionCallback CloseCallbackPtr
)
{
    NTSTATUS status;

    status = StartClose(ParentAsync, CloseCallbackPtr);

    return(status);
}        

VOID
OverlayManager::QueryVolumeListCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        //
        // Note that even if getting the volume list fails, we do not
        // fail the service open but proceed with an empty volume list.
        // Although this will cause every log stream to use the same
        // gate and perhaps throttle too much, the logger will still
        // run
        //
    }
    
    _ObjectState = Opened;
    CompleteOpen(STATUS_SUCCESS);   
}

#if defined(UPASSTHROUGH)
NTSTATUS OverlayManager::CreateOpenWaitContext(
    __out KAsyncEvent::WaitContext::SPtr & WaitContext
    )
{
    NTSTATUS status;

    status = _OpenWaitEvent.CreateWaitContext(GetThisAllocationTag(),
                                               GetThisAllocator(),
                                               WaitContext);
    return(status);
}
#endif      

NTSTATUS OverlayManager::MapSharedLogDefaultSettings(
    __inout KGuid& DiskId,
    __inout KString::CSPtr& Path,
    __inout KtlLogContainerId& LogId,
    __inout LONGLONG& LogSize,
    __inout ULONG& MaxAllowedStreams,
    __inout ULONG& MaxRecordSize,
    __inout DWORD& Flags            
)
{
    NTSTATUS status = STATUS_SUCCESS;

#ifdef UPASSTHROUGH
    UNREFERENCED_PARAMETER(DiskId);
    UNREFERENCED_PARAMETER(Path);
    UNREFERENCED_PARAMETER(LogId);
    UNREFERENCED_PARAMETER(LogSize);
    UNREFERENCED_PARAMETER(MaxAllowedStreams);
    UNREFERENCED_PARAMETER(MaxRecordSize);
    UNREFERENCED_PARAMETER(Flags);
#endif

#if !defined(PLATFORM_UNIX)
#if defined(KDRIVER) || defined(UDRIVER)
    KGuid defaultGuid(0x3CA2CCDA,0xDD0F,0x49c8,0xA7,0x41,0x62,0xAA,0xC0,0xD4,0xEB,0x62);
    KGuid nullGuid(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);

    if (LogId.Get() == defaultGuid)
    {
        if (_SharedLogContainerSettings.LogContainerId.Get() != nullGuid)
        {
            //
            // We are mapping the default shared log to something else
            //
            if (*_SharedLogContainerSettings.Path != 0)
            {
                KString::SPtr path = KString::Create(_SharedLogContainerSettings.Path, GetThisAllocator());
                Path = KString::CSPtr(path.RawPtr());
                if (Path == nullptr)
                {
                    return(STATUS_INSUFFICIENT_RESOURCES);
                }
            } else {
                Path = nullptr;
            }

            DiskId = _SharedLogContainerSettings.DiskId;
            LogId = _SharedLogContainerSettings.LogContainerId;
            LogSize = _SharedLogContainerSettings.LogSize;
            MaxAllowedStreams = _SharedLogContainerSettings.MaximumNumberStreams;
            MaxRecordSize = _SharedLogContainerSettings.MaximumRecordSize;
            Flags = _SharedLogContainerSettings.Flags;
            
            KDbgCheckpointWDataInformational(0, "Map SharedLog", status,
                                             (ULONGLONG)LogId.Get().Data1,
                                             (ULONGLONG)LogSize,
                                             (ULONGLONG)MaxAllowedStreams,
                                             (ULONGLONG)(Flags * 100000000) + MaxRecordSize);
            
        } else {
            //
            // Map LogSize, MaxAllowedStreams, MaxRecordSize and Flags only
            //
            LogSize = _SharedLogContainerSettings.LogSize;
            MaxAllowedStreams = _SharedLogContainerSettings.MaximumNumberStreams;
            MaxRecordSize = _SharedLogContainerSettings.MaximumRecordSize;
            Flags = _SharedLogContainerSettings.Flags;

            KDbgCheckpointWDataInformational(0, "Map SharedLog", status,
                                             (ULONGLONG)LogId.Get().Data1,
                                             (ULONGLONG)LogSize,
                                             (ULONGLONG)MaxAllowedStreams,
                                             (ULONGLONG)(Flags * 100000000) + MaxRecordSize);       
        }
    } else {
        //
        // No mapping when log container id is not the default
        //
    }   
#endif    
#endif
    return(status);
}



VOID
OverlayManager::OnServiceOpen(
    )
{
    NTSTATUS status;

    KInvariant(_ObjectState == Closed);
    
    //
    // Ensure that all in flight operations finish before close started
    //
    SetDeferredCloseBehavior();

    _State = OpenAttempted;

    //
    // Allocate throttled memory allocator
    //
    status = ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(GetMemoryThrottleLimits(),
                                                                            GetThisAllocator(),
                                                                            GetThisAllocationTag(),
                                                                            _ThrottledAllocator);
    if (! NT_SUCCESS(status))
    {
        CompleteOpen(status);
        return;
    }

#if !defined(PLATFORM_UNIX)
    //
    // Setup performance counters
    //
    status = ConfigurePerfCounters();
    if (! NT_SUCCESS(status))
    {
        CompleteOpen(status);
        return;
    }
#endif
    
    //
    // Register verification callback for winfab logical log records
    // and Activate the underlying log manager
    //
    status = _BaseLogManager->RegisterVerificationCallback(KtlLogInformation::KtlLogDefaultStreamType(),
                                                           &KtlLogVerify::VerifyRecordCallback);

    if (! NT_SUCCESS(status))
    {
        CompleteOpen(status);
        return;
    }
    
    status = _BaseLogManager->RegisterVerificationCallback(KLogicalLogInformation::GetLogicalLogStreamType(),
                                                           &KtlLogVerify::VerifyRecordCallback);

    if (! NT_SUCCESS(status))
    {
        CompleteOpen(status);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::CloseServiceCompletion);
    status = _BaseLogManager->Activate(this,
                                       completion);

    KInvariant(status == STATUS_PENDING);

    if (! NT_SUCCESS(status))
    {
        CompleteOpen(status);
        return;
    }

#if !defined(PLATFORM_UNIX) 
    //
    // Last step is to query the volume list for Windows so log files can be mapped to diskid.
    // This is not needed on Linux.
    //
    KAsyncContextBase::CompletionCallback queryVolumeListCompletion(this, &OverlayManager::QueryVolumeListCompletion);
    status = KVolumeNamespace::QueryVolumeListEx(_VolumeList,
                                               GetThisAllocator(),
                                               queryVolumeListCompletion,
                                               this);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        //
        // Note that even if getting the volume list fails, we do not
        // fail the service open but proceed with an empty volume list.
        // Although this will cause every log stream to use the same
        // gate and perhaps throttle too much, the logger will still
        // run
        //
        _ObjectState = Opened;
        CompleteOpen(STATUS_SUCCESS);   
    }
#endif
    
    _ObjectState = Opened;
    CompleteOpen(STATUS_SUCCESS);

#if defined(UPASSTHROUGH)
    _OpenWaitEvent.SetEvent();  
#endif
}

VOID
OverlayManager::QueueDeactivated(
    __in ServiceWrapper&
    )
{
    CloseServiceFSM(STATUS_SUCCESS);
}

VOID
OverlayManager::DoCompleteClose(
    __in NTSTATUS Status
    )
{
    if (_LastOverlayLogFS)
    {
        BOOLEAN freed;
        
        freed = _ContainersTable.ReleaseOverlayContainer(*_LastOverlayLogFS);

        if (freed)
        {
            //
            // Stop queue as we shut this down. 
            //
            _State = CloseCleanup;
            OverlayLogFreeService::SPtr oc = Ktl::Move(_LastOverlayLogFS);            
            _FinalStatus = Status;
            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                   &OverlayManager::QueueDeactivated);           
            oc->DeactivateQueue(queueDeactivated);
            return;
        }
    }
    _CloseOperation = nullptr;
    
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    CompleteClose(Status);
}

VOID
OverlayManager::CloseServiceFSM(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::CloseServiceCompletion);

    KDbgCheckpointWDataInformational(0, "CloseServiceFSM", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
                
    if ((! NT_SUCCESS(Status)) && (_State != WaitForQuotaGateDeactivation))
    {
        
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoCompleteClose(Status);
        return;
    }
    
    switch (_State)
    {
        case CloseInitial:
        {
            KInvariant(_ObjectState == Opened);

            _ThrottledAllocator->Shutdown();
            _ThrottledAllocator = nullptr;
            
            _LastOverlayLogFS = nullptr;
            
            _GateCount = DeactivateAllLogStreamGates();
            if (_GateCount != 0)
            {
                _State = WaitForQuotaGateDeactivation;
                return;
            }
            
            _State = CloseContainers;
            goto CloseContainersCode;
        }

        case WaitForQuotaGateDeactivation:
        {
            // NOTE: Errors are ignored

            _GateCount--;
            if (_GateCount != 0)
            {
                return;
            }

            _State = CloseContainers;
            goto CloseContainersCode;
        }
        
        case CloseContainers:
        {
CloseContainersCode:            
            //
            // First step is to force close all of the open containers
            //
            if (_LastOverlayLogFS)
            {
                BOOLEAN freed;
                
                do
                {
                    freed = _ContainersTable.ReleaseOverlayContainer(*_LastOverlayLogFS);
                } while (! freed);

                OverlayLogFreeService::SPtr oc = Ktl::Move(_LastOverlayLogFS);
            
                ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                       &OverlayManager::QueueDeactivated);
                _LastOverlayLogFS = nullptr;
                _CloseOperation = nullptr;
                
                oc->DeactivateQueue(queueDeactivated);
                
                return;
            }

            Status = _ContainersTable.GetFirstContainer(_LastOverlayLogFS);
            if (! NT_SUCCESS(Status))
            {
                //
                // No more containers to close, move on to disable base
                // log manager
                //
                _State = DeactivateBaseLogManager;

                _BaseLogManager->Deactivate();
                break;
            }

            //
            // Close the overlay log
            //
            Status = ServiceWrapper::Operation::Create(_CloseOperation,
                                                     *(static_cast<ServiceWrapper*>(_LastOverlayLogFS.RawPtr())),
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteClose(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LastOverlayLogFS->GetContainerId().Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoCompleteClose(Status);
                return;
            }
            
            _CloseOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));          
            
            _CloseOperation->StartShutdownOperation(this,
                                                    completion);
            break;
        }

        case DeactivateBaseLogManager:
        {
            _ObjectState = Closed;
            DoCompleteClose(Status);
            break;
        }

        case CloseCleanup:
        {
            DoCompleteClose(_FinalStatus);
            break;
        }
                
        default:
        {
            KInvariant(FALSE);

            Status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            
            DoCompleteClose(Status);
            return;
        }
    }    
}

VOID
OverlayManager::CloseServiceCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    
    CloseServiceFSM(status);
}

VOID
OverlayManager::OnServiceClose(
    )
{
    _State = CloseInitial;
    CloseServiceFSM(STATUS_SUCCESS);
}

VOID
OverlayManager::OnServiceReuse()
{
    KInvariant(FALSE);
    _BaseLogManager->Reuse();
    
}

NTSTATUS
OverlayManager::TryAcquireRequestRef()
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
OverlayManager::ReleaseRequestRef()
{
    ReleaseServiceActivity();
}

NTSTATUS
OverlayManager::Activate(
    __in_opt KAsyncContextBase* const,
    __in_opt KAsyncContextBase::CompletionCallback)
{
    KInvariant(FALSE);
    
    return(STATUS_UNSUCCESSFUL);
}

VOID
OverlayManager::Deactivate()
{
    KInvariant(FALSE);
}

#if !defined(PLATFORM_UNIX)
//
// Perfcounter support
//
NTSTATUS 
OverlayManager::ConfigurePerfCounters(
    )
{
    NTSTATUS status;

    status = KPerfCounterManager::Create(GUID_KTLLOGGER_COUNTER_PROVIDER,
                                         GetThisAllocator(), 
                                         _PerfCounterMgr);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    //
    // Log stream counter set
    //
    KWString counterSetNameLogStream(GetThisAllocator(), KTLLOGGER_COUNTER_LOGSTREAM_NAME);

    if (! NT_SUCCESS(counterSetNameLogStream.Status()))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 0, 0);
        return(status);
    }

    status = _PerfCounterMgr->CreatePerfCounterSet(counterSetNameLogStream,
                                                   GUID_KTLLOGGER_COUNTER_LOGSTREAM,
                                                   KPerfCounterSet::CounterSetMultiInstance,
                                                   _PerfCounterSetLogStream);

    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(1,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(2,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(3,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(4,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(5,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(6,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(7,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(8,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->DefineCounter(9,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogStream->CommitDefinition();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }


    //
    // Log container counter set
    //
    KWString counterSetNameLogContainer(GetThisAllocator(), KTLLOGGER_COUNTER_LOGCONTAINER_NAME);

    if (! NT_SUCCESS(counterSetNameLogContainer.Status()))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 0, 0);
        return(status);
    }

    status = _PerfCounterMgr->CreatePerfCounterSet(counterSetNameLogContainer,
                                                   GUID_KTLLOGGER_COUNTER_LOGCONTAINER,
                                                   KPerfCounterSet::CounterSetMultiInstance,
                                                   _PerfCounterSetLogContainer);

    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogContainer->DefineCounter(1,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogContainer->DefineCounter(2,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogContainer->CommitDefinition();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }


    //
    // LogManager counters
    //
    KWString counterSetNameLogManager(GetThisAllocator(), KTLLOGGER_COUNTER_LOGMANAGER_NAME);

    if (! NT_SUCCESS(counterSetNameLogManager.Status()))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 0, 0);
        return(status);
    }

    status = _PerfCounterMgr->CreatePerfCounterSet(counterSetNameLogManager,
                                                   GUID_KTLLOGGER_COUNTER_LOGMANAGER,
                                                   KPerfCounterSet::CounterSetSingleton,
                                                   _PerfCounterSetLogManager);

    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogManager->DefineCounter(1,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogManager->DefineCounter(2,  KPerfCounterSet::Type_ULONGLONG);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    status = _PerfCounterSetLogManager->CommitDefinition();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    //
    // Create counter set associated with log manager
    //
    status = CreateNewLogManagerPerfCounterSet(_PerfCounterSetLogManagerInstance);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::ConfigurePerfCounters", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }
                                               
    return(status);
}

NTSTATUS 
OverlayManager::CreateNewLogStreamPerfCounterSet(
    __in KtlLogStreamId StreamId,
    __in OverlayStream& LogStream,
    __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
)
{
    NTSTATUS status;
    KDynString s(GetThisAllocator(),40);
    BOOLEAN b;

    b = s.FromGUID(StreamId.Get());
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogStreamPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }
    s.SetNullTerminator();

    status = _PerfCounterSetLogStream->SpawnInstance(s, 
                                                     PerfCounterSetInstance);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogStreamPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    PerfCounterSetInstance->SetCounterAddress(1, (PVOID)LogStream.GetPerfConterApplicationBytesWrittenPointer());
    PerfCounterSetInstance->SetCounterAddress(2, (PVOID)LogStream.GetPerfCounterDedicatedBytesWrittenPointer());
    PerfCounterSetInstance->SetCounterAddress(3, (PVOID)LogStream.GetPerfCounterSharedBytesWrittenPointer());
    PerfCounterSetInstance->SetCounterAddress(4, (PVOID)LogStream.GetDedicatedWriteBytesOutstandingThresholdPointer());
    PerfCounterSetInstance->SetCounterAddress(5, (PVOID)LogStream.GetDedicatedWriteBytesOutstandingPointer());
    PerfCounterSetInstance->SetCounterAddress(6, (PVOID)LogStream.GetSharedQuotaPointer());
    PerfCounterSetInstance->SetCounterAddress(7, (PVOID)LogStream.GetSharedWriteBytesOutstandingPointer());
    PerfCounterSetInstance->SetCounterAddress(8, (PVOID)LogStream.GetSharedWriteLatencyTimeAveragePointer());
    PerfCounterSetInstance->SetCounterAddress(9, (PVOID)LogStream.GetDedicatedWriteLatencyTimeAveragePointer());

    status = PerfCounterSetInstance->Start();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogStreamPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        PerfCounterSetInstance = nullptr;
        return(status);
    }
    return(status);
}

NTSTATUS 
OverlayManager::CreateNewLogContainerPerfCounterSet(
    __in KtlLogContainerId ContainerId,
    __in LogContainerPerfCounterData* Data,
    __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
)
{
    NTSTATUS status;
    KDynString s(GetThisAllocator(),40);
    BOOLEAN b;

    b = s.FromGUID(ContainerId.Get());
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogContainerPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }
    s.SetNullTerminator();

    status = _PerfCounterSetLogContainer->SpawnInstance(s, 
                                                     PerfCounterSetInstance);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogContainerPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    PerfCounterSetInstance->SetCounterAddress(1, &Data->DedicatedBytesWritten);
    PerfCounterSetInstance->SetCounterAddress(2, &Data->SharedBytesWritten);

    status = PerfCounterSetInstance->Start();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogContainerPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        PerfCounterSetInstance = nullptr;
        return(status);
    }
    return(status);
}

NTSTATUS 
OverlayManager::CreateNewLogManagerPerfCounterSet(
    __out KPerfCounterSetInstance::SPtr& PerfCounterSetInstance
)
{
    NTSTATUS status;

    status = _PerfCounterSetLogManager->SpawnInstance(L"LogManager", 
                                                        PerfCounterSetInstance);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogContainerPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        return(status);
    }

    PerfCounterSetInstance->SetCounterAddress(1, _ThrottledAllocator->GetTotalAllocationLimitPointer());
    PerfCounterSetInstance->SetCounterAddress(2, _ThrottledAllocator->GetCurrentAllocationsPointer());

    status = PerfCounterSetInstance->Start();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayManager::CreateNewLogContainerPerfCounterSet", status,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
        PerfCounterSetInstance = nullptr;
        return(status);
    }
    return(status);
}
#endif

//
// KQuotaGate management
//

VOID
LogStreamOpenGateContext::OnCompleted(
    )
{
    _QuotaAcquireContext = nullptr;
}

VOID
LogStreamOpenGateContext::OperationCompleted(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status;

    status = Async.Status();
        
    FSMContinue(status);
}

VOID
LogStreamOpenGateContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &LogStreamOpenGateContext::OperationCompleted);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        _Gate = nullptr;
        _State = CompletedWithError;
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            KQuotaGate::SPtr quotaGate;

            //
            // Create or get the entry for our specific disk id
            //
            Status = _OverlayManager->CreateOrGetLogStreamGate(_DiskId,
                                                               quotaGate);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            _State = WaitForQuanta;

            Status = quotaGate->CreateAcquireContext(_QuotaAcquireContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            _Gate = quotaGate;
            _QuotaAcquireContext->StartAcquire(1,
                                               this,
                                               completion);
            
            break;
        }

        case WaitForQuanta:
        {
            //
            // Quanta acquired from the gate, we can proceed
            //
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
LogStreamOpenGateContext::OnStart(
    )
{
    FSMContinue(STATUS_SUCCESS);
}

VOID
LogStreamOpenGateContext::StartWaitForGate(
    __in KGuid& DiskId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;
    _DiskId = DiskId;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
LogStreamOpenGateContext::OnReuse(
    )
{
    KInvariant(_Gate == nullptr);
    KInvariant(_QuotaAcquireContext == nullptr);
}

VOID
LogStreamOpenGateContext::ReleaseGate(
    __in OverlayStream& 
)
{
    KInvariant(_Gate);

    _Gate->ReleaseQuanta(1);
    _Gate = nullptr;
}


VOID
LogStreamOpenGateContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    _QuotaAcquireContext->Cancel();       
}

LogStreamOpenGateContext::LogStreamOpenGateContext(
    __in OverlayManager& OM
    ) : _OverlayManager(&OM)
{
    _Gate = nullptr;
    _QuotaAcquireContext = nullptr;
}

LogStreamOpenGateContext::~LogStreamOpenGateContext()
{
}

NTSTATUS
OverlayManager::CreateLogStreamOpenGateContext(
    __out LogStreamOpenGateContext::SPtr& Context
    )
{
    NTSTATUS status;
    LogStreamOpenGateContext::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) LogStreamOpenGateContext(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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

NTSTATUS
OverlayManager::CreateOrGetLogStreamGate(
    __in KGuid& DiskId,
    __out KQuotaGate::SPtr& Gate
)
{
    NTSTATUS status;    
    GateTableEntry* gateTableEntry;
    KQuotaGate::SPtr gate;

    GUID nullGuid = { 0 };  
    KInvariant(DiskId != nullGuid);
    
    status = STATUS_NOT_FOUND;
    Gate = nullptr;
    
    K_LOCK_BLOCK(_GateTableSpinLock)
    {
        //
        // See if there is an already existing gate for this diskid
        //
        gateTableEntry = _GateList.PeekHead();
        while (gateTableEntry != NULL)
        {
            if (gateTableEntry->DiskId == DiskId)
            {
                Gate = gateTableEntry->Gate;
                return(STATUS_SUCCESS);
            }

            gateTableEntry = _GateList.Successor(gateTableEntry);
        }

        //
        // As there is not already a gate for this diskid, create a new
        // one
        //
        status = KQuotaGate::Create(GetThisAllocationTag(),
                                    GetThisAllocator(),
                                    gate);
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
            return(status);
        }

        gateTableEntry = _new(GetThisAllocationTag(), GetThisAllocator()) GateTableEntry();
        if (! gateTableEntry)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
            return(status);
        }

        gateTableEntry->DiskId = DiskId;
        gateTableEntry->Gate = gate;
        _GateList.InsertHead(gateTableEntry);

        //
        // Activate gate but have the deactivation completion callback
        // be the CloseServiceCompletion as that is the component that
        // will handle deactivation
        //
        KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::CloseServiceCompletion);        
        gate->Activate(1,
                       this,
                       completion);
        Gate = gate;
        status = STATUS_SUCCESS;
    }
    
    return(status);
}

ULONG OverlayManager::DeactivateAllLogStreamGates(
    )
{
    ULONG gateCount = 0;
    GateTableEntry* gateTableEntry;

    K_LOCK_BLOCK(_GateTableSpinLock)
    {
        while ((gateTableEntry = _GateList.RemoveHead()) != NULL)
        {
            gateTableEntry->Gate->Deactivate();
            _delete(gateTableEntry);
            gateCount++;
        }
    }
    
    return(gateCount);
}


NTSTATUS
OverlayManager::FindLogStreamGate(
    __in KGuid& DiskId,
    __out KQuotaGate::SPtr& Gate
)
{
    NTSTATUS status;    
    GateTableEntry* gateTableEntry;

    status = STATUS_NOT_FOUND;
    Gate = nullptr;
    
    K_LOCK_BLOCK(_GateTableSpinLock)
    {
        gateTableEntry = _GateList.PeekHead();
        while (gateTableEntry != NULL)
        {
            if (gateTableEntry->DiskId == DiskId)
            {
                Gate = gateTableEntry->Gate;
                return(STATUS_SUCCESS);
            }
        }
    }

    return(status);
}

NTSTATUS
OverlayManager::MapPathToDiskId(
    __in KString const & Path,
    __out KGuid& DiskId
    )
{
    //
    // On windows, determine the diskId associated with the drive on which the path is located. For Linux
    // we hardcode the diskId to a specific guid as there are other dependencies in the code for a diskId,
    // specifically a gate that limits the number of streams open on a drive at one time. CONSIDER: adding
    // functionality in Linux KTL to determine the physical drives behind the different pathnames and map
    // them to different guids so different drives can have different gates.
    //

    //
    // For windows, use this random guid as the default in case we are
    // not able to map the path. In this way any paths that aren't
    // mapped will still have some kind of disk id
    //
    
    // {3C391CBC-4AC5-4a8b-B38B-413D17E1DB46}
    static const GUID aGuid = 
        { 0x3c391cbc, 0x4ac5, 0x4a8b, { 0xb3, 0x8b, 0x41, 0x3d, 0x17, 0xe1, 0xdb, 0x46 } };
    KGuid diskId = aGuid;
    
#if !defined(PLATFORM_UNIX)
    ULONG l;
    BOOLEAN b = FALSE;
    UCHAR driveLetter;
    PWCHAR p;
    ULONG len;

    DiskId = diskId;
    
    //
    // Expecting a path of the forms:
    //  Set A:
    //   "\\??\\Volume{<guid?}\..."
    //   "\\Global??\\Volume{<guid?}\..."
    //
    //  Set B:
    //   "\\Global??\\c:\\..."
    //   "\\??\\c:\\..."
    //
    p = static_cast<PWCHAR>(Path);
    len = Path.Length();

    //
    // See if the path contains the guid itself (set A)
    //
    Path.CompareNoCase(KStringView(L"\\??\\Volume"), l);
    if (l != (strlen("\\??\\Volume")))
    {
        Path.CompareNoCase(KStringView(L"\\Global??\\Volume"), l);
    }
    
    if ((l == (strlen("\\??\\Volume"))) || (l == (strlen("\\Global??\\Volume"))))
    {
        if (len >= l+(ULONG)strlen("{78572cc3-1981-11e2-be6c-806e6f6e6963}"))
        {
            KStringView guidString(p+l,
                                   (ULONG)strlen("{78572cc3-1981-11e2-be6c-806e6f6e6963}"),
                                   (ULONG)strlen("{78572cc3-1981-11e2-be6c-806e6f6e6963}"));
            b = guidString.ToGUID(DiskId);
        }
        return(STATUS_SUCCESS);
    }

    //
    // Otherwise see if the drive letter is there
    //
    Path.CompareNoCase(KStringView(L"\\??\\"), l);
    if (l != (strlen("\\??\\")))
    {
        Path.CompareNoCase(KStringView(L"\\Global??\\"), l);        
    }

    if ((l == (strlen("\\??\\"))) || (l == (strlen("\\Global??\\"))))
    {
        if ((len >= (l+2)) && (p[l+1] == L':'))
        {
            driveLetter = (UCHAR)KStringView::CharToUpper(p[l]);
            b = KVolumeNamespace::QueryVolumeIdFromDriveLetter(_VolumeList,
                                             driveLetter,
                                             DiskId);
            return(STATUS_SUCCESS);
        }
    }

    //
    // Otherwise we think it is a bad path
    //
    return(STATUS_OBJECT_PATH_INVALID);
#else
    DiskId = diskId;    
    return(STATUS_SUCCESS);
#endif
}


//
// Enum Logs
//
OverlayManagerBase::AsyncEnumerateLogs::AsyncEnumerateLogs()
{
}

OverlayManagerBase::AsyncEnumerateLogs::~AsyncEnumerateLogs()
{
}

VOID
OverlayManager::AsyncEnumerateLogsOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID OverlayManager::AsyncEnumerateLogsOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    _OM->ReleaseRequestRef();
    
    Complete(Async.Status());
}

VOID
OverlayManager::AsyncEnumerateLogsOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
        
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncEnumerateLogsOverlay::OperationCompletion);
    _BaseEnumerateLogs->StartEnumerateLogs(_DiskId,
                                           *_LogIdArray,
                                           this,
                                           completion);
}

VOID
OverlayManager::AsyncEnumerateLogsOverlay::StartEnumerateLogs(
    __in const KGuid& DiskId,
    __out KArray<RvdLogId>& LogIdArray,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _LogIdArray = &LogIdArray;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayManager::AsyncEnumerateLogsOverlay::~AsyncEnumerateLogsOverlay(
    )
{
}

VOID
OverlayManager::AsyncEnumerateLogsOverlay::OnReuse(
    )
{
    _BaseEnumerateLogs->Reuse();
}

OverlayManager::AsyncEnumerateLogsOverlay::AsyncEnumerateLogsOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
    NTSTATUS status;
    
    status = _OM->_BaseLogManager->CreateAsyncEnumerateLogsContext(_BaseEnumerateLogs);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

NTSTATUS
OverlayManager::CreateAsyncEnumerateLogsContext(
    __out AsyncEnumerateLogs::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncEnumerateLogsOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateLogsOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Create log
//
OverlayManagerBase::AsyncCreateLog::AsyncCreateLog()
{
}

OverlayManagerBase::AsyncCreateLog::~AsyncCreateLog()
{
}

VOID
OverlayManager::AsyncCreateLogOverlay::QueueDeactivated(
    __in ServiceWrapper&
    )
{
    DoComplete(_FinalStatus);
}

VOID
OverlayManager::AsyncCreateLogOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    if ((! NT_SUCCESS(Status)) && (_OCReferenced))
    {
        //
        // On a failure we need to release any ref to the OC that is
        // keeping it on the list.
        //        
        BOOLEAN freed = _OM->_ContainersTable.ReleaseOverlayContainer(*_OC);
        if (freed)
        {
            _State = Cleanup;
            _OCReferenced = FALSE;
            _FinalStatus = Status;
                        
            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                   &OverlayManager::AsyncCreateLogOverlay::QueueDeactivated);           
            _OC->DeactivateQueue(queueDeactivated);
            return;                     
        }
    }
    
    _OC = nullptr;
    _OCOperation = nullptr;
    _Path = nullptr;

    _OM->ReleaseRequestRef();
    
    Complete(Status);
}

VOID
OverlayManager::AsyncCreateLogOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncCreateLogOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            BOOLEAN addedToList;
            
            _State = InitializeContainer;
            //
            // First thing is to get the OverlayContainer
            //

            Status = _OM->_ContainersTable.CreateOrGetOverlayContainer(_DiskId,
                                                                       _LogId,
                                                                       _OC,
                                                                       addedToList);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            //
            // Next step is to kick off the create process
            //
            _OCReferenced = TRUE;

            _OCOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                OverlayLogFreeService::OperationCreate(*_OC,
                                                        _DiskId,
                                                        _Path.RawPtr(),
                                                        _MaxRecordSize,
                                                        _MaxNumberStreams,
                                                        *_LogType,
                                                        _LogSize,
                                                        _Flags);

            if (! _OCOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;             
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _OCOperation->Status();
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _OCOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            
            _OCOperation->StartCreateOperation(this,
                                               completion);
            
            break;
        }

        case InitializeContainer:
        {
            //
            // Now that the container is initialized we can go ahead
            // and reopen it
            //
            _State = ReopenContainer;
            _OCOperation->Reuse();
            _OCOperation->StartOpenOperation(this,
                                             completion);
            break;
        }

        case ReopenContainer:
        {
            //
            // Container is opened and ready to go
            //
            _State = Completed;         
            _OCReferenced = FALSE;    // Keep this on the list
            *_LogPtr = down_cast<OverlayLog>(_OC->GetUnderlyingService());
            DoComplete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncCreateLogOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncCreateLogOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
        
    _State = Initial;
    _OCReferenced = FALSE;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncCreateLogOverlay::StartCreateLog(
    __in KGuid const& DiskId,
    __in_opt KString const * const Path,
    __in RvdLogId const& LogId,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in ULONG MaxAllowedStreams,
    __in ULONG MaxRecordSize,
    __in DWORD Flags,
    __out OverlayLog::SPtr& Log,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = Path;
    _LogId = LogId;
    _LogType = &LogType;
    _LogSize = LogSize;
    _LogPtr = &Log;
    _MaxNumberStreams = MaxAllowedStreams == 0 ? _DefaultMaxNumberStreams : MaxAllowedStreams;
    _MaxRecordSize = MaxRecordSize;
    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncCreateLogOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            
OverlayManager::AsyncCreateLogOverlay::~AsyncCreateLogOverlay(
    )
{
}

OverlayManager::AsyncCreateLogOverlay::AsyncCreateLogOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

VOID
OverlayManager::AsyncCreateLogOverlay::OnReuse(
    )
{
}

NTSTATUS
OverlayManager::CreateAsyncCreateLogContext(
    __out AsyncCreateLog::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncCreateLogOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Open Log
//

OverlayManagerBase::AsyncOpenLog::AsyncOpenLog()
{
}

OverlayManagerBase::AsyncOpenLog::~AsyncOpenLog()
{
}

VOID
OverlayManager::AsyncOpenLogOverlay::QueueDeactivated(
    __in ServiceWrapper&
    )
{
    DoComplete(_FinalStatus);
}

VOID
OverlayManager::AsyncOpenLogOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    if ((! NT_SUCCESS(Status)) && (_OCReferenced))
    {
        //
        // On a failure we need to release any ref to the OC that is
        // keeping it on the list.
        //
        
        BOOLEAN freed = _OM->_ContainersTable.ReleaseOverlayContainer(*_OC);
        if (freed)
        {
            _State = Cleanup;
            _OCReferenced = FALSE;
            _FinalStatus = Status;
            
            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                   &OverlayManager::AsyncOpenLogOverlay::QueueDeactivated);
            _OC->DeactivateQueue(queueDeactivated);
            return;                     
        }
    }
    
    _OC = nullptr;
    _OCOperation = nullptr;
    _Path = nullptr;

    _OM->ReleaseRequestRef();
    
    Complete(Status);
}


VOID
OverlayManager::AsyncOpenLogOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncOpenLogOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            BOOLEAN addedToList;
            //
            // First thing is to get the OverlayContainer
            //
            _State = OpenLog;

            Status = _OM->_ContainersTable.CreateOrGetOverlayContainer(_DiskId,
                                                                       _LogId,
                                                                       _OC,
                                                                       addedToList);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _OCReferenced = TRUE;

            _OCOperation = _new(GetThisAllocationTag(), GetThisAllocator()) OverlayLogFreeService::OperationOpen(
                                                                               *_OC,
                                                                               _DiskId,
                                                                               _Path.RawPtr());
            if (! _OCOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _OCOperation->Status();
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _OCOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));            
            
            _OCOperation->StartOpenOperation(this,
                                             completion);
            
            break;
        }

        case OpenLog:
        {
            //
            // Log opened successfully
            //
            *_LogPtr = down_cast<OverlayLog>(_OC->GetUnderlyingService());
            DoComplete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncOpenLogOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncOpenLogOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
        
    _State = Initial;
    _OCReferenced = FALSE;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncOpenLogOverlay::StartOpenLog(
    __in const KGuid& DiskId,
    __in_opt KString const * const Path,                    
    __in const RvdLogId& LogId,
    __out OverlayLog::SPtr& Log,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = Path;
    _LogId = LogId;
    _LogPtr = &Log;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncOpenLogOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            

OverlayManager::AsyncOpenLogOverlay::~AsyncOpenLogOverlay(
    )
{
}

VOID
OverlayManager::AsyncOpenLogOverlay::OnReuse(
    )
{
}

OverlayManager::AsyncOpenLogOverlay::AsyncOpenLogOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

NTSTATUS
OverlayManager::CreateAsyncOpenLogContext(
    __out AsyncOpenLog::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncOpenLogOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// QueryLogId
//

OverlayManagerBase::AsyncQueryLogId::AsyncQueryLogId()
{
}

OverlayManagerBase::AsyncQueryLogId::~AsyncQueryLogId()
{
}

VOID
OverlayManager::AsyncQueryLogIdOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    _OM->ReleaseRequestRef();
    
    Complete(Status);
}


VOID
OverlayManager::AsyncQueryLogIdOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncQueryLogIdOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            RvdLogManager::AsyncQueryLogId::SPtr context;

            Status = _OM->GetBaseLogManager()->CreateAsyncQueryLogIdContext(context);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _State = QueryLogId;
            context->StartQueryLogId(*_Path, *_LogId, this, completion);
            
            break;
        }

        case QueryLogId:
        {           
            DoComplete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncQueryLogIdOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncQueryLogIdOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
        
    _State = Initial;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncQueryLogIdOverlay::StartQueryLogId(
    __in const KStringView& FullyQualifiedLogFilePath,
    __out RvdLogId& LogId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Path = &FullyQualifiedLogFilePath;
    _LogId = &LogId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncQueryLogIdOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            

OverlayManager::AsyncQueryLogIdOverlay::~AsyncQueryLogIdOverlay(
    )
{
}

VOID
OverlayManager::AsyncQueryLogIdOverlay::OnReuse(
    )
{
}

OverlayManager::AsyncQueryLogIdOverlay::AsyncQueryLogIdOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

NTSTATUS
OverlayManager::CreateAsyncQueryLogIdContext(
    __out AsyncQueryLogId::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncQueryLogIdOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryLogIdOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Close Log
//

VOID
OverlayManager::AsyncCloseLogOverlay::QueueDeactivated(
    __in ServiceWrapper&
    )
{
    DoComplete(_FinalStatus);
}

VOID
OverlayManager::AsyncCloseLogOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    if (_OCReferenced)
    {
        BOOLEAN freed = _OM->_ContainersTable.ReleaseOverlayContainer(*_OC);
        if (freed)
        {
            _State = Cleanup;
            _OCReferenced = FALSE;
            _FinalStatus = Status;
            
            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                   &OverlayManager::AsyncCloseLogOverlay::QueueDeactivated);           
            _OC->DeactivateQueue(queueDeactivated);
            return;
        }
   }

    _Log = nullptr;
    _OC = nullptr;
    _OCOperation = nullptr;

    _OM->ReleaseRequestRef();
    
    Complete(Status);
}


VOID
OverlayManager::AsyncCloseLogOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncCloseLogOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        if ((_State == CloseLog) && (Status == STATUS_OBJECT_NO_LONGER_EXISTS))
        {
            //
            // This is the case where a log was deleted while it was
            // opened. We need to be sure to take away the onlist count
            // for that open
            //
            _State = CloseLog;
        } else {        
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            DoComplete(Status);
            return;
        }
    }
    
    switch (_State)
    {
        case Initial:
        {
            BOOLEAN addedToList;
            
            //
            // First thing is to get the OverlayContainer
            //
            _State = CloseLog;

            Status = _OM->_ContainersTable.CreateOrGetOverlayContainer(_Log->GetDiskId(),
                                                                       _Log->GetContainerId(),
                                                                       _OC,
                                                                       addedToList);

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            KInvariant(! addedToList);

            _OCReferenced = TRUE;
            Status = ServiceWrapper::Operation::Create(_OCOperation,
                                                     *(static_cast<ServiceWrapper*>(_OC.RawPtr())),
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());

            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_Log->GetContainerId().Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _OCOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));         
            
            _OCOperation->StartCloseOperation(_Log.RawPtr(),
                                              this,
                                              completion);
            
            break;
        }

        case CloseLog:
        {
            //
            // Log Closeed successfully, release the ref from the open
            // that was keeping it on the list
            //
            BOOLEAN freed = _OM->_ContainersTable.ReleaseOverlayContainer(*_OC);

            if (freed)
            {
                _State = Cleanup;
                _FinalStatus = Status;

                ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                       &OverlayManager::AsyncCloseLogOverlay::QueueDeactivated);           
                _OC->DeactivateQueue(queueDeactivated);
                return;
            }

            DoComplete(Status);
            return;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncCloseLogOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncCloseLogOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
        
    _State = Initial;
    _OCReferenced = FALSE;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncCloseLogOverlay::StartCloseLog(
    __in const OverlayLog::SPtr& OverlayLog,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Log = OverlayLog;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncCloseLogOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            

OverlayManager::AsyncCloseLogOverlay::~AsyncCloseLogOverlay(
    )
{
}

VOID
OverlayManager::AsyncCloseLogOverlay::OnReuse(
    )
{
}

OverlayManager::AsyncCloseLogOverlay::AsyncCloseLogOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

NTSTATUS
OverlayManager::CreateAsyncCloseLogContext(
    __out AsyncCloseLogOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncCloseLogOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseLogOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// AllocateKIoBuffer
//

const ULONG ThrottledKIoBufferAllocator::_ThrottleLinkageOffset = FIELD_OFFSET(ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext,
                                                                 _AllocsWaitingListEntry);

ThrottledKIoBufferAllocator::ThrottledKIoBufferAllocator() :
    _WaitingAllocsList(_ThrottleLinkageOffset)
{
}

ThrottledKIoBufferAllocator::ThrottledKIoBufferAllocator(
    __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits
    ) :
   _WaitingAllocsList(_ThrottleLinkageOffset)

{
    NTSTATUS status;

    _AllocationTimeoutInMs = MemoryThrottleLimits.AllocationTimeoutInMs;
    _TotalAllocationLimit = MemoryThrottleLimits.WriteBufferMemoryPoolMin;
    
    _CurrentAllocations = 0;
    _ShuttingDown = FALSE;
    _TimerRunning = FALSE;

    status = KTimer::Create(_Timer, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

ThrottledKIoBufferAllocator::~ThrottledKIoBufferAllocator()
{
    KInvariant(_CurrentAllocations == 0);
}

NTSTATUS
ThrottledKIoBufferAllocator::CreateThrottledKIoBufferAllocator(
    __in KtlLogManager::MemoryThrottleLimits& MemoryThrottleLimits,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __out ThrottledKIoBufferAllocator::SPtr& Context
    )
{
    NTSTATUS status;
    ThrottledKIoBufferAllocator::SPtr context;

    context = _new(AllocationTag, Allocator) ThrottledKIoBufferAllocator(MemoryThrottleLimits);
    
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, 0, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->SetLimit(MemoryThrottleLimits.WriteBufferMemoryPoolMin,
                      MemoryThrottleLimits.WriteBufferMemoryPoolMax,
                      MemoryThrottleLimits.WriteBufferMemoryPoolPerStream);
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

VOID ThrottledKIoBufferAllocator::SetLimit(
    __in LONGLONG Min,
    __in LONGLONG Max,
    __in LONG PerStream                                         
)
{
    LONGLONG newLimit = Min;
    LONGLONG actual, expected;
    do
    {
        expected = _TotalAllocationLimit;
        actual = InterlockedCompareExchange64(&_TotalAllocationLimit, newLimit, expected);
    } while (actual != expected);

    if (newLimit > expected)
    {
        //
        // Since the limit has grown, we may be able to provide memory to
        // requests that are waiting
        //
        ProcessFreedMemory();
    }

    if (Max == KtlLogManager::MemoryThrottleLimits::_NoLimit)
    {
        Max = LONGLONG_MAX;
    }
    _MaxAllocationLimit = Max;
    _PerStreamAllocation = PerStream;
    
}

LONG ThrottledKIoBufferAllocator::AddToLimit(
)
{
    LONG addedToLimit = 0;
    
    if ((_TotalAllocationLimit != KtlLogManager::MemoryThrottleLimits::_NoLimit) &&
        (_PerStreamAllocation != 0))
    {
        LONGLONG expected;
        LONGLONG newLimit;
        LONGLONG actual;
        do
        {
            expected = _TotalAllocationLimit;
            newLimit = expected + _PerStreamAllocation;
            
            if (newLimit <= _MaxAllocationLimit)
            {
                addedToLimit = _PerStreamAllocation;
            } else {
                break;
            }

            actual = InterlockedCompareExchange64(&_TotalAllocationLimit, newLimit, expected);
        } while (actual != expected);
    }

    //
    // Since the limit has grown, we may be able to provide memory to
    // requests that are waiting
    //
    ProcessFreedMemory();

    return(addedToLimit);
}

VOID ThrottledKIoBufferAllocator::RemoveFromLimit(
    LONG Limit
)
{
    if (_TotalAllocationLimit != KtlLogManager::MemoryThrottleLimits::_NoLimit)
    {
        LONG limit = Limit;

        KInvariant(limit <= _TotalAllocationLimit);

        InterlockedAdd64(&_TotalAllocationLimit, -1*limit);
        //
        // Allocator may be running above the new limit but won't be
        // providing allocations until the allocator is back under the
        // limit
        //
    }
}

VOID ThrottledKIoBufferAllocator::ProcessFreedMemory()
{
    NTSTATUS status;
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr waitingAlloc;

    do
    {
        waitingAlloc = RemoveTopOfList();

        if (waitingAlloc)
        {
            //
            // If there is not enough free memory to satisfy the item
            // at the top of the list then put it back on the top of
            // the list for next time. This way larger allocations
            // won't get starved while smaller allocations are
            // satisfied
            //
            status = AllocateIfPossible(TRUE,      // Insert at head of list
                                        *waitingAlloc);
            
            if (status != STATUS_PENDING)
            {
                //
                // alloc was either successful or failed. Since it was actionable then 
                // we want to get the next one on the list.
                //
                KDbgCheckpointWDataInformational(waitingAlloc->GetActivityId(), "ThrottledKIoBufferAllocator waiter processed", status,
                    (ULONGLONG)waitingAlloc.RawPtr(),
                    (ULONGLONG)waitingAlloc->GetSize(),
                    (ULONGLONG)_CurrentAllocations,
                    (ULONGLONG)_TotalAllocationLimit);
                waitingAlloc->DoComplete(status);
            }
        }
        else 
        {
            //
            // if list is empty then nothing to do
            //
            status = STATUS_PENDING;
        }
    } while (status != STATUS_PENDING);
}

VOID ThrottledKIoBufferAllocator::ProcessTimedOutRequests()
{
    NTSTATUS status = STATUS_SUCCESS;
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr waitingAlloc;
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr nextAlloc;

    if (_TotalAllocationLimit != KtlLogManager::MemoryThrottleLimits::_NoLimit)
    {   
        ULONGLONG tickCount = KNt::GetTickCount64();
        
        K_LOCK_BLOCK(_Lock)
        {
            waitingAlloc = _WaitingAllocsList.PeekHead();
            while (waitingAlloc)
            {

                KDbgCheckpointWData(waitingAlloc->GetActivityId(), "ThrottledKIoBufferAllocator consider timeout", status,
                    (ULONGLONG)waitingAlloc.RawPtr(),
                    (ULONGLONG)waitingAlloc->GetSize(),
                    (ULONGLONG)waitingAlloc->GetTickTimeout(),
                    (ULONGLONG)tickCount);
                
                KDbgCheckpointWData(waitingAlloc->GetActivityId(), "ThrottledKIoBufferAllocator consider timeout 2", status,
                    (ULONGLONG)waitingAlloc.RawPtr(),
                    (ULONGLONG)waitingAlloc->GetSize(),
                    (ULONGLONG)_CurrentAllocations,
                    (ULONGLONG)_TotalAllocationLimit);
                
                nextAlloc = _WaitingAllocsList.Successor(waitingAlloc.RawPtr());
                if ((waitingAlloc->GetTickTimeout() != 0) && (waitingAlloc->GetTickTimeout() <= tickCount))
                {
                    //
                    // Allocation has timed out, complete with failure
                    //
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    KDbgCheckpointWDataInformational(waitingAlloc->GetActivityId(), "ThrottledKIoBufferAllocator OutOfRetries", status,
                        (ULONGLONG)waitingAlloc.RawPtr(),
                        (ULONGLONG)waitingAlloc->GetSize(),
                        (ULONGLONG)_CurrentAllocations,
                        (ULONGLONG)_TotalAllocationLimit);
                    
                    _WaitingAllocsList.Remove(waitingAlloc.RawPtr());
                    waitingAlloc->CompleteRequest(status);
                }
                waitingAlloc = nextAlloc;
            }
        }
    }
}


VOID ThrottledKIoBufferAllocator::AddToList(
    __in BOOLEAN InsertHead,
    __in AsyncAllocateKIoBufferContext& WaitingAlloc
)
{
    //
    // NOTE that there is no need to take ref for allocs on the list as they should be
    //      kept alive by the fact that they are running asyncs.
    //
    //      It is assumed that this is only called when holding the
    //      lock for the list
    //
    if (InsertHead)
    {
        _WaitingAllocsList.InsertHead(&WaitingAlloc);
    } else {
        _WaitingAllocsList.AppendTail(&WaitingAlloc);
    }

    BOOLEAN startTimer = RestartTimerIfNeeded();

    if (startTimer)
    {
        KAsyncContextBase::CompletionCallback completion(this,
                                                         &ThrottledKIoBufferAllocator::TimerCompletion);
        _Timer->Reuse();
        _Timer->StartTimer(_PingTimerInMs, nullptr, completion);
    }
    
}

ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr ThrottledKIoBufferAllocator::RemoveTopOfList()
{
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr alloc;

    K_LOCK_BLOCK(_Lock)
    {
        alloc = _WaitingAllocsList.RemoveHead();
        if (alloc)
        {
#if VERBOSE
            KDbgCheckpointWData(alloc->GetActivityId(), "ThrottledKIoBufferAllocator RemoveFromList", STATUS_SUCCESS,
                (ULONGLONG)alloc.RawPtr(),
                (ULONGLONG)alloc->GetSize(),
                (ULONGLONG)_CurrentAllocations,
                (ULONGLONG)_TotalAllocationLimit);
#endif
        }
    }

    return(alloc);
}

VOID
ThrottledKIoBufferAllocator::FreeKIoBuffer(
    __in KActivityId ,
    __in LONG Size
    )
{
    
    KInvariant(_CurrentAllocations >= Size);
    InterlockedAdd64(&_CurrentAllocations, -1*Size);

    //
    // Since memory has been freed, there may be waiting allocs that
    // can be specified
    //
    ProcessFreedMemory();
}

#pragma warning(disable:4702)

NTSTATUS
ThrottledKIoBufferAllocator::AllocateIfPossible(
    __in BOOLEAN RetryAlloc,                                                
    __in AsyncAllocateKIoBufferContext& Alloc
)
{
    NTSTATUS status = STATUS_PENDING;
    AsyncAllocateKIoBufferContext::SPtr alloc = &Alloc;

    if (_TotalAllocationLimit != KtlLogManager::MemoryThrottleLimits::_NoLimit)
    {   
        //
        // If the allocation is more than the limit then fail the request
        //
        if (_TotalAllocationLimit < alloc->GetSize())
        {
            status = STATUS_INVALID_BUFFER_SIZE;
            KTraceFailedAsyncRequest(status, alloc.RawPtr(), alloc->GetSize(), _TotalAllocationLimit);
            return(status);
        }

        //
        // Is there enough allocation for our request
        //
        K_LOCK_BLOCK(_Lock)
        {
            if ((alloc->GetSize() <= GetFreeAllocation()) &&
                ((RetryAlloc) ||
                 ((! RetryAlloc) && (_WaitingAllocsList.Count() == 0))))
            {
                //
                // We do not want to allow newer smaller allocs to go
                // ahead of older larger allocs. So if there is space to
                // satisfy the alloc and it is either a retry for an older alloc
                // or a newer alloc with no older allocs waiting then move
                // forward
                // 
                InterlockedAdd64(&_CurrentAllocations, alloc->GetSize());
            } else {
                //
                // Not enough space, if this is a retry of an allocation
                // that is already on the list then place this on the back of the list
                // at the head so next time around we try to satisfy
                // this allocation
                //
                
#if VERBOSE
                KDbgCheckpointWDataInformational(alloc->GetActivityId(), "ThrottledKIoBufferAllocator AddToAlloclist", STATUS_SUCCESS,
                    (ULONGLONG)alloc.RawPtr(),
                    (ULONGLONG)alloc->GetSize(),
                    (ULONGLONG)_CurrentAllocations,
                    (ULONGLONG)_TotalAllocationLimit);
                KDbgCheckpointWDataInformational(alloc->GetActivityId(), "ThrottledKIoBufferAllocator AddToAlloclist2", STATUS_SUCCESS,
                    (ULONGLONG)alloc.RawPtr(),
                    (ULONGLONG)alloc->GetSize(),
                    (ULONGLONG)alloc->GetTimeout(),
                    (ULONGLONG)alloc->GetTickTimeout());
#endif
                
                AddToList(RetryAlloc, Alloc);
                // NOTE: once placed back on the list, another thread
                //       may complete it
                return(STATUS_PENDING);
            }
        }
    } else {
        InterlockedAdd64(&_CurrentAllocations, alloc->GetSize());
    }

    //
    // Perform allocation outside of lock. Instead of allocating a
    // single large contiguous blocks, we allocate a series of smaller
    // blocks. Since IO is done via scatter gather anyway there is not
    // much downside to the smaller blocks and the advantage of a
    // smaller chance of failing when trying to create a large contiguous
    // block. 
    //
    // TODO: _AllocationExtentSize may need to be configurable or
    //       adjusted.
    //
    ULONG elementCount;
    ULONG lastElementSize;

    elementCount = alloc->GetSizeUlong() / alloc->GetAllocationExtentSize();
    lastElementSize = alloc->GetSizeUlong() % alloc->GetAllocationExtentSize();

    KIoBuffer::SPtr ioBuffer;
    KIoBufferElement::SPtr ioBufferElement;
    PVOID p;

    status = KIoBuffer::CreateEmpty(ioBuffer, GetThisAllocator(), GetThisAllocationTag());

    for (ULONG i = 0; i < elementCount; i++)
    {
        status = KIoBufferElement::CreateNew(alloc->GetAllocationExtentSize(), ioBufferElement, p, GetThisAllocator(), GetThisAllocationTag());
        if (!NT_SUCCESS(status))
        {
            break;
        }

        ioBuffer->AddIoBufferElement(*ioBufferElement);
    }

    if (NT_SUCCESS(status))
    {
        if (lastElementSize != 0)
        {
            status = KIoBufferElement::CreateNew(lastElementSize, ioBufferElement, p, GetThisAllocator(), GetThisAllocationTag());
            if (NT_SUCCESS(status))
            {
                ioBuffer->AddIoBufferElement(*ioBufferElement);
                alloc->SetIoBufferPtr(ioBuffer.RawPtr());
            }
        } else {
            alloc->SetIoBufferPtr(ioBuffer.RawPtr());
        }
    }

    //
    // If we failed to allocate all of the memory from the system then
    // we need to try again or give up
    //    
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(alloc->GetActivityId(), status, alloc.RawPtr(), alloc->GetSize(), _CurrentAllocations);
        
        K_LOCK_BLOCK(_Lock)   
        {
            InterlockedAdd64(&_CurrentAllocations,  -1* alloc->GetSize());

            //
            // Not enough space, if this is a retry of an allocatin
            // that is already on the list then place this on the back of the list
            // at the head so next time around we try to satisfy
            // this allocation
            //
#if VERBOSE            
            KDbgCheckpointWDataInformational(alloc->GetActivityId(), "ThrottledKIoBufferAllocator AddToAlloclistB", status,
                (ULONGLONG)alloc.RawPtr(),
                (ULONGLONG)alloc->GetSize(),
                (ULONGLONG)_CurrentAllocations,
                (ULONGLONG)_TotalAllocationLimit);
            KDbgCheckpointWDataInformational(alloc->GetActivityId(), "ThrottledKIoBufferAllocator AddToAlloclistB2", STATUS_SUCCESS,
                (ULONGLONG)alloc.RawPtr(),
                (ULONGLONG)alloc->GetSize(),
                (ULONGLONG)alloc->GetTimeout(),
                (ULONGLONG)alloc->GetTickTimeout());
#endif            
            AddToList(RetryAlloc, Alloc);
            // NOTE: once placed back on the list, another thread
            //       may complete it
        }        
        return(STATUS_PENDING);
    }
    return(status);
}

#pragma warning(default:4702)

VOID ThrottledKIoBufferAllocator::TimerCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();

    _TimerRunning = FALSE;
    
    if (status != STATUS_CANCELLED)
    {
        //
        // Try to satisfy waiting requests
        //
        ProcessFreedMemory();

        //
        // Go through the waiting lists and see if any allocations have
        // timed out
        //
        ProcessTimedOutRequests();

        // CONSIDER: If total requests are above a threshold then take
        // action by asking streams to trim their allocations and/or
        // flush
        
        BOOLEAN startTimer = FALSE;
        K_LOCK_BLOCK(_Lock)
        {
            startTimer = RestartTimerIfNeeded();
        }

        if (startTimer)
        {
            KAsyncContextBase::CompletionCallback completion(this,
                                                             &ThrottledKIoBufferAllocator::TimerCompletion);
            _Timer->Reuse();
            _Timer->StartTimer(_PingTimerInMs, nullptr, completion);
        }
    } else {
        //
        // Remove ref taken when cancelled. After release the object
        // might no longer exist
        //
        KDbgPrintfInformational("Timer Cancelled");
        KInvariant(_ShuttingDown);
        Release();
    }
}

BOOLEAN ThrottledKIoBufferAllocator::RestartTimerIfNeeded()
{
    //
    // This routine assumes that the _Lock is held
    //
    BOOLEAN startTimer = FALSE;
    
    if ((_WaitingAllocsList.Count() > 0) && (! _TimerRunning) && (! _ShuttingDown))
    {
        _TimerRunning = TRUE;
        startTimer = TRUE;
    }
    return(startTimer);
}

VOID ThrottledKIoBufferAllocator::Shutdown()
{
    //
    // Take refcount so this will live until the timer is cancelled
    //
    K_LOCK_BLOCK(_Lock)
    {
        _ShuttingDown = TRUE;
        if (_TimerRunning)
        {
            AddRef();
            _Timer->Cancel();
        }
    }   
}

VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::DoComplete(
    __in NTSTATUS Status 
    )
{
    Complete(Status);
}


VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::OperationCompletion);

#if VERBOSE
    KDbgCheckpointWDataInformational(GetActivityId(), "mthrottle AsyncAllocateKIoBufferContext", Status,
                        (ULONGLONG)this,
                        (ULONGLONG)_Size,
                        (ULONGLONG)_OM->GetCurrentAllocations(),
                        (ULONGLONG)_OM->GetTotalAllocationLimit());         
#endif        
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OM->GetTotalAllocationLimit(), _OM->GetCurrentAllocations());
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            if (_Timeout == KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs)
            {
                _Timeout = _OM->GetAllocationTimeoutInMs();
            }
            
            if (_Timeout == KtlLogManager::MemoryThrottleLimits::_NoAllocationTimeoutInMs)
            {
                _TickTimeout = 0;
            } else {
                _TickTimeout = KNt::GetTickCount64() + _Timeout;
            }
            
            Status =  _OM->AllocateIfPossible(FALSE,    // Insert at end of list
                                              *this);
            if (Status != STATUS_PENDING)
            {
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _OM->GetTotalAllocationLimit(), _OM->GetCurrentAllocations());
                }
                DoComplete(Status);
            }
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
    // CONSIDER: Do we need a cancel ?
}

VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::OnStart(
    )
{
    _State = Initial;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::StartAllocateKIoBuffer(
    __in ULONG Size,
    __in ULONGLONG Timeout,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    //
    // Size must be a multiple of 4K
    //
    KInvariant((Size % 0x1000) == 0);
    
    
    _Size = (LONG)Size;
    _IoBufferPtr = &IoBuffer;
    *_IoBufferPtr = nullptr;
    _Timeout = Timeout;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            

ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::~AsyncAllocateKIoBufferContext(
    )
{
}

VOID
ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::OnReuse(
    )
{
}

ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::AsyncAllocateKIoBufferContext(
    ThrottledKIoBufferAllocator& ThrottledAllocator
    ) : _OM(&ThrottledAllocator)
{
}

NTSTATUS
ThrottledKIoBufferAllocator::CreateAsyncAllocateKIoBufferContext(
    __out AsyncAllocateKIoBufferContext::SPtr& Context
    )
{
    NTSTATUS status;
    ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAllocateKIoBufferContext(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Throttled Allocator
//
VOID OverlayManager::FreeKIoBuffer(
    __in KActivityId ActivityId,
    __in ULONG Size
  )
{
    _ThrottledAllocator->FreeKIoBuffer(ActivityId, Size);
}

NTSTATUS
OverlayManager::CreateAsyncAllocateKIoBufferContext(
    __out ThrottledKIoBufferAllocator::AsyncAllocateKIoBufferContext::SPtr& Context)
{
    return(_ThrottledAllocator->CreateAsyncAllocateKIoBufferContext(Context));
}


//
// ConfigureContext
//
VOID
OverlayManager::AsyncConfigureContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _InBuffer = nullptr;
    
    _OM->ReleaseRequestRef();
        
    Complete(Status);
}

VOID
OverlayManager::AsyncConfigureContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    ULONG sizeExpected;
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncConfigureContextOverlay::OperationCompletion);
    
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            KDbgCheckpointWDataInformational(_OM->GetActivityId(),
                                "OverlayManager::AsyncConfigureContextOverlay::FSMContinue", Status,
                                (ULONGLONG)_State,
                                (ULONGLONG)this,
                                (ULONGLONG)_Code,
                                (ULONGLONG)0);

            //
            // assume that the control is not valid
            //
            Status = STATUS_INVALID_PARAMETER;
            
            switch (_Code)
            {
                case KtlLogManager::ConfigureSharedLogContainerSettings:
                {                                                                          
                    //
                    // Establish the new write throttle threshold
                    //
                    KtlLogManager::SharedLogContainerSettings* settings;

                    if (! _InBuffer || (_InBuffer->QuerySize() < sizeof(KtlLogManager::SharedLogContainerSettings)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        DoComplete(Status);
                        return;
                    }

                    settings = (KtlLogManager::SharedLogContainerSettings*)_InBuffer->GetBuffer();
                    
                    //
                    // Validate settings
                    //
                    if (settings->LogSize < KtlLogManager::SharedLogContainerSettings::_DefaultLogSizeMin)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->LogSize,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((settings->MaximumNumberStreams < KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMin) ||
                        (settings->MaximumNumberStreams > KtlLogManager::SharedLogContainerSettings::_DefaultMaximumNumberStreamsMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->MaximumNumberStreams,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if (settings->MaximumRecordSize != 0)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->MaximumRecordSize,
                                                 0);

                        DoComplete(Status);
                        return;
                    }
                    
                    if ((settings->Flags != 0) &&
                        (settings->Flags != KtlLogManager::FlagSparseFile))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->Flags,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    KGuid guidNull(0x00000000,0x0000,0x0000,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
                    if ((settings->DiskId != guidNull) && (*settings->Path != 0))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->DiskId.Data1,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if (((settings->LogContainerId.Get() == guidNull) && (*settings->Path != 0)) ||
                        ((settings->LogContainerId.Get() != guidNull) && (*settings->Path == 0)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->DiskId.Data1,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if (*settings->Path != 0)
                    {
                        KString::SPtr path;
                        KGuid diskId;

                        path = KString::Create(settings->Path,
                                               GetThisAllocator());
                        if (path == nullptr)
                        {
                            Status = STATUS_INSUFFICIENT_RESOURCES;
                            KTraceFailedAsyncRequest(Status, this,
                                                     settings->DiskId.Data1,
                                                     0);

                            DoComplete(Status);
                            return;
                        }

                        //
                        // Validate format of the path passed in
                        //
#if !defined(PLATFORM_UNIX)                     
                        Status = _OM->MapPathToDiskId(*path, diskId);
#else
                        // CONSIDER; Doing the below for both windows
                        // and linux
                        KWString objectRoot(GetThisAllocator());
                        KWString rootRelative(GetThisAllocator());

                        Status = objectRoot.Status();
                        if (NT_SUCCESS(Status))
                        {
                            Status = rootRelative.Status();
                            if (NT_SUCCESS(Status))
                            {
                                Status = KVolumeNamespace::SplitAndNormalizeObjectPath(*path,
                                                                                       objectRoot,
                                                                                       rootRelative);
                            }
                        }
#endif
                        if (! NT_SUCCESS(Status))
                        {
                            KTraceFailedAsyncRequest(Status, this,
                                                     0,
                                                     0);
                            DoComplete(Status);
                            return;
                        }
                    }
                    
                    settings = (KtlLogManager::SharedLogContainerSettings*)_InBuffer->GetBuffer();
                    _OM->_SharedLogContainerSettings = *settings;

                    Status = STATUS_SUCCESS;
                    break;
                }

                case KtlLogManager::ConfigureMemoryThrottleLimits:
                {
                    sizeExpected = FIELD_OFFSET(KtlLogManager::MemoryThrottleLimits, MaximumDestagingWriteOutstanding);
                    goto DoConfigureMemoryThrottleLimits;
                }
                
                case KtlLogManager::ConfigureMemoryThrottleLimits2:
                {
                    sizeExpected = FIELD_OFFSET(KtlLogManager::MemoryThrottleLimits, SharedLogThrottleLimit);
                    goto DoConfigureMemoryThrottleLimits;
                }
                
                case KtlLogManager::ConfigureMemoryThrottleLimits3:
                {
                    sizeExpected = sizeof(KtlLogManager::MemoryThrottleLimits);                                         ;
DoConfigureMemoryThrottleLimits:
                    
                    //
                    // Establish the new throttle memory limits
                    //
                    KtlLogManager::MemoryThrottleLimits* limits;                                                                                               
                    
                    if (! _InBuffer || (_InBuffer->QuerySize() < sizeExpected))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        DoComplete(Status);
                        return;
                    }

                    limits = (KtlLogManager::MemoryThrottleLimits*)_InBuffer->GetBuffer();

                    //
                    // Validate settings
                    //
                    if ((limits->PeriodicFlushTimeInSec < KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSecMin) ||
                        (limits->PeriodicFlushTimeInSec > KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicFlushTimeInSecMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->PeriodicFlushTimeInSec,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((limits->PeriodicTimerIntervalInSec < KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSecMin) ||
                        (limits->PeriodicTimerIntervalInSec > KtlLogManager::MemoryThrottleLimits::_DefaultPeriodicTimerIntervalInSecMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->PeriodicTimerIntervalInSec,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((limits->AllocationTimeoutInMs != KtlLogManager::MemoryThrottleLimits::_UseDefaultAllocationTimeoutInMs) &&
                        (limits->AllocationTimeoutInMs > KtlLogManager::MemoryThrottleLimits::_DefaultAllocationTimeoutInMsMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->AllocationTimeoutInMs,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((limits->PinnedMemoryLimit != KtlLogManager::MemoryThrottleLimits::_NoLimit) &&
                        (limits->PinnedMemoryLimit < KtlLogManager::MemoryThrottleLimits::_PinnedMemoryLimitMin))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->PinnedMemoryLimit,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if (limits->WriteBufferMemoryPoolMin != KtlLogManager::MemoryThrottleLimits::_NoLimit)
                    {
                        if (limits->WriteBufferMemoryPoolMin < KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolMinMin)
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            KTraceFailedAsyncRequest(Status, this,
                                                     limits->WriteBufferMemoryPoolMin,
                                                     0);

                            DoComplete(Status);
                            return;
                        }

                        if ((limits->WriteBufferMemoryPoolMax != KtlLogManager::MemoryThrottleLimits::_NoLimit) && 
                            (limits->WriteBufferMemoryPoolMin > limits->WriteBufferMemoryPoolMax))
                        {
                            Status = STATUS_INVALID_PARAMETER;
                            KTraceFailedAsyncRequest(Status, this,
                                                     limits->WriteBufferMemoryPoolMin,
                                                     limits->WriteBufferMemoryPoolMax);

                            DoComplete(Status);
                            return;
                        }
                    } else if (limits->WriteBufferMemoryPoolMax != KtlLogManager::MemoryThrottleLimits::_NoLimit) {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->WriteBufferMemoryPoolMin,
                                                 limits->WriteBufferMemoryPoolMax);

                        DoComplete(Status);
                        return;
                    }

                    if ((limits->WriteBufferMemoryPoolPerStream != 0) &&
                        (limits->WriteBufferMemoryPoolPerStream < KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStreamMin))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 limits->WriteBufferMemoryPoolPerStream,
                                                 KtlLogManager::MemoryThrottleLimits::_DefaultWriteBufferMemoryPoolPerStreamMin);

                        DoComplete(Status);
                        return;
                    }                   
                                        
                    _OM->_ThrottledAllocator->SetLimit(limits->WriteBufferMemoryPoolMin,
                                                       limits->WriteBufferMemoryPoolMax,
                                                       limits->WriteBufferMemoryPoolPerStream);

                    // TODO: Configure and enforce PinnedMemoryLimit

                    LONGLONG maximumDestagingWriteOutstanding;
                    if (_Code >= KtlLogManager::ConfigureMemoryThrottleLimits2)
                    {
                        maximumDestagingWriteOutstanding = limits->MaximumDestagingWriteOutstanding;
                    } else {
                        maximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_NoLimit;
                    }

                    if ((maximumDestagingWriteOutstanding != KtlLogManager::MemoryThrottleLimits::_NoLimit) &&
                        (maximumDestagingWriteOutstanding < KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstandingMin))
                    {
                        maximumDestagingWriteOutstanding = KtlLogManager::MemoryThrottleLimits::_DefaultMaximumDestagingWriteOutstandingMin;
                    }

                    ULONG sharedLogThrottleLimit;
                    if (_Code >= KtlLogManager::ConfigureMemoryThrottleLimits3)
                    {
                        sharedLogThrottleLimit = limits->SharedLogThrottleLimit;
                    } else {
                        sharedLogThrottleLimit = KtlLogManager::MemoryThrottleLimits::_DefaultSharedLogThrottleLimit;
                    }

                    if ((sharedLogThrottleLimit == 0) || (sharedLogThrottleLimit > 100))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 sharedLogThrottleLimit,
                                                 0);

                        DoComplete(Status);
                        return;
                    }
                    

                    KDbgCheckpointWDataInformational(GetActivityId(), "SetMemoryThrottleLimits", STATUS_SUCCESS,
                        (ULONGLONG)limits->WriteBufferMemoryPoolMin,
                        (ULONGLONG)limits->WriteBufferMemoryPoolMax,
                        (ULONGLONG)limits->PeriodicFlushTimeInSec * 0x100000000 + limits->PeriodicTimerIntervalInSec,
                        (ULONGLONG)limits->WriteBufferMemoryPoolPerStream);

                    KDbgCheckpointWDataInformational(GetActivityId(), "SetMemoryThrottleLimits2", STATUS_SUCCESS,
                        (ULONGLONG)limits->AllocationTimeoutInMs,
                        (ULONGLONG)limits->PinnedMemoryLimit,
                        (ULONGLONG)maximumDestagingWriteOutstanding,
                        (ULONGLONG)sharedLogThrottleLimit);                  

                    //
                    // Remember the limits that are originally set
                    //
                    _OM->_MemoryThrottleLimits.WriteBufferMemoryPoolMax = limits->WriteBufferMemoryPoolMax;
                    _OM->_MemoryThrottleLimits.WriteBufferMemoryPoolMin = limits->WriteBufferMemoryPoolMin;
                    _OM->_MemoryThrottleLimits.WriteBufferMemoryPoolPerStream = limits->WriteBufferMemoryPoolPerStream;
                    _OM->_MemoryThrottleLimits.PinnedMemoryLimit = limits->PinnedMemoryLimit;
                    _OM->_MemoryThrottleLimits.PeriodicFlushTimeInSec = limits->PeriodicFlushTimeInSec;
                    _OM->_MemoryThrottleLimits.PeriodicTimerIntervalInSec = limits->PeriodicTimerIntervalInSec;
                    _OM->_MemoryThrottleLimits.AllocationTimeoutInMs = limits->AllocationTimeoutInMs;
                    _OM->_ThrottledAllocator->SetAllocationTimeoutInMs(limits->AllocationTimeoutInMs);
            
                    _OM->_MemoryThrottleLimits.MaximumDestagingWriteOutstanding = maximumDestagingWriteOutstanding;
                    _OM->_MemoryThrottleLimits.SharedLogThrottleLimit = sharedLogThrottleLimit;
                    
                    Status = STATUS_SUCCESS;
                    break;
                }

                case KtlLogManager::QueryMemoryThrottleUsage:
                {
                    KBuffer::SPtr outBuffer;
                    KtlLogManager::MemoryThrottleUsage* outStruct;

                    if (! _OutBuffer)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        DoComplete(Status);
                        return;
                    }

                    
                    Status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleUsage),
                                             outBuffer,
                                             GetThisAllocator());
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        Complete(Status);
                        return;
                    }

                    outStruct = (KtlLogManager::MemoryThrottleUsage*)outBuffer->GetBuffer();
                    outStruct->ConfiguredLimits = _OM->GetMemoryThrottleLimits();
                    outStruct->TotalAllocationLimit = _OM->_ThrottledAllocator->GetTotalAllocationLimit();
                    outStruct->CurrentAllocations = _OM->_ThrottledAllocator->GetCurrentAllocations();
                    outStruct->IsUnderMemoryPressure = _OM->_ThrottledAllocator->IsUnderMemoryPressure();
                    outStruct->PinnedMemoryAllocations = _OM->GetPinnedIoBufferUsage();

                    *_OutBuffer = outBuffer;
                    Status = STATUS_SUCCESS;
                    break;
                }

#if DBG
                case KtlLogManager::QueryDebugUnitTestInformation:
                {
                    KBuffer::SPtr outBuffer;
                    KtlLogManager::DebugUnitTestInformation* outStruct;

                    if (! _OutBuffer)
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        DoComplete(Status);
                        return;
                    }

                    
                    Status = KBuffer::Create(sizeof(KtlLogManager::MemoryThrottleUsage),
                                             outBuffer,
                                             GetThisAllocator());
                    if (! NT_SUCCESS(Status))
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, 0);

                        Complete(Status);
                        return;
                    }

                    outStruct = (KtlLogManager::DebugUnitTestInformation*)outBuffer->GetBuffer();
                    outStruct->PinMemoryFailureCount = _OM->GetPinMemoryFailureCount();

                    *_OutBuffer = outBuffer;
                    Status = STATUS_SUCCESS;
                    break;
                }
#endif

                case KtlLogManager::ConfigureAcceleratedFlushLimits:
                {                                                                          
                    //
                    // Establish the new write throttle threshold
                    //
                    KtlLogManager::AcceleratedFlushLimits* settings;

                    if (! _InBuffer || (_InBuffer->QuerySize() < sizeof(KtlLogManager::AcceleratedFlushLimits)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this, _State, _InBuffer ? _InBuffer->QuerySize() : (ULONGLONG)-1);

                        DoComplete(Status);
                        return;
                    }

                    settings = (KtlLogManager::AcceleratedFlushLimits*)_InBuffer->GetBuffer();
                    
                    //
                    // Validate settings
                    //
                    if ((settings->AccelerateFlushActiveTimerInMs != KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsNoAction) &&
                        ((settings->AccelerateFlushActiveTimerInMs < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsMin) ||
                         (settings->AccelerateFlushActiveTimerInMs > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActiveTimerInMsMax)))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->AccelerateFlushActiveTimerInMs,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((settings->AccelerateFlushPassiveTimerInMs < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassiveTimerInMsMin) ||
                        (settings->AccelerateFlushPassiveTimerInMs > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassiveTimerInMsMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->AccelerateFlushPassiveTimerInMs,
                                                 0);

                        DoComplete(Status);
                        return;
                    }

                    if ((settings->AccelerateFlushActivePercent < settings->AccelerateFlushPassivePercent) ||
                        (settings->AccelerateFlushActivePercent < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActivePercentMin) ||
                        (settings->AccelerateFlushActivePercent > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushActivePercentMax) ||
                        (settings->AccelerateFlushPassivePercent < KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassivePercentMin) ||
                        (settings->AccelerateFlushPassivePercent > KtlLogManager::AcceleratedFlushLimits::AccelerateFlushPassivePercentMax))
                    {
                        Status = STATUS_INVALID_PARAMETER;
                        KTraceFailedAsyncRequest(Status, this,
                                                 settings->AccelerateFlushActivePercent,
                                                 settings->AccelerateFlushPassivePercent);

                        DoComplete(Status);
                        return;
                    }

                    
                    _OM->_AcceleratedFlushLimits = *settings;

                    Status = STATUS_SUCCESS;
                    break;
                }
                
                default:
                {
                    break;
                }
            }
            
            _State = Completed;
            DoComplete(Status);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncConfigureContextOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncConfigureContextOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    
    _State = Initial;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncConfigureContextOverlay::StartConfigure(
    __in ULONG Code,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = Initial;
    
    _Code = Code;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;
    *_OutBuffer = nullptr;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncConfigureContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            
OverlayManager::AsyncConfigureContextOverlay::~AsyncConfigureContextOverlay(
    )
{
}

VOID
OverlayManager::AsyncConfigureContextOverlay::OnReuse(
    )
{
}

OverlayManager::AsyncConfigureContextOverlay::AsyncConfigureContextOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

NTSTATUS
OverlayManager::CreateAsyncConfigureContext(
    __out AsyncConfigureContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncConfigureContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncConfigureContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Delete Log
//
OverlayManagerBase::AsyncDeleteLog::AsyncDeleteLog()
{
}

OverlayManagerBase::AsyncDeleteLog::~AsyncDeleteLog()
{
}
 
VOID
OverlayManager::AsyncDeleteLogOverlay::QueueDeactivated(
    __in ServiceWrapper&
    )
{
    DoComplete(_FinalStatus);
}

VOID
OverlayManager::AsyncDeleteLogOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    if (_OCReferenced)
    {
        BOOLEAN freed = _OM->_ContainersTable.ReleaseOverlayContainer(*_OC);
        if (freed)
        {
            _State = Cleanup;
            _OCReferenced = FALSE;
            _FinalStatus = Status;

            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                   &OverlayManager::AsyncDeleteLogOverlay::QueueDeactivated);           
            _OC->DeactivateQueue(queueDeactivated);
            return;
        }
    }
    
    _OC = nullptr;
    _OCOperation = nullptr;
    _Path = nullptr;

    _OM->ReleaseRequestRef();
        
    Complete(Status);
}

VOID
OverlayManager::AsyncDeleteLogOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayManager::AsyncDeleteLogOverlay::OperationCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        DoComplete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            BOOLEAN addedToList;
                        
            //
            // First thing is to get the OverlayContainer
            //
            _State = DeleteLog;
            
            Status = _OM->_ContainersTable.CreateOrGetOverlayContainer(_DiskId,
                                                                       _LogId,
                                                                       _OC,
                                                                       addedToList);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _OCReferenced = TRUE;
            
            _OCOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                                             OverlayLogFreeService::OperationDelete(*_OC,
                                                                                    _DiskId,
                                                                                    _Path.RawPtr());
            
            if (! _OCOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                DoComplete(Status);
                return;
            }

            Status = _OCOperation->Status();
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _OCOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));         
            
            KDbgCheckpointWDataInformational(
                KAsyncGlobalContext::GetActivityId(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext)),
                "OverlayManager::AsyncDeleteLogOverlay StartDeleteOperation",
                Status,
                (ULONGLONG)this,
                (ULONGLONG)_OC.RawPtr(),
                (ULONGLONG)_OCOperation.RawPtr(),
                (ULONGLONG)_State);

            _OCOperation->StartDeleteOperation(this,
                                               completion);

            break;
        }

        case DeleteLog:
        {
            DoComplete(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}
               
VOID
OverlayManager::AsyncDeleteLogOverlay::OnCancel(
    )
{   
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayManager::AsyncDeleteLogOverlay::OnStart(
    )
{
    NTSTATUS status = _OM->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }
    
    _State = Initial;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayManager::AsyncDeleteLogOverlay::StartDeleteLog(
    __in const KGuid& DiskId,
    __in_opt KString const * const Path,
    __in const RvdLogId& LogId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = Path;
    _LogId = LogId;
    
    Start(ParentAsyncContext, CallbackPtr);
}

VOID OverlayManager::AsyncDeleteLogOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}
            
OverlayManager::AsyncDeleteLogOverlay::~AsyncDeleteLogOverlay(
    )
{
}

VOID
OverlayManager::AsyncDeleteLogOverlay::OnReuse(
    )
{
}

OverlayManager::AsyncDeleteLogOverlay::AsyncDeleteLogOverlay(
    __in OverlayManager& OM
    ) : _OM(&OM)
{
}

NTSTATUS
OverlayManager::CreateAsyncDeleteLogContext(
    __out AsyncDeleteLog::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayManager::AsyncDeleteLogOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, this, GetThisAllocationTag(), 0);
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
// Verification callbacks
//
NTSTATUS
OverlayManager::RegisterVerificationCallback(
    __in const RvdLogStreamType&,
    __in RvdLogManager::VerificationCallback
    )
{
    KInvariant(FALSE);  
    
    return(STATUS_UNSUCCESSFUL);
}

RvdLogManager::VerificationCallback
OverlayManager::UnRegisterVerificationCallback(
    __in const RvdLogStreamType&
    )
{
    KInvariant(FALSE);
    
    return(NULL);
}

RvdLogManager::VerificationCallback
OverlayManager::QueryVerificationCallback(
    __in const RvdLogStreamType&
    )
{    
    KInvariant(FALSE);
    
    return(NULL);
}
