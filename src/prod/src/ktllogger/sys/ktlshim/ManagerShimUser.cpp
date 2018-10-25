// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define KDRIVER 1
#endif

#include "KtlLogShimUser.h"

//************************************************************************************
//
// KtlLogManager Implementation
//
//************************************************************************************


KtlLogManagerUser::KtlLogManagerUser(
    )
{
    SetObjectId(0);     
}

#ifdef _WIN64
VOID
KtlLogManagerUser::DeleteObjectCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, ParentAsync, 0, 0);          
    }
}
#endif

KtlLogManagerUser::~KtlLogManagerUser()
{
    NTSTATUS status;
    ULONGLONG objectId;

    objectId = GetObjectId();

    if (objectId == 0)
    {
        return;
    }
        
    status = _Marshaller->InitializeForDeleteObject(RequestMarshaller::LogManager,
                                                    GetObjectId());
    if (! NT_SUCCESS(status))
    {
        return;             
    }

    _DevIoCtl->Reuse();

#ifdef _WIN64
    KAsyncContextBase::CompletionCallback completion(&KtlLogManagerUser::DeleteObjectCompletion);
#endif
    _DevIoCtl->StartDeviceIoControl(_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    NULL,
                                    0,
                                    nullptr,
                                    nullptr);
}

NTSTATUS
KtlLogManager::CreateDriver(
    __in ULONG AllocationTag,
    __in KAllocator& Allocator,
    __out KtlLogManager::SPtr& Result
    )
{
    NTSTATUS status;
    KtlLogManagerUser::SPtr logManUser;

    Result = nullptr;

    logManUser = _new(AllocationTag, Allocator) KtlLogManagerUser();
    if (! logManUser)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, AllocationTag, 0);
        return(status);
    }

    status = logManUser->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = OpenCloseDriver::Create(logManUser->_DeviceHandle,
                                     Allocator,
                                     AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Allocate resources needed
    //
    status = DevIoControlUser::Create(logManUser->_DevIoCtl,
                                      Allocator,
                                      AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = RequestMarshallerUser::Create(logManUser->_Marshaller,
                                      Allocator,
                                      AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    Result = logManUser.RawPtr();
    return(status);
}

VOID
KtlLogManagerUser::CloseLogManagerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    CloseLogManagerFSM(status); 
}


VOID
KtlLogManagerUser::CloseLogManagerFSM(
    __in NTSTATUS Status
)
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::CloseLogManagerCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _CloseState, 0);
        CompleteClose(Status);
        return;
    }
    
    switch (_CloseState)
    {
        case Initial:
        {
            _CloseState = CloseDeviceHandle;

            _DeviceHandle->SignalCloseDevice();
            
            break;
        }

        case CloseDeviceHandle:
        {
            _CloseState = Completed;
            _DeviceHandle->PostCloseCompletion();
            CompleteClose(STATUS_SUCCESS);   
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }       
}

VOID
KtlLogManagerUser::OnServiceClose(
    )
{
    _CloseState = Initial;
    CloseLogManagerFSM(STATUS_SUCCESS);
}

VOID
KtlLogManagerUser::OnServiceReuse(
    )
{
    // CONSIDER: What should this really do ?
    KInvariant(FALSE);
}

VOID
KtlLogManagerUser::DoCompleteOpen(
    __in NTSTATUS Status
)
{
    if (! NT_SUCCESS(Status))
    {
        _OpenState = CompletedWithError;
    }

    CompleteOpen(Status);
}

VOID
KtlLogManagerUser::OpenLogManagerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    OpenLogManagerFSM(status); 
}


VOID
KtlLogManagerUser::OpenLogManagerFSM(
    __in NTSTATUS Status
)
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::OpenLogManagerCompletion);

    if (! NT_SUCCESS(Status))
    {
        KTraceFailedAsyncRequest(Status, this, _OpenState, 0);
        DoCompleteOpen(Status);
        return;
    }
    
    switch (_OpenState)
    {
        case Initial:
        {
            _OpenState = OpenDeviceHandle;

            _DeviceHandle->Reuse();
            _DeviceHandle->StartOpenDevice(KStringView(RequestMarshaller::KTLLOGGER_DEVICE_NAME),
                                              this,
                                              completion);
            
            break;
        }

        case OpenDeviceHandle:
        {           
            _OpenState = CreateLogManager;

            //
            // Start the async that completes when the handle to the
            // device is completely closed. This needs to be done
            // before the first DeviceIoControl is issued
            //
            KAsyncContextBase::CompletionCallback closeCompletion(this, &KtlLogManagerUser::CloseLogManagerCompletion);
            _DeviceHandle->Reuse();
            _DeviceHandle->StartWaitForCloseDevice(this,
                                                   closeCompletion);
            
            Status = _Marshaller->InitializeForCreateObject(RequestMarshaller::LogManager);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OpenState, 0);
                DoCompleteOpen(Status);
                return;             
            }

            
            _DevIoCtl->Reuse();
            _DevIoCtl->StartDeviceIoControl(_DeviceHandle,
                                            RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                            _Marshaller->GetInKBuffer(),
                                            _Marshaller->GetWriteSize(),
                                            _Marshaller->GetOutKBuffer(),
                                            _Marshaller->GetMaxOutBufferSize(),
                                            this,
                                            completion);                                                
            break;
        }
        case CreateLogManager:
        {
            RequestMarshaller::OBJECT_TYPEANDID objectTypeAndId;
            
            _OpenState = Completed;

            Status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OpenState, 0);
                DoCompleteOpen(Status);
                return;             
            }

            Status = _Marshaller->ReadObjectTypeAndId(objectTypeAndId);
            if (! NT_SUCCESS(Status))
            {
                KTraceFailedAsyncRequest(Status, this, _OpenState, 0);
                DoCompleteOpen(Status);
                return;             
            }

            if (objectTypeAndId.ObjectType != RequestMarshaller::LogManager)
            {
                Status = STATUS_OBJECT_TYPE_MISMATCH;
                KTraceFailedAsyncRequest(Status, this, _OpenState, 0);
                DoCompleteOpen(Status);
                return;             
            }

            SetObjectId(objectTypeAndId.ObjectId);

            DoCompleteOpen(STATUS_SUCCESS);   
            return;
        }
        
        default:
        {
            KInvariant(FALSE);
        }
    }       
}


VOID
KtlLogManagerUser::OnServiceOpen(
    )
{
    _OpenState = Initial;
    OpenLogManagerFSM(STATUS_SUCCESS);
}


NTSTATUS
KtlLogManagerUser::StartOpenLogManager(  
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt OpenCompletionCallback OpenCallbackPtr,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;
    
    status = StartOpen(ParentAsync,
                       OpenCallbackPtr,
                       GlobalContextOverride);
    
    return(status);
}


#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::OpenLogManagerAsync(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    ktl::kservice::OpenAwaiter::SPtr awaiter;
    NTSTATUS status = ktl::kservice::OpenAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        ParentAsync,
        GlobalContextOverride);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif


NTSTATUS
KtlLogManagerUser::StartCloseLogManager(
    __in_opt KAsyncContextBase* const ParentAsync, 
    __in_opt CloseCompletionCallback CloseCallbackPtr
    )
{
    NTSTATUS status;
    
    status = StartClose(ParentAsync,
                        CloseCallbackPtr);

    return(status);
}


#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::CloseLogManagerAsync(
    __in_opt KAsyncContextBase* const ParentAsync
    )
{
    ktl::kservice::CloseAwaiter::SPtr awaiter;
    NTSTATUS status = ktl::kservice::CloseAwaiter::Create(
        GetThisAllocator(),
        GetThisAllocationTag(),
        *this,
        awaiter,
        ParentAsync,
        nullptr);

    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    status = co_await *awaiter;
    co_return status;
}
#endif
                

//
// CreateLogContainer
//
VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::OnCompleted(
    )
{
    _LogContainer = nullptr;
    _Path = nullptr;
}

VOID 
KtlLogManagerUser::AsyncCreateLogContainerUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::LogContainerCreated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONGLONG objectId;

    _LogManager->ReleaseActivities();
    
    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadObject(RequestMarshaller::LogContainer,
                                         objectId);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        _LogContainer->SetObjectId(objectId);
        
        (*_ResultLogContainerPtr) = Ktl::Move(reinterpret_cast<KtlLogContainer::SPtr&>(_LogContainer));
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::OnStart(
    )
{
    NTSTATUS status;
    
    if (!_LogManager->TryAcquireActivities())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Ensure path name is not too long
    //
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            DoComplete(status);
            return;
        }
    }

    _LogContainer = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogContainerUser(_LogContainerId,
                                                                                 *(_LogManager->_DeviceHandle)
                                                                                 );
    if (! _LogContainer)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        DoComplete(status);
        return;
    }
    
    status = _LogContainer->Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }



    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogManagerObject    CreateLogContainer,
    //                                           DiskId, Path
    //                                           LogContainerId,
    //                                           LogSize,
    //                                           MaximumRecordSize,
    //                                           MaximumNumberStreams,
    //                                           Flags
    //
    // <-                                      LogContainerObject

    RequestMarshallerUser::REQUEST_BUFFER* requestBuffer;
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogManager->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogManager->GetObjectId(),
                                                    RequestMarshaller::CreateLogContainer);
    if (! NT_SUCCESS(status))
    {
        _LogContainer = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteGuid(_DiskId);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteString(_Path);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);    

    status = _Marshaller->WriteKtlLogContainerId(_LogContainerId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteData<LONGLONG>(_LogSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteData<ULONG>(_MaximumNumberStreams);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteData<ULONG>(_MaximumRecordSize);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    status = _Marshaller->WriteData<DWORD>(_Flags);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);

    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::AsyncCreateLogContainerUser::LogContainerCreated);
    _DevIoCtl->StartDeviceIoControl(_LogManager->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::StartCreateLogContainer(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::StartCreateLogContainer(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::StartCreateLogContainer(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::StartCreateLogContainer(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncCreateLogContainerUser::CreateLogContainerAsync(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncCreateLogContainerUser::CreateLogContainerAsync(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = 0;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncCreateLogContainerUser::CreateLogContainerAsync(
    __in KGuid const& DiskId,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncCreateLogContainerUser::CreateLogContainerAsync(
    __in KString const & Path,
    __in KtlLogContainerId const& LogContainerId,
    __in LONGLONG LogSize,
    __in ULONG MaximumNumberStreams,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);
    
    GUID nullGUID = { 0 };

    _Path = &Path;
    _DiskId = nullGUID;
    _LogContainerId = LogContainerId;
    _LogSize = LogSize;
    _MaximumRecordSize = MaximumRecordSize;
    _MaximumNumberStreams = MaximumNumberStreams;
    _Flags = Flags;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerUser::AsyncCreateLogContainerUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogManagerUser::AsyncCreateLogContainerUser::AsyncCreateLogContainerUser()
{
}

KtlLogManagerUser::AsyncCreateLogContainerUser::~AsyncCreateLogContainerUser()
{
}

NTSTATUS
KtlLogManagerUser::CreateAsyncCreateLogContainerContext(
    __out AsyncCreateLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerUser::AsyncCreateLogContainerUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogContainerUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;
    
    status = RequestMarshallerUser::Create(context->_Marshaller,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DevIoControlUser::Create(context->_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Open log container
//
VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::OnCompleted(
    )
{
    _LogContainer = nullptr;
    _Path = nullptr;
}

VOID 
KtlLogManagerUser::AsyncOpenLogContainerUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::OpenLogContainerCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONGLONG objectId;

    _LogManager->ReleaseActivities();
    
    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadObject(RequestMarshaller::LogContainer,
                                         objectId);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        _LogContainer->SetObjectId(objectId);

        (*_ResultLogContainerPtr) = Ktl::Move(reinterpret_cast<KtlLogContainer::SPtr&>(_LogContainer));
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::OnStart(
    )
{
    NTSTATUS status;
    RequestMarshallerUser::REQUEST_BUFFER* requestBuffer;
    
    if (!_LogManager->TryAcquireActivities())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

                
    //
    // Ensure path name is not too long
    //
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            KDbgPrintf("%s: pathname %ws is longer than the limit of %d characters",
                       __FUNCTION__, _Path.RawPtr(), KtlLogManager::MaxPathnameLengthInChar-1);
            DoComplete(status);
            return;
        }
    }

    _LogContainer = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogContainerUser(_LogContainerId,
                                                                                         *(_LogManager->_DeviceHandle));
    
    if (_LogContainer == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, GetThisAllocationTag(), 0);
        DoComplete(status);
    }

    status = _LogContainer->Status();
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogManagerObject     OpenLogContainer, DiskId, Path LogContainerId
    //
    // <-                                      LogContainerObject    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogManager->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogManager->GetObjectId(),
                                                    RequestMarshaller::OpenLogContainer);
    
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);
    
    status = _Marshaller->WriteGuid(_DiskId);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);
    
    status = _Marshaller->WriteString(_Path);

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);
    
    status = _Marshaller->WriteKtlLogContainerId(_LogContainerId);
    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    requestBuffer = (RequestMarshallerUser::REQUEST_BUFFER*)_Marshaller->GetInKBuffer()->GetBuffer();   
    KInvariant(_LogManager->GetObjectId() == requestBuffer->ObjectTypeAndId.ObjectId);
    
    KAsyncContextBase::CompletionCallback completion(this,
                                                     &KtlLogManagerUser::AsyncOpenLogContainerUser::OpenLogContainerCompletion);
    
    _DevIoCtl->StartDeviceIoControl(_LogManager->_DeviceHandle,
        RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
        _Marshaller->GetInKBuffer(),
        _Marshaller->GetWriteSize(),
        _Marshaller->GetOutKBuffer(),
        _Marshaller->GetMaxOutBufferSize(),
        this,
        completion);
}


VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::StartOpenLogContainer(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncOpenLogContainerUser::OpenLogContainerAsync(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __out KtlLogContainer::SPtr& LogContainer
)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::StartOpenLogContainer(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __out KtlLogContainer::SPtr& LogContainer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncOpenLogContainerUser::OpenLogContainerAsync(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __out KtlLogContainer::SPtr& LogContainer
)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;
    _ResultLogContainerPtr = &LogContainer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogManagerUser::AsyncOpenLogContainerUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogManagerUser::AsyncOpenLogContainerUser::AsyncOpenLogContainerUser()
{
}

KtlLogManagerUser::AsyncOpenLogContainerUser::~AsyncOpenLogContainerUser()
{
}

NTSTATUS
KtlLogManagerUser::CreateAsyncOpenLogContainerContext(
    __out AsyncOpenLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerUser::AsyncOpenLogContainerUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogContainerUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;

    status = RequestMarshallerUser::Create(context->_Marshaller,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DevIoControlUser::Create(context->_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Delete log container
//
VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::OnCompleted(
    )
{
    _Path = nullptr;
}

VOID 
KtlLogManagerUser::AsyncDeleteLogContainerUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::LogContainerDeleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    _LogManager->ReleaseActivities();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::OnStart(
    )
{
    NTSTATUS status;
    
    if (!_LogManager->TryAcquireActivities())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Ensure path name is not too long
    //
    if (_Path != nullptr)
    {
        if (_Path->Length() > KtlLogManager::MaxPathnameLengthInChar-1)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _Path->Length(), KtlLogManager::MaxPathnameLengthInChar-1);
            DoComplete(status);
            return;
        }
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogManagerObject     DeleteLogContainer, DiskId, Path, LogContainerId, LogSize
    //
    // <-                                      
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogManager->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogManager->GetObjectId(),
                                                    RequestMarshaller::DeleteLogContainer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteGuid(_DiskId);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteString(_Path);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogContainerId(_LogContainerId);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::AsyncDeleteLogContainerUser::LogContainerDeleted);
    _DevIoCtl->StartDeviceIoControl(_LogManager->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
    
}

VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::StartDeleteLogContainer(
    __in const KGuid& DiskId,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _Path = nullptr;
    _LogContainerId = LogContainerId;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::StartDeleteLogContainer(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    GUID nullGUID = { 0 };
    
    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogManagerUser::AsyncDeleteLogContainerUser::DeleteLogContainerAsync(
    __in KString const & Path,
    __in const KtlLogContainerId& LogContainerId,
    __in_opt KAsyncContextBase* const ParentAsyncContext)
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    GUID nullGUID = { 0 };

    _DiskId = nullGUID;
    _Path = &Path;
    _LogContainerId = LogContainerId;

    status = co_await awaiter;
    co_return status;
}
#endif  

VOID
KtlLogManagerUser::AsyncDeleteLogContainerUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogManagerUser::AsyncDeleteLogContainerUser::AsyncDeleteLogContainerUser()
{
}

KtlLogManagerUser::AsyncDeleteLogContainerUser::~AsyncDeleteLogContainerUser()
{
}

NTSTATUS
KtlLogManagerUser::CreateAsyncDeleteLogContainerContext(
    __out AsyncDeleteLogContainer::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerUser::AsyncDeleteLogContainerUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogContainerUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;
    
    status = RequestMarshallerUser::Create(context->_Marshaller,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DevIoControlUser::Create(context->_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}


//
// Enumerate log containers
//
VOID
KtlLogManagerUser::AsyncEnumerateLogContainersUser::OnCompleted(
    )
{
}

VOID 
KtlLogManagerUser::AsyncEnumerateLogContainersUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerUser::AsyncEnumerateLogContainersUser::LogContainersEnumerated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    _LogManager->ReleaseActivities();

    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadKtlLogContainerIdArray(*_LogIdArray);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogManagerUser::AsyncEnumerateLogContainersUser::OnStart(
    )
{
    NTSTATUS status;
    
    if (!_LogManager->TryAcquireActivities())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogManagerObject     EnumerateLogContainers, DiskId
    //
    // <-                                      LogIdArray
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogManager->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogManager->GetObjectId(),
                                                    RequestMarshaller::EnumerateLogContainers);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteGuid(_DiskId);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    _DevIoCtl->SetRetryCount(1);
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::AsyncEnumerateLogContainersUser::LogContainersEnumerated);
    _DevIoCtl->StartDeviceIoControl(_LogManager->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
    
}

VOID
KtlLogManagerUser::AsyncEnumerateLogContainersUser::StartEnumerateLogContainers(
    __in const KGuid& DiskId,
    __out KArray<KtlLogContainerId>& LogIdArray,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _DiskId = DiskId;
    _LogIdArray = &LogIdArray;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogManagerUser::AsyncEnumerateLogContainersUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogManagerUser::AsyncEnumerateLogContainersUser::AsyncEnumerateLogContainersUser()
{
}

KtlLogManagerUser::AsyncEnumerateLogContainersUser::~AsyncEnumerateLogContainersUser()
{
}

NTSTATUS
KtlLogManagerUser::CreateAsyncEnumerateLogContainersContext(
    __out AsyncEnumerateLogContainers::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerUser::AsyncEnumerateLogContainersUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateLogContainersUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_LogManager = this;
    
    status = RequestMarshallerUser::Create(context->_Marshaller,
                                          GetThisAllocator(),
                                          GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DevIoControlUser::Create(context->_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
}

//
// Configure
//
VOID
KtlLogManagerUser::AsyncConfigureContextUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _InBuffer = nullptr;
    
    _LogManager->ReleaseActivities();
    Complete(Status);
}

VOID
KtlLogManagerUser::AsyncConfigureContextUser::ConfigureCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();

    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;             
        }

        status = _Marshaller->ReadData<ULONG>(*_Result);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;
        }
        
        status = _Marshaller->ReadKBuffer(*_OutBuffer);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;
        }
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    DoComplete(status);
}

VOID
KtlLogManagerUser::AsyncConfigureContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    if (!_LogManager->TryAcquireActivities())
    {
        status = K_STATUS_SHUTDOWN_PENDING;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    if (! _InBuffer)
    {
        //
        // If user didn't pass an input buffer then build an empty one
        //
        status = KBuffer::Create(0, _InBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;
        }
    }
    
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogManagerObject    ConfigureLogManager, Configure Code, InBuffer
    //
    // <-                                     Result, OutBuffer
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogManager->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogManager->GetObjectId(),
                                                    RequestMarshaller::ConfigureLogManager);
    status = _Marshaller->WriteData<ULONG>(_Code);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteKBuffer(_InBuffer);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    _DevIoCtl->SetRetryCount(1);
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogManagerUser::AsyncConfigureContextUser::ConfigureCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogManager->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogManagerUser::AsyncConfigureContextUser::StartConfigure(
    __in ULONG Code,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Code = Code;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;
    *_OutBuffer = nullptr;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogManagerUser::AsyncConfigureContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
    _OutBuffer = NULL;
}


KtlLogManagerUser::AsyncConfigureContextUser::AsyncConfigureContextUser()
{
}

KtlLogManagerUser::AsyncConfigureContextUser::~AsyncConfigureContextUser()
{
}

NTSTATUS
KtlLogManagerUser::CreateAsyncConfigureContext(
    __out AsyncConfigureContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogManagerUser::AsyncConfigureContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncConfigureContextUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = RequestMarshallerUser::Create(context->_Marshaller,
                                          GetThisAllocator(),
                                          GetThisAllocationTag(),
                                          RequestMarshallerUser::_InitialInBufferSize +
                                          sizeof(KtlLogManager::SharedLogContainerSettings));
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = DevIoControlUser::Create(context->_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
        
    context->_LogManager = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}
