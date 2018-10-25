// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

OverlayLogFreeService::~OverlayLogFreeService()
{
}

OverlayLogFreeService::OverlayLogFreeService(
    __in OverlayManager& OM,
    __in RvdLogManager& BaseLogManager,
    __in KGuid DiskId,
    __in KtlLogContainerId ContainerId
) : ServiceWrapper(&OverlayLog::CreateUnderlyingService),
    _OverlayManager(OM),
    _BaseLogManager(&BaseLogManager),
    _DiskId(DiskId),
    _ContainerId(ContainerId),
    _OnListCount(0)
{
}

VOID
OverlayLogFreeService::IncrementOnListCount(
    )
{
    InterlockedIncrement((LONG*)&_OnListCount);
}
        
BOOLEAN
OverlayLogFreeService::DecrementOnListCount(
)
{
    ULONG count;

    count = InterlockedDecrement((LONG*)&_OnListCount);

    return(count == 0);
}

//
// Initialize a brand new shared log container
//
VOID
OverlayLogFreeService::AsyncInitializeContainerContext::OnCompleted(
    )
{
    NTSTATUS status;
    
    _State = NT_SUCCESS(Status()) ? Completed : CompletedWithError;
    _BaseLogManager = nullptr;
    _LCMBInfo = nullptr;
    _CreateLogContext = nullptr;
    _Path = nullptr;

    if (_BaseLogContainer)
    {
        status = _BaseLogContainer->SetShutdownEvent(nullptr);
        KInvariant(NT_SUCCESS(status));
        _BaseLogContainer = nullptr;
    }
}

VOID
OverlayLogFreeService::AsyncInitializeContainerContext::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLogFreeService::AsyncInitializeContainerContext::OperationCompletion);

    KDbgCheckpointWDataInformational(GetActivityId(), "AsyncInitializeContainerContext", Status,
                        (ULONGLONG)_State,
                        (ULONGLONG)this,
                        (ULONGLONG)0,
                        (ULONGLONG)0);                                

    if ((Status == STATUS_ACCESS_DENIED) &&
        ((_State == MarkContainerReadyMetadata) || (_State == MarkContainerReady)))
    {
        //
        // If one of the renames fail with STATUS_ACCESS_DENIED then
        // remap to a more accurate error.
        //
        Status = STATUS_OBJECT_NAME_COLLISION;
    }
          
    if ((! NT_SUCCESS(Status)) && (_State != DeleteTempContainer) && (_State != DeleteTempMBInfo))
    {
        KTraceFailedAsyncRequest(Status, this, _State, 0);
        Complete(Status);
        return;
    }
    
    switch (_State)
    {
        case Initial:
        {
            if (_Path)
            {
                BOOLEAN b;
                
                //
                // Create the log container with a temp filename first,
                // it will be renamed to the actual filename once all
                // initialization is complete.
                //
                Status = KString::Create(_PathTemp,
                                         GetThisAllocator(),
                                         (LPCWSTR)(*_Path));
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }

                KStringView tempString(L".temp");
                b = _PathTemp->Concat(tempString);
                if (! b)
                {
                    Status = STATUS_INSUFFICIENT_RESOURCES;
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;                 
                }
            } else {
                _PathTemp = nullptr;
            }

            //
            // Gather up resources needed such as the CreateLog context
            //
            Status = _BaseLogManager->CreateAsyncCreateLogContext(_CreateLogContext);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            //
            // Also get a context to access the LCMB
            //
            Status = SharedLCMBInfoAccess::Create(_LCMBInfo,
                                                  _DiskId,
                                                  _PathTemp.RawPtr(),
                                                  _ContainerId,
                                                  _MaxNumberStreams,
                                                  GetThisAllocator(),
                                                  GetThisAllocationTag());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }

            if (_PathTemp)
            {
                //
                // Delete any old temporary log container files
                //
                KWString fileName(GetThisAllocator(), (WCHAR*)(*_PathTemp));

                Status = fileName.Status();
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }

                _State = DeleteTempContainer;
                Status = KVolumeNamespace::DeleteFileOrDirectory(fileName, GetThisAllocator(), completion);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }

                break;
            } else {
                //
                // No temporaries, advance state
                //
                _State = DeleteTempMBInfo;
                goto DeleteTempMBInfo;
            }
        }

        case DeleteTempContainer:
        {
#if defined(PLATFORM_UNIX)
            //
            // Delete any old temporary log container files
            //
            KWString fileName(GetThisAllocator(), (WCHAR*)(*_PathTemp));
            fileName += L":MBInfo";

            Status = fileName.Status();
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }
            
            _State = DeleteTempMBInfo;
            Status = KVolumeNamespace::DeleteFileOrDirectory(fileName, GetThisAllocator(), completion);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }
            
            break;
#else
            // Fall through
#endif
        }

        case DeleteTempMBInfo:
        {
DeleteTempMBInfo:           
            //
            // First step is to create the base log container
            //
            _State = CreateBaseContainer;
            if (_PathTemp)
            {                
                _CreateLogContext->StartCreateLog(*_PathTemp,
                                                  _ContainerId,
                                                  *_LogType,
                                                  _LogSize,
                                                  _MaxNumberStreams,
                                                  _MaxRecordSize,
                                                  _Flags,
                                                  _BaseLogContainer,
                                                  this,
                                                  completion);
            } else {
                _CreateLogContext->StartCreateLog(_DiskId,
                                                  _ContainerId,
                                                  *_LogType,
                                                  _LogSize,
                                                  _MaxNumberStreams,
                                                  _MaxRecordSize,
                                                  _Flags,
                                                  _BaseLogContainer,
                                                  this,
                                                  completion);
            }
            
            break;
        }

        case CreateBaseContainer:
        {
            //
            // Next step is to create the shared container metadata
            // info
            //
            _State = CreateLCMBInfo;

            //
            // Set shutdown event on the dedicated log container. It will fire
            // when the container is completely shutdown in the
            // destructor.
            //
            Status = _BaseLogContainer->SetShutdownEvent(&_BaseLogShutdownEvent);
            KInvariant(NT_SUCCESS(Status));
                        
            _LCMBInfo->Reuse();
            _LCMBInfo->StartInitializeMetadataBlock(this,
                                                    completion);
            
            break;
        }

        case CreateLCMBInfo:
        {
            _State = WaitForContainerClose;
            _BaseLogContainer.Reset();

            _BaseLogShutdownWait->StartWaitUntilSet(this,
                                                    completion);
            
            break;
            
        }

        //
        // The order that the files get renamed is important. On
        // windows, there is only a single file with two streams and so
        // only the MarkContainerReady step is needed. On linux, there
        // are two files, the MBInfo file and the log file. Rename the
        // metadata file first since the container open command will
        // first check for the log container file first.
        //
        case WaitForContainerClose:
        {
            if (_PathTemp)
            {                
                _State = MarkContainerReadyMetadata;

#if defined(PLATFORM_UNIX)
                //
                // Rename the metdata file
                //
                KStringView pathTempMBInfoView((WCHAR*)(*_PathTemp));
                KWString pathTempMBInfo(GetThisAllocator(), pathTempMBInfoView);
                KStringView pathMBInfoView((WCHAR*)(*_Path));
                KWString pathMBInfo(GetThisAllocator(), pathMBInfoView);

                pathTempMBInfo += L":MBInfo";
                pathMBInfo += L":MBInfo";

                Status = pathTempMBInfo.Status();
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }

                Status = pathMBInfo.Status();
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }

                Status = KVolumeNamespace::RenameFile(pathTempMBInfo,
                                                      pathMBInfo,
                                                      TRUE,
                                                      GetThisAllocator(),
                                                      completion,
                                                      this);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }


                break;
#else
                // Fall through
#endif
            } else {
                _State = MarkContainerReady;
                goto MarkContainerReady;
            }
        }

        case MarkContainerReadyMetadata:
        {
            //
            // Rename the log container file
            //
            _State = MarkContainerReady;
            Status = KVolumeNamespace::RenameFile((PWCHAR)(*_PathTemp),
                                                  (PWCHAR)(*_Path),
                                                  TRUE,
                                                  GetThisAllocator(),
                                                  completion,
                                                  this);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }
            
            break;
        }       

        case MarkContainerReady:
        {
MarkContainerReady:         
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
OverlayLogFreeService::AsyncInitializeContainerContext::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLogFreeService::AsyncInitializeContainerContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLogFreeService::AsyncInitializeContainerContext::OnStart(
    )
{
    KInvariant(_State == Initial);
    
    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLogFreeService::AsyncInitializeContainerContext::StartInitializeContainer(
    __in RvdLogManager& BaseLogManager,
    __in KGuid DiskId,
    __in_opt KString const * const Path,   
    __in KtlLogContainerId ContainerId,                    
    __in ULONG MaxRecordSize,
    __in ULONG MaxNumberStreams,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in DWORD Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _BaseLogManager = &BaseLogManager;
    _DiskId = DiskId;
    _Path = Path;
    _ContainerId = ContainerId;
    _MaxRecordSize = MaxRecordSize;
    _MaxNumberStreams = MaxNumberStreams;
    _LogType = &LogType;
    _LogSize = LogSize;
    _Flags = Flags;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayLogFreeService::AsyncInitializeContainerContext::~AsyncInitializeContainerContext()
{
}

VOID
OverlayLogFreeService::AsyncInitializeContainerContext::OnReuse(
    )
{
    _BaseLogShutdownEvent.ResetEvent();
    _BaseLogShutdownWait->Reuse();
}

OverlayLogFreeService::AsyncInitializeContainerContext::AsyncInitializeContainerContext(
    __in OverlayLogFreeService& OC
    )
{
    NTSTATUS status;

    //
    // Allocate a waiter for the base container close event
    //
    status = _BaseLogShutdownEvent.CreateWaitContext(GetThisAllocationTag(),
                                                     GetThisAllocator(),
                                                     _BaseLogShutdownWait);
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _OC = &OC;
}

NTSTATUS
OverlayLogFreeService::CreateAsyncInitializeContainerContext(
    __out AsyncInitializeContainerContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLogFreeService::AsyncInitializeContainerContext::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncInitializeContainerContext(*this);
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
OverlayLogFreeService::OperationCreateCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    ServiceWrapper::Operation* op = (ServiceWrapper::Operation*)ParentAsync;
    
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(op != nullptr ? op->GetActivityId() : 0, "OverlayLogFreeService::OperationCreateCompletion", status,
                            (ULONGLONG)op,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
    }
    
    op->CompleteOpAndStartNext(status);
}


VOID
OverlayLogFreeService::OnOperationCreate(
    __in ServiceWrapper::Operation& Op
    )
{
    NTSTATUS status;
    OverlayLogFreeService::OperationCreate* operationCreate;
    AsyncInitializeContainerContext::SPtr context;

    operationCreate = (OverlayLogFreeService::OperationCreate*)(&Op);
    
    status = CreateAsyncInitializeContainerContext(context);

    if (NT_SUCCESS(status))
    {
        context->SetGlobalContext(Op.GetGlobalContext());

        KAsyncContextBase::CompletionCallback completion(this, &OverlayLogFreeService::OperationCreateCompletion);
        context->StartInitializeContainer(*_BaseLogManager,
                                          operationCreate->_DiskId,
                                          operationCreate->_Path.RawPtr(),
                                          _ContainerId,
                                          operationCreate->_MaxRecordSize,
                                          operationCreate->_MaxNumberStreams,
                                          *operationCreate->_LogType,
                                          operationCreate->_LogSize,
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

OverlayLogFreeService::OperationCreate::~OperationCreate()
{
}

OverlayLogFreeService::OperationCreate::OperationCreate(
    __in OverlayLogFreeService& OC,
    __in KGuid& DiskId,
    __in_opt KString const * const Path,
    __in ULONG MaxRecordSize,
    __in ULONG MaxNumberStreams,
    __in KWString& LogType,
    __in LONGLONG LogSize,
    __in DWORD Flags
   ) : ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OC)),
       _DiskId(DiskId),
       _Path(Path),
       _MaxNumberStreams(MaxNumberStreams),
       _LogType(&LogType),
       _LogSize(LogSize),
       _MaxRecordSize(MaxRecordSize),
       _Flags(Flags)
{
}

//
// Delete a shared log container
//
VOID
OverlayLogFreeService::AsyncDeleteContainerContext::DoComplete(
    __in NTSTATUS Status
    )
{
    _State = NT_SUCCESS(Status) ? Completed : CompletedWithError;
    
    _BaseLogManager = nullptr;
    _LCMBInfo = nullptr;
    _DeleteLogContext = nullptr;    
    _OpenBaseLog = nullptr;
    _BaseSharedLog = nullptr;
    _BaseSharedLogShutdownWait = nullptr;
    _Path = nullptr;

    Complete(Status);
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::DeleteStreamCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _State, 0);
    }

    LONG count = InterlockedDecrement(&_DeleteStreamAsyncCount);
    if ((count == 0) && (_State >= CloseLCMBInfo))
    {
        _DeleteStreamCompletionEvent.SetEvent();
    }
}

NTSTATUS
OverlayLogFreeService::AsyncDeleteContainerContext::LCEntryCallback(
    __in SharedLCMBInfoAccess::LCEntryEnumAsync& Async
    )
{
    NTSTATUS status;
    SharedLCMBInfoAccess::SharedLCEntry& entry = Async.GetEntry();
    OverlayStreamFreeService::AsyncDeleteStreamContext::SPtr deleteStreamContext;


    status = OverlayStreamFreeService::CreateAsyncDeleteStreamContext(deleteStreamContext,
                                                                      GetThisAllocator(),
                                                                      GetThisAllocationTag());

    if (! NT_SUCCESS(status))
    {
        //
        // Fail this back to the caller
        //
        KTraceFailedAsyncRequest(status, this, (ULONGLONG)&entry, 0);
        return(status);
    }

    KString::SPtr path;

    if (entry.PathToDedicatedContainer[0] == 0)
    {
        path = nullptr;
    } else {
        path = KString::Create(&entry.PathToDedicatedContainer[0], GetThisAllocator());
        if (! path)
        {
            KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }
    
    //
    // Kick off the delete stream async. Note that these are run
    // independently and in parallel of each other and decoupled from this parent
    // context. A refcount is taken before starting and then released
    // on completion to ensure this context stay around. It would be
    // possible that the shared container deletion finishes before the
    // streams deletion. Also note that if the streams deletion fails
    // then it doesn't effect the returned status of the container
    // deletion.
    //    
    InterlockedIncrement(&_DeleteStreamAsyncCount);
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLogFreeService::AsyncDeleteContainerContext::DeleteStreamCompletion);
    deleteStreamContext->StartDeleteStream(*_BaseLogManager,
                                           NULL,            // No shared rvdlog ptr
                                           NULL,            // No shared LCMBInfo ptr
                                           _DiskId,
                                           path.RawPtr(),
                                           entry.LogStreamId,
                                           entry.Index,
                                           entry.MaxLLMetadataSize,
                                           NULL,               // No parent async
                                           completion);

    return(STATUS_SUCCESS);
}


VOID
OverlayLogFreeService::AsyncDeleteContainerContext::FSMContinue(
    __in NTSTATUS Status
    )
{   
    KAsyncContextBase::CompletionCallback completion(this, &OverlayLogFreeService::AsyncDeleteContainerContext::OperationCompletion);

    KDbgCheckpointWDataInformational(GetActivityId(), "AsyncDeleteContainerContext", Status,
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
    
#pragma warning(disable:4127)   // C4127: conditional expression is constant    
    while (TRUE)
    {
        switch (_State)
        {
            case Initial:
            {
                //
                // First grab some contexts for us to do our work
                //
                _State = OpenBaseContainer;
                Status = _BaseLogManager->CreateAsyncOpenLogContext(_OpenBaseLog);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }

                Status = _BaseSharedLogShutdownEvent.CreateWaitContext(GetThisAllocationTag(),
                                                                       GetThisAllocator(),
                                                                       _BaseSharedLogShutdownWait);
                
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }
                
                Status = _DeleteStreamCompletionEvent.CreateWaitContext(GetThisAllocationTag(),
                                                                       GetThisAllocator(),
                                                                       _DeleteStreamCompletionWait);
                
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }
                
                Status = _BaseLogManager->CreateAsyncDeleteLogContext(_DeleteLogContext);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }

                //
                // Next, open the base log container as we need to grab
                // the max number streams
                //
                if (_Path)
                {
                    _OpenBaseLog->StartOpenLog(*_Path,
                                               _BaseSharedLog,
                                               this,
                                               completion);
                } else {
                    _OpenBaseLog->StartOpenLog(_DiskId,
                                               _ContainerId,
                                               _BaseSharedLog,
                                               this,
                                               completion);
                }
                                           
                return;
            }
            
            case OpenBaseContainer:
            {
                //
                // Next open up the shared LCMB info for the shared
                // container. The LCMBENtry callback is invoked for
                // each stream and so in the callback each stream will
                // start an async to delete itself. Note that the
                // delete asyncs run in parallel and not in the
                // apartment of this delete container async. In this
                // way they can all run in parallel.
                //
                _State = DeleteAllStreams;
                Status = SharedLCMBInfoAccess::Create(_LCMBInfo,
                                                      _DiskId,
                                                      _Path.RawPtr(),
                                                      _ContainerId,
                                                      _BaseSharedLog->QueryMaxAllowedStreams(),
                                                      GetThisAllocator(),
                                                      GetThisAllocationTag());
                if (! NT_SUCCESS(Status))
                {
                    BOOLEAN freed;
#if !defined(PLATFORM_UNIX)
                    //
                    // This assert seems to be hitting sometimes on Linux
                    // which is a timing issue
                    //
                    freed = _BaseSharedLog.Reset();
                    KInvariant(freed);
#else
                    freed = _BaseSharedLog.Reset();             
#endif
                    _BaseSharedLogShutdownWait->StartWaitUntilSet(this,
                                                                  completion);
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    DoComplete(Status);
                    return;
                }

                _DeleteStreamAsyncCount = 0;
                SharedLCMBInfoAccess::LCEntryEnumCallback lCEntryCallback(this,
                                                         &OverlayLogFreeService::AsyncDeleteContainerContext::LCEntryCallback);
                _LCMBInfo->StartOpenMetadataBlock(lCEntryCallback,
                                                  this,
                                                  completion);

                return;
            }

            case DeleteAllStreams:
            {
                //
                // At this point all of the delete stream asyncs should
                // be running. We can move ahead and close the base log
                // container and its metadata in preparation of
                // deleting them.
                //
                _State = CloseLCMBInfo;

                if (_DeleteStreamAsyncCount == 0)
                {
                    _DeleteStreamCompletionEvent.SetEvent();
                }
                
                _LCMBInfo->Reuse();
                _LCMBInfo->StartCloseMetadataBlock(this,
                                                   completion);
                return;
            }

            case CloseLCMBInfo:
            {
                //
                // Now that the base container is opened, get the max
                // number of streams and then close it. 
                //
                _State = CloseBaseContainer;                
                Status = _BaseSharedLog->SetShutdownEvent(&_BaseSharedLogShutdownEvent);
                KInvariant(NT_SUCCESS(Status));

#if DBG
                BOOLEAN freed;              
                {
                    //
                    // This assert seems to be hitting sometimes on Linux
                    // which is a timing issue
                    //
                    RvdLog* _BaseSharedLogRaw = _BaseSharedLog.RawPtr();
                    
                    freed = _BaseSharedLog.Reset();
                    if (! freed)
                    {
                        KTraceFailedAsyncRequest(Status, this, _State, (ULONGLONG)_BaseSharedLogRaw);
                    }
                }
#else
                _BaseSharedLog.Reset();
#endif
                _BaseSharedLogShutdownWait->StartWaitUntilSet(this,
                                                              completion);
                
                return;
            }

            case CloseBaseContainer:
            {
                _State = DeleteBaseContainer;

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
                return;                
            }

            case DeleteBaseContainer:
            {
                //
                // As a last step, delete the shared metadata
                _State = DeleteLCMBInfo;
                _LCMBInfo->Reuse();
                _LCMBInfo->StartCleanupMetadataBlock(this,
                                                     completion);
                return;                
            }
            
            case DeleteLCMBInfo:

            {
                _State = WaitForAllDeleteStreams;
                _DeleteStreamCompletionWait->StartWaitUntilSet(this,
                                                               completion);
                return;
            }

            case WaitForAllDeleteStreams:
            {
                //
                // All done
                //
                DoComplete(STATUS_SUCCESS);
                return;
            }

            default:
            {
                KInvariant(FALSE);
            }
        }
    }
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    FSMContinue(status); 
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::OnStart(
    )
{
    KInvariant(_State == Initial);

    FSMContinue(STATUS_SUCCESS);   
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::StartDeleteContainer(
    __in RvdLogManager& BaseLogManager,
    __in KGuid DiskId,
    __in_opt KString const * const Path,   
    __in KtlLogContainerId ContainerId,                    
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = Initial;

    _BaseLogManager = &BaseLogManager;
    _DiskId = DiskId;
    _Path = Path;
    _ContainerId = ContainerId;
    
    Start(ParentAsyncContext, CallbackPtr);
}

OverlayLogFreeService::AsyncDeleteContainerContext::~AsyncDeleteContainerContext()
{
}

VOID
OverlayLogFreeService::AsyncDeleteContainerContext::OnReuse(
    )
{
}

OverlayLogFreeService::AsyncDeleteContainerContext::AsyncDeleteContainerContext(
    __in OverlayLogFreeService& OC
    )
{
    _OC = &OC;
}

NTSTATUS
OverlayLogFreeService::CreateAsyncDeleteContainerContext(
    __out AsyncDeleteContainerContext::SPtr& Context
    )
{
    NTSTATUS status;
    OverlayLogFreeService::AsyncDeleteContainerContext::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteContainerContext(*this);
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
// Operation Delete
//
VOID
OverlayLogFreeService::OperationDeleteCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    NTSTATUS status = Async.Status();
    ServiceWrapper::Operation* op = (ServiceWrapper::Operation*)ParentAsync;
    
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWDataInformational(op != nullptr ? op->GetActivityId() : 0, "OverlayLogFreeService::OperationDeleteCompletion", status,
                            (ULONGLONG)op,
                            (ULONGLONG)this,
                            (ULONGLONG)0,
                            (ULONGLONG)0);                                
    }
    
    op->CompleteOpAndStartNext(status);
}

VOID
OverlayLogFreeService::OnOperationDelete(
    __in ServiceWrapper::Operation& Op
    )
{
    NTSTATUS status;
    AsyncDeleteContainerContext::SPtr context;
    OverlayLogFreeService::OperationDelete& op = static_cast<OverlayLogFreeService::OperationDelete&>(Op);

    status = CreateAsyncDeleteContainerContext(context);

    if (NT_SUCCESS(status))
    {
        context->SetGlobalContext(Op.GetGlobalContext());

        KAsyncContextBase::CompletionCallback completion(this, &OverlayLogFreeService::OperationDeleteCompletion);
        context->StartDeleteContainer(*_BaseLogManager,
                                      op._DiskId,
                                      op._Path.RawPtr(),
                                      _ContainerId,
                                      &Op,
                                      completion);
    } else {
        KTraceFailedAsyncRequest(status, nullptr, (ULONGLONG)this, (ULONGLONG)&Op);

        //
        // We need to fail the delete back to the original caller
        //
        Op.CompleteOpAndStartNext(status);
    }
}

OverlayLogFreeService::OperationDelete::~OperationDelete()
{
}

OverlayLogFreeService::OperationDelete::OperationDelete(
    __in OverlayLogFreeService& OC,
    __in KGuid DiskId,
    __in_opt KString const * const Path
   ) : ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OC)),
       _DiskId(DiskId),
       _Path(Path)
{
}

OverlayLogFreeService::OperationOpen::~OperationOpen()
{
}

OverlayLogFreeService::OperationOpen::OperationOpen(
    __in OverlayLogFreeService& OC,
    __in KGuid& DiskId,
    __in_opt KString const * const Path                              
) :
     ServiceWrapper::Operation(static_cast<ServiceWrapper&>(OC)),
    _DiskId(DiskId),
    _Path(Path)
{
}

