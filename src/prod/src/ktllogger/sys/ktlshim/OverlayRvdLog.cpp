// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

//
// Create Log Stream
//
VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    ServiceWrapper::DeactivateQueueCallback completion(this, &OverlayLog::AsyncCreateLogStreamContextOverlay::QueueDeactivated);
    
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;

    if ((! NT_SUCCESS(Status)) && (_OSRefCountTaken))
    {
        BOOLEAN freed;
        
        if (_QueueActivated)
        {
            _FinalStatus = Status;
            _QueueActivated = FALSE;
            _State = DeactivateQueue;
            _OverlayStreamFS->DeactivateQueue(completion);
            return;
        }
        
        freed = _OverlayLog->_StreamsTable.ReleaseOverlayStream(*_OverlayStreamFS);

        //
        // The overlay stream free service should have been removed
        // from the list if this finishes unsuccessfully
        //
        KInvariant(freed);
    }
    
    if (_OwnSyncEvent)
    {
        _OverlayStreamFS->ResetSyncEvent();
        _OwnSyncEvent = FALSE;
    }

    _OverlayStreamFS = nullptr;
    _FSCreateOperation = nullptr;
    _FSOpenOperation = nullptr;
    _Path = nullptr;
    _SingleAccessWaitContext = nullptr;
    
    _OverlayLog->ReleaseRequestRef();
    
    Complete(Status);
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AsyncCreateLogStreamContextOverlay::OperationCompletion);

    KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                        "AsyncCreateLogStreamContextOverlay", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
        
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
            BOOLEAN created;
            
            //
            // First step is to create the entry in the global streams
            // table for the new stream and then kick off its creation.
            // This will create the stream data structures on disk.
            //
            Status = _OverlayLog->_StreamsTable.CreateOrGetOverlayStream(FALSE,    // Read Only
                                                                         _LogStreamId,
                                                                         _DiskId,
                                                                         _Path.RawPtr(),
                                                                         _MaxRecordSize,
                                                                         _StreamSize,
                                                                         _StreamMetadataSize,
                                                                         _OverlayStreamFS,
                                                                         created);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _OSRefCountTaken = TRUE;
            
            if (! created)
            {
                //
                // There is already a stream that has been created with
                // the same id.
                //
                Status = STATUS_SHARING_VIOLATION;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _QueueActivated = TRUE;

            _FSCreateOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                                    OverlayStreamFreeService::OperationCreate(*_OverlayStreamFS,
                                                                            *_OverlayLog->_BaseLogManager,
                                                                            *_OverlayLog->_BaseLogContainer,
                                                                            _DiskId,
                                                                            _Path.RawPtr(),
                                                                            _Alias.RawPtr(),
                                                                            _Flags,
                                                                            _LogStreamType,
                                                                            *_OverlayLog->_LCMBInfo);
            if (! _FSCreateOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _FSCreateOperation->Status();            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogStreamId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _FSCreateOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            
            
            Status = _OverlayStreamFS->CreateSyncWaitContext(_SingleAccessWaitContext); 
            if (! NT_SUCCESS(Status))
            {
                KTraceOutOfMemory(0, Status, this, _State, _OverlayLog);
                DoComplete(Status);
                return;
            }

            //
            // Synchronize with Delete and Open
            //
            _State = SyncStreamAccess;
            _SingleAccessWaitContext->StartWaitUntilSet(this, completion);
            return;
        }

        case SyncStreamAccess:
        {
            _OwnSyncEvent = TRUE;

            _State = CreateOverlayStream;

            _FSCreateOperation->StartCreateOperation(this,
                                                     completion);
            break;
        }

        case CreateOverlayStream:
        {
            //
            // Now that the stream data structures are created on disk,
            // open them up
            //            
            _State = OpenOverlayStream;

            _FSOpenOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                                             OverlayStreamFreeService::OperationOpen(*_OverlayStreamFS,
                                                                                     *_OverlayLog,
                                                                                     *_OverlayLog->_BaseLogManager,
                                                                                     *_OverlayLog->_BaseLogContainer,
                                                                                     *_OverlayLog->_LCMBInfo); 
                                                                                     
            if (! _FSOpenOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _FSOpenOperation->Status();            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogStreamId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _FSOpenOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            _FSOpenOperation->StartOpenOperation(this,
                                                 completion);           
            break;
        }

        case OpenOverlayStream:
        {
            //
            // If there is an alias then add it to the table
            //
            if (_Alias)
            {
                Status = _OverlayLog->AddOrUpdateAlias(*_Alias,
                                                       _LogStreamId);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }
            }
                        
            //
            // All done
            //
            *_ResultLogStreamPtr = _OverlayStreamFS->GetOverlayStream();
            DoComplete(STATUS_SUCCESS);
            break;
        }

        case DeactivateQueue:
        {
            DoComplete(_FinalStatus);
            break;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID 
OverlayLog::AsyncCreateLogStreamContextOverlay::QueueDeactivated(
    __in ServiceWrapper& Service
    )
{
    UNREFERENCED_PARAMETER(Service);
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    NTSTATUS status = _OverlayLog->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    _OSRefCountTaken = FALSE;
    _QueueActivated = FALSE;
    _OwnSyncEvent = FALSE;
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::StartCreateLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __in const RvdLogStreamType& LogStreamType,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in ULONG StreamMetadataSize,                    
    __in_opt KString const * const Alias,
    __in DWORD Flags,
    __out OverlayStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;
    
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _DiskId = DiskId;
    _Path = Path;
    _MaxRecordSize = MaxRecordSize;
    _StreamSize = StreamSize;
    _StreamMetadataSize = StreamMetadataSize;
    _ResultLogStreamPtr = &LogStream;
    _Alias = Alias;
    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::StartCreateLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __in const RvdLogStreamType& LogStreamType,
    __out RvdLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    UNREFERENCED_PARAMETER(LogStreamId);
    UNREFERENCED_PARAMETER(LogStreamType);
    UNREFERENCED_PARAMETER(LogStream);
    UNREFERENCED_PARAMETER(ParentAsyncContext);
    UNREFERENCED_PARAMETER(CallbackPtr);
    
    //
    // OverlayStream should be created and not RvdLogStream
    //
    KInvariant(FALSE);
}

OverlayLogBase::AsyncCreateLogStreamContext::AsyncCreateLogStreamContext()
{
}

OverlayLogBase::AsyncCreateLogStreamContext::~AsyncCreateLogStreamContext()
{
}

OverlayLog::AsyncCreateLogStreamContextOverlay::~AsyncCreateLogStreamContextOverlay()
{
}

VOID
OverlayLog::AsyncCreateLogStreamContextOverlay::OnReuse(
    )
{
}

OverlayLog::AsyncCreateLogStreamContextOverlay::AsyncCreateLogStreamContextOverlay(
    __in OverlayLog& OContainer
    ) 
{
    _OverlayLog = &OContainer;
}

NTSTATUS
OverlayLog::CreateAsyncCreateLogStreamContext(
    __out AsyncCreateLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AsyncCreateLogStreamContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogStreamContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
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

//
// Delete Log Stream
//
VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;

    if (_OSRefCountTaken)
    {
        _OverlayLog->_StreamsTable.ReleaseOverlayStream(*_OverlayStreamFS);
    }

    if (_OwnSyncEvent)
    {
        _OverlayStreamFS->ResetSyncEvent();
        _OwnSyncEvent = FALSE;
    }

    _FSDeleteOperation = nullptr;
    _OverlayStreamFS = nullptr;
    _SingleAccessWaitContext = nullptr;

    _OverlayLog->ReleaseRequestRef();

    Complete(Status);
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AsyncDeleteLogStreamContextOverlay::OperationCompletion);

    KDbgCheckpointWDataInformational(0, "AsyncDeleteLogStreamContextOverlay", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
        
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
            Status = _OverlayLog->_StreamsTable.FindOverlayStream(_LogStreamId,
                                                                  _OverlayStreamFS);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _OSRefCountTaken = TRUE;

            _FSDeleteOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                                    OverlayStreamFreeService::OperationDelete(*_OverlayStreamFS,
                                                                            *_OverlayLog->_BaseLogManager,
                                                                            *_OverlayLog->_BaseLogContainer,
                                                                            *_OverlayLog->_LCMBInfo);
            
            if (! _FSDeleteOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _FSDeleteOperation->Status();            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogStreamId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _FSDeleteOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            

            Status = _OverlayStreamFS->CreateSyncWaitContext(_SingleAccessWaitContext); 
            if (! NT_SUCCESS(Status))
            {
                KTraceOutOfMemory(0, Status, this, _State, _OverlayLog);
                DoComplete(Status);
                return;
            }

            //
            // Synchronize with Create and Open
            //
            _State = SyncStreamAccess;
            _SingleAccessWaitContext->StartWaitUntilSet(this, completion);
            return;
        }

        case SyncStreamAccess:
        {
            _OwnSyncEvent = TRUE;
            _State = DeleteOnDisk;

            _FSDeleteOperation->StartDeleteOperation(this,
                                                     completion);         
            
            break;
        }

        case DeleteOnDisk:
        {
            _State = ShutdownQueue;

            //
            // Remove any alias for this stream
            //
            Status = _OverlayLog->RemoveAliasByStreamId(_LogStreamId);


            if (! NT_SUCCESS(Status))
            {
                //
                // StreamId not found
                //
                KTraceFailedAsyncRequest(Status, this, _State, 0);
            }
            
            
            ServiceWrapper::DeactivateQueueCallback queueDeactivated(this,
                                                                &OverlayLog::AsyncDeleteLogStreamContextOverlay::QueueDeactivated);           
            _OverlayStreamFS->DeactivateQueue(queueDeactivated);
            
            break;
        }

        case ShutdownQueue:
        {
            //
            // Release refcount for creation of the stream
            //
            _OverlayLog->_StreamsTable.ReleaseOverlayStream(*_OverlayStreamFS);
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
OverlayLog::AsyncDeleteLogStreamContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::QueueDeactivated(
    __in ServiceWrapper& Service
    )
{
    UNREFERENCED_PARAMETER(Service);
    FSMContinue(STATUS_SUCCESS); 
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    NTSTATUS status = _OverlayLog->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    _OSRefCountTaken = FALSE;
    _OwnSyncEvent = FALSE;
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::StartDeleteLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;
    
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

OverlayLogBase::AsyncDeleteLogStreamContext::~AsyncDeleteLogStreamContext()
{
}

OverlayLogBase::AsyncDeleteLogStreamContext::AsyncDeleteLogStreamContext()
{
}

OverlayLog::AsyncDeleteLogStreamContextOverlay::~AsyncDeleteLogStreamContextOverlay()
{
}

VOID
OverlayLog::AsyncDeleteLogStreamContextOverlay::OnReuse(
    )
{
}

OverlayLog::AsyncDeleteLogStreamContextOverlay::AsyncDeleteLogStreamContextOverlay(
    __in OverlayLog& OC
    )
{
    _OverlayLog = &OC;
}

NTSTATUS
OverlayLog::CreateAsyncDeleteLogStreamContext(
    __out AsyncDeleteLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AsyncDeleteLogStreamContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogStreamContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
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

//
// Open Log Stream
//
VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;

    if (_OSRefCountTaken)
    {
        BOOLEAN freed;
        
        freed = _OverlayLog->_StreamsTable.ReleaseOverlayStream(*_OverlayStreamFS);

    }
    
    if (_OwnSyncEvent)
    {
        _OverlayStreamFS->ResetSyncEvent();
        _OwnSyncEvent = FALSE;
    }

    _FSOpenOperation = nullptr;
    _OverlayStreamFS = nullptr;
    _SingleAccessWaitContext = nullptr;
        
    _OverlayLog->ReleaseRequestRef();
    
    Complete(Status);
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AsyncOpenLogStreamContextOverlay::OperationCompletion);

    KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                        "AsyncOpenLogStreamContextOverlay", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
        
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
            Status = _OverlayLog->_StreamsTable.FindOverlayStream(_LogStreamId,
                                                                  _OverlayStreamFS);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            _OSRefCountTaken = TRUE;

            _FSOpenOperation = _new(GetThisAllocationTag(), GetThisAllocator())
                                    OverlayStreamFreeService::OperationOpen(*_OverlayStreamFS,
                                                                            *_OverlayLog,
                                                                            *_OverlayLog->_BaseLogManager,
                                                                            *_OverlayLog->_BaseLogContainer,
                                                                            *_OverlayLog->_LCMBInfo); 
            if (! _FSOpenOperation)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            Status = _FSOpenOperation->Status();            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_LogStreamId.Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _FSOpenOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            
            
            Status = _OverlayStreamFS->CreateSyncWaitContext(_SingleAccessWaitContext); 
            if (! NT_SUCCESS(Status))
            {
                KTraceOutOfMemory(0, Status, this, _State, _OverlayLog);
                DoComplete(Status);
                return;
            }

            //
            // Synchronize with Create and Delete
            //
            _State = SyncStreamAccess;
            _SingleAccessWaitContext->StartWaitUntilSet(this, completion);
            return;
        }

        case SyncStreamAccess:
        {
            _OwnSyncEvent = TRUE;

            _State = OpenUp;
            
            _FSOpenOperation->StartOpenOperation(this,
                                                 completion);         
            
            break;
        }

        case OpenUp:
        {
            *_ResultLogStreamPtr = _OverlayStreamFS->GetOverlayStream();
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
OverlayLog::AsyncOpenLogStreamContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    NTSTATUS status = _OverlayLog->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    _OwnSyncEvent = FALSE;
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::StartOpenLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __out OverlayStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _OSRefCountTaken = FALSE;
    _LogStreamId = LogStreamId;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::StartOpenLogStream(
    __in const RvdLogStreamId& LogStreamId,
    __out RvdLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    UNREFERENCED_PARAMETER(LogStreamId);
    UNREFERENCED_PARAMETER(LogStream);
    UNREFERENCED_PARAMETER(ParentAsyncContext);
    UNREFERENCED_PARAMETER(CallbackPtr);
    
    //
    // OverlayStream should be opened not RvdLogStream
    //
    KInvariant(FALSE);
}

OverlayLogBase::AsyncOpenLogStreamContext::AsyncOpenLogStreamContext()
{
}

OverlayLogBase::AsyncOpenLogStreamContext::~AsyncOpenLogStreamContext()
{
}

OverlayLog::AsyncOpenLogStreamContextOverlay::~AsyncOpenLogStreamContextOverlay()
{
}

VOID
OverlayLog::AsyncOpenLogStreamContextOverlay::OnReuse(
    )
{
}

OverlayLog::AsyncOpenLogStreamContextOverlay::AsyncOpenLogStreamContextOverlay(
    __in OverlayLog& OContainer
    )
{
    _OverlayLog = &OContainer;
}

NTSTATUS
OverlayLog::CreateAsyncOpenLogStreamContext(
    __out AsyncOpenLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AsyncOpenLogStreamContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogStreamContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
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


//
// Close Log Stream
//
VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;

    if (_OSRefCountTaken)
    {
        _OverlayLog->_StreamsTable.ReleaseOverlayStream(*_OverlayStreamFS);
    }
    
    _FSOperation = nullptr;
    _OverlayStreamFS = nullptr;
    _OverlayStream = nullptr;
        
    _OverlayLog->ReleaseRequestRef();
    
    Complete(Status);
}

VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AsyncCloseLogStreamContextOverlay::OperationCompletion);

    KDbgCheckpointWDataInformational(KLoggerUtilities::ActivityIdFromStreamId(_OverlayStream->GetStreamId()),
                                     "OverlayLog::AsyncCloseLogStreamContextOverlay", Status,
                                      (ULONGLONG)_State,
                                      (ULONGLONG)this,
                                      (ULONGLONG)0,
                                      (ULONGLONG)0);                                
        
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
            _State = CloseStream;

            Status = _OverlayLog->_StreamsTable.FindOverlayStream(_OverlayStream->GetStreamId(),
                                                                                          _OverlayStreamFS);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            _OSRefCountTaken = TRUE;

            Status = ServiceWrapper::Operation::Create(_FSOperation,
                                                     *(static_cast<ServiceWrapper*>(_OverlayStreamFS.RawPtr())),
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }

            ServiceWrapper::GlobalContext::SPtr globalContext;
            Status = ServiceWrapper::GlobalContext::Create(_OverlayStream->GetStreamId().Get(),
                                                           GetThisAllocator(),
                                                           GetThisAllocationTag(),
                                                           globalContext);
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                DoComplete(Status);
                return;
            }
            
            _FSOperation->SetGlobalContext(up_cast<KAsyncGlobalContext, ServiceWrapper::GlobalContext>(globalContext));
            
            _FSOperation->StartCloseOperation(_OverlayStream.RawPtr(),
                                              this,
                                              completion);         
            
            break;
        }

    
        case CloseStream:
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
OverlayLog::AsyncCloseLogStreamContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::OnStart(
    )
{
    KInvariant(_State == Initial);

    NTSTATUS status = _OverlayLog->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    _OSRefCountTaken = FALSE;
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::StartCloseLogStream(
    __in const OverlayStream::SPtr& OS,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _OverlayStream = OS;

    Start(ParentAsyncContext, CallbackPtr);
}

OverlayLog::AsyncCloseLogStreamContextOverlay::~AsyncCloseLogStreamContextOverlay()
{
}

VOID
OverlayLog::AsyncCloseLogStreamContextOverlay::OnReuse(
    )
{
}

OverlayLog::AsyncCloseLogStreamContextOverlay::AsyncCloseLogStreamContextOverlay(
    __in OverlayLog& OContainer
    )
{
    _OverlayLog = &OContainer;
}

NTSTATUS
OverlayLog::CreateAsyncCloseLogStreamContext(
    __out AsyncCloseLogStreamContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AsyncCloseLogStreamContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseLogStreamContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
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


//
// EnumerateStreams
//
VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::DoComplete(
    __in NTSTATUS Status 
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
        
    _OverlayLog->ReleaseRequestRef();
    
    Complete(Status);
}

VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLog::AsyncEnumerateStreamsContextOverlay::OperationCompletion);

    KDbgCheckpointWData(0,
                        "OverlayLog::AsyncEnumerateStreamsContextOverlay", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
        
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
            ULONG resultCount, streamCount;
            RvdLogStreamId* streamIds;
            static GUID const checkpointStreamGuid = {0xc9dc46e5, 0xb669, 0x4a25, {0xa9, 0xe2, 0xd, 0xff, 0x3d, 0x95, 0xc1, 0x63}};
            RvdLogStreamId checkpointStreamId(checkpointStreamGuid);

            do
            {
                streamCount = _OverlayLog->GetNumberOfStreams();

                _LogStreamIds->Clear();
                Status = _LogStreamIds->Reserve(streamCount);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;                 
                }               

                PUCHAR p;
                ULONG sizeNeeded = streamCount * sizeof(RvdLogStreamId);
                p = _new(GetThisAllocationTag(), GetThisAllocator()) UCHAR[sizeNeeded];
                streamIds = (RvdLogStreamId*)p;
                if (streamIds == NULL)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;                 
                }

                resultCount = _OverlayLog->GetStreams(streamCount, streamIds);
                if (resultCount <= streamCount)
                {
                    for (ULONG i = 0; i < resultCount; i++)
                    {
                        if (streamIds[i] != checkpointStreamId)
                        {
                            Status = _LogStreamIds->Append(streamIds[i]);
                            if (! NT_SUCCESS(Status))
                            {
                                _delete(streamIds);
                                streamIds = NULL;
                                KTraceFailedAsyncRequest(Status, this, _State, 0);
                                DoComplete(Status);
                                return;                 
                            }
                        }
                    }
                }
                
                _delete(streamIds);
                streamIds = NULL;               
            } while (resultCount > streamCount);
            
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
OverlayLog::AsyncEnumerateStreamsContextOverlay::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::OnStart(
    )
{
    KAssert(_State == Initial);

    NTSTATUS status = _OverlayLog->TryAcquireRequestRef();
    if (! NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::StartEnumerateStreams(
    __out KArray<KtlLogStreamId>& LogStreamIds,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _LogStreamIds = &LogStreamIds;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayLog::AsyncEnumerateStreamsContextOverlay::~AsyncEnumerateStreamsContextOverlay()
{
}

VOID
OverlayLog::AsyncEnumerateStreamsContextOverlay::OnReuse(
    )
{
}

OverlayLog::AsyncEnumerateStreamsContextOverlay::AsyncEnumerateStreamsContextOverlay(
    __in OverlayLog& OContainer
    )
{
    _OverlayLog = &OContainer;
}

NTSTATUS
OverlayLog::CreateAsyncEnumerateStreamsContext(
    __out AsyncEnumerateStreamsContextOverlay::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLog::AsyncEnumerateStreamsContextOverlay::SPtr context;

    status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    KFinally([&] { ReleaseRequestRef(); });
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateStreamsContextOverlay(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogContainer.RawPtr(), GetThisAllocationTag(), 0);
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

//
// Random APIs
//

VOID
OverlayLog::QueryLogType(__out KWString& LogType)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }
    
    KFinally([&] { ReleaseRequestRef(); });
    _BaseLogContainer->QueryLogType(LogType);
}

VOID
OverlayLog::QueryLogId(__out KGuid& DiskId, __out RvdLogId& LogId)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }
    
    _BaseLogContainer->QueryLogId(DiskId,
                                 LogId);
}

VOID
OverlayLog::QuerySpaceInformation(
    __out_opt ULONGLONG* const TotalSpace,
    __out_opt ULONGLONG* const FreeSpace)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }
    
    // TODO: De-Staging ?
    _BaseLogContainer->QuerySpaceInformation(TotalSpace,
                                             FreeSpace);
}

ULONGLONG
OverlayLog::QueryReservedSpace()
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return(0);
    }
    
    // TODO: De-Staging ?
    return(_BaseLogContainer->QueryReservedSpace());
}

ULONGLONG
OverlayLog::QueryCurrentReservation()
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return(0);
    }
    
    // TODO: De-Staging ?
    return(_BaseLogContainer->QueryCurrentReservation());
}

VOID
OverlayLog::QueryCacheSize(__out_opt LONGLONG* const ReadCacheSizeLimit, __out_opt LONGLONG* const ReadCacheUsage)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }
    
    _BaseLogContainer->QueryCacheSize(ReadCacheSizeLimit,
                                     ReadCacheUsage);
}

VOID
OverlayLog::SetCacheSize(__in LONGLONG CacheSizeLimit)
{
    NTSTATUS status = TryAcquireRequestRef();
    if (!NT_SUCCESS(status))
    {
        KInvariant(FALSE);
        return;
    }
    
    // TODO: De-Staging ?
    _BaseLogContainer->SetCacheSize(CacheSizeLimit);
}


//** Test support API
BOOLEAN
OverlayLog::IsStreamIdValid(__in RvdLogStreamId StreamId)
{
    // TODO: De-Staging ?
    return(_BaseLogContainer->IsStreamIdValid(StreamId));
}

BOOLEAN
OverlayLog::GetStreamState(
    __in RvdLogStreamId StreamId,
    __out_opt BOOLEAN * const IsOpen,
    __out_opt BOOLEAN * const IsClosed,
    __out_opt BOOLEAN * const IsDeleted)
{
    // TODO: De-Staging ?
    return(_BaseLogContainer->GetStreamState(StreamId, IsOpen, IsClosed, IsDeleted));
}

BOOLEAN
OverlayLog::IsLogFlushed()
{
    // TODO: De-Staging ?
    return(_BaseLogContainer->IsLogFlushed());
}

ULONG
OverlayLog::GetNumberOfStreams()
{
    return(_BaseLogContainer->GetNumberOfStreams());
}

ULONG
OverlayLog::GetStreams(__in ULONG MaxResults, __in RvdLogStreamId* Streams)
{
    return(_BaseLogContainer->GetStreams(MaxResults, Streams));
}

NTSTATUS
OverlayLog::SetShutdownEvent(
    __in_opt KAsyncEvent* const EventToSignal
    )
{
    UNREFERENCED_PARAMETER(EventToSignal);
    
    //
    // This is used between the overlay and core logger only
    //
    KInvariant(FALSE);
    return(STATUS_UNSUCCESSFUL);
}
        
ULONG
OverlayLog::QueryMaxAllowedStreams()
{
    return(_BaseLogContainer->QueryMaxAllowedStreams());
}

ULONG
OverlayLog::QueryMaxRecordSize()
{
    return(_BaseLogContainer->QueryMaxRecordSize());
}

ULONG 
OverlayLog::QueryMaxUserRecordSize() 
{
    return _BaseLogContainer->QueryMaxUserRecordSize();
}

ULONG 
OverlayLog::QueryUserRecordSystemMetadataOverhead() 
{
    return _BaseLogContainer->QueryUserRecordSystemMetadataOverhead();
}

VOID
OverlayLog::QueryLsnRangeInformation(
    __out LONGLONG& LowestLsn,
    __out LONGLONG& HighestLsn,
    __out RvdLogStreamId& LowestLsnStreamId
    )
{
    return _BaseLogContainer->QueryLsnRangeInformation(LowestLsn, HighestLsn, LowestLsnStreamId);
}

