// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#ifdef UNIFY
#define UPASSTHROUGH 1
#endif

#include "KtlLogShimKernel.h"

//************************************************************************************
//
// KtlLogStream Implementation
//
//************************************************************************************
const ULONG KtlLogStreamKernel::OverlayLinkageOffset = FIELD_OFFSET(KtlLogStreamKernel, _OverlayStreamEntry);

KtlLogStreamKernel::KtlLogStreamKernel(
    __in KtlLogContainerKernel& LogContainer,
    __in KGuid DiskId,
    __in ULONG CoreLoggerMetadataOverhead
) : _ContextList(ActiveContext::_LinkOffset),
    _CoreLoggerMetadataOverhead(CoreLoggerMetadataOverhead)
{
    NTSTATUS status;

    _BaseLogStream = nullptr;
    _DiskId = DiskId;
#if defined(UDRIVER) || defined(KDRIVER)
    _ObjectId = 0;
#endif
#ifdef UPASSTHROUGH
    _ReservationSpaceHeld = 0;  
#endif
    _IsClosed = FALSE;
    
    _LogContainer = &LogContainer;

    status = KInstrumentedComponent::Create(_WriteIC, GetThisAllocator(), GetThisAllocationTag());
    if ( ! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
    
    status = CreateAsyncCloseContext(_AutoCloseContext);
    if ( ! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
        
    status = _LogContainer->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // It turns out that the container is closed at this point and
        // so this new stream can't be added to the list. So propogate
        // the error back so that the stream create/open can be failed
        //
        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                            "Container closed when adding stream", status,
                            (ULONGLONG)0,
                            (ULONGLONG)this,
                            (ULONGLONG)_LogContainer.RawPtr(),
                            (ULONGLONG)0);
        _AutoCloseContext = nullptr;
        _IsOnContainerActiveList = FALSE;
        SetConstructorStatus(status);
    } else {
        _IsOnContainerActiveList = TRUE;
    }
}

KtlLogStreamKernel::~KtlLogStreamKernel()
{   
    if (_IsOnContainerActiveList)
    {
        _LogContainer->RemoveActiveContext(_ActiveContext);
        //
        // Do not check if removed from list successfully as it may not
        // have been added in case of a race between stream create/open and
        // container close
        //
    }
    _IsOnContainerActiveList = FALSE;
}

//
// Stream specific routines
//
VOID
KtlLogStreamKernel::QueryLogStreamId(__out KtlLogStreamId& LogStreamId
    )
{
    KInvariant(_BaseLogStream != nullptr);

    //
    // Returned cached value for log stream id
    //
    LogStreamId = _LogStreamId;
}


ULONG KtlLogStreamKernel::QueryReservedMetadataSize(
    )
{
    return(sizeof(KtlLogVerify::KtlMetadataHeader) + _CoreLoggerMetadataOverhead);
}

BOOLEAN KtlLogStreamKernel::IsFunctional(
    )
{
    return(TRUE);
}

//
// Query Record Range async
//
VOID
KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    status = _LogStream->_BaseLogStream->QueryRecordRange(_LowestAsn,
                                                          _HighestAsn,
                                                          _LogTruncationAsn);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::StartQueryRecordRange(
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
KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::AsyncQueryRecordRangeContextKernel()
{
}

KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::~AsyncQueryRecordRangeContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncQueryRecordRangeContext(
    __out AsyncQueryRecordRangeContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncQueryRecordRangeContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordRangeContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Query Record async
//
VOID
KtlLogStreamKernel::AsyncQueryRecordContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncQueryRecordContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    status = _LogStream->_BaseLogStream->QueryRecord(_RecordAsn,
                                                     _Version,
                                                     _Disposition,
                                                     _IoBufferSize,
                                                     _DebugInfo);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }

    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncQueryRecordContextKernel::StartQueryRecord(
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
KtlLogStreamKernel::AsyncQueryRecordContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncQueryRecordContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogStreamKernel::AsyncQueryRecordContextKernel::AsyncQueryRecordContextKernel()
{
}

KtlLogStreamKernel::AsyncQueryRecordContextKernel::~AsyncQueryRecordContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncQueryRecordContext(__out AsyncQueryRecordContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncQueryRecordContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Query Records async
//
VOID
KtlLogStreamKernel::AsyncQueryRecordsContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncQueryRecordsContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    status = _LogStream->_BaseLogStream->QueryRecords(_LowestAsn,
                                                      _HighestAsn,
                                                      *_ResultsArray);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncQueryRecordsContextKernel::StartQueryRecords(
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
KtlLogStreamKernel::AsyncQueryRecordsContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncQueryRecordsContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogStreamKernel::AsyncQueryRecordsContextKernel::AsyncQueryRecordsContextKernel()
{
}

KtlLogStreamKernel::AsyncQueryRecordsContextKernel::~AsyncQueryRecordsContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncQueryRecordsContext(__out AsyncQueryRecordsContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncQueryRecordsContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncQueryRecordsContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Write Record
//

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    _BaseAsyncWrite = nullptr;
}

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::WriteCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    ULONG size = _MetaDataBuffer->QuerySize() + (_IoBuffer ? _IoBuffer->QuerySize() : 0);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);

        //
        // TODO: For now fire any truncation needed events, in the future we
        // will need to fire at the appropriate %
        //
        if (status == STATUS_LOG_FULL)
        {
            _LogStream->CheckThresholdContexts(100);
        }
    } else {
#ifdef UPASSTHROUGH
        _LogStream->AdjustReservationSpace(-1 * _ReservationSpace);
#endif
    }
    
    _WriteOperation.EndOperation(size);
    
    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif  

#ifdef UPASSTHROUGH
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
#endif

#ifdef UPASSTHROUGH
    //
    // Create a new KIoBuffer that is used only on the "kernel" side
    //
    KIoBuffer::SPtr copyIoBuffer;

    status = KIoBuffer::CreateEmpty(copyIoBuffer, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _IoBuffer->QuerySize());
        Complete(status);
        return;     
    }

    status = copyIoBuffer->AddIoBufferReference(*_IoBuffer, 0, _IoBuffer->QuerySize());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), _IoBuffer->QuerySize());
        Complete(status);
        return;     
    }

    _IoBuffer = copyIoBuffer;
#endif
    
    //
    // Copy metadata from its KIoBuffer into a KBuffer as the
    // current RvdLogger wants a KBuffer.
    //
    KBuffer::SPtr metaDataKBuffer;
    KIoBufferElement::SPtr ioBufferElement;
    KIoBufferView::SPtr ioBufferView;
    ULONG offset;
    PUCHAR buffer;
    ULONG metaDataSize = _MetaDataBuffer->QuerySize();

    if (metaDataSize < _LogStream->QueryReservedMetadataSize())
    {
        //
        // Caller did not pass a metadata buffer large enough
        // for our header so fail this request
        //
        status = STATUS_BUFFER_TOO_SMALL;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    if (_MetaDataLength > metaDataSize)
    {
        //
        // Caller did not pass a value for the actual metadata size
        // that is less than or equal to the metadata buffer size
        //
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;     
    }

    HRESULT hr;
    ULONG result;
    hr = ULongSub(metaDataSize, _LogStream->_CoreLoggerMetadataOverhead, &result);
    KInvariant(SUCCEEDED(hr));
    status = KBuffer::Create(result,
                             metaDataKBuffer, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        //
        // Error trying to allocate the KBuffer. Pass back to
        // the caller
        //
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    offset = 0;
    buffer = static_cast<PUCHAR>(metaDataKBuffer->GetBuffer());

    //
    // Although the core logger overhead is included in the KIoBuffer
    // created by the caller, the core logger will pre-pend its own
    // overhead so we do not want to provide space in the buffer we pass on to it.
    //
    ULONG bytesToSkip = _LogStream->_CoreLoggerMetadataOverhead;
    
    for (ioBufferElement = _MetaDataBuffer->First();
         (ioBufferElement);
         ioBufferElement = _MetaDataBuffer->Next(*ioBufferElement))
    {
        ULONG bytesToCopy = ioBufferElement->QuerySize() - bytesToSkip;

        KMemCpySafe(
            &buffer[offset],
            metaDataKBuffer->QuerySize() - offset,
            (PUCHAR)(ioBufferElement->GetBuffer()) + bytesToSkip,
            bytesToCopy);

        offset += bytesToCopy;

        //
        // Only skip bytes on the first io buffer element
        //
        bytesToSkip = 0;
    }           

    //
    // Set the actual metadata size
    //
    struct KtlLogVerify::KtlMetadataHeader* metaDataHeader =
                             static_cast<KtlLogVerify::KtlMetadataHeader*>(metaDataKBuffer->GetBuffer());
    
    metaDataHeader->ActualMetadataSize = _MetaDataLength;
    
    //
    // Add verification information to the metadata header
    //
    KtlLogVerify::ComputeRecordVerification(metaDataHeader->VerificationData,
                                                _RecordAsn,
                                                _Version,
                                                metaDataSize,
                                                _MetaDataBuffer,
                                                _IoBuffer);
    
    //
    // Finally call down to the logger to do the write
    //
    status = _LogStream->_BaseLogStream->CreateAsyncWriteContext(_BaseAsyncWrite);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncWriteContextKernel::WriteCompleted);

    if (_LogSize != nullptr)
    {
        KAssert(_LogSpaceRemaining != nullptr);

        _BaseAsyncWrite->StartReservedWrite(_ReservationSpace,
            _RecordAsn,
            _Version,
            metaDataKBuffer,
            _IoBuffer,
            *_LogSize,
            *_LogSpaceRemaining,
            this,
            completion);
    }
    else
    {
        _BaseAsyncWrite->StartReservedWrite(_ReservationSpace,
            _RecordAsn,
            _Version,
            metaDataKBuffer,
            _IoBuffer,
            this,
            completion);
    }
}

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::StartWrite(
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
    _WriteOperation.BeginOperation();
    
    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _ReservationSpace = ReservationSpace;
    _IoBuffer = IoBuffer;
    _LogSize = nullptr;
    _LogSpaceRemaining = nullptr;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::StartWrite(
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
    _WriteOperation.BeginOperation();

    _RecordAsn = RecordAsn;
    _Version = Version;
    _MetaDataLength = MetaDataLength;
    _MetaDataBuffer = MetaDataBuffer;
    _ReservationSpace = ReservationSpace;
    _IoBuffer = IoBuffer;
    _LogSize = &LogSize;
    _LogSpaceRemaining = &LogSpaceRemaining;

    Start(ParentAsyncContext, CallbackPtr);
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamKernel::AsyncWriteContextKernel::WriteAsync(
    __in KtlLogAsn RecordAsn,
    __in ULONGLONG Version,
    __in ULONG MetaDataLength,
    __in const KIoBuffer::SPtr& MetaDataBuffer,
    __in const KIoBuffer::SPtr& IoBuffer,
    __in ULONGLONG ReservationSpace,
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    _WriteOperation.BeginOperation();
    
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
KtlLogStreamKernel::AsyncWriteContextKernel::WriteAsync(
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
    _WriteOperation.BeginOperation();

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
KtlLogStreamKernel::AsyncWriteContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncWriteContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncWrite->Cancel();
}


KtlLogStreamKernel::AsyncWriteContextKernel::AsyncWriteContextKernel()
{
}

KtlLogStreamKernel::AsyncWriteContextKernel::~AsyncWriteContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncWriteContext(__out AsyncWriteContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncWriteContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;
    context->_WriteOperation.SetInstrumentedComponent(*_WriteIC);
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Allocate Reservation Context
//
VOID
KtlLogStreamKernel::AsyncReservationContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    _BaseAsyncReservation = nullptr;
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::ReservationSizeChanged(
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
#ifdef UPASSTHROUGH
        _LogStream->AdjustReservationSpace(_ReservationDelta);      
#endif
    }

    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif

#ifdef UPASSTHROUGH
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
#endif    
    status = _LogStream->_BaseLogStream->CreateUpdateReservationContext(_BaseAsyncReservation);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncReservationContextKernel::ReservationSizeChanged);
    _BaseAsyncReservation->StartUpdateReservation(_ReservationDelta,
                                                  this,
                                                  completion);
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::StartUpdateReservation(
    __in LONGLONG ReservationDelta,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{   
    _ReservationDelta = ReservationDelta;
    
    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::StartExtendReservation(
    __in ULONGLONG ReservationSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
#if defined(UDRIVER) || defined(KDRIVER)    
    //
    // All calls to extend or contract should go through
    // StartUpdateReservation()
    KInvariant(FALSE);
#endif

    StartUpdateReservation((LONGLONG)ReservationSize, ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::StartContractReservation(
    __in ULONGLONG ReservationSize,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
#if defined(UDRIVER) || defined(KDRIVER)    
    //
    // All calls to extend or contract should go through
    // StartUpdateReservation()
    KInvariant(FALSE);
#endif
    LONGLONG reservationSize = -1 * ((LONGLONG)ReservationSize);    
    StartUpdateReservation(reservationSize, ParentAsyncContext, CallbackPtr);   
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncReservationContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncReservation->Cancel();
}


KtlLogStreamKernel::AsyncReservationContextKernel::AsyncReservationContextKernel()
{
}

KtlLogStreamKernel::AsyncReservationContextKernel::~AsyncReservationContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncReservationContext(__out AsyncReservationContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncReservationContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReservationContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

ULONGLONG
KtlLogStreamKernel::QueryReservationSpace()
{
    ULONGLONG reservationSpace = 0;
    
#if defined(KDRIVER) || defined(UDRIVER)
    //
    // This is implemented on the user mode side only
    //
    KInvariant(FALSE);
#endif

#ifdef UPASSTHROUGH
    reservationSpace = _ReservationSpaceHeld;
#endif
    
    return(reservationSpace);
}



//
// Read Record
//
VOID
KtlLogStreamKernel::AsyncReadContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    _BaseAsyncRead = nullptr;
}

VOID
KtlLogStreamKernel::AsyncReadContextKernel::ReadCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    //
    // Copy output from KBuffer to KIoBuffer
    //
    KIoBuffer::SPtr metaDataKIoBuffer;
    PVOID metaDataBuffer;
    ULONG metaDataSize = _MetaDataKBuffer->QuerySize();

    //
    // Allocate extra space for the presumed metadata overhead of the
    // core logger so that the caller will find its metadata at the
    // offset expected.
    //
    ULONG bytesToSkip = _LogStream->_CoreLoggerMetadataOverhead;

    HRESULT hr;
    ULONG result;
    hr = ULongAdd(metaDataSize, bytesToSkip, &result);
    KInvariant(SUCCEEDED(hr));
    status = KIoBuffer::CreateSimple(result,
                                     metaDataKIoBuffer,
                                     metaDataBuffer,
                                     GetThisAllocator(),
                                     GetThisAllocationTag());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    KMemCpySafe(
        (PUCHAR)(metaDataBuffer) + bytesToSkip, 
        metaDataKIoBuffer->QuerySize() - bytesToSkip,
        _MetaDataKBuffer->GetBuffer(), 
        metaDataSize);

    *_MetaDataBuffer = metaDataKIoBuffer;   

    //
    // Retrieve the actual metadata size
    //
    struct KtlLogVerify::KtlMetadataHeader* metadataHeader;
    metadataHeader = (struct KtlLogVerify::KtlMetadataHeader*)_MetaDataKBuffer->GetBuffer();
    *_MetaDataLength = metadataHeader->ActualMetadataSize;
    
    //
    // Validate data read
    //
    status = KtlLogVerify::VerifyRecordCallback((UCHAR const * const)_MetaDataKBuffer->GetBuffer(),
                                                    _MetaDataKBuffer->QuerySize(),
                                                    *_IoBuffer);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    Complete(STATUS_SUCCESS);
}

VOID
KtlLogStreamKernel::AsyncReadContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
    KInvariant(_ReadType != RvdLogStream::AsyncReadContext::ReadTypeNotSpecified);

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif  
        
    status = _LogStream->_BaseLogStream->CreateAsyncReadContext(_BaseAsyncRead);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncReadContextKernel::ReadCompleted);
    _BaseAsyncRead->StartRead(_RecordAsn,
                                _ReadType,
                                _ActualReadAsnPtr,
                                _Version,
                                _MetaDataKBuffer,
                                *_IoBuffer,
                                this,
                                completion);
}

VOID
KtlLogStreamKernel::AsyncReadContextKernel::StartRead(
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
KtlLogStreamKernel::AsyncReadContextKernel::StartReadPrevious(
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
KtlLogStreamKernel::AsyncReadContextKernel::StartReadNext(
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
KtlLogStreamKernel::AsyncReadContextKernel::StartReadContaining(
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
KtlLogStreamKernel::AsyncReadContextKernel::ReadContainingAsync(
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
KtlLogStreamKernel::AsyncReadContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
    _ReadType = RvdLogStream::AsyncReadContext::ReadTypeNotSpecified;
}

VOID
KtlLogStreamKernel::AsyncReadContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncRead->Cancel();
}

KtlLogStreamKernel::AsyncReadContextKernel::AsyncReadContextKernel()
{
}

KtlLogStreamKernel::AsyncReadContextKernel::~AsyncReadContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncReadContext(__out AsyncReadContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncReadContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Multi Record Read Record
// 
VOID
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    _BaseAsyncRead = nullptr;
}

VOID
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::ReadCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status = Async.Status();
    
    if (! NT_SUCCESS(status))
    {
        if (status == STATUS_NOT_FOUND)
        {
            KDbgCheckpointWDataInformational(KLoggerUtilities::ActivityIdFromStreamId(_LogStream->_LogStreamId),
                                "AsyncMultiRecordReadContextUser", status, (ULONGLONG)this, _RecordAsn.Get(), 0, 0);
        } else {        
            KTraceFailedAsyncRequest(status, this, _RecordAsn.Get(), 0);
        }
                
        Complete(status);
        return;
    }

    Complete(STATUS_SUCCESS);
}

VOID
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::OnStart(
    )
{
    NTSTATUS status;

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
    
    //
    // Ensure that passed KIoBuffers have only a single KIoBufferElement
    //
    if ((_MetaDataBuffer->QueryNumberOfIoBufferElements() != 1) ||
        (_IoBuffer->QueryNumberOfIoBufferElements() > 1))
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
#endif
    
    status = _LogStream->_BaseLogStream->CreateAsyncMultiRecordReadContextOverlay(_BaseAsyncRead);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }

    
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::ReadCompleted);
    _BaseAsyncRead->StartRead(_RecordAsn,
                                *_MetaDataBuffer,
                                *_IoBuffer,
                                this,
                                completion);
}

VOID
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::StartRead(
    __in KtlLogAsn RecordAsn,
    __inout KIoBuffer& MetaDataBuffer,
    __inout KIoBuffer& IoBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _RecordAsn = RecordAsn;
    _MetaDataBuffer = &MetaDataBuffer,
    _IoBuffer = &IoBuffer;  

    Start(ParentAsyncContext, CallbackPtr); 
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::ReadAsync(
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
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _BaseAsyncRead->Cancel();
}

KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::AsyncMultiRecordReadContextKernel()
{
}

KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::~AsyncMultiRecordReadContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncMultiRecordReadContext(__out AsyncMultiRecordReadContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncMultiRecordReadContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncMultiRecordReadContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Truncate
//
VOID
KtlLogStreamKernel::AsyncTruncateContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncTruncateContextKernel::OnStart(
    )
{
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    _LogStream->_BaseLogStream->Truncate(_TruncationPoint,
                                         _PreferredTruncationPoint);
    
    Complete(STATUS_SUCCESS);
}

VOID
KtlLogStreamKernel::AsyncTruncateContextKernel::StartTruncate(
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
KtlLogStreamKernel::AsyncTruncateContextKernel::Truncate(
    __in KtlLogAsn TruncationPoint,
    __in KtlLogAsn PreferredTruncationPoint
    )
{
#if defined(UDRIVER) || defined(KDRIVER)
    KInvariant(FALSE);
#endif

    StartTruncate(TruncationPoint, PreferredTruncationPoint, nullptr, nullptr);     
}

VOID
KtlLogStreamKernel::AsyncTruncateContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncTruncateContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
}

KtlLogStreamKernel::AsyncTruncateContextKernel::AsyncTruncateContextKernel()
{
}

KtlLogStreamKernel::AsyncTruncateContextKernel::~AsyncTruncateContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncTruncateContext(
    __out AsyncTruncateContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncTruncateContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncTruncateContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

//
// Metadata
//
NTSTATUS
KtlLogStreamKernel::GenerateMetadataFileName(
    __out KWString& FileName
    )
{
    NTSTATUS status;
    KtlLogStreamId logStreamId;
    KGuid logStreamGuid;
    KString::SPtr guidString;
    BOOLEAN b;

    //
    // Filename expected to be of the form:
    //    "\GLOBAL??\Volume{78572cc3-1981-11e2-be6c-806e6f6e6963}\RvdLog\Log{39a26fb9-eaf6-49d8-8432-cf6d9fb9b5e2}.log"


    guidString = KString::Create(GetThisAllocator(),
                                    40);                   // Guid length
    if (! guidString)
    {
        KTraceOutOfMemory( 0, STATUS_INSUFFICIENT_RESOURCES, 0, 0, 0);
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    b = guidString->FromGUID(_DiskId);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   
        
    FileName = L"\\GLOBAL??\\Volume";
    FileName += static_cast<WCHAR*>(*guidString);


    QueryLogStreamId(logStreamId);
    logStreamGuid = logStreamId.GetReference();
    
    b = guidString->FromGUID(logStreamGuid);
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }

    b = guidString->SetNullTerminator();
    if (! b)
    {
        return(STATUS_UNSUCCESSFUL);
    }   
    
    FileName += L"\\RvdLog\\Stream";
    FileName += static_cast<WCHAR*>(*guidString);
    FileName += L".Metadata";
    status = FileName.Status();
    
    return(status);
}

//
// Write Metadata
//
VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif

#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::WriteMetadataCompleted(
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
    }

    _MetadataBuffer = nullptr;

    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::OnStart(
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncWriteMetadataContextKernel::WriteMetadataCompleted);

#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    _AsyncWriteFileContext->StartWriteMetadata(_MetadataBuffer,
                                               this,
                                               completion);
}

VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::StartWriteMetadata(
    __in const KIoBuffer::SPtr& MetadataBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _MetadataBuffer = MetadataBuffer;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
    _AsyncWriteFileContext->Reuse();
}

VOID
KtlLogStreamKernel::AsyncWriteMetadataContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _AsyncWriteFileContext->Cancel();
}


KtlLogStreamKernel::AsyncWriteMetadataContextKernel::AsyncWriteMetadataContextKernel()
{
}

KtlLogStreamKernel::AsyncWriteMetadataContextKernel::~AsyncWriteMetadataContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncWriteMetadataContext(
    __out AsyncWriteMetadataContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncWriteMetadataContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncWriteMetadataContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());
    status = _BaseLogStream->CreateAsyncWriteMetadataContext(context->_AsyncWriteFileContext);
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
KtlLogStreamKernel::AsyncReadMetadataContextKernel::OnCompleted(
    )
{
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncReadMetadataContextKernel::ReadMetadataCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();
        
    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncReadMetadataContextKernel::OnStart(
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncReadMetadataContextKernel::ReadMetadataCompleted);
        
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    _AsyncReadFileContext->StartReadMetadata(*_MetadataBuffer,
                                             this,
                                             completion);
}

VOID
KtlLogStreamKernel::AsyncReadMetadataContextKernel::StartReadMetadata(
    __out KIoBuffer::SPtr& MetadataBuffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{

    _MetadataBuffer = &MetadataBuffer;

    Start(ParentAsyncContext, CallbackPtr); 
}

VOID
KtlLogStreamKernel::AsyncReadMetadataContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
    _AsyncReadFileContext->Reuse();
}

VOID
KtlLogStreamKernel::AsyncReadMetadataContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _AsyncReadFileContext->Cancel();
}

KtlLogStreamKernel::AsyncReadMetadataContextKernel::AsyncReadMetadataContextKernel()
{
}

KtlLogStreamKernel::AsyncReadMetadataContextKernel::~AsyncReadMetadataContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncReadMetadataContext(
    __out AsyncReadMetadataContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncReadMetadataContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncReadMetadataContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_ActiveContext.SetParent(context.RawPtr());
    status = _BaseLogStream->CreateAsyncReadMetadataContext(context->_AsyncReadFileContext);
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
KtlLogStreamKernel::AsyncIoctlContextKernel::OnCompleted(
    )
{    
#ifdef UPASSTHROUGH
    if (_IsOnActiveList)
    {
        BOOLEAN b = _LogStream->RemoveActiveContext(_ActiveContext);
        KInvariant(b);
    }
#endif
    
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    
    _InBuffer = nullptr;
}

VOID
KtlLogStreamKernel::AsyncIoctlContextKernel::IoctlCompleted(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    
    NTSTATUS status;

    status = Async.Status();
        
    Complete(status);
}

VOID
KtlLogStreamKernel::AsyncIoctlContextKernel::OnStart(
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncIoctlContextKernel::IoctlCompleted);
        
#ifdef UPASSTHROUGH
    _IsOnActiveList = FALSE;
    NTSTATUS status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
        return;
    }
    _IsOnActiveList = TRUE;
#endif
    
    _AsyncIoctlContext->StartIoctl(_ControlCode,
                                      _InBuffer.RawPtr(),
                                      *_Result,
                                      *_OutBuffer,
                                      this,
                                      completion);
}

VOID
KtlLogStreamKernel::AsyncIoctlContextKernel::StartIoctl(
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
KtlLogStreamKernel::AsyncIoctlContextKernel::IoctlAsync(
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
KtlLogStreamKernel::AsyncIoctlContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
    _AsyncIoctlContext->Reuse();
}

VOID
KtlLogStreamKernel::AsyncIoctlContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    _AsyncIoctlContext->Cancel();
}

KtlLogStreamKernel::AsyncIoctlContextKernel::AsyncIoctlContextKernel()
{
}

KtlLogStreamKernel::AsyncIoctlContextKernel::~AsyncIoctlContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncIoctlContext(
    __out AsyncIoctlContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncIoctlContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncIoctlContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_ActiveContext.SetParent(context.RawPtr());
    status = _BaseLogStream->CreateAsyncIoctlContext(context->_AsyncIoctlContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    context->_LogStream = this;
    
    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

   

//
// Close Stream
//
VOID
KtlLogStreamKernel::AsyncCloseContextKernel::OnCompleted(
    )
{
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
    
    _BaseAsyncCloseStream = nullptr;
    
#ifdef UPASSTHROUGH
    if (_CloseCallbackPtr)
    {
        _CloseCallbackPtr(_ParentAsyncContext,
            *up_cast<KtlLogStream, KtlLogStreamKernel>(_LogStream),
            Status());
    }
    
    _LogStream->_LogContainer->RemoveActiveContext(_LogStream->_ActiveContext);
    _LogStream->_IsOnContainerActiveList = FALSE;
#endif
}

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::FSMContinue(
    __in NTSTATUS Status
    )
{
    KAsyncContextBase::CompletionCallback completion(this, &KtlLogStreamKernel::AsyncCloseContextKernel::OperationCompletion);

    KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStream->_LogStreamId),
                        "KtlLogStreamKernel::AsyncCloseContextKernel", Status,
                        _State, (ULONGLONG)this, 0, 0);
    
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
            _State = StartOperationDrain;
            
            //
            // Take lock here to ensure that no new requests get added to the
            // stream after marking the stream as closed
            //
            K_LOCK_BLOCK(_LogStream->_ThisLock)
            {
                if (_LogStream->_IsClosed)
                {
                    //
                    // Since this shim object was already closed there
                    // is nothing to do
                    //
                    _State = Completed;
#if UPASSTHROUGH
                    Complete(STATUS_OBJECT_NO_LONGER_EXISTS);
#else
                    Complete(STATUS_SUCCESS);
#endif
                    return;
                }
                
                //
                // Indicate that the stream is closed and no new requests should be
                // added
                //
                _LogStream->_IsClosed = TRUE;

#ifdef UPASSTHROUGH
                _LogStream->_ReservationSpaceHeld = 0;
#endif
                //
                // Set flag so that when list is empty the event is signalled.
                // Functionality for Close is to wait for all operations to be
                // drained.
                //
                _LogStream->_ContextList.SignalWhenEmpty();
            }

            //
            // Cancel all of the outstanding threshold contexts
            //
            _LogStream->CancelAllThresholdContexts();

#if 0
            if (_Abort)
            {
                // TODO: abort all other requests
            }
#endif

            _WaitContext->StartWaitUntilSet(this,
                                            completion);

            break;
        }   

        case StartOperationDrain:
        {
            //
            // Now close overlay stream downstream
            //
            _State = CloseOverlayStream;

            _LogStream->_BaseLogStream->RemoveKtlLogStream(*_LogStream);

            if (! _LogStream->_BaseLogStream->IsClosed())
            {
                Status = _LogStream->_BaseLogStream->GetOverlayLog()->CreateAsyncCloseLogStreamContext(_BaseAsyncCloseStream);
                if (! NT_SUCCESS(Status))
                {
                    KTraceFailedAsyncRequest(Status, this, _State, 0);
                    _State = CompletedWithError;
                    Complete(Status);
                    return;
                }        
            
                _BaseAsyncCloseStream->StartCloseLogStream(_LogStream->_BaseLogStream,
                                                           this,
                                                           completion);
            } else {
                Status = STATUS_OBJECT_NO_LONGER_EXISTS;
                KTraceFailedAsyncRequest(Status, this, _State, 0);
                _State = CompletedWithError;
                Complete(Status);
                return;
            }
            
            break;
        }

        case CloseOverlayStream:
        {
            Complete(STATUS_SUCCESS);
            break;
        }

        
        default:
        {
            KInvariant(FALSE);
            
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            
            _State = CompletedWithError;
            Complete(STATUS_UNSUCCESSFUL);
            return;
        }
    }           
}

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::OnStart(
    )
{
    _State = Initial;
    
    FSMContinue(STATUS_SUCCESS);
}

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    Start(ParentAsyncContext, CallbackPtr); 
}

#ifdef UPASSTHROUGH
VOID
KtlLogStreamKernel::AsyncCloseContextKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
    )
{
    _CloseCallbackPtr = CloseCallbackPtr;
    _ParentAsyncContext = ParentAsyncContext;
    Start(ParentAsyncContext, nullptr); 
}
#endif

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::OperationCompletion(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);
    FSMContinue(Async.Status());
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamKernel::AsyncCloseContextKernel::CloseAsync(
    __in_opt KAsyncContextBase* const ParentAsyncContext
    )
{
    NTSTATUS status;
    ktl::kasync::InplaceStartAwaiter awaiter(*((KAsyncContextBase*)this), ParentAsyncContext, NULL);

    status = co_await awaiter;
    co_return status;
}
#endif

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::OnReuse(
    )
{
    _WaitContext->Reuse();
}

VOID
KtlLogStreamKernel::AsyncCloseContextKernel::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

KtlLogStreamKernel::AsyncCloseContextKernel::AsyncCloseContextKernel()
{
#ifdef UPASSTHROUGH
    _CloseCallbackPtr = NULL;
    _ParentAsyncContext = NULL;
#endif
}

KtlLogStreamKernel::AsyncCloseContextKernel::~AsyncCloseContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncCloseContext(__out AsyncCloseContextKernel::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr context;

    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncCloseContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    status = _ContextList.CreateWaitContext(GetThisAllocator(),
                                            GetThisAllocationTag(),
                                            context->_WaitContext);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context = Ktl::Move(context);
    
    return(STATUS_SUCCESS); 
}

NTSTATUS
KtlLogStreamKernel::StartClose(
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CloseCompletionCallback CloseCallbackPtr
)
{
#ifdef UPASSTHROUGH
    NTSTATUS status;
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (NT_SUCCESS(status))
    {
        closeContext->_LogStream = this;
        closeContext->StartClose(ParentAsyncContext,
                                 CloseCallbackPtr);
    }
    
    return(status);
#endif
#if defined(UDRIVER) || defined(KDRIVER)
    UNREFERENCED_PARAMETER(ParentAsyncContext);
    UNREFERENCED_PARAMETER(CloseCallbackPtr);
    
    KInvariant(FALSE);
    return(STATUS_SUCCESS);
#endif
}

#if defined(K_UseResumable)
ktl::Awaitable<NTSTATUS>
KtlLogStreamKernel::CloseAsync(__in_opt KAsyncContextBase* const ParentAsyncContext)
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncCloseContextKernel::SPtr closeContext;

    status = CreateAsyncCloseContext(closeContext);
    if (!NT_SUCCESS(status))
    {
        co_return status;
    }
    
    closeContext->_LogStream = this;
    co_return co_await closeContext->CloseAsync(ParentAsyncContext);
}
#endif

//
// StreamNotification
//
NTSTATUS
KtlLogStreamKernel::AddActiveContext(
    __in ActiveContext& Context
)
{
    NTSTATUS status = STATUS_SUCCESS;
    
    K_LOCK_BLOCK(_ThisLock)
    {
        KInvariant(! _ContextList.IsOnList(&Context));
        
        //
        // Stream is already closed, we don't want to move forward here
        //
        if (_IsClosed)
        {
            status = STATUS_OBJECT_NO_LONGER_EXISTS;
#ifdef VERBOSE
            KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                                "Stream Already Closed", status,
                                (ULONGLONG)Context.GetMarshaller()->GetRequestId(),
                                (ULONGLONG)this,
                                (ULONGLONG)Context,
                                (ULONGLONG)Context.GetMarshaller());
#endif
            return(status);
        }

        if (Context.IsCancelled())
        {
#ifdef VERBOSE
            KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                                "Context Already Cancelled", status,
                                (ULONGLONG)Context.GetMarshaller()->GetRequestId(),
                                (ULONGLONG)this,
                                (ULONGLONG)Context,
                                (ULONGLONG)Context.GetMarshaller());
#endif
            return(STATUS_CANCELLED);
        }
        
        //
        // Put the request on the list
        //
        _ContextList.Add(&Context);
        status = STATUS_SUCCESS;
    }
    
    return(status);
}

BOOLEAN
KtlLogStreamKernel::RemoveActiveContextNoSpinLock(
    __in ActiveContext& Context
)
{
    BOOLEAN b;

    //
    // NOTE: This routine assumes that the caller holds the spinlock
    //
    
    b = _ContextList.Remove(&Context);
    
    return(b);
}

BOOLEAN
KtlLogStreamKernel::RemoveActiveContext(
    __in ActiveContext& Context
)
{
    BOOLEAN b = FALSE;

    K_LOCK_BLOCK(_ThisLock)
    {
        b = _ContextList.Remove(&Context);
    }
    
    return(b);
}

VOID
KtlLogStreamKernel::SetCancelFlag(
    __in ActiveContext& Context
)
{
    K_LOCK_BLOCK(_ThisLock)
    {
        Context.SetCancelFlag();
    }   
}


VOID
KtlLogStreamKernel::CheckThresholdContexts(
    __in ULONG Percentage,
    __in NTSTATUS CompletionStatus
)
{
    UNREFERENCED_PARAMETER(Percentage);

    ActiveContext* context = NULL;
    ActiveContext* nextContext = NULL;
        
    K_LOCK_BLOCK(_ThisLock)
    {
        context = _ContextList.PeekHead();
        while (context)
        {
            nextContext = _ContextList.Next(context);
            
            if (context->GetMarshaller()->GetObjectMethod() == RequestMarshaller::StreamNotification)
            {
                AsyncStreamNotificationContextKernel::SPtr notificationContext;
                notificationContext = (AsyncStreamNotificationContextKernel*)context->GetParent();
                
                if (notificationContext->_ThresholdValue <= Percentage)
                {
                    BOOLEAN b;
                    
                    b = RemoveActiveContextNoSpinLock(*context);
                    if (b)
                    {
#ifdef VERBOSE
                        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                                            "Threshold context completing", CompletionStatus,
                                            (ULONGLONG)context->GetMarshaller()->GetRequestId(),
                                            (ULONGLONG)this,
                                            (ULONGLONG)notificationContext->_ThresholdValue,
                                            (ULONGLONG)Percentage);
#endif
                        notificationContext->Complete(CompletionStatus);
                        //
                        // NOTE: context and notificationContext are no
                        //       longer valid.
                    } else {
                        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                                            "Threshold context not found", STATUS_NOT_FOUND,
                                            (ULONGLONG)context->GetMarshaller()->GetRequestId(),
                                            (ULONGLONG)this,
                                            (ULONGLONG)context,
                                            (ULONGLONG)context->GetMarshaller());                                
                    }
                }
            }
            
            context = nextContext;
        }
    }   
}

VOID
KtlLogStreamKernel::CancelAllThresholdContexts(
    )
{
    CheckThresholdContexts(100,                 // 100% - all get triggered
                           STATUS_CANCELLED);   // Complete with STATUS_CANCELLED
}

VOID
KtlLogStreamKernel::AsyncStreamNotificationContextKernel::OnCompleted(
    )
{
#if defined(UDRIVER) || defined(KDRIVER)
    //
    // Remove the ref to the log stream here so that destructor for
    // log stream will be run as part of the request completion
    // instead of when this context is destructed. This is needed so
    // that we can be sure that the log stream is cleaned up before
    // the request completes
    //
    _LogStream = nullptr;
#endif
}

VOID
KtlLogStreamKernel::AsyncStreamNotificationContextKernel::OnStart(
    )
{
    NTSTATUS status;
    
    //
    // Verify parameters passed
    //
    if (_ThresholdType != KtlLogStream::AsyncStreamNotificationContext::LogSpaceUtilization)
    {
        status = STATUS_INVALID_PARAMETER;
        KTraceFailedAsyncRequest(status, this, _ThresholdType, _ThresholdValue);
        Complete(status);
        return;
    } else {
        if (_ThresholdValue > 100)
        {
            status = STATUS_INVALID_PARAMETER;
            KTraceFailedAsyncRequest(status, this, _ThresholdType, _ThresholdValue);
            Complete(status);
            return;
        }
    }

    //
    // Check for current utilitzation to see if async should
    // be completed right away.    
    ULONG percentageUsed;
    status = _LogStream->_BaseLogStream->ComputeLogPercentageUsed(percentageUsed);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _ThresholdType, _ThresholdValue);
        Complete(status);
        return;
    }

    if (percentageUsed >= _ThresholdValue)
    {
        status = STATUS_SUCCESS;
#ifdef VERBOSE
        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStreamId),
                            "Complete Log Usage notification immediately", status, (ULONGLONG)this, _ThresholdValue, percentageUsed, 0);
#endif
        Complete(status);
        return;
    }

#ifdef UPASSTHROUGH
    _ActiveContext.GetMarshaller()->SetObjectMethod(RequestMarshaller::OBJECT_METHOD::StreamNotification);
#endif
    
    //
    // Add this to the list of contexts that are waiting
    //
    status = _LogStream->AddActiveContext(_ActiveContext);
    if (! NT_SUCCESS(status))
    {
        //
        // Request must have been cancelled before we got here
        //
        KTraceFailedAsyncRequest(status, this, 0, 0);          
        Complete(status);
    }
}

VOID
KtlLogStreamKernel::AsyncStreamNotificationContextKernel::StartWaitForNotification(
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
KtlLogStreamKernel::AsyncStreamNotificationContextKernel::OnReuse(
    )
{
    KInvariant( ! _LogStream->_ContextList.IsOnList(&_ActiveContext));
}

VOID
KtlLogStreamKernel::AsyncStreamNotificationContextKernel::OnCancel(
    )
{
    NTSTATUS status = STATUS_CANCELLED;
    BOOLEAN b;

    KTraceCancelCalled(this, FALSE, FALSE, _ActiveContext.GetMarshaller()->GetRequestId());
    
    // This sets the cancel flag associated with the active
    // context. If the context has not been added to the list then
    // AddActiveContext will see the flag and cancel the context.
    _LogStream->SetCancelFlag(_ActiveContext);

    b = _LogStream->RemoveActiveContext(_ActiveContext);
    if (b)
    {
#ifdef VERBOSE
        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStream->_LogStreamId),
                            "Threshold context cancelled", status,
                            (ULONGLONG)_ActiveContext.GetMarshaller()->GetRequestId(),
                            (ULONGLONG)this,
                            (ULONGLONG)_ActiveContext,
                            (ULONGLONG)_ActiveContext.GetMarshaller());
#endif
        Complete(status);
    } else {
        KDbgCheckpointWData(KLoggerUtilities::ActivityIdFromStreamId(_LogStream->_LogStreamId),
                            "Threshold context not found at cancel", STATUS_NOT_FOUND,
                            (ULONGLONG)_ActiveContext.GetMarshaller()->GetRequestId(),
                            (ULONGLONG)this,
                            (ULONGLONG)&_ActiveContext,
                            (ULONGLONG)_ActiveContext.GetMarshaller());   
    }
}

KtlLogStreamKernel::AsyncStreamNotificationContextKernel::AsyncStreamNotificationContextKernel()
{
}

KtlLogStreamKernel::AsyncStreamNotificationContextKernel::~AsyncStreamNotificationContextKernel()
{
}

NTSTATUS
KtlLogStreamKernel::CreateAsyncStreamNotificationContext(__out AsyncStreamNotificationContext::SPtr& Context
    )
{
    NTSTATUS status;
    KtlLogStreamKernel::AsyncStreamNotificationContextKernel::SPtr context;

    KInvariant(_BaseLogStream != nullptr);
    
    context = _new(GetThisAllocationTag(), GetThisAllocator()) AsyncStreamNotificationContextKernel();
    if (context == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory( 0, status, _BaseLogStream.RawPtr(), GetThisAllocationTag(), 0);
        return(status);
    }

    status = context->Status();
    if (!NT_SUCCESS(status))
    {
        return(status);
    }

    context->_ActiveContext.SetParent(context.RawPtr());    
    context->_LogStream = this;

    Context = context.RawPtr();
    
    return(STATUS_SUCCESS); 
}

