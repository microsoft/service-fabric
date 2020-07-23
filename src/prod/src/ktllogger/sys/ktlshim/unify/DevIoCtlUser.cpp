// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "KtlLogShimUser.h"

class DevIoControlUmKm;

class OpenCloseUmKm : public OpenCloseDriver, public KThreadPool::WorkItem
{
    K_FORCE_SHARED(OpenCloseUmKm);

    friend DevIoControlUmKm;
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
            return(NULL);
        }
        
        
    protected:
        VOID
        OnStart(
            );

        VOID
        OnReuse(
            );

        VOID
        Execute(
            );

    private:
        VOID TraceTokenInformation();
                
        NTSTATUS DevIoCtlStarted(
        )
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            
            if (TryAcquireActivities())
            {
                status = STATUS_SUCCESS;
            }
            
            return(status);
        }
        
        VOID DevIoCtlCompleted(
        )
        {
            ReleaseActivities();
        }

        HANDLE GetHandle()
        {
            return(_Handle);
        }
        
        static VOID RawIoCompletion(
            __in_opt VOID* Context,
            __inout OVERLAPPED* Overlapped,
            __in ULONG Error,
            __in ULONG BytesTransferred
            );
                
    private:
        //
        // General members
        //

        //
        // Parameters to api
        //
        KStringView _FileName;
        
        //
        // Members needed for functionality
        //
        HANDLE _Handle;
        PVOID _IoRegistrationContext;

        enum { NotDefined, OpenDevice, WaitForCloseDevice, CloseDevice } _Mode;
        
};

class DevIoControlUmKm : public DevIoControlUser
{
    K_FORCE_SHARED(DevIoControlUmKm);

    friend DevIoControlUser;
    friend OpenCloseUmKm;
    
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

        VOID SetRetryCount(
            __in ULONG MaxBufferTooSmallRetries
        ) override
        {
            _MaxBufferTooSmallRetries = MaxBufferTooSmallRetries;
            _BufferTooSmallRetries = 0;
        }
                
        ULONG GetOutBufferSize() override
        {
            return(_OutBufferSize);
        }

    protected:
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
        OnCompleted(
            ) override ;

        VOID
        Execute(
            );


    private:
        VOID WaitOOMRetryCompletion(
            __in_opt KAsyncContextBase* const ParentAsync,
            __in KAsyncContextBase& Async
            );
        
        VOID
        DoComplete(
            NTSTATUS Status
            );
        
        VOID IoCompletion(
            __in ULONG Error,
            __in ULONG BytesTransferred
            );
        
        struct Overlapped : OVERLAPPED
        {
            Overlapped()
            {
                _Parent = NULL;
            }

            DevIoControlUmKm* _Parent;
        };

        ULONGLONG GetRequestId()
        {
            return(_RequestId);
        }
        
    private:
        //
        // General members
        //

        //
        // Parameters to api
        //
        OpenCloseUmKm::SPtr _OpenCloseHandle;
        ULONG _Ioctl;
        KBuffer::SPtr _InKBuffer;
        ULONG _InBufferSize;
        KBuffer::SPtr _OutKBuffer;
        ULONG _OutBufferSize;
        ULONG _MaxBufferTooSmallRetries;

        //
        // Members needed for functionality
        //
        Overlapped _Overlapped;                
        ULONG _BufferTooSmallRetries;
        ULONGLONG _RequestId;
        KTimer::SPtr _OOMRetryTimer;
        ULONG _OOMRetryTimerWaitInMs;
};

DevIoControlUser::DevIoControlUser()
{
}

DevIoControlUser::~DevIoControlUser()
{
}

DevIoControlUmKm::DevIoControlUmKm()
{
}

DevIoControlUmKm::~DevIoControlUmKm()
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
    DevIoControlUmKm::SPtr context;
    
    context = _new(AllocationTag, Allocator) DevIoControlUmKm();
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

    status = KTimer::Create(context->_OOMRetryTimer, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {                   
        return(status);
    }   
    
    RtlZeroMemory(&context->_Overlapped, sizeof(OVERLAPPED));
    context->_Overlapped._Parent = context.RawPtr();

    context->_KBuffer = nullptr;
    context->_InKBuffer = nullptr;
    context->_OutKBuffer = nullptr;
    context->_BufferTooSmallRetries = 0;
    context->_OOMRetryTimerWaitInMs = 1;
    context->_OpenCloseHandle = nullptr;
    context->_RequestId = (ULONGLONG)-1;
    context->_MaxBufferTooSmallRetries = 0;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
    
}

VOID
DevIoControlUmKm::OnCompleted(
    )
{
    _OpenCloseHandle = nullptr; 
}


VOID
DevIoControlUmKm::DoComplete(
    NTSTATUS Status
    )
{
    _OpenCloseHandle->DevIoCtlCompleted();
    Complete(Status);    
}

VOID
DevIoControlUmKm::StartDeviceIoControl(
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
    _Ioctl = Ioctl;

    KAssert(InKBuffer->QuerySize() >= InBufferSize);
    KAssert((OutKBuffer == nullptr) || ( OutKBuffer->QuerySize() >= OutBufferSize));
    
    _OpenCloseHandle = down_cast<OpenCloseUmKm, OpenCloseDriver>(OpenCloseHandle);
    _InBufferSize = InBufferSize;   
    _InKBuffer = InKBuffer;
    _OutBufferSize = OutBufferSize;
    _OutKBuffer = OutKBuffer;

    //
    // Kick off the async that will be used when the request completes
    // from user mode
    //
    Start(ParentAsyncContext, CallbackPtr);

    //
    // Call down to kernel mode to get the request started
    //
    NTSTATUS status;
    status = _OpenCloseHandle->DevIoCtlStarted();
    
    if (NT_SUCCESS(status))
    {
        //
        // Extract requestId from the buffer
        //
        RequestMarshaller::REQUEST_BUFFER* r;

        r = (RequestMarshaller::REQUEST_BUFFER*)_InKBuffer->GetBuffer();
        _RequestId = r->RequestId;
        
        //
        // We expect that the device io control will not block and so
        // dispatch driectly
        //
        ULONGLONG ticksBefore = GetTickCount64();
#ifdef VERBOSE
        KDbgCheckpointWData(0, "ms TicksBefore DeviceIoControl", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, ticksBefore, 0);
#endif
        Execute();
        ULONGLONG ticksAfter = GetTickCount64();
#ifdef VERBOSE
        KDbgCheckpointWData(0, "ms TicksAfter DeviceIoControl", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, ticksAfter, 0);
#endif

        //
        // Ensure that operation takes less than this number of
        // milliseconds
        //
        if ((ticksAfter - ticksBefore) > 3000)
        {
            KDbgCheckpointWData(0, "ms for DeviceIoControl", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, (ticksAfter - ticksBefore), 0);
        }
    } else {
        //
        // Handle is shutting down
        //
        Complete(status);
    }
    
}

VOID
DevIoControlUmKm::OnStart(
    )
{
}

VOID
DevIoControlUmKm::WaitOOMRetryCompletion(
    __in_opt KAsyncContextBase* const ,
    __in KAsyncContextBase& 
    )
{
    KDbgCheckpointWDataInformational(0, "WaitOOMRetry IOCTL request", STATUS_SUCCESS,
                                     GetRequestId(), (ULONGLONG)this, _OOMRetryTimerWaitInMs, _BufferTooSmallRetries);
    Execute();
}


VOID
DevIoControlUmKm::IoCompletion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    NTSTATUS status;
    BOOLEAN retryWithTimer = FALSE;
    ULONG outBufferSize = _OutBufferSize;

    _OutBufferSize = BytesTransferred;
    
    if (Error == ERROR_SUCCESS)
    {
        if (_OutBufferSize >= FIELD_OFFSET(RequestMarshaller::REQUEST_BUFFER, Data))
        {
            RequestMarshaller::REQUEST_BUFFER* requestBuffer = (RequestMarshaller::REQUEST_BUFFER*)_OutKBuffer->GetBuffer();
            status = requestBuffer->FinalStatus;

            if (status == STATUS_WORKING_SET_QUOTA)
            {
                //
                // This error indicates that the pages could not be
                // locked on transition into kernel mode. Rather than
                // propogate the error back up to the caller, we
                // throttle here to give time for the kernel mode side
                // to complete operations
                //
                retryWithTimer = TRUE;
            } else if (status == STATUS_BUFFER_TOO_SMALL) {
                //
                // Retry the request with a larger outbuffer size
                //
                ULONG outSizeNeeded = requestBuffer->SizeNeeded + 0x10000;

                status = _OutKBuffer->SetSize(outSizeNeeded,
                                              FALSE);         // Do not preserve contents
                if (NT_SUCCESS(status))
                {
                    KDbgCheckpointWData(0, "BufferTooSmall Retry IOCTL request", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, outSizeNeeded, _BufferTooSmallRetries);
                    _OutBufferSize = outSizeNeeded;
                    if (_BufferTooSmallRetries++ >= _MaxBufferTooSmallRetries)
                    {
                        status = STATUS_BUFFER_TOO_SMALL;
                        KTraceFailedAsyncRequest(status, this, outSizeNeeded, _BufferTooSmallRetries);
                        KAssert(FALSE);
                    } else {                    
                        Execute();
                        return;
                    }                   
                } else {
                    KTraceFailedAsyncRequest(status, this, outSizeNeeded, _BufferTooSmallRetries);
                }
            }           
        } else {
            status = STATUS_SUCCESS;
        }
    } else {
        if (Error == ERROR_NO_SYSTEM_RESOURCES)
        {
            retryWithTimer = TRUE;
        }
        
        status = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(status, this, BytesTransferred, Error);
    }

    if (retryWithTimer)
    {
        KAsyncContextBase::CompletionCallback completion(this, &DevIoControlUmKm::WaitOOMRetryCompletion);

        _OutBufferSize = outBufferSize;
        _OOMRetryTimer->Reuse();
        _OOMRetryTimer->StartTimer(_OOMRetryTimerWaitInMs,
                                   this,
                                   completion);
        _OOMRetryTimerWaitInMs = _OOMRetryTimerWaitInMs * 2;
        return;
    }
    
    DoComplete(status);
}

VOID
DevIoControlUmKm::Execute(
    )

{
    NTSTATUS status;
    ULONG error;
    BOOL requestSent;

    //
    // Reference this object while we are running in this work
    // item. It is possible that the completion could happen as
    // a result of the io completion port and that would happen
    // in another thread before the DeviceIoControl returns. In 
    // the case of the delete object, which is a fire and forget 
    // async, the async (ie, the this pointer) will be freed on
    // that completion and thus disappear underneath this
    // routine
    //
    AddRef();
    KFinally([&]() { Release(); });

    requestSent = DeviceIoControl(_OpenCloseHandle->GetHandle(),
                                  _Ioctl,
                                  _InKBuffer->GetBuffer(),
                                  _InBufferSize,
                                  _OutKBuffer ? _OutKBuffer->GetBuffer() : NULL,
                                  _OutBufferSize,
                                  NULL,
                                  &_Overlapped);

    if (! requestSent)
    {
        error = GetLastError();
        if (error != ERROR_IO_PENDING)
        {
            status = STATUS_UNSUCCESSFUL;
            KTraceFailedAsyncRequest(status, this, error, 0);
            DoComplete(status);
            return;
        }
    }
    //
    // Continued in RawIoCompletion
    //
}

VOID
DevIoControlUmKm::OnReuse(
    )
{
    _KBuffer = nullptr;
    _InKBuffer = nullptr;
    _OutKBuffer = nullptr;
    _OpenCloseHandle = nullptr;
    _BufferTooSmallRetries = 0;
    _OOMRetryTimerWaitInMs = 1;
    _RequestId = (ULONGLONG)-1;
    RtlZeroMemory(&_Overlapped, sizeof(OVERLAPPED));
}

VOID
DevIoControlUmKm::OnCancel(
    )
{
    BOOL b;
    OpenCloseUmKm::SPtr openClose;
        
    openClose = _OpenCloseHandle;

    if (openClose)
    {
        ULONGLONG ticksBefore = GetTickCount64();
#ifdef VERBOSE
        KDbgCheckpointWData(0, "ms TicksBefore Cancel", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, ticksBefore, 0);
#endif

        b = CancelIoEx(openClose->GetHandle(),
                       &_Overlapped);

        ULONGLONG ticksAfter = GetTickCount64();
        
#ifdef VERBOSE
        KDbgCheckpointWData(0, "ms TicksAfter Cancel", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, ticksAfter, 0);
#endif

        //
        // Ensure that operation takes less than this number of
        // milliseconds
        //
        if ((ticksAfter - ticksBefore) > 3000)
        {
            KDbgCheckpointWData(0, "Number ms for Cancel", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, (ticksAfter - ticksBefore), 0);

            // TODO: Remove this KInvariant before release
            KInvariant(FALSE);
        }

        if (! b)
        {
            ULONG error = GetLastError();
            KTraceFailedAsyncRequest(error, this, GetRequestId(), 0);
        }
    } else {
        //
        // If _OpenCloseHandle is not set then the async has already
        // been completed. Nothing to do
        //
        KDbgCheckpointWData(0, "Skip work on Cancel", STATUS_SUCCESS, GetRequestId(), (ULONGLONG)this, 0, 0);
    }   
}

OpenCloseDriver::OpenCloseDriver()
{
}

OpenCloseDriver::~OpenCloseDriver()
{
}

OpenCloseUmKm::OpenCloseUmKm()
{
}

OpenCloseUmKm::~OpenCloseUmKm()
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
    OpenCloseUmKm::SPtr context;
    
    context = _new(AllocationTag, Allocator) OpenCloseUmKm();
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

    context->_Mode = OpenCloseUmKm::NotDefined;
    context->_IoRegistrationContext = NULL;
    context->_Handle = INVALID_HANDLE_VALUE;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS);
    
}

VOID
OpenCloseUmKm::StartOpenDevice(
    __in const KStringView& FileName,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    KAssert(_Mode == NotDefined);

    _FileName = FileName;    
    _Mode = OpenDevice;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
OpenCloseUmKm::StartWaitForCloseDevice(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    KAssert(_Mode == NotDefined);
    KAssert(_Handle != INVALID_HANDLE_VALUE);
    
    _Mode = WaitForCloseDevice;
    Start(ParentAsyncContext, CallbackPtr);
}           

VOID
OpenCloseUmKm::OnStart(
    )
{
    if ((_Mode == OpenDevice) || (_Mode == CloseDevice))
    {
        //
        // Since CreateFile and CloseHandle are potentialy
        // blocking operations, we queue this to a worker thread. The worker thread is invoked at
        // OpenCloseUmKm::Execute.
        //
        // Refcount is taken here and released at the end of Execute()
        //
        AddRef();
        GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
    }
}

VOID
OpenCloseUmKm::OnReuse(
    )
{
    _Mode = NotDefined;
}

VOID
OpenCloseUmKm::RawIoCompletion(
    __in_opt VOID* Context,
    __inout OVERLAPPED* Overlap,
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    UNREFERENCED_PARAMETER(Context);

    DevIoControlUmKm::Overlapped* overlapped = (DevIoControlUmKm::Overlapped*)Overlap;
    
    KAssert(overlapped->_Parent != NULL);
    overlapped->_Parent->IoCompletion(Error,
                                      BytesTransferred);
}

VOID OpenCloseUmKm::TraceTokenInformation()
{
    NTSTATUS status = STATUS_SUCCESS;
    HANDLE token;
    BOOL b;
    ULONG error;

    b = OpenProcessToken(GetCurrentProcess(), // ProcessHandle
                         TOKEN_ALL_ACCESS,
                         &token);
    if (! b)
    {
        error = GetLastError();
        KTraceFailedAsyncRequest(status, this, error, 0);
        return;
    }
    KFinally([&]() { CloseHandle(token); });

    TOKEN_INFORMATION_CLASS tokenInformationClass;
    PUCHAR tokenInformationBuffer;
    ULONG lengthNeeded;
    PTOKEN_GROUPS tokenGroups;
    PWCHAR name;
    PWCHAR domainName;
    ULONG nameLengthInChar = MAX_PATH;
    ULONG domainNameLengthInChar = MAX_PATH;
    SID_NAME_USE sidNameUse;

    tokenInformationClass = TokenGroups;

    b = GetTokenInformation(token,
                            tokenInformationClass,
                            NULL,      // TokenInformation
                            0,         // TokenInformationLength
                            &lengthNeeded);

    if (! b)
    {
        error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER)
        {
            KTraceFailedAsyncRequest(status, this, error, 0);
            return;
        }
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return;
    }

    tokenInformationBuffer = (PUCHAR)_new(GetThisAllocationTag(), GetThisAllocator()) UCHAR[lengthNeeded];
    if (tokenInformationBuffer == nullptr)
    {
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return;     
    }
    KFinally([&]() { _delete(tokenInformationBuffer); tokenInformationBuffer = NULL; });

    b = GetTokenInformation(token,
                            tokenInformationClass,
                            tokenInformationBuffer,      // TokenInformation
                            lengthNeeded,               // TokenInformationLength
                            &lengthNeeded);

    if (! b)
    {
        error = GetLastError();
        KTraceFailedAsyncRequest(status, this, error, lengthNeeded);
        return;
    }

    tokenGroups = (PTOKEN_GROUPS)tokenInformationBuffer;
    
    KTraceFailedAsyncRequest(status, this, tokenGroups->GroupCount, 0);

    name = (PWCHAR) _new(GetThisAllocationTag(), GetThisAllocator()) WCHAR[nameLengthInChar];   
    if (name == nullptr)
    {
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return;     
    }
    KFinally([&]() { _delete(name); name = NULL; });    
    
    domainName = (PWCHAR) _new(GetThisAllocationTag(), GetThisAllocator()) WCHAR[domainNameLengthInChar];   
    if (domainName == nullptr)
    {
        KTraceOutOfMemory( 0, status, nullptr, GetThisAllocationTag(), 0);
        return;     
    }
    KFinally([&]() { _delete(domainName); domainName = NULL; });    
    
    for (ULONG i = 0; i < tokenGroups->GroupCount; i++)
    {
        ULONG namelen = nameLengthInChar;
        ULONG domainlen = domainNameLengthInChar;
        b = LookupAccountSid(NULL,
                             tokenGroups->Groups[i].Sid,
                             name,
                             &namelen,
                             domainName,
                             &domainlen,
                             &sidNameUse);
        if (! b)
        {
            error = GetLastError();
            KTraceFailedAsyncRequest(status, this, error, 0);
            return;
        }
        
        KDbgPrintfInformational("Group %d - %ws\\%ws", i, domainName, name);
    }
}

VOID
OpenCloseUmKm::Execute(
    )

{
    NTSTATUS status;
    ULONG error;

    KAssert( (_Mode == OpenDevice) ||
             (_Mode == CloseDevice) );
    
    switch(_Mode)
    {
        case OpenDevice:
        {
            _Handle = CreateFile(_FileName,
                                     GENERIC_READ | GENERIC_WRITE,
                                     0,
                                    NULL,          // SecurityAttributes
                                    OPEN_ALWAYS,
                                    FILE_FLAG_OVERLAPPED,
                                    NULL);         // Template

            if (_Handle == INVALID_HANDLE_VALUE)
            {
                error = GetLastError();
                status = STATUS_NO_SUCH_DEVICE;
                KTraceFailedAsyncRequest(status, this, error, 0);
                if (error == ERROR_ACCESS_DENIED)
                {
                    //
                    // See TFS 4655200. Trace out additional
                    // information about the process to see why this
                    // failed
                    //
                    TraceTokenInformation();
                }
            } else {
                //
                // Register for IoCompletion on this handle
                //
                KAssert(_IoRegistrationContext == NULL);
                status = GetThisKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
                            _Handle,
                            RawIoCompletion,
                            nullptr,
                            &_IoRegistrationContext
                            );              

                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, (ULONGLONG)_Handle, 0);
                    CloseHandle(_Handle);
                    _Handle = INVALID_HANDLE_VALUE;
                }               
            }
            break;
        }

        case CloseDevice:
        {
            BOOL b;

            b = CloseHandle(_Handle);
            _Handle = INVALID_HANDLE_VALUE;

            if (! b)
            {
                error = GetLastError();
                status = STATUS_UNSUCCESSFUL;
                KTraceFailedAsyncRequest(status, this, error, 0);
            } else {
                status = STATUS_SUCCESS;
            }

            break;
        }

        default:
        {
            KAssert(FALSE);
            status = STATUS_UNSUCCESSFUL;
        }
    }
    
    Complete(status);
    
    //
    // Refcount is taken in OnStart
    //
    Release();  
}

VOID
OpenCloseUmKm::SignalCloseDevice(
    )
{
    //
    // Since CreateFile and CloseHandle are potentialy
    // blocking operations, we queue this to a worker thread. The worker thread is invoked at
    // OpenCloseUmKm::Execute
    //
    KAssert(_Handle != INVALID_HANDLE_VALUE);
    
    _Mode = CloseDevice;
    // Refcount is taken here and released at the end of Execute()
    //
    AddRef();
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}


VOID
OpenCloseUmKm::PostCloseCompletion(
    )
{           
    KAssert(_IoRegistrationContext);

    GetThisKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(
                _IoRegistrationContext
                );
    _IoRegistrationContext = NULL;
}
