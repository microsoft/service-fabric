// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

OverlayStreamFreeService::OverlayStreamFreeService(
    __in KtlLogStreamId& StreamId,
    __in KGuid DiskId,
    __in_opt KString const * const Path,
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in ULONG StreamMetadataSize
    ) : ServiceWrapper(&OverlayStream::CreateUnderlyingService),
        _StreamId(StreamId),
        _DiskId(DiskId),
        _Path(Path),
        _OnListCount(0),
        _DedicatedLogContainerType(GetThisAllocator()),
        _LCMBEntryIndex((ULONG)-1),
        _MaxRecordSize(MaxRecordSize),
        _StreamSize(StreamSize),
        _StreamMetadataSize(StreamMetadataSize),
        _SingleAccessEvent(FALSE, TRUE)              // Auto reset, initially signalled
{
    NTSTATUS status;

    _DedicatedLogContainerType = L"Winfab Dedicated Logical Log";
    status = _DedicatedLogContainerType.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }   
}

OverlayStreamFreeService::~OverlayStreamFreeService()
{
}

VOID
OverlayStreamFreeService::IncrementOnListCount(
    )
{
    InterlockedIncrement((LONG*)&_OnListCount);
}
        
BOOLEAN
OverlayStreamFreeService::DecrementOnListCount(
)
{
    ULONG count;

    count = InterlockedDecrement((LONG*)&_OnListCount);

    return(count == 0);
}


//
// Service OperationClose support
//
VOID
OverlayStreamFreeService::OpenToCloseTransitionCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();
    ServiceWrapper::Operation* op = _CloseOp;
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, op, 0, 0);

        //
        // If the final flush fails then we don't want to fail the
        // close
        //
        status = STATUS_SUCCESS;
    }
        
    __super::OpenToCloseTransitionWorker(status, *op, _CloseCompletion);
}

NTSTATUS OverlayStreamFreeService::OpenToCloseTransitionWorker(
    __in NTSTATUS,
    __in ServiceWrapper::Operation& Op,
    __in KAsyncServiceBase::CloseCompletionCallback CloseCompletion                                           
    )
{
    NTSTATUS status;

    _CloseOp = &Op;
    _CloseCompletion = CloseCompletion;

    OverlayStream::SPtr overlayStream = down_cast<OverlayStream>(GetUnderlyingService());
    OverlayStream::CoalesceRecords::SPtr coalesceRecords = overlayStream->GetCoalesceRecords();

    if (coalesceRecords)
    {        
        KAsyncContextBase::CompletionCallback completion(this, &OverlayStreamFreeService::OpenToCloseTransitionCompletion);
        coalesceRecords->FlushAllRecordsForClose(completion);
        status = STATUS_PENDING;
    } else {
        status = __super::OpenToCloseTransitionWorker(STATUS_SUCCESS, Op, _CloseCompletion);        
    }
    
    return(status);
}

//
// Initialize a brand new shared log stream
//
VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::OnCompleted(
    )
{
    _State = NT_SUCCESS(Status()) ? Completed : CompletedWithError;
    _LogManager = nullptr;
    _SharedLog = nullptr;
    _DedMBInfo = nullptr;

    _DedicatedLogStream = nullptr;
    if (_DedicatedLogContainer)
    {
        _DedicatedLogContainer->SetShutdownEvent(nullptr);
        _DedicatedLogContainer = nullptr;
    }
    _CreateLogContext = nullptr;
    _CreateStreamContext = nullptr;
    
    _SharedLogStream = nullptr;
    _LCMBInfo = nullptr;
    _AddEntryContext = nullptr;
    _UpdateEntryContext = nullptr;
    _Path = nullptr;
}

NTSTATUS 
OverlayStreamFreeService::CreateSyncWaitContext(
    __out KAsyncEvent::WaitContext::SPtr& WaitContext
    )
{
    NTSTATUS status;

    status = _SingleAccessEvent.CreateWaitContext(GetThisAllocationTag(), GetThisAllocator(), WaitContext); 
    return(status);
}

VOID 
OverlayStreamFreeService::ResetSyncEvent(
    )
{
    _SingleAccessEvent.SetEvent();
}


VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStreamFreeService::AsyncInitializeStreamContext::OperationCompletion);

    KDbgCheckpointWData(GetActivityId(), "AsyncInitializeStreamContext", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            KGuid diskId;
            KtlLogContainerId sharedLogId;

            //
            // First step is to allocate all of the resources needed
            // including shared metadata access, log container and log
            // stream creation asyncs
            //
            _State = AllocateResources;
            
            _SharedLog->QueryLogId(diskId, sharedLogId);
            
            Status = DedicatedLCMBInfoAccess::Create(_DedMBInfo,
                                                  _DiskId,
                                                  _Path.RawPtr(),
                                                  _ContainerId,
                                                  _StreamMetadataSize,
                                                  GetThisAllocator(),
                                                  GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }
            
            Status = _LCMBInfo->CreateAsyncAddEntryContext(_AddEntryContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            Status = _LCMBInfo->CreateAsyncUpdateEntryContext(_UpdateEntryContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            Status = _LogManager->CreateAsyncCreateLogContext(_CreateLogContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }
            
            Status = _SharedLog->CreateAsyncCreateLogStreamContext(_CreateStreamContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            // AsyncCreateLogStreamContext for dedicated container will
            // be allocated later after dedicated container is created
            
            //
            // Next step is to add an entry in the shared metadata
            // associated with the shared log container.
            //
            _State = UpdateSharedMetadata;
            _AddEntryContext->StartAddEntry(_StreamId,
                                           _StreamMetadataSize,
                                           _MaxRecordSize,
                                           _StreamSize,
                                           SharedLCMBInfoAccess::FlagCreating,
                                           _Path.RawPtr(),
                                           _Alias.RawPtr(),
                                           &_LCMBEntryIndex,
                                           this,
                                           completion);
            
            break;
        }

        case UpdateSharedMetadata:
        {
            _State = CreateSharedStream;

            KInvariant(_LCMBEntryIndex != (ULONG)-1);                                      

            _CreateStreamContext->StartCreateLogStream(_StreamId,
                                                       _LogStreamType,
                                                       _SharedLogStream,
                                                       this,
                                                       completion);
            
            break;
        }

        case CreateSharedStream:
        {
            _State = CreateDedicatedContainer;

            if (_Path)
            {
                _CreateLogContext->StartCreateLog(*_Path,
                                                  _ContainerId,
                                                  _OS->_DedicatedLogContainerType,
                                                  _StreamSize + DedicatedContainerSizeFudgeFactor(_StreamSize),
                                                  _OS->_DedicatedLogContainerStreamCount,
                                                  _OS->_MaxRecordSize,
                                                  _Flags,
                                                  _DedicatedLogContainer,
                                                  this,
                                                  completion);
            } else {
                _CreateLogContext->StartCreateLog(_DiskId,
                                                  _ContainerId,
                                                  _OS->_DedicatedLogContainerType,
                                                  _StreamSize + DedicatedContainerSizeFudgeFactor(_StreamSize),
                                                  _OS->_DedicatedLogContainerStreamCount,
                                                  _OS->_MaxRecordSize,
                                                  _Flags,
                                                  _DedicatedLogContainer,
                                                  this,
                                                  completion);
            }
            break;
        }

        case CreateDedicatedContainer:
        {
            KBuffer::SPtr securityDescriptor = nullptr;

            //
            // Set shutdown event on the dedicated log container. It will fire
            // when the container is completely shutdown in the
            // destructor.
            //
            Status = _DedicatedLogContainer->SetShutdownEvent(&_DedicatedLogShutdownEvent);
            KInvariant(NT_SUCCESS(Status));
            
            _State = CreatedDedicatedMetadata;

            _DedMBInfo->StartInitializeMetadataBlock(securityDescriptor,
                                                     this,
                                                     completion);
            
            break;
        }

        case CreatedDedicatedMetadata:
        {
            _State = CreateDedicatedStream;

            _CreateStreamContext = nullptr;

            Status = _DedicatedLogContainer->CreateAsyncCreateLogStreamContext(_CreateStreamContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }           

            _CreateStreamContext->StartCreateLogStream(_StreamId,
                                                       _LogStreamType,
                                                       _DedicatedLogStream,
                                                       this,
                                                       completion);
                        
            break;
        }

        case CreateDedicatedStream:
        {
            _State = MarkStreamCreated;

            _UpdateEntryContext->StartUpdateEntry(_StreamId,
                                                 NULL,          // DedicatedContainerPath
                                                 NULL,          // Alias
                                                 SharedLCMBInfoAccess::FlagCreated,
                                                 FALSE,         // RemoveEntry
                                                 _LCMBEntryIndex,
                                                 this,
                                                 completion);

                     
            break;
        }

        case MarkStreamCreated:
        {
            _OS->SetLCMBEntryIndex(_LCMBEntryIndex);

            _State = WaitForDedicatedContainerClose;
            _CreateLogContext = nullptr;
            _CreateStreamContext = nullptr;
            _DedicatedLogStream = nullptr;
            _DedicatedLogContainer = nullptr;
            _DedicatedLogShutdownWait->StartWaitUntilSet(this,
                                                         completion);
            break;
        }

        case WaitForDedicatedContainerClose:
        {

            Complete(STATUS_SUCCESS);
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{   
    NTSTATUS status = Async.Status();

    FSMContinue(status);
}

VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::OnStart(
    )
{
    KInvariant(_State == Initial);
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::StartInitializeStream(
    __in RvdLogManager& LogManager,
    __in RvdLog& SharedLog,
    __in SharedLCMBInfoAccess& LCMBInfo,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in const RvdLogStreamType LogStreamType,  
    __in KtlLogStreamId& StreamId,
    __in ULONG MaxRecordSize,
    __in LONGLONG StreamSize,
    __in ULONG StreamMetadataSize,
    __in_opt KString const * const Alias,
    __in DWORD Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _LogManager = &LogManager;
    _SharedLog = &SharedLog;
    _LCMBInfo = &LCMBInfo;
    _DiskId = DiskId;
    _StreamId = StreamId;
    _LogStreamType = LogStreamType;
    _ContainerId = *((KtlLogContainerId*)(&_StreamId));
    _MaxRecordSize = MaxRecordSize;
    _StreamSize = StreamSize;
    _StreamMetadataSize = StreamMetadataSize;
    _Alias = Alias;
    _Path = Path;
    _Flags = Flags;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStreamFreeService::AsyncInitializeStreamContext::~AsyncInitializeStreamContext()
{
}

VOID
OverlayStreamFreeService::AsyncInitializeStreamContext::OnReuse(
    )
{
    _DedicatedLogShutdownEvent.ResetEvent();
    _DedicatedLogShutdownWait->Reuse();
}

OverlayStreamFreeService::AsyncInitializeStreamContext::AsyncInitializeStreamContext(
    __in OverlayStreamFreeService& OS
    )
{
    NTSTATUS status;

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

    _OS = &OS;
}

NTSTATUS
OverlayStreamFreeService::CreateAsyncInitializeStreamContext(
    __out AsyncInitializeStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayStreamFreeService::AsyncInitializeStreamContext::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncInitializeStreamContext(*this);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, context.RawPtr(), GetThisAllocationTag(), 0);
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
// Service Operation Create
//
VOID
OverlayStreamFreeService::OperationCreateCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    ServiceWrapper::Operation* op = (ServiceWrapper::Operation*)ParentAsync;
    OverlayStreamFreeService::OperationCreate* operationCreate = (OverlayStreamFreeService::OperationCreate*)(op);
    
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayStreamFreeService::OperationCreateCompletion", status,
                            (ULONGLONG)op,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
    }

    operationCreate->_InitializeStreamContext = nullptr;
    op->CompleteOpAndStartNext(status);
}


VOID
OverlayStreamFreeService::OnOperationCreate(
    __in ServiceWrapper::Operation& Op
    )
{
    NTSTATUS status;
    OverlayStreamFreeService::OperationCreate* operationCreate;
    AsyncInitializeStreamContext::SPtr context;

    operationCreate = (OverlayStreamFreeService::OperationCreate*)(&Op);
    
    status = CreateAsyncInitializeStreamContext(operationCreate->_InitializeStreamContext);

    if (NT_SUCCESS(status))
    {
        KAsyncContextBase::CompletionCallback completion(this, &OverlayStreamFreeService::OperationCreateCompletion);
        operationCreate->_InitializeStreamContext->StartInitializeStream(*operationCreate->_BaseLogManager,
                                                        *operationCreate->_BaseSharedLog,
                                                        *operationCreate->_LCMBInfo,
                                                        operationCreate->_DiskId,
                                                        operationCreate->_Path.RawPtr(),
                                                        operationCreate->_LogStreamType,
                                                        _StreamId,
                                                        _MaxRecordSize,
                                                        _StreamSize,
                                                        _StreamMetadataSize,
                                                        operationCreate->_Alias.RawPtr(),
                                                        operationCreate->_Flags,
                                                        &Op,
                                                        completion);
    } else {
        //
        // We need to fail the create back to the original caller
        //
        Op.CompleteOpAndStartNext(status);
    }
}

OverlayStreamFreeService::OperationCreate::~OperationCreate()
{
}

OverlayStreamFreeService::OperationCreate::OperationCreate(
    __in OverlayStreamFreeService& OS,
    __in RvdLogManager& BaseLogManager,
    __in RvdLog& BaseSharedLog,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in_opt KString const * const Alias,
    __in DWORD Flags,
    __in const RvdLogStreamType LogStreamType,
    __in SharedLCMBInfoAccess& LCMBInfo
   ) :  ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OS)),
        _BaseLogManager(&BaseLogManager),
        _BaseSharedLog(&BaseSharedLog),
        _LCMBInfo(&LCMBInfo),
        _DiskId(DiskId),
        _Path(Path),
        _Alias(Alias),
        _Flags(Flags),
        _LogStreamType(LogStreamType)
{
}

//
// Open
//

OverlayStreamFreeService::OperationOpen::~OperationOpen()
{
}

OverlayStreamFreeService::OperationOpen::OperationOpen(
    __in OverlayStreamFreeService& OS,
    __in OverlayLog& OverlayLog,                              
    __in RvdLogManager& BaseLogManager,
    __in RvdLog& BaseSharedLog,
    __in SharedLCMBInfoAccess& LCMBInfo
   ) :  ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OS)),
        _OverlayLog(&OverlayLog),
        _BaseLogManager(&BaseLogManager),
        _BaseSharedLog(&BaseSharedLog),
        _LCMBInfo(&LCMBInfo)
{
}

//
// Delete
//
VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::OnCompleted(
    )
{
    _State = NT_SUCCESS(Status()) ? Completed : CompletedWithError;
    _LogManager = nullptr;
    _SharedLog = nullptr;
    _LCMBInfo = nullptr;
    _DedMBInfo = nullptr;
    _DeleteStreamContext = nullptr;
    _DeleteLogContext = nullptr;
    _UpdateEntryContext = nullptr;
    _Path = nullptr;
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayStreamFreeService::AsyncDeleteStreamContext::OperationCompletion);

    KDbgCheckpointWDataInformational((KActivityId)_StreamId.Get().Data1, "AsyncDeleteStreamContext", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                
        
    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        if ((Status != STATUS_OBJECT_NAME_NOT_FOUND) && (Status != STATUS_OBJECT_PATH_NOT_FOUND))
        {
            Complete(Status);
            return;
        }

        //
        // Allow delete to continue if files or directories expected do
        // not exist
        //
    }
    
    switch (_State)
    {
        case Initial:
        {
            _State = AllocateResources;

            Status = _LogManager->CreateAsyncDeleteLogContext(_DeleteLogContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            if (_SharedLog)
            {
                Status = _SharedLog->CreateAsyncDeleteLogStreamContext(_DeleteStreamContext);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            } else {
                //
                // We go through this codepath when a log container is
                // being deleted. In this case there is not a base
                // RvdLog pointer available however it doesn't matter
                // if the stream within the log container is not
                // deleted as the whole container is going away. We do
                // need to delete the dedicated container and metadata
                // however
                //
                _DeleteStreamContext = nullptr;
            }

            if (_LCMBInfo)
            {
                Status = _LCMBInfo->CreateAsyncUpdateEntryContext(_UpdateEntryContext);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    Complete(Status);
                    return;
                }
            } else {                
                _UpdateEntryContext = nullptr;
            }
            
            Status = DedicatedLCMBInfoAccess::Create(_DedMBInfo,
                                                     _DiskId,
                                                     _Path.RawPtr(),
                                                     _ContainerId,
                                                     _StreamMetadataSize,
                                                     GetThisAllocator(),
                                                     GetThisAllocationTag());
            
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                Complete(Status);
                return;
            }

            //
            // Now that all contexts are allocated, mark the shared
            // metadata as the being deleted
            //
            _State = UpdateSharedMetadata1;
            
            if (_UpdateEntryContext)
            {
                _UpdateEntryContext->StartUpdateEntry(_StreamId,
                                                     NULL,           // DedicatedContainerPath
                                                     NULL,           // Alias
                                                     SharedLCMBInfoAccess::FlagDeleting,
                                                     FALSE,          // RemoveEntry
                                                     _LCMBEntryIndex,
                                                     this,
                                                     completion);
            
                break;
            } else {
                // fall through
            }
            
        }

        case UpdateSharedMetadata1:
        {
            //
            // Next, delete the dedicated metadata file. Skip ahead if
            // we don't need to delete the shared stream
            //
            _State = _DeleteStreamContext ? DeleteDedMBInfo : DeleteSharedStream;
            
            _DedMBInfo->StartCleanupMetadataBlock(this,
                                                  completion);
            break;
        }

        case DeleteDedMBInfo:
        {
            _State = DeleteSharedStream;

            KInvariant(_DeleteStreamContext);
            _DeleteStreamContext->StartDeleteLogStream(_StreamId,
                                                       this,
                                                       completion);
            break;
        }

        case DeleteSharedStream:
        {
            _State = DeleteDedicatedContainer;
            if (_Path)
            {
                _DeleteLogContext->StartDeleteLog(*_Path,
                                                  this,
                                                  completion);
            } else {
                _DeleteLogContext->StartDeleteLog(_DiskId,
                                                  _ContainerId,
                                                  this,
                                                  completion);
            }
            break;
        }

        case DeleteDedicatedContainer:
        {
            _State = UpdateSharedMetadata2;

            if (_UpdateEntryContext)
            {
                _UpdateEntryContext->Reuse();
                _UpdateEntryContext->StartUpdateEntry(_StreamId,
                                                     NULL,           // DedicatedContainerPath
                                                     NULL,           // Alias
                                                     SharedLCMBInfoAccess::FlagDeleted,
                                                     TRUE,           // RemoveEntry
                                                     _LCMBEntryIndex,
                                                     this,
                                                     completion);
            
                break;
            } else {
                // fall through
            }
        }

        case UpdateSharedMetadata2:
        {
            Complete(STATUS_SUCCESS);
            return;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }    
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::OperationCompletion(
    __in_opt KAsyncContextBase* const,
    __in KAsyncContextBase& Async
    )
{    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::StartDeleteStream(
    __in RvdLogManager& LogManager,
    __in_opt RvdLog* const SharedLog,
    __in_opt SharedLCMBInfoAccess* const LCMBInfo,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in KtlLogStreamId& StreamId,                    
    __in ULONG LCMBEntryIndex,
    __in ULONG StreamMetadataSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _LogManager = &LogManager;
    _SharedLog = SharedLog;
    _LCMBInfo = LCMBInfo;
    _DiskId = DiskId;
    _Path = Path;
    _StreamId = StreamId;
    _ContainerId = *((KtlLogContainerId*)(&_StreamId));
    _LCMBEntryIndex = LCMBEntryIndex;
    _StreamMetadataSize = StreamMetadataSize;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayStreamFreeService::AsyncDeleteStreamContext::~AsyncDeleteStreamContext()
{
}

VOID
OverlayStreamFreeService::AsyncDeleteStreamContext::OnReuse(
    )
{
}

OverlayStreamFreeService::AsyncDeleteStreamContext::AsyncDeleteStreamContext(
    )
{
}

NTSTATUS
OverlayStreamFreeService::CreateAsyncDeleteStreamContext(
    __out AsyncDeleteStreamContext::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    OverlayStreamFreeService::AsyncDeleteStreamContext::SPtr context;

    context = _new(AllocationTag, Allocator) AsyncDeleteStreamContext();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, context.RawPtr(), AllocationTag, 0);
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
// Operation Delete
//
VOID
OverlayStreamFreeService::OperationDeleteCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    ServiceWrapper::Operation* op = (ServiceWrapper::Operation*)ParentAsync;
    
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "OverlayStreamFreeService::OperationDeleteCompletion", status,
                            (ULONGLONG)op,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
    }
    
    op->CompleteOpAndStartNext(status);
}

VOID
OverlayStreamFreeService::OnOperationDelete(
    __in ServiceWrapper::Operation& Op
    )
{
    NTSTATUS status;
    OverlayStreamFreeService::OperationDelete* operationDelete;
    AsyncDeleteStreamContext::SPtr context;
    ULONG lcmbEntryIndex = GetLCMBEntryIndex();

    if (lcmbEntryIndex == (ULONG)-1)
    {
        //
        // We need to fail the delete back to the original caller
        //
        status = STATUS_OBJECT_NAME_NOT_FOUND;
        Op.CompleteOpAndStartNext(status);
        return;
    }

    operationDelete = (OverlayStreamFreeService::OperationDelete*)(&Op);
    
    status = CreateAsyncDeleteStreamContext(context,
                                            GetThisAllocator(),
                                            GetThisAllocationTag()
                                           );

    if (NT_SUCCESS(status))
    {
        KAsyncContextBase::CompletionCallback completion(this, &OverlayStreamFreeService::OperationDeleteCompletion);
        context->StartDeleteStream(*operationDelete->_BaseLogManager,
                                   operationDelete->_BaseSharedLog.RawPtr(),
                                   operationDelete->_LCMBInfo.RawPtr(),
                                   _DiskId,
                                   _Path.RawPtr(),
                                   _StreamId,
                                   lcmbEntryIndex,
                                   _StreamMetadataSize,
                                   &Op,
                                   completion);
    } else {
        //
        // We need to fail the delete back to the original caller
        //
        Op.CompleteOpAndStartNext(status);
    }
}

OverlayStreamFreeService::OperationDelete::~OperationDelete()
{
}

OverlayStreamFreeService::OperationDelete::OperationDelete(
    __in OverlayStreamFreeService& OS,
    __in RvdLogManager& BaseLogManager,
    __in RvdLog& BaseSharedLog,
    __in SharedLCMBInfoAccess& LCMBInfo
   ) :  ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OS)),
        _BaseLogManager(&BaseLogManager),
        _BaseSharedLog(&BaseSharedLog),
        _LCMBInfo(&LCMBInfo)
{
}

