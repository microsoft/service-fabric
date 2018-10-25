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
// KtlLogContainer Implementation
//
//************************************************************************************


KtlLogContainerUser::KtlLogContainerUser(
    __in KtlLogContainerId LogContainerId,
    __in OpenCloseDriver& DeviceHandle
    )
{
    NTSTATUS status;
    
    SetObjectId(0);
    
    //
    // Allocate resources needed
    //
    status = DevIoControlUser::Create(_DevIoCtl,
                                      GetThisAllocator(),
                                      GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = RequestMarshallerUser::Create(_Marshaller,
                                           GetThisAllocator(),
                                           GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _IsClosed = FALSE;
    _DeviceHandle = &DeviceHandle;
    _LogContainerId = LogContainerId;
}

#ifdef _WIN64
VOID
KtlLogContainerUser::DeleteObjectCompletion(
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

KtlLogContainerUser::~KtlLogContainerUser()
{
    NTSTATUS status;
    ULONGLONG objectId;

    if (! _IsClosed)
    {
        objectId = GetObjectId();
        if (objectId == 0)
        {
            return;
        }

        status = _Marshaller->InitializeForDeleteObject(RequestMarshaller::LogContainer,
                                                        GetObjectId());
        if (! NT_SUCCESS(status))
        {
            return;             
        }

        _DevIoCtl->Reuse();
        
#ifdef _WIN64
        KAsyncContextBase::CompletionCallback completion(&KtlLogContainerUser::DeleteObjectCompletion);
#endif
        _DevIoCtl->StartDeviceIoControl(_DeviceHandle,
                                        RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                        _Marshaller->GetInKBuffer(),
                                        _Marshaller->GetWriteSize(),
                                        nullptr,
                                        0,
                                        nullptr,    // ParentAsync
#ifdef _WIN64
                                        completion);
#else
                                        nullptr);
#endif
    }
}

//
// IsFunctional()
//
BOOLEAN KtlLogContainerUser::IsFunctional(
    )
{    
    // TODO: Implement this
    return(TRUE);
}


//
// Create Log Stream
//
VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::OnCompleted(
    )
{
    _SecurityDescriptor = nullptr;
    _Path = nullptr;
    _LogStreamAlias = nullptr;
}

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::LogStreamCreated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONGLONG objectId;
    ULONG coreLoggerMetadataOverhead;

    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadObject(RequestMarshaller::LogStream,
                                         objectId);
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadData<ULONG>(coreLoggerMetadataOverhead);

        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        KStringView prefix(L"UserWrite");
        status = _LogStream->_InstrumentedComponent->SetComponentName(prefix, _LogStreamId.Get());
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
		
        _LogStream->SetObjectId(objectId);
        _LogStream->_LogStreamId = _LogStreamId;
        
        _LogStream->_InstrumentedComponent->SetReportFrequency(KInstrumentedComponent::DefaultReportFrequency);
        
        _LogStream->_CoreLoggerMetadataOverhead = coreLoggerMetadataOverhead;
        
        (*_ResultLogStreamPtr) = Ktl::Move(reinterpret_cast<KtlLogStream::SPtr&>(_LogStream));
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::OnStart(
    )
{
    NTSTATUS status;
    GUID guidNull = { 0 };

    //
    // Validate log stream id is not null
    //
    if (IsEqualGUID(_LogStreamId.Get(),guidNull))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Validate the log stream type
    //
    if ((_LogStreamType != KtlLogInformation::KtlLogDefaultStreamType()) &&
        (_LogStreamType != KLogicalLogInformation::GetLogicalLogStreamType()))
    {
        status = STATUS_INVALID_PARAMETER;
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
            Complete(status);
            return;
        }
    }

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    _LogStream = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogStreamUser((_LogContainer->_DeviceHandle));
    if (_LogStream == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        Complete(status);
        return;
    }
    
    status = _LogStream->Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     CreateLogStream,  LogStreamId,  
    //                                                             MaximumMetaDataLength, MaximumStreamSize, MaximumRecordSize
    //                                                             LogStreamAlias, Path, SecurityDescriptor
    //
    // <-                                      LogStreamObject, CoreLoggerMetadataOverhead
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::CreateLogStream);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogStreamId(_LogStreamId);
                                   
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogStreamType(_LogStreamType);
                                   
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_MaximumMetaDataLength);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_MaximumStreamSize);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_MaximumRecordSize);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_Flags);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteString(_LogStreamAlias);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteString(_Path);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKBuffer(_SecurityDescriptor);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncCreateLogStreamContextUser::LogStreamCreated);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    _LogStreamType = KtlLogInformation::KtlLogDefaultStreamType();
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;   
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = 0;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;   
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = 0;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::StartCreateLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;   
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = Flags;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncCreateLogStreamContextUser::CreateLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __in const KtlLogStreamType& LogStreamType,
    __in_opt const KString::CSPtr& LogStreamAlias,
    __in_opt const KString::CSPtr& Path,
    __in_opt KBuffer::SPtr& SecurityDescriptor,
    __in ULONG MaximumMetaDataLength,
    __in LONGLONG MaximumStreamSize,
    __in ULONG MaximumRecordSize,
    __in DWORD Flags,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext
)
{
    _LogStreamId = LogStreamId;
    _LogStreamType = LogStreamType;
    _LogStreamAlias = LogStreamAlias;
    _Path = Path;
    _SecurityDescriptor = SecurityDescriptor;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _MaximumStreamSize = MaximumStreamSize;
    _MaximumRecordSize = MaximumRecordSize;
    _Flags = Flags;
    _ResultLogStreamPtr = &LogStream;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncCreateLogStreamContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncCreateLogStreamContextUser::AsyncCreateLogStreamContextUser()
{
}

KtlLogContainerUser::AsyncCreateLogStreamContextUser::~AsyncCreateLogStreamContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncCreateLogStreamContext(
    __out AsyncCreateLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncCreateLogStreamContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCreateLogStreamContextUser();
    if (context == nullptr)
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

    context->_LogContainer = this;
    
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
// Delete log stream
//
VOID
KtlLogContainerUser::AsyncDeleteLogStreamContextUser::LogStreamDeleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncDeleteLogStreamContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     DeleteLogStream, LogStreamId
    //
    // <-                                      
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::DeleteLogStream);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogStreamId(_LogStreamId);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncDeleteLogStreamContextUser::LogStreamDeleted);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogContainerUser::AsyncDeleteLogStreamContextUser::StartDeleteLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncDeleteLogStreamContextUser::DeleteLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamId = LogStreamId;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncDeleteLogStreamContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncDeleteLogStreamContextUser::AsyncDeleteLogStreamContextUser()
{
}

KtlLogContainerUser::AsyncDeleteLogStreamContextUser::~AsyncDeleteLogStreamContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncDeleteLogStreamContext(
    __out AsyncDeleteLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncDeleteLogStreamContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncDeleteLogStreamContextUser();
    if (context == nullptr)
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
    
    context->_LogContainer = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Open log stream
//
VOID
KtlLogContainerUser::AsyncOpenLogStreamContextUser::LogStreamOpened(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONG maximumMetaDataLength = 0;
    ULONGLONG objectId;
    ULONG coreLoggerMetadataOverhead;

    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadObject(RequestMarshaller::LogStream,
                                         objectId);
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadData<ULONG>(maximumMetaDataLength);

        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadData<ULONG>(coreLoggerMetadataOverhead);

        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        KStringView prefix(L"UserWrite");
        status = _LogStream->_InstrumentedComponent->SetComponentName(prefix, _LogStreamId.Get());
        if (! NT_SUCCESS(status))
        {
            _LogStream = nullptr;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
		
        _LogStream->SetObjectId(objectId);
        
        _LogStream->_LogStreamId = _LogStreamId;

        _LogStream->_InstrumentedComponent->SetReportFrequency(KInstrumentedComponent::DefaultReportFrequency);
        
        *_MaximumMetaDataLength = maximumMetaDataLength;
        _LogStream->_CoreLoggerMetadataOverhead = coreLoggerMetadataOverhead;
        (*_ResultLogStreamPtr) = Ktl::Move(reinterpret_cast<KtlLogStream::SPtr&>(_LogStream));
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncOpenLogStreamContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    _LogStream = _new(GetThisAllocationTag(), GetThisAllocator()) KtlLogStreamUser((_LogContainer->_DeviceHandle));
    if (_LogStream == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        Complete(status);
        return;
    }
    
    status = _LogStream->Status();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }   

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     OpenLogStream, LogStreamId
    //
    // <-                                      LogStreamObject, MaximumMetaDataLength, CoreLoggerMetadataOverhead
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::OpenLogStream);
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogStreamId(_LogStreamId);
                                   
    if (! NT_SUCCESS(status))
    {
        _LogStream = nullptr;
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncOpenLogStreamContextUser::LogStreamOpened);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogContainerUser::AsyncOpenLogStreamContextUser::StartOpenLogStream(
    __in const KtlLogStreamId& LogStreamId,
    __out ULONG* MaximumMetaDataLength,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LogStreamId = LogStreamId;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _ResultLogStreamPtr = &LogStream;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncOpenLogStreamContextUser::OpenLogStreamAsync(
    __in const KtlLogStreamId& LogStreamId,
    __out ULONG* MaximumMetaDataLength,
    __out KtlLogStream::SPtr& LogStream,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamId = LogStreamId;
    _MaximumMetaDataLength = MaximumMetaDataLength;
    _ResultLogStreamPtr = &LogStream;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncOpenLogStreamContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncOpenLogStreamContextUser::AsyncOpenLogStreamContextUser()
{
}

KtlLogContainerUser::AsyncOpenLogStreamContextUser::~AsyncOpenLogStreamContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncOpenLogStreamContext(
    __out AsyncOpenLogStreamContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncOpenLogStreamContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncOpenLogStreamContextUser();
    if (context == nullptr)
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

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// assign alias
//
VOID
KtlLogContainerUser::AsyncAssignAliasContextUser::AliasAssigned(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }   

    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncAssignAliasContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     AssignAlias, LogStreamId, LogStreamAlias
    //
    // <-                                      
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::AssignAlias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKtlLogStreamId(_LogStreamId);
                                   
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncAssignAliasContextUser::AliasAssigned); 
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogContainerUser::AsyncAssignAliasContextUser::StartAssignAlias(
    __in KString::CSPtr& Alias,
    __in KtlLogStreamId LogStreamId,                             
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{    
    _LogStreamId = LogStreamId;
    _Alias = Alias;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncAssignAliasContextUser::AssignAliasAsync(
    __in KString::CSPtr& Alias,
    __in KtlLogStreamId LogStreamId,                             
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _LogStreamId = LogStreamId;
    _Alias = Alias;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncAssignAliasContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncAssignAliasContextUser::AsyncAssignAliasContextUser()
{
}

KtlLogContainerUser::AsyncAssignAliasContextUser::~AsyncAssignAliasContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncAssignAliasContext(
    __out AsyncAssignAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncAssignAliasContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncAssignAliasContextUser();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

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
            
    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Remove alias
//
VOID
KtlLogContainerUser::AsyncRemoveAliasContextUser::AliasRemoved(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }   

    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncRemoveAliasContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     RemoveAlias, LogStreamAlias
    //
    // <-                                      
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::RemoveAlias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
        
    status = _Marshaller->WriteString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncRemoveAliasContextUser::AliasRemoved);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                        
}

VOID
KtlLogContainerUser::AsyncRemoveAliasContextUser::StartRemoveAlias(
    __in KString::CSPtr& Alias,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Alias = Alias;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncRemoveAliasContextUser::RemoveAliasAsync(
    __in KString::CSPtr& Alias,                         
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _Alias = Alias;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncRemoveAliasContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncRemoveAliasContextUser::AsyncRemoveAliasContextUser()
{
}

KtlLogContainerUser::AsyncRemoveAliasContextUser::~AsyncRemoveAliasContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncRemoveAliasContext(
    __out AsyncRemoveAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncRemoveAliasContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncRemoveAliasContextUser();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

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

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Resolve alias
//
VOID
KtlLogContainerUser::AsyncResolveAliasContextUser::AliasResolved(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    _Alias = nullptr;
    
    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        status = _Marshaller->ReadKtlLogStreamId(*_LogStreamId);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }       
    } else {
        if (status == STATUS_NOT_FOUND)
        {
            KDbgCheckpointWDataInformational(0,
                                "AsyncResolveAlias", status, (ULONGLONG)this, 0, 0, 0);
        } else {
            KTraceFailedAsyncRequest(status, this, 0, 0);
        }
    }
    
    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncResolveAliasContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     ResolveAlias, Alias
    //
    // <-                                      LogStreamId
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::ResolveAlias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
        
    status = _Marshaller->WriteString(_Alias);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncResolveAliasContextUser::AliasResolved);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
    
}

VOID
KtlLogContainerUser::AsyncResolveAliasContextUser::StartResolveAlias(
    __in KString::CSPtr & Alias,
    __out KtlLogStreamId* LogStreamId,                           
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _Alias = Alias;
    _LogStreamId = LogStreamId;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncResolveAliasContextUser::ResolveAliasAsync(
    __in KString::CSPtr & Alias,
    __out KtlLogStreamId* LogStreamId,            
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{    
    _Alias = Alias;
    _LogStreamId = LogStreamId;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncResolveAliasContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncResolveAliasContextUser::AsyncResolveAliasContextUser()
{
}

KtlLogContainerUser::AsyncResolveAliasContextUser::~AsyncResolveAliasContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncResolveAliasContext(
    __out AsyncResolveAliasContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncResolveAliasContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncResolveAliasContextUser();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
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

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
//
// EnumerateStreams
//
VOID
KtlLogContainerUser::AsyncEnumerateStreamsContextUser::StreamsEnumerated(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        status = _Marshaller->ReadKtlLogStreamIdArray(*_LogStreamIdArray);
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
KtlLogContainerUser::AsyncEnumerateStreamsContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     EnumerateStreams
    //
    // <-                                      LogStreamIdArray
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::EnumerateStreams);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
        
    _DevIoCtl->SetRetryCount(1);
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncEnumerateStreamsContextUser::StreamsEnumerated);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
    
}

VOID
KtlLogContainerUser::AsyncEnumerateStreamsContextUser::StartEnumerateStreams(
    __out KArray<KtlLogStreamId>& LogStreamIds,                           
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LogStreamIdArray = &LogStreamIds;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncEnumerateStreamsContextUser::EnumerateStreamsAsync(
    __out KArray<KtlLogStreamId>& LogStreamIds,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _LogStreamIdArray = &LogStreamIds;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncEnumerateStreamsContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncEnumerateStreamsContextUser::AsyncEnumerateStreamsContextUser()
{
}

KtlLogContainerUser::AsyncEnumerateStreamsContextUser::~AsyncEnumerateStreamsContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncEnumerateStreamsContext(
    __out AsyncEnumerateStreamsContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncEnumerateStreamsContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncEnumerateStreamsContextUser();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, NULL, GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
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

    context->_LogContainer = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Close
//
NTSTATUS
KtlLogContainerUser::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
)
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncCloseContextUser::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (NT_SUCCESS(status))
    {
        closeContext->StartClose(ParentAsyncContext,
                                 CloseCallbackPtr);
    }
    
    return(status);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncCloseContextUser::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }

    co_return co_await closeContext->CloseAsync(ParentAsyncContext);
}
#endif

VOID
KtlLogContainerUser::AsyncCloseContextUser::OnCompleted(
    )
{
    if (_CloseCallbackPtr)
    {
        _CloseCallbackPtr(_ParentAsyncContext,
            *up_cast<KtlLogContainer, KtlLogContainerUser>(_LogContainer),
            Status());
    }
}

VOID
KtlLogContainerUser::AsyncCloseContextUser::CloseCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    } else {
        _LogContainer->_IsClosed = TRUE;
    }
    
    Complete(status);   
}

VOID
KtlLogContainerUser::AsyncCloseContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogContainer->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogContainerObject     CloseConatainer
    //
    // <-                                          
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogContainer->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogContainer->GetObjectId(),
                                                    RequestMarshaller::CloseContainer);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogContainerUser::AsyncCloseContextUser::CloseCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogContainer->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogContainerUser::AsyncCloseContextUser::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
    )
{
    _CloseCallbackPtr = CloseCallbackPtr;
    _ParentAsyncContext = ParentAsyncContext;
    Start(ParentAsyncContext, nullptr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogContainerUser::AsyncCloseContextUser::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _ParentAsyncContext = ParentAsyncContext;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogContainerUser::AsyncCloseContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

KtlLogContainerUser::AsyncCloseContextUser::AsyncCloseContextUser()
{
}

KtlLogContainerUser::AsyncCloseContextUser::~AsyncCloseContextUser()
{
}

NTSTATUS
KtlLogContainerUser::CreateAsyncCloseContext(
    __out AsyncCloseContextUser::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogContainerUser::AsyncCloseContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseContextUser();
    if (context == nullptr)
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
    
    context->_LogContainer = this;

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}
