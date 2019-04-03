// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KtlLogShimUser.h"
#include "KtlLogShimKernel.h"

class DevIoControlInprocUser;

class OpenCloseUser : public OpenCloseDriver, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(OpenCloseUser);

    friend DevIoControlInprocUser;
    friend OpenCloseDriver;
    
    public:     
        VOID StartOpenDevice(
            __in const KStringView& FileName,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) override;
            
        VOID StartWaitForCloseDevice(
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) override;

        VOID SignalCloseDevice(
        ) override;
        
        VOID PostCloseCompletion(
        ) override;

        FileObjectTable* GetFileObjectTable(
            ) override
        {
            return(_FOT.RawPtr());
        }

        PVOID GetKernelPointerFromObjectId(
            __in ULONGLONG ObjectId
        ) override ;
        
    protected:
        VOID
        OnStart(
            );

        VOID
        OnReuse(
            );

    private:
        HANDLE GetHandle()
        {
            return((HANDLE)_FOT.RawPtr());
        }

        VOID
        Execute(
            );

    private:
        //
        // General members
        //
        ULONG _AllocationTag;

        //
        // Parameters to api
        //
        KStringView _FileName;
        
        //
        // Members needed for functionality
        //
        FileObjectTable::SPtr _FOT;

        enum { NotDefined, OpenDevice, WaitForCloseDevice, CloseDevice } _Mode;     
};

class DevIoControlInprocKernel;

class DevIoControlInprocUser : public DevIoControlUser
{
    K_FORCE_SHARED(DevIoControlInprocUser);

    friend DevIoControlUser;
    friend DevIoControlInprocKernel;
    
    DevIoControlInprocUser(
        __in ULONG AllocationTag
    );
    
    public:
        VOID StartDeviceIoControl(
            __in OpenCloseDriver::SPtr& OpenCloseHandle,
            __in ULONG Ioctl,
            __in KBuffer::SPtr InKBuffer,
            __in ULONG InBufferSize,
            __in_opt KBuffer::SPtr OutKBuffer,
            __in ULONG OutBufferSize,
            __in_opt KAsyncContextBase* const ParentAsyncContext,
            __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
        ) override ;
            
        ULONG GetOutBufferSize() override
        {
            return(_OutBufferSize);
        }

        VOID SetRetryCount(
            __in ULONG MaxRetries
        ) override
        {
            _MaxRetries = MaxRetries;
            _Retries = 0;
        }
                
        PUCHAR GetTransferBuffer()
        {
            return((PUCHAR)_TransferKBuffer->GetBuffer());
        }

        ULONG GetTransferBufferSize()
        {
            return(_TransferKBuffer->QuerySize());
        }

    private:
        VOID
        OnStart(
            ) override ;

        VOID
        OnReuse(
            ) override ;

        VOID
        OnCancel(
            ) override ;

        VOID
        Execute(
            );

        ULONGLONG GetRequestId()
        {
            return(_RequestId);
        }

        NTSTATUS
        RequestMarkCancelableEx(
            );

        NTSTATUS
        RequestUnmarkCancelable(
            );
        
    private:
        //
        // General members
        //
        ULONG _AllocationTag;
        KSpinLock _CancelLock;

        //
        // Parameters to api
        //
        OpenCloseUser::SPtr _OpenCloseHandle;
        ULONG _Ioctl;
        ULONG _InBufferSize;
        KBuffer::SPtr _InKBuffer;
        ULONG _OutBufferSize;
        KBuffer::SPtr _OutKBuffer;
        ULONG _MaxRetries;

        //
        // Members needed for functionality
        //
        RequestMarshallerKernel::SPtr _Marshaller;
        DevIoControlKernel::SPtr _DevIoCtlKernel;
        ULONGLONG _RequestId;

        KBuffer::SPtr _TransferKBuffer;
        ULONG _Retries;
        
        BOOLEAN _IsCancelled;         // TRUE if cancel called on this async
        BOOLEAN _CancelCallbackSet;   // TRUE if cancel callback should be invoked
};

class DevIoControlInprocKernel : public DevIoControlKernel
{
    K_FORCE_SHARED(DevIoControlInprocKernel);

    friend DevIoControlInprocUser;
    
    DevIoControlInprocKernel(
        __in ULONG AllocationTag
        );

    public:
        static NTSTATUS Create(
            __out DevIoControlKernel::SPtr& Result,
            __in DevIoControlInprocUser* UserAsync,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag
        );

        VOID Initialize(
            __in PUCHAR Buffer,
            __in ULONG InBufferSize,
            __in ULONG OutBufferSize
        );

        VOID CompleteRequest(
            __in NTSTATUS Status,
            __in ULONG OutBufferSize
            ) override ;

        ULONG GetInBufferSize() override 
        {
            return(_InBufferSize);
        }
        
        ULONG GetOutBufferSize() override 
        {
            return(_OutBufferSize);
        }
        
        PUCHAR GetBuffer() override 
        {
            return(_Buffer);
        }
        
    private:
        ULONG _AllocationTag;
        
        PUCHAR _Buffer;
        ULONG _InBufferSize;
        ULONG _OutBufferSize;
        
        DevIoControlInprocUser* _UserAsync;
};


DevIoControlUser::DevIoControlUser()
{
}

DevIoControlUser::~DevIoControlUser()
{
}

DevIoControlInprocUser::DevIoControlInprocUser()
{
}

DevIoControlInprocUser::DevIoControlInprocUser(
    __in ULONG AllocationTag
    ) :
   _AllocationTag(AllocationTag),
   _MaxRetries(0)
{
}

DevIoControlInprocUser::~DevIoControlInprocUser()
{
}

NTSTATUS
DevIoControlUser::Create(
    __out DevIoControlUser::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    DevIoControlInprocUser::SPtr context;
    
    context = _new(AllocationTag, Allocator) DevIoControlInprocUser(AllocationTag);
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_AllocationTag = AllocationTag;
    context->_TransferKBuffer = nullptr;
    context->_InKBuffer = nullptr;
    context->_OutKBuffer = nullptr;
    context->_Retries = 0;
    context->_IsCancelled = FALSE;
    context->_CancelCallbackSet = FALSE;
    context->_RequestId = (ULONGLONG)-1;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
    
}

VOID
DevIoControlInprocUser::StartDeviceIoControl(
    __in OpenCloseDriver::SPtr& OpenCloseHandle,
    __in ULONG Ioctl,
    __in KBuffer::SPtr InKBuffer,
    __in ULONG InBufferSize,
    __in_opt KBuffer::SPtr OutKBuffer,
    __in ULONG OutBufferSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _OpenCloseHandle = down_cast<OpenCloseUser, OpenCloseDriver>(OpenCloseHandle);
    _Ioctl = Ioctl;

    KAssert(InKBuffer->QuerySize() >= InBufferSize);
    KAssert((OutKBuffer == nullptr) || ( OutKBuffer->QuerySize() >= OutBufferSize));  
    _InBufferSize = InBufferSize;   
    _InKBuffer = InKBuffer;
    _OutBufferSize = OutBufferSize;
    _OutKBuffer = OutKBuffer;
    
    Start(ParentAsyncContext, CallbackPtr);
    
    Execute();
}

VOID
DevIoControlInprocUser::OnStart(
    )
{
}

VOID
DevIoControlInprocUser::Execute(
    )

{
    NTSTATUS status = STATUS_SUCCESS;
    RequestMarshaller::REQUEST_BUFFER* r;

    r = (RequestMarshaller::REQUEST_BUFFER*)_InKBuffer->GetBuffer();
    _RequestId = r->RequestId;
    
    KFinally([&]() 
    {
        if (status != STATUS_PENDING)
        {
            Complete(status);
        }
        
        //
        // If returned pending then async gets completed by the devIoCtlKernel
        // CompleteRequest()
        //
    });
    
    //
    // Check for an invalid handle. It may be that the handle
    // has been closed already
    //
    if (_OpenCloseHandle->GetHandle() == NULL)
    {
        status = STATUS_INVALID_HANDLE;
        KTraceFailedAsyncRequest(status, this, GetRequestId(), 0);
        return;
    }

    status = RequestMarshallerKernel::Create(_Marshaller,
                                             GetThisAllocator(),
                                             _AllocationTag);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, GetRequestId(), 0);
        return;
    }

    status = DevIoControlInprocKernel::Create(_DevIoCtlKernel,
                                              this,
                                              GetThisAllocator(),
                                              _AllocationTag);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, GetRequestId(), 0);
        return;
    }

    //
    // Emulate buffered IO by creating a buffer and using this
    // buffer for the call to the "kernel" side
    //
    status = KBuffer::Create(MAX(_InBufferSize, _OutBufferSize),
                             _TransferKBuffer,
                             GetThisAllocator(),
                             _AllocationTag);
    if (! NT_SUCCESS(status))
    {
        return;
    }
    KMemCpySafe(GetTransferBuffer(), GetTransferBufferSize(), _InKBuffer->GetBuffer(), _InBufferSize);

    //
    // User mode is invoking a device io control and so we
    // package this up and pass off to the kernel mode
    // environment
    //
    FileObjectTable::SPtr fOT;
    BOOLEAN completeRequest;

    //
    // Reconstitute the pointer to the FOT that is associated
    // with this handle
    //
    fOT = reinterpret_cast<FileObjectTable*>(_OpenCloseHandle->GetHandle());

    _DevIoCtlKernel->Initialize(GetTransferBuffer(),
                                _InBufferSize,
                                _OutBufferSize);

    status = _Marshaller->MarshallRequest((PVOID)this,
                                          fOT,
                                          _DevIoCtlKernel,
                                          &completeRequest,
                                          &_OutBufferSize);

    if (completeRequest)
    {
        KTraceFailedAsyncRequest(status, this, GetRequestId(), 0);
        _DevIoCtlKernel->CompleteRequest(status, _OutBufferSize);
        return;
    }

    status = _Marshaller->PerformRequest(fOT,
                                        _DevIoCtlKernel);

    if (status != STATUS_PENDING)
    {
        KTraceFailedAsyncRequest(status, this, GetRequestId(), 0);
        _DevIoCtlKernel->CompleteRequest(status, 0);
        return;
    }

    RequestMarkCancelableEx();
}


VOID
DevIoControlInprocUser::OnReuse(
    )
{
    _TransferKBuffer = nullptr;
    _OutKBuffer = nullptr;
    _InKBuffer = nullptr;
    _Retries = 0;
    _DevIoCtlKernel = nullptr;
    _Marshaller = nullptr;
    _IsCancelled = FALSE;
    _CancelCallbackSet = FALSE;
    _RequestId = (ULONGLONG)-1;
}

VOID
DevIoControlInprocUser::OnCancel(
    )
{
    K_LOCK_BLOCK(_CancelLock)
    {
        //
        // Remember that cancel has been called
        //
        _IsCancelled = TRUE;
        if (_CancelCallbackSet)
        {
            //
            // Invoke the cancel callback if the kernel side has set a
            // callback
            //
            _Marshaller->PerformCancel();
        }
    }
}

NTSTATUS
DevIoControlInprocUser::RequestMarkCancelableEx(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    K_LOCK_BLOCK(_CancelLock)
    {
        if (_IsCancelled)
        {
            //
            // if cancel was already called then return an error so
            // that the caller can know its request was cancelled and
            // that it needs to complete with a cancelled status
            //
            return(STATUS_CANCELLED);
        }

        //
        // Set flag that cancel callback is set. If async is cancelled
        // the cancel callback is going to be invoked.
        //
        _CancelCallbackSet = TRUE;
    }
    
    return(status);
}

NTSTATUS
DevIoControlInprocUser::RequestUnmarkCancelable(
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    
    K_LOCK_BLOCK(_CancelLock)
    {        
        //
        // Reset flag that cancel callback is set. If async is cancelled
        // the cancel callback is not going to be invoked.
        //
        _CancelCallbackSet = FALSE;
    }

    return(status);
}

//
// Kernel side of IoControl
//

DevIoControlKernel::DevIoControlKernel()
{
}

DevIoControlKernel::~DevIoControlKernel()
{
}

DevIoControlInprocKernel::DevIoControlInprocKernel()
{
}

DevIoControlInprocKernel::DevIoControlInprocKernel(
    __in ULONG AllocationTag
    ) :
   _AllocationTag(AllocationTag)
{
}

DevIoControlInprocKernel::~DevIoControlInprocKernel()
{
}

NTSTATUS
DevIoControlInprocKernel::Create(
    __out DevIoControlKernel::SPtr& Context,
    __in DevIoControlInprocUser* UserAsync,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    DevIoControlInprocKernel::SPtr context;
    
    context = _new(AllocationTag, Allocator) DevIoControlInprocKernel(AllocationTag);
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_AllocationTag = AllocationTag;
    context->_UserAsync = UserAsync;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

VOID
DevIoControlInprocKernel::Initialize(
    __in PUCHAR Buffer,
    __in ULONG InBufferSize,
    __in ULONG OutBufferSize
)
{
    _Buffer = Buffer;
    _InBufferSize = InBufferSize;
    _OutBufferSize = OutBufferSize;
}

VOID
DevIoControlInprocKernel::CompleteRequest(
    __in NTSTATUS Status,
    __in ULONG OutBufferSize
    )
{
    NTSTATUS finalStatus;
    RequestMarshaller::REQUEST_BUFFER* requestBuffer;

    _UserAsync->RequestUnmarkCancelable();
    
    if (NT_SUCCESS(Status))
    {
        requestBuffer = (RequestMarshaller::REQUEST_BUFFER*)_UserAsync->GetTransferBuffer();
        finalStatus = requestBuffer->FinalStatus;

        if (finalStatus == STATUS_BUFFER_TOO_SMALL)
        {
            //
            // Retry the request with a larger outbuffer size
            //
            ULONG outSizeNeeded = requestBuffer->SizeNeeded + 0x10000;

            Status = _UserAsync->_OutKBuffer->SetSize(outSizeNeeded,
                                                      TRUE);         // Preserve contents
            if (NT_SUCCESS(Status))
            {
                KDbgCheckpointWData(0, "Retry IOCTL request", STATUS_SUCCESS, 0, (ULONGLONG)this, outSizeNeeded, 0);
                _UserAsync->_OutBufferSize = outSizeNeeded;

                if (_UserAsync->_Retries++ >= _UserAsync->_MaxRetries)
                {
                    KAssert(FALSE);
                    Status = STATUS_BUFFER_TOO_SMALL;
                } else {                    
                    _UserAsync->_TransferKBuffer = nullptr;
                    _UserAsync->_Marshaller = nullptr;
                    _UserAsync->_DevIoCtlKernel = nullptr;
                    _UserAsync->Execute();
                    return;
                }
            }
            
        } else {
            Status = finalStatus;
            _UserAsync->_OutBufferSize = OutBufferSize;

            if ((NT_SUCCESS(Status)) && (_UserAsync->_OutKBuffer))
            {
                KMemCpySafe(
                    _UserAsync->_OutKBuffer->GetBuffer(), 
                    _UserAsync->_OutKBuffer->QuerySize(), 
                    _UserAsync->GetTransferBuffer(), 
                    OutBufferSize);
            }
        }
    }
    
    _UserAsync->_OutKBuffer = nullptr;
    _UserAsync->_InKBuffer = nullptr;
    _UserAsync->_TransferKBuffer = nullptr;    
    _UserAsync->_Marshaller = nullptr;

    AddRef();
    _UserAsync->AddRef();
    _UserAsync->Complete(Status);
    
    _UserAsync->_DevIoCtlKernel = nullptr;
    _UserAsync->Release();
    Release();
}

OpenCloseDriver::OpenCloseDriver()
{
}

OpenCloseDriver::~OpenCloseDriver()
{
}

OpenCloseUser::OpenCloseUser()
{
}

OpenCloseUser::~OpenCloseUser()
{
}

NTSTATUS
OpenCloseDriver::Create(
    __out OpenCloseDriver::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
)
{
    NTSTATUS status;
    OpenCloseUser::SPtr context;
    
    context = _new(AllocationTag, Allocator) OpenCloseUser();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, nullptr, AllocationTag, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_AllocationTag = AllocationTag;
    context->_FOT = nullptr;
    context->_Mode = OpenCloseUser::NotDefined;
                                    
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);    
}

VOID
OpenCloseUser::StartOpenDevice(
    __in const KStringView& FileName,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    UNREFERENCED_PARAMETER(FileName);
    
    KAssert(_Mode == NotDefined);
    
    _Mode = OpenDevice; 
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
OpenCloseUser::StartWaitForCloseDevice(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    KAssert(_Mode == NotDefined);
    KAssert(_FOT);
    
    _Mode = WaitForCloseDevice;
    Start(ParentAsyncContext, CallbackPtr);
}           

VOID
OpenCloseUser::OnStart(
    )
{
    if (_Mode != WaitForCloseDevice)
    {
        //
        // Since CreateFile, DeviceIoControl and CloseHandle are potentialy
        // blocking operations, we queue this to a worker thread. The worker thread is invoked at
        // DevIoControlInprocUser::Execute. Take a ref count here to
        // account for the work item.
        //
        AddRef();
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
    }
}

VOID
OpenCloseUser::Execute(
    )
{
    NTSTATUS status;

    KAssert( (_Mode == OpenDevice) ||
             (_Mode == WaitForCloseDevice) ||
             (_Mode == CloseDevice) );
    
    switch(_Mode)
    {
        case OpenDevice:
        {
            //
            // Open device will instantiate the kernel mode
            // environment, in this case create the file object table
            // associated with this "handle"
            //
            status = FileObjectTable::Create(GetThisAllocator(),
                                             _AllocationTag,
                                             _FOT);
            if (NT_SUCCESS(status))
            {
                //
                // Take refcount on the FOT, it will be released when
                // the "handle" is closed. 
                //
                _FOT->AddRef();
            } else {
                KTraceFailedAsyncRequest(status, this, 0, 0);
            }
            
            break;
        }

        case WaitForCloseDevice:
        {
            //
            // Async doesn't get completed until SignalCloseDevice()
            // is called and we are kicked again
            //
            KInvariant(FALSE);
            return;
        }
        
        case CloseDevice:
        {            
            RequestMarshallerKernel::Cleanup(*_FOT, GetThisAllocator());

            status = STATUS_SUCCESS;
            
            break;
        }

        default:
        {
            KAssert(FALSE);
            status = STATUS_UNSUCCESSFUL;
        }
    }

    //
    // Release ref taken in OnStart() to account for the work item
    //
    Release();
    Complete(status);
}

VOID
OpenCloseUser::OnReuse(
    )
{
    _Mode = NotDefined;
}

VOID
OpenCloseUser::SignalCloseDevice(
    )
{
    //
    // Since CreateFile and CloseHandle are potentialy
    // blocking operations, we queue this to a worker thread. The worker thread is invoked at
    // OpenCloseUser::Execute. Take ref count here to account for the
    // work item.
    //
    KAssert(_FOT);
    
    _Mode = CloseDevice;
    AddRef();
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}


VOID
OpenCloseUser::PostCloseCompletion(
    )
{           
}

PVOID OpenCloseUser::GetKernelPointerFromObjectId(
    __in ULONGLONG ObjectId
    )
{
    PVOID p;    

    p = _FOT->LookupObjectPointer(ObjectId);

	return(p);
}
