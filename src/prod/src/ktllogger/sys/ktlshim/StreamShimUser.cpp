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
// KtlLogStream Implementation
//
//************************************************************************************
KtlLogStreamUser::KtlLogStreamUser(
    __in OpenCloseDriver::SPtr DeviceHandle
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

	status = KInstrumentedComponent::Create(_InstrumentedComponent, GetThisAllocator(), GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
	
    _DeviceHandle = DeviceHandle;
    
    _PreAllocDataSize = RequestMarshaller::DefaultPreAllocDataSize;
    _PreAllocRecordMetadataSize = RequestMarshaller::DefaultPreAllocRecordMetadataSize;
    _PreAllocMetadataSize = RequestMarshaller::DefaultPreAllocMetadataSize;

    _ReservationSpaceHeld = 0;

    _IsClosed = FALSE;
}

#ifdef _WIN64
VOID
KtlLogStreamUser::DeleteObjectCompletion(
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
   
KtlLogStreamUser::~KtlLogStreamUser()
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

        status = _Marshaller->InitializeForDeleteObject(RequestMarshaller::LogStream,
                                                        objectId);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, nullptr, 0, 0);          
            return;             
        }

        _DevIoCtl->Reuse();

#ifdef _WIN64
        KAsyncContextBase::CompletionCallback completion(&KtlLogStreamUser::DeleteObjectCompletion);
#endif
        _DevIoCtl->StartDeviceIoControl(_DeviceHandle,
                                        RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                        _Marshaller->GetInKBuffer(),
                                        _Marshaller->GetWriteSize(),
                                        NULL,
                                        0,
                                        nullptr,
#ifdef _WIN64
                                        completion);
#else
                                        nullptr);
#endif
    }
}

#ifdef UDRIVER
PVOID KtlLogStreamUser::GetKernelPointer()
{
    PVOID p;
    
    p = _DeviceHandle->GetKernelPointerFromObjectId(_ObjectId);
    
    return(p);
}
#endif

//
// Stream specific routines
//
VOID
KtlLogStreamUser::QueryLogStreamId(__out KtlLogStreamId& LogStreamId
    )
{
    //
    // Returned cached value for log stream id
    //
    LogStreamId = _LogStreamId;
}


ULONG KtlLogStreamUser::QueryReservedMetadataSize(
    )
{
    return(sizeof(KtlLogVerify::KtlMetadataHeader) + _CoreLoggerMetadataOverhead);
}

BOOLEAN KtlLogStreamUser::IsFunctional(
    )
{
    return(TRUE);
}

//
// Query Record Range async
//
VOID
KtlLogStreamUser::AsyncQueryRecordRangeContextUser::RecordRangeQueried(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    KtlLogAsn lowestAsn, highestAsn, logTruncationAsn;
    
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

        status = _Marshaller->ReadData<KtlLogAsn>(lowestAsn);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        if (_LowestAsn)
        {
            *_LowestAsn = lowestAsn;
        }
        
        status = _Marshaller->ReadData<KtlLogAsn>(highestAsn);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        if (_HighestAsn)
        {
            *_HighestAsn = highestAsn;
        }
        
        status = _Marshaller->ReadData<KtlLogAsn>(logTruncationAsn);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }
        
        if (_LogTruncationAsn)
        {
            *_LogTruncationAsn = logTruncationAsn;
        }
        
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogStreamUser::AsyncQueryRecordRangeContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     QueryRecordRange
    //
    // <-                                     LowestAsn, HighestAsn, TruncationAsn        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::QueryRecordRange);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncQueryRecordRangeContextUser::RecordRangeQueried);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);
}

VOID
KtlLogStreamUser::AsyncQueryRecordRangeContextUser::StartQueryRecordRange(
    __out_opt KtlLogAsn* const LowestAsn,
    __out_opt KtlLogAsn* const HighestAsn,
    __out_opt KtlLogAsn* const LogTruncationAsn,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LowestAsn = LowestAsn;
    _HighestAsn = HighestAsn;
    _LogTruncationAsn = LogTruncationAsn;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncQueryRecordRangeContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncQueryRecordRangeContextUser::AsyncQueryRecordRangeContextUser()
{
}

KtlLogStreamUser::AsyncQueryRecordRangeContextUser::~AsyncQueryRecordRangeContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncQueryRecordRangeContext(
    __out AsyncQueryRecordRangeContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncQueryRecordRangeContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordRangeContextUser();
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

    context->_LogStream = this;

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
// Query Record async
//
VOID
KtlLogStreamUser::AsyncQueryRecordContextUser::QueryRecordCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONGLONG version = 0;
#ifdef USE_RVDLOGGER_OBJECTS
    RvdLogStream::RecordDisposition disposition;
#else
    KtlLogStream::RecordDisposition disposition;
#endif
    ULONG ioBufferSize = 0;
    ULONGLONG debugInfo = 0;
        
    if (NT_SUCCESS(status))
    {
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        status = _Marshaller->ReadData<ULONGLONG>(version);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        if (_Version)
        {
            *_Version = version;
        }       

        status = _Marshaller->ReadRecordDisposition(disposition);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }       

        if (_Disposition)
        {
            *_Disposition =  disposition;
        }
        
        status = _Marshaller->ReadData<ULONG>(ioBufferSize);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        if (_IoBufferSize)
        {
            *_IoBufferSize = ioBufferSize;
        }
        
        status = _Marshaller->ReadData<ULONGLONG>(debugInfo);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            Complete(status);
            return;             
        }

        if (_DebugInfo)
        {
            *_DebugInfo = debugInfo;
        }

    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogStreamUser::AsyncQueryRecordContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     QueryRecord, Asn
    //
    // <-                                     Version, Disposition, IoBufferSize, DebugInfo
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::QueryRecord);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncQueryRecordContextUser::QueryRecordCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncQueryRecordContextUser::StartQueryRecord(
    __in KtlLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
#ifdef USE_RVDLOGGER_OBJECTS
    __out_opt RvdLogStream::RecordDisposition* const Disposition,
#else
    __out_opt KtlLogStream::RecordDisposition* const Disposition,
#endif
    __out_opt ULONG* const IoBufferSize,
    __out_opt ULONGLONG* const DebugInfo,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _RecordAsn = RecordAsn;
    _Version =  Version;
    _Disposition = Disposition;
    _IoBufferSize =  IoBufferSize;
    _DebugInfo =  DebugInfo;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncQueryRecordContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncQueryRecordContextUser::AsyncQueryRecordContextUser()
{
}

KtlLogStreamUser::AsyncQueryRecordContextUser::~AsyncQueryRecordContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncQueryRecordContext(__out AsyncQueryRecordContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncQueryRecordContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordContextUser();
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

    context->_LogStream = this;

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
// Query Records async
//
VOID
KtlLogStreamUser::AsyncQueryRecordsContextUser::QueryRecordsCompleted(
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

        status = _Marshaller->ReadRecordMetadataArray(*_ResultsArray);
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
KtlLogStreamUser::AsyncQueryRecordsContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     QueryRecords, LowestAsn. HighestAsn
    //
    // <-                                     RecordArray
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::QueryRecords);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData(_LowestAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_HighestAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // This may require a retry if there are many records
    //
    _DevIoCtl->SetRetryCount(4);
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncQueryRecordsContextUser::QueryRecordsCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncQueryRecordsContextUser::StartQueryRecords(
    __in KtlLogAsn LowestAsn,
    __in KtlLogAsn HighestAsn,
#ifdef USE_RVDLOGGER_OBJECTS
    __in KArray<RvdLogStream::RecordMetadata>& ResultsArray,
#else
    __in KArray<RecordMetadata>& ResultsArray,     
#endif
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _LowestAsn = LowestAsn;
    _HighestAsn = HighestAsn;
    _ResultsArray = &ResultsArray;     

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncQueryRecordsContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncQueryRecordsContextUser::AsyncQueryRecordsContextUser()
{
}

KtlLogStreamUser::AsyncQueryRecordsContextUser::~AsyncQueryRecordsContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncQueryRecordsContext(
    __out AsyncQueryRecordsContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncQueryRecordsContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordsContextUser();
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

    context->_LogStream = this;

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

VOID
KtlLogStreamUser::AsyncWriteContextUser::WriteCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    KFinally([&status, this]
    {
        if (NT_SUCCESS(status))
        {
            _InstrumentedOperation.EndOperation(_MetaDataBuffer->QuerySize() + _IoBuffer->QuerySize());
        }

        KDbgCheckpointWDataInformational(
            _LogStream->GetActivityId(),
            "UserWriteCompletion",
            status,
            (ULONGLONG)this,
            _RecordAsn.Get(),
            _Version,
            0);

        Complete(status);
    });

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return;
    } 
		
    if (_MetaDataBuffer->QuerySize() > _LogStream->_PreAllocRecordMetadataSize)
    {
        _LogStream->_PreAllocRecordMetadataSize = _MetaDataBuffer->QuerySize();
    }

    if (_IoBuffer->QuerySize() > _LogStream->_PreAllocDataSize)
    {
        _LogStream->_PreAllocDataSize = _IoBuffer->QuerySize();
    }

    _LogStream->AdjustReservationSpace(-1 * _ReservationSpace);

    status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());

    if (!NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return;
    }

    if (_LogSize != nullptr)
    {
        KAssert(_LogSpaceRemaining != nullptr);

        status = _Marshaller->ReadData<ULONGLONG>(*_LogSize);
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return;
        }

        status = _Marshaller->ReadData<ULONGLONG>(*_LogSpaceRemaining);
        if (!NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return;
        }
    }
}

VOID
KtlLogStreamUser::AsyncWriteContextUser::OnStart(
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    KDbgCheckpointWDataInformational(_LogStream->GetActivityId(),
                        "UserWriteStart", status, (ULONGLONG)this, _RecordAsn.Get(), _Version, 0);
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    //
    // If write uses reservation space, verify that reservation space is available
    //
    if (! _LogStream->IsReservationSpaceAvailable(_ReservationSpace))
    {
        status = K_STATUS_LOG_RESERVE_TOO_SMALL;
        KTraceFailedAsyncRequest(status, this, _ReservationSpace, _LogStream->_ReservationSpaceHeld);
        Complete(status);
        return;
    }


    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     Write, RecordAsn, Version, MetaDataLength, MetaDataBuffer, IoBuffer
    //
    // <-
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::WriteAndReturnLogUsage);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_Version);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
        
    status = _Marshaller->WriteData(_MetaDataLength);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData<ULONGLONG>(_ReservationSpace);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKIoBuffer(*_MetaDataBuffer,
                                         TRUE);             // Make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteKIoBuffer(*_IoBuffer,
                                         TRUE);             // Make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncWriteContextUser::WriteCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncWriteContextUser::StartWrite(
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONGLONG ReservationSpace,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
	_InstrumentedOperation.BeginOperation();
	
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ReservationSpace = ReservationSpace;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncWriteContextUser::StartWrite(
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONGLONG ReservationSpace,
    __out ULONGLONG& LogSize,
    __out ULONGLONG& LogSpaceRemaining,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
)
{
    _InstrumentedOperation.BeginOperation();

    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ReservationSpace = ReservationSpace;
    _LogSize = &LogSize;
    _LogSpaceRemaining = &LogSpaceRemaining;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamUser::AsyncWriteContextUser::WriteAsync(
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONGLONG ReservationSpace,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
	_InstrumentedOperation.BeginOperation();
	
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ReservationSpace = ReservationSpace;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}

ktl::Awaitable<NTSTATUS>
KtlLogStreamUser::AsyncWriteContextUser::WriteAsync(
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONGLONG ReservationSpace,
    __out ULONGLONG& LogSize,
    __out ULONGLONG& LogSpaceRemaining,
    __in_opt KAsyncContextBase* const ParentAsyncContext
)
{
    _InstrumentedOperation.BeginOperation();

    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _IoBuffer = IoBuffer;
    _ReservationSpace = ReservationSpace;
    _LogSize = &LogSize;
    _LogSpaceRemaining = &LogSpaceRemaining;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamUser::AsyncWriteContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncWriteContextUser::AsyncWriteContextUser()
{
}

KtlLogStreamUser::AsyncWriteContextUser::~AsyncWriteContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncWriteContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteContextUser();
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

    context->_LogStream = this;
	context->_InstrumentedOperation.SetInstrumentedComponent(*_InstrumentedComponent);
    
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
// Allocate Reservation Context
//
VOID
KtlLogStreamUser::AsyncReservationContextUser::ReservationSizeChanged(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();

    if (NT_SUCCESS(status))
    {
        _LogStream->AdjustReservationSpace(_ReservationDelta);
    } else {
        KTraceFailedAsyncRequest(status, this, _ReservationDelta, 0);
    }
    
    Complete(status);
}

VOID
KtlLogStreamUser::AsyncReservationContextUser::OnStart(
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // If contract reservation space, verify that reservation space is available
    //
    if (_ReservationDelta < 0)
    {
        if (!_LogStream->IsReservationSpaceAvailable(-1 * _ReservationDelta))
        {
            status = K_STATUS_LOG_RESERVE_TOO_SMALL;
            KTraceFailedAsyncRequest(status, this, _ReservationDelta, _LogStream->_ReservationSpaceHeld);
            Complete(status);
            return;
        }
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     Reservation, ReservationDelta
    //
    // <-
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::Reservation);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData<LONGLONG>(_ReservationDelta);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncReservationContextUser::ReservationSizeChanged);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncReservationContextUser::StartExtendReservation(
    __in ULONGLONG ReservationSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _ReservationDelta = ReservationSize;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReservationContextUser::StartContractReservation(
    __in ULONGLONG ReservationSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    //
    // Core logger api takes a negative value for the reservation delta
    // to effect a contraction of the reservation size
    //
    _ReservationDelta = -1 * ReservationSize;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReservationContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncReservationContextUser::AsyncReservationContextUser()
{
}

KtlLogStreamUser::AsyncReservationContextUser::~AsyncReservationContextUser()
{
}

ULONGLONG
KtlLogStreamUser::QueryReservationSpace(
    )
{
    return(_ReservationSpaceHeld);
}

NTSTATUS
KtlLogStreamUser::CreateAsyncReservationContext(__out AsyncReservationContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncReservationContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReservationContextUser();
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

    context->_LogStream = this;
    
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
// Read Record
//
VOID
KtlLogStreamUser::AsyncReadContextUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _PreAllocMetadataBuffer = nullptr;
    _PreAllocDataBuffer = nullptr;
    
    Complete(Status);
}

VOID
KtlLogStreamUser::AsyncReadContextUser::ReadCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    BOOLEAN tryAgain = FALSE;
    ULONG size;
    
    if (NT_SUCCESS(status))
    {
        KtlLogAsn actualAsn;
        
        status = _Marshaller->InitializeForResponse(_DevIoCtl->GetOutBufferSize());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;             
        }

        status = _Marshaller->ReadData<KtlLogAsn>(actualAsn);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;             
        }

        if (_ActualReadAsnPtr)
        {
            *_ActualReadAsnPtr = actualAsn;
        }
        
        status = _Marshaller->ReadData<ULONGLONG>(*_Version);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;             
        }
        
        status = _Marshaller->ReadData<ULONG>(*_MetaDataLength);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, 0, 0);
            DoComplete(status);
            return;             
        }
        
        status = _Marshaller->ReadKIoBufferPreAlloc(*_MetaDataBuffer,
                                                    _PreAllocMetadataBuffer,
                                                    size);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, size, 0);
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                tryAgain = TRUE;
                if (size > _LogStream->_PreAllocRecordMetadataSize)
                {
                    _LogStream->_PreAllocRecordMetadataSize = size;
                }
            } else {
                DoComplete(status);
                return;
            }
        }
        
        status = _Marshaller->ReadKIoBufferPreAlloc(*_IoBuffer,
                                                    _PreAllocDataBuffer,
                                                    size);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, size, 0);
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                tryAgain = TRUE;
                if (size > _LogStream->_PreAllocDataSize)
                {
                    _LogStream->_PreAllocDataSize = size;
                }
            } else if (! tryAgain) {
                DoComplete(status);
                return;
            }
        }

    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;             
    }

    if (tryAgain)
    {
        //
        // We want to try again with larger pre allocated data buffers
        //
        _PreAllocMetadataBuffer = nullptr;
        _PreAllocDataBuffer = nullptr;
        _DevIoCtl->Reuse();
        _Marshaller->Reset();
        OnStart();
    } else {
        DoComplete(STATUS_SUCCESS);
    }
}

VOID
KtlLogStreamUser::AsyncReadContextUser::OnStart(
    )
{
    NTSTATUS status;
    PVOID p;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    KInvariant(_ReadType != RvdLogStream::AsyncReadContext::ReadTypeNotSpecified);
    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     Read, ReadType, RecordAsn, PreAlloc Metadata buffer, PreAlloc Io Buffer
    //
    // <-                                     Actual Read Asn, Version, Metadata length, Metadata Buffer, IoBuffer
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::Read);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_ReadType);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    //
    // Assumption is that any preallocated IoBuffers have only a single
    // element.
    //
    status = KIoBuffer::CreateSimple(_LogStream->_PreAllocRecordMetadataSize,
                                     _PreAllocMetadataBuffer,
                                     p,
                                     GetThisAllocator(),
                                     GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    status = _Marshaller->WriteKIoBuffer(*_PreAllocMetadataBuffer,
                                        FALSE);       // Do not make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    status = KIoBuffer::CreateSimple(_LogStream->_PreAllocDataSize,
                                     _PreAllocDataBuffer,
                                     p,
                                     GetThisAllocator(),
                                     GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    status = _Marshaller->WriteKIoBuffer(*_PreAllocDataBuffer,
                                         FALSE);       // Do not make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncReadContextUser::ReadCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncReadContextUser::StartRead(
    __in KtlLogAsn RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out ULONG& MetaDataLength,
    __out KIoBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _ReadType = RvdLogStream::AsyncReadContext::ReadExactRecord;
    _RecordAsn = RecordAsn;
    _ActualReadAsnPtr = NULL;
    _Version = Version;
    _MetaDataLength = &MetaDataLength,
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReadContextUser::StartReadPrevious(
    __in KtlLogAsn PreviousRecordAsn,
    __out_opt KtlLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out ULONG& MetaDataLength,
    __out KIoBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _ReadType = RvdLogStream::AsyncReadContext::ReadPreviousRecord;
    _RecordAsn = PreviousRecordAsn;
    _ActualReadAsnPtr = RecordAsn;
    _Version = Version;
    _MetaDataLength = &MetaDataLength,
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReadContextUser::StartReadNext(
    __in KtlLogAsn NextRecordAsn,
    __out_opt KtlLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out ULONG& MetaDataLength,
    __out KIoBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _ReadType = RvdLogStream::AsyncReadContext::ReadNextRecord;
    _RecordAsn = NextRecordAsn;
    _ActualReadAsnPtr = RecordAsn;
    _Version = Version;
    _MetaDataLength = &MetaDataLength,
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReadContextUser::StartReadContaining(
    __in KtlLogAsn ContainingRecordAsn,
    __out_opt KtlLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out ULONG& MetaDataLength,
    __out KIoBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _ReadType = RvdLogStream::AsyncReadContext::ReadContainingRecord;
    _RecordAsn = ContainingRecordAsn;
    _ActualReadAsnPtr = RecordAsn;
    _Version = Version;
    _MetaDataLength = &MetaDataLength,
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(ParentAsyncContext, CallbackPtr); 
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamUser::AsyncReadContextUser::ReadContainingAsync(
    __in KtlLogAsn ContainingRecordAsn,
    __out_opt KtlLogAsn* RecordAsn,
    __out_opt ULONGLONG* const Version,
    __out ULONG& MetaDataLength,
    __out KIoBuffer::SPtr& MetaDataBuffer,
    __out KIoBuffer::SPtr& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _ReadType = RvdLogStream::AsyncReadContext::ReadContainingRecord;
    _RecordAsn = ContainingRecordAsn;
    _ActualReadAsnPtr = RecordAsn;
    _Version = Version;
    _MetaDataLength = &MetaDataLength,
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamUser::AsyncReadContextUser::OnReuse(
    )
{
    _ReadType = RvdLogStream::AsyncReadContext::ReadTypeNotSpecified;
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncReadContextUser::AsyncReadContextUser()
{
}

KtlLogStreamUser::AsyncReadContextUser::~AsyncReadContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncReadContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadContextUser();
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

    context->_LogStream = this;
    
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
// Multi Record Read Record
//
VOID
KtlLogStreamUser::AsyncMultiRecordReadContextUser::DoComplete(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID
KtlLogStreamUser::AsyncMultiRecordReadContextUser::ReadCompleted(
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
            DoComplete(status);
            return;             
        }
    } else {
        if (status == STATUS_NOT_FOUND)
        {
            KDbgCheckpointWDataInformational(_LogStream->GetActivityId(),
                                "AsyncMultiRecordReadContextUser", status, (ULONGLONG)this, _RecordAsn.Get(), 0, 0);
        } else {        
            KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), 0);
        }
        DoComplete(status);
        return;             
    }

    DoComplete(STATUS_SUCCESS);
}

VOID
KtlLogStreamUser::AsyncMultiRecordReadContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Ensure that passed KIoBuffers have only a single KIoBufferElement
    //
    if ((_MetaDataBuffer->QueryNumberOfIoBufferElements() != 1) ||
        (_IoBuffer->QueryNumberOfIoBufferElements() > 1))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     MultiRecordRead, RecordAsn, PreAlloc Metadata IO buffer, PreAlloc Io Buffer
    //
    // <-                                     <data is already written to user buffers>
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::MultiRecordRead);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_RecordAsn);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    status = _Marshaller->WriteKIoBuffer(*_MetaDataBuffer,
                                         FALSE);       // Do not make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    status = _Marshaller->WriteKIoBuffer(*_IoBuffer,
                                         FALSE);       // Do not make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncMultiRecordReadContextUser::ReadCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncMultiRecordReadContextUser::StartRead(
    __in KtlLogAsn RecordAsn,
    __out KIoBuffer& MetaDataBuffer,
    __out KIoBuffer& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _RecordAsn = RecordAsn;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;  
    
    Start(ParentAsyncContext, CallbackPtr); 
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamUser::AsyncMultiRecordReadContextUser::ReadAsync(
    __in KtlLogAsn RecordAsn,
    __inout KIoBuffer& MetaDataBuffer,
    __inout KIoBuffer& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _RecordAsn = RecordAsn;
    _MetaDataBuffer = &MetaDataBuffer;
    _IoBuffer = &IoBuffer;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamUser::AsyncMultiRecordReadContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncMultiRecordReadContextUser::AsyncMultiRecordReadContextUser()
{
}

KtlLogStreamUser::AsyncMultiRecordReadContextUser::~AsyncMultiRecordReadContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncMultiRecordReadContext(__out AsyncMultiRecordReadContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncMultiRecordReadContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncMultiRecordReadContextUser();
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

    context->_LogStream = this;
    
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
// Truncate
//
VOID
KtlLogStreamUser::AsyncTruncateContextUser::TruncationCompleted(
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
KtlLogStreamUser::AsyncTruncateContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     Truncate, Truncate Asn, Preferred truncate Asn
    //
    // <-                                      
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::Truncate);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData(_TruncationPoint);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData(_PreferredTruncationPoint);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncTruncateContextUser::TruncationCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncTruncateContextUser::StartTruncate(
    __in KtlLogAsn TruncationPoint,
    __in KtlLogAsn PreferredTruncationPoint,         
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _TruncationPoint = TruncationPoint;
    _PreferredTruncationPoint = PreferredTruncationPoint;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncTruncateContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncTruncateContextUser::AsyncTruncateContextUser()
{
}

KtlLogStreamUser::AsyncTruncateContextUser::~AsyncTruncateContextUser()
{
}

VOID
KtlLogStreamUser::AsyncTruncateContextUser::Truncate(
    __in KtlLogAsn TruncationPoint,
    __in KtlLogAsn PreferredTruncationPoint
    )
{
    StartTruncate(TruncationPoint, PreferredTruncationPoint, nullptr, nullptr); 
}

NTSTATUS
KtlLogStreamUser::CreateAsyncTruncateContext(
    __out AsyncTruncateContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncTruncateContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncTruncateContextUser();
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

    context->_LogStream = this;

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
// Wait For Notification
//
VOID
KtlLogStreamUser::AsyncStreamNotificationContextUser::WaitCompleted(
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
KtlLogStreamUser::AsyncStreamNotificationContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     StreamNotification, Threshold, Threshold Value
    //
    // <-                                      
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::StreamNotification);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteData<ThresholdTypeEnum>(_ThresholdType);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    status = _Marshaller->WriteData<ULONGLONG>(_ThresholdValue);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncStreamNotificationContextUser::WaitCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncStreamNotificationContextUser::StartWaitForNotification(
    __in ThresholdTypeEnum ThresholdType,
    __in ULONGLONG ThresholdValue,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _ThresholdType = ThresholdType;
    _ThresholdValue = ThresholdValue;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncStreamNotificationContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}

VOID
KtlLogStreamUser::AsyncStreamNotificationContextUser::OnCancel(
    )
{
    _DevIoCtl->Cancel();
}

KtlLogStreamUser::AsyncStreamNotificationContextUser::AsyncStreamNotificationContextUser()
{
}

KtlLogStreamUser::AsyncStreamNotificationContextUser::~AsyncStreamNotificationContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncStreamNotificationContext(
    __out AsyncStreamNotificationContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncStreamNotificationContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncStreamNotificationContextUser();
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

    context->_LogStream = this;

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
// Write Metadata
//
VOID
KtlLogStreamUser::AsyncWriteMetadataContextUser::WriteMetadataCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    
    status = Async.Status();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);          
    } else {
        if (_MetadataBuffer->QuerySize() > _LogStream->_PreAllocMetadataSize)
        {
            _LogStream->_PreAllocMetadataSize = _MetadataBuffer->QuerySize();
        }
    }

    _MetadataBuffer = nullptr;
    
    Complete(status);
}

VOID
KtlLogStreamUser::AsyncWriteMetadataContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        _MetadataBuffer = nullptr;
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     WriteMetadata, MetadataBuffer
    //
    // <-                                      
        
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::WriteMetadata);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    status = _Marshaller->WriteKIoBuffer(*_MetadataBuffer,
                                         TRUE);       // Make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncWriteMetadataContextUser::WriteMetadataCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncWriteMetadataContextUser::StartWriteMetadata(
    __in const KIoBuffer::SPtr& MetadataBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _MetadataBuffer = MetadataBuffer;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncWriteMetadataContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncWriteMetadataContextUser::AsyncWriteMetadataContextUser()
{
}

KtlLogStreamUser::AsyncWriteMetadataContextUser::~AsyncWriteMetadataContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncWriteMetadataContext(
    __out AsyncWriteMetadataContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncWriteMetadataContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteMetadataContextUser();
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
    
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Read Metadata
//
VOID
KtlLogStreamUser::AsyncReadMetadataContextUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _PreAllocMetadataBuffer = nullptr;
    
    Complete(Status);
}

VOID
KtlLogStreamUser::AsyncReadMetadataContextUser::ReadMetadataCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;
    ULONG size;
    BOOLEAN tryAgain = FALSE;

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

        status = _Marshaller->ReadKIoBufferPreAlloc(*_MetadataBuffer,
                                                    _PreAllocMetadataBuffer,
                                                    size);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, size, 0);
            if (status == STATUS_BUFFER_TOO_SMALL)
            {
                tryAgain = TRUE;
                if (size > _LogStream->_PreAllocMetadataSize)
                {
                    _LogStream->_PreAllocMetadataSize = size;
                }
            } else {
                DoComplete(status);
                return;
            }
        }
    } else {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    if (tryAgain)
    {
        //
        // We want to try again with larger pre allocated data buffers
        //
        _PreAllocMetadataBuffer = nullptr;
        _DevIoCtl->Reuse();
        _Marshaller->Reset();
        OnStart();
    } else {
        DoComplete(status);
    }   
}

VOID
KtlLogStreamUser::AsyncReadMetadataContextUser::OnStart(
    )
{
    NTSTATUS status;
    PVOID p;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     ReadMetadata, PreAlloc MetaData Buffer
    //
    // <-                                     Metadata buffer
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::ReadMetadata);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }

    //
    // Assumption is that all preallocated IoBuffers have only a single
    // element.
    //
    status = KIoBuffer::CreateSimple(_LogStream->_PreAllocMetadataSize,
                                     _PreAllocMetadataBuffer,
                                     p,
                                     GetThisAllocator(),
                                     GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    status = _Marshaller->WriteKIoBuffer(*_PreAllocMetadataBuffer,
                                         FALSE);     // Do not make read only
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
        return;
    }
        
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncReadMetadataContextUser::ReadMetadataCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncReadMetadataContextUser::StartReadMetadata(
    __out KIoBuffer::SPtr& MetadataBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _MetadataBuffer = &MetadataBuffer;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamUser::AsyncReadMetadataContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncReadMetadataContextUser::AsyncReadMetadataContextUser()
{
}

KtlLogStreamUser::AsyncReadMetadataContextUser::~AsyncReadMetadataContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncReadMetadataContext(
    __out AsyncReadMetadataContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncReadMetadataContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadMetadataContextUser();
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
        
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}


//
// Ioctl
//
VOID
KtlLogStreamUser::AsyncIoctlContextUser::DoComplete(
    __in NTSTATUS Status
    )
{
    _InBuffer = nullptr;
    
    Complete(Status);
}

VOID
KtlLogStreamUser::AsyncIoctlContextUser::IoctlCompleted(
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
KtlLogStreamUser::AsyncIoctlContextUser::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        DoComplete(status);
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
    // ->   ObjectMethod, LogStreamObject     Ioctl, Control Code, InBuffer
    //
    // <-                                     Result, OutBuffer
    
    
    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::Ioctl);
    status = _Marshaller->WriteData<ULONG>(_ControlCode);
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
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncIoctlContextUser::IoctlCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                    
}

VOID
KtlLogStreamUser::AsyncIoctlContextUser::StartIoctl(
    __in ULONG ControlCode,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _ControlCode = ControlCode;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;
        
    Start(ParentAsyncContext, CallbackPtr); 
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamUser::AsyncIoctlContextUser::IoctlAsync(
    __in ULONG ControlCode,
    __in_opt KBuffer* const InBuffer,
    __out ULONG& Result,
    __out KBuffer::SPtr& OutBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _ControlCode = ControlCode;
    _InBuffer = InBuffer;
    _Result = &Result;
    _OutBuffer = &OutBuffer;

    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamUser::AsyncIoctlContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncIoctlContextUser::AsyncIoctlContextUser()
{
}

KtlLogStreamUser::AsyncIoctlContextUser::~AsyncIoctlContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncIoctlContext(
    __out AsyncIoctlContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncIoctlContextUser::SPtr context;
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncIoctlContextUser();
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
        
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}
   

//
// Close
//
NTSTATUS
KtlLogStreamUser::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
)
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncCloseContextUser::SPtr closeContext;

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
KtlLogStreamUser::CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext)
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncCloseContextUser::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }
    
    co_return co_await closeContext->CloseAsync(ParentAsyncContext);
}
#endif

VOID
KtlLogStreamUser::AsyncCloseContextUser::OnCompleted(
    )
{
    if (_CloseCallbackPtr)
    {
        _CloseCallbackPtr(_ParentAsyncContext,
            *up_cast<KtlLogStream, KtlLogStreamUser>(_LogStream),
            Status());
    }
}

VOID
KtlLogStreamUser::AsyncCloseContextUser::CloseCompleted(
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
        _LogStream->_IsClosed = TRUE;
        _LogStream->_ReservationSpaceHeld = 0;
    }
    
    Complete(status);
}

VOID
KtlLogStreamUser::AsyncCloseContextUser::OnStart(
    )
{
    NTSTATUS status;

    //
    // Fail if object already closed
    //
    if (_LogStream->_IsClosed)
    {
        status = STATUS_OBJECT_NO_LONGER_EXISTS;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    
    //    [RequestHeader                   ] [Parameters ]
    // ->   ObjectMethod, LogStreamObject     CloseStream
    //
    // <-                                      

    status = _Marshaller->InitializeForObjectMethod(static_cast<RequestMarshaller::OBJECT_TYPE>(_LogStream->GetObjectType()),
                                                    (ULONGLONG)this,
                                                    _LogStream->GetObjectId(),
                                                    RequestMarshaller::CloseStream);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamUser::AsyncCloseContextUser::CloseCompleted);
    _DevIoCtl->StartDeviceIoControl(_LogStream->_DeviceHandle,
                                    RequestMarshaller::KTLLOGGER_DEVICE_IOCTL,
                                    _Marshaller->GetInKBuffer(),
                                    _Marshaller->GetWriteSize(),
                                    _Marshaller->GetOutKBuffer(),
                                    _Marshaller->GetMaxOutBufferSize(),
                                    this,
                                    completion);                                                
    
}

VOID
KtlLogStreamUser::AsyncCloseContextUser::StartClose(
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
KtlLogStreamUser::AsyncCloseContextUser::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    _ParentAsyncContext = ParentAsyncContext;

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamUser::AsyncCloseContextUser::OnReuse(
    )
{
    _DevIoCtl->Reuse();
    _Marshaller->Reset();
}


KtlLogStreamUser::AsyncCloseContextUser::AsyncCloseContextUser()
{
}

KtlLogStreamUser::AsyncCloseContextUser::~AsyncCloseContextUser()
{
}

NTSTATUS
KtlLogStreamUser::CreateAsyncCloseContext(
    __out AsyncCloseContextUser::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamUser::AsyncCloseContextUser::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseContextUser();
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

    context->_LogStream = this;

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

