/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    khttpserver.cpp

    Description:
      Kernel Tempate Library (KTL): HTTP Package

      Describes KTL-based HTTP Server

    History:
      raymcc          3-Mar-2012         Initial version.
      alanwar         1-Nov-2013         Added multipart support, tracing
                                         code cleanup

    Known issues:
      [1] HTTP.SYS filtering the fragment portion of URI
      [2] Test the Transfer-Encoding header on both ends
      [3] Excessively long header value check (cookies from BING, etc.)
--*/

#include <ktl.h>
#include <ktrace.h>


// No kernel mode implementation for now

#if KTL_USER_MODE

#if NTDDI_VERSION >= NTDDI_WIN8
#include <websocket.h>
#endif

BOOLEAN
KInterlockedSetFlag(
    __in volatile ULONG& Flag
    )
{
    ULONG currentFlag = Flag;  //  Snap the current value of Flag atomically
    ULONG newFlag = TRUE;
    ULONG icx = InterlockedCompareExchange(&Flag, newFlag, currentFlag);

    return !icx && icx == currentFlag;  //  Return whether the value changed from FALSE to TRUE
}


#pragma region khttpServerRequest
class KHttpServerRequestImpl : public KHttpServerRequest, public KHttp_IOCP_Handler
{
    K_FORCE_SHARED(KHttpServerRequestImpl);

public:
    KHttpServerRequestImpl(
        __in KSharedPtr<KHttpServer> Parent
        ) 
        :
            KHttpServerRequest(Parent)
    {
        _Overlapped._Handler = this;
    }

    VOID
    IOCP_Completion(
        __in ULONG Error,
        __in ULONG BytesTransferred
        ) override;
};


//
//   KHttpServerRequest::Create
//
//   Creates a new request.
//
NTSTATUS
KHttpServerRequest::Create(
    __in   KSharedPtr<KHttpServer> Parent,
    __in   KAllocator& Allocator,
    __out  KHttpServerRequest::SPtr& NewRequest
    )
{
    NTSTATUS status;
    KHttpServerRequest::SPtr tmp = _new(KTL_TAG_HTTP, Allocator) KHttpServerRequestImpl(Parent);

    if (! tmp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, sizeof(KHttpServerRequestImpl), Parent);
        return status;
    }

    status = tmp->Initialize();
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    NewRequest = Ktl::Move(tmp);

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpServerRequest::Initialize()
{
    //
    // Now allocate the buffer that contains the HTTP_REQUEST struct
    //
    NTSTATUS status = KBuffer::Create(_Parent->GetDefaultHeaderBufferSize(), _RequestStructBuffer, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, _Parent->GetDefaultHeaderBufferSize(), _Parent);
        return status;
    }

    _PHttpRequest = (PHTTP_REQUEST_V2) _RequestStructBuffer->GetBuffer();
    return STATUS_SUCCESS;
}

//
//  KHttpServerRequest::GetOpType
//
KHttpUtils::OperationType
KHttpServerRequest::GetOpType()
{
    KAssert(_PHttpRequest);

    switch (_PHttpRequest->Verb)
    {
        case HttpVerbGET:
            return KHttpUtils::eHttpGet;

        case HttpVerbPOST:
            return KHttpUtils::eHttpPost;

        case HttpVerbPUT:
            return KHttpUtils::eHttpPut;

        case HttpVerbDELETE:
            return KHttpUtils::eHttpDelete;

        case HttpVerbOPTIONS:
            return KHttpUtils::eHttpOptions;

        case HttpVerbHEAD:
            return KHttpUtils::eHttpHead;

        case HttpVerbTRACE:
            return KHttpUtils::eHttpTrace;

        case HttpVerbCONNECT:
            return KHttpUtils::eHttpConnect;

        case HttpVerbTRACK:
            return KHttpUtils::eHttpTrack;

        case HttpVerbMOVE:
            return KHttpUtils::eHttpMove;

        case HttpVerbCOPY:
            return KHttpUtils::eHttpCopy;

        case HttpVerbPROPFIND:
            return KHttpUtils::eHttpPropFind;

        case HttpVerbPROPPATCH:
            return KHttpUtils::eHttpPropPatch;

        case HttpVerbMKCOL:
            return KHttpUtils::eHttpMkcol;

        case HttpVerbSEARCH:
            return KHttpUtils::eHttpSearch;

        case HttpVerbLOCK:
            return KHttpUtils::eHttpLock;

        case HttpVerbUNLOCK:
            return KHttpUtils::eHttpUnlock;
    }

    return KHttpUtils::eUnassigned; // Unknown/unsupported/obscure HTTP verb, or internal error
}

BOOLEAN
KHttpServerRequest::GetOpString(
    __out KString::SPtr& OpStr
)
{
    KAssert(_PHttpRequest);

    LPCWSTR Str = 0;
    NTSTATUS status;

    switch (_PHttpRequest->Verb)
    {
        case HttpVerbGET:
            Str = KHttpUtils::HttpVerb::GET;
            break;

        case HttpVerbPOST:
            Str = KHttpUtils::HttpVerb::POST;
            break;

        case HttpVerbPUT:
            Str = KHttpUtils::HttpVerb::PUT;
            break;

        case HttpVerbDELETE:
            Str = KHttpUtils::HttpVerb::DEL;
            break;

        case HttpVerbOPTIONS:
            Str = KHttpUtils::HttpVerb::OPTIONS;
            break;

        case HttpVerbHEAD:
            Str = KHttpUtils::HttpVerb::HEAD;
            break;

        case HttpVerbTRACE:
            Str = KHttpUtils::HttpVerb::TRACE;
            break;

        case HttpVerbCONNECT:
            Str = KHttpUtils::HttpVerb::CONNECT;
            break;

        case HttpVerbTRACK:
            Str = KHttpUtils::HttpVerb::TRACK;
            break;

        case HttpVerbMOVE:
            Str = KHttpUtils::HttpVerb::MOVE;
            break;

        case HttpVerbCOPY:
            Str = KHttpUtils::HttpVerb::COPY;
            break;

        case HttpVerbPROPFIND:
            Str = KHttpUtils::HttpVerb::PROPFIND;
            break;

        case HttpVerbPROPPATCH:
            Str = KHttpUtils::HttpVerb::PROPPATCH;
            break;

        case HttpVerbMKCOL:
            Str = KHttpUtils::HttpVerb::MKCOL;
            break;

        case HttpVerbSEARCH:
            Str = KHttpUtils::HttpVerb::SEARCH;
            break;

        case HttpVerbLOCK:
            Str = KHttpUtils::HttpVerb::LOCK;
            break;

        case HttpVerbUNLOCK:
            Str = KHttpUtils::HttpVerb::UNLOCK;
            break;

        case HttpVerbUnknown:
        {
            PCHAR Verb = (PCHAR)_PHttpRequest->pUnknownVerb;
            ULONG VerbLength = _PHttpRequest->UnknownVerbLength;
            if (Verb == 0 || VerbLength == 0)
            {
                return FALSE;
            }

            HRESULT hr;
            ULONG result;
            hr = ULongAdd(VerbLength, 1, &result);
            KInvariant(SUCCEEDED(hr));

            KString::SPtr UnknownVerbStr = KString::Create(GetThisAllocator(), result);
            if (!UnknownVerbStr)
            {
                KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, this, result, 0);
                return FALSE;
            }
        
            if (!UnknownVerbStr->CopyFromAnsi(Verb, VerbLength))
            {
                KTraceOutOfMemory(0, STATUS_INTERNAL_ERROR, this, result, 0);
                return FALSE;
            }

            if (!UnknownVerbStr->SetNullTerminator())
            {
                KTraceOutOfMemory(0, STATUS_INTERNAL_ERROR, this, result, 0);
                return FALSE;
            }

            OpStr = UnknownVerbStr;
            return TRUE;
        }

        default:
            // unsupported/obscure HTTP verb, or internal error
            return FALSE;
    }

    status = KString::Create(OpStr, GetThisAllocator(), Str);
    if (!NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, this, 0, 0);
        return FALSE;
    }

    return TRUE;
}

//
//  KHttpServerRequest::GetRequestHeaderByCode
//
NTSTATUS
KHttpServerRequest::GetRequestHeaderByCode(
    __in  KHttpUtils::HttpHeaderType HeaderId,
    __out KString::SPtr& Value
    )
{
    NTSTATUS status;

    KAssert(_PHttpRequest);
    if (HeaderId >= KHttpUtils::HttpHeaderRequestMaximum)
    {
        return STATUS_INVALID_PARAMETER;
    }

    PCHAR Str = (PCHAR) _PHttpRequest->Headers.KnownHeaders[HeaderId].pRawValue;
    ULONG Len = _PHttpRequest->Headers.KnownHeaders[HeaderId].RawValueLength;

    if (Len == 0)
    {
        return STATUS_NOT_FOUND;
    }
    
    // REVIEW: TyAdam, BharatN
    HRESULT hr;
    ULONG result;
    hr = ULongAdd(Len, 1, &result);
    KInvariant(SUCCEEDED(hr));
    KString::SPtr TmpValue = KString::Create(GetThisAllocator(), result);
    
    if (!TmpValue)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, Len+1, 0);
        return(status);
    }

    if (! TmpValue->CopyFromAnsi(Str, Len))
    {
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return(status);
    }

    if (! TmpValue->SetNullTerminator())
    {
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return(status);
    }

    Value = TmpValue;
    return(STATUS_SUCCESS);
}

//
//  KHttpServerRequest::GetRequestHeader
//
//  Retrieves a header by name. Since the headers are not
//  sorted, this must loop through all of them.
//
NTSTATUS
KHttpServerRequest::GetRequestHeader(
    __in  KStringView& RequiredHeader,
    __out KString::SPtr& Value
    )
{
    NTSTATUS status;

    //
    //  Check for match in unknown headers
    //
    KAssert(_PHttpRequest);
    USHORT Count = _PHttpRequest->Headers.UnknownHeaderCount;
    for (USHORT i = 0; i < Count; i++)
    {
        PCHAR HeaderName = (PCHAR) _PHttpRequest->Headers.pUnknownHeaders[i].pName;
        ULONG HeaderLength =  _PHttpRequest->Headers.pUnknownHeaders[i].NameLength;

        if (RequiredHeader.CompareNoCaseAnsi(HeaderName, HeaderLength) == 0)
        {
            ULONG ValueLength =  _PHttpRequest->Headers.pUnknownHeaders[i].RawValueLength;
            // REVIEW: TyAdam, BharatN
            HRESULT hr;
            ULONG result;
            hr = ULongAdd(ValueLength, 1, &result);
            KInvariant(SUCCEEDED(hr));
            KString::SPtr Tmp = KString::Create(GetThisAllocator(), result);
            
            if (! Tmp)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, this, ValueLength + 1, 0);
                return(status);
            }

            if (! Tmp->CopyFromAnsi(_PHttpRequest->Headers.pUnknownHeaders[i].pRawValue, ValueLength))
            {
                status = STATUS_INTERNAL_ERROR;
                KTraceFailedAsyncRequest(status, this, 0, 0);
                return(status);
            }

            if (! Tmp->SetNullTerminator())
            {
                status = STATUS_INTERNAL_ERROR;
                KTraceFailedAsyncRequest(status, this, 0, 0);
                return(status);
            }

            Value = Tmp;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

//
//   KHttpServerRequest::GetAllHeaders
//
//   Primarily reserved for debugging.
//
NTSTATUS
KHttpServerRequest::GetAllHeaders(
    __out KString::SPtr& HeaderBlock
    )
{
    NTSTATUS status;
    KAssert(_PHttpRequest);

    KString::SPtr Tmp = KString::Create(GetThisAllocator(), 32);
    if (! Tmp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 32, 0);
        return(status);
    }

    // Standard headers
    //
    for (ULONG ix = 0; ix < KHttpUtils::HttpHeaderRequestMaximum; ix++)
    {
        PCHAR Value = (PCHAR) _PHttpRequest->Headers.KnownHeaders[ix].pRawValue;
        ULONG ValueLength = _PHttpRequest->Headers.KnownHeaders[ix].RawValueLength;

        if (Value == 0 || ValueLength == 0)
        {
            continue;
        }

        // Conver the abstract id to a string header name.
        KStringView HeaderName;

        if (! KHttpUtils::StdHeaderToString(static_cast<KHttpUtils::HttpHeaderType>(ix), HeaderName))
        {
            status = STATUS_INTERNAL_ERROR;
            KTraceFailedAsyncRequest(status, this, ix, 0);
            return(status);
        }

        // If here, we have a value.

        if (Tmp->FreeSpace() < (HeaderName.Length() + ValueLength + 16))
        {
			HRESULT hr;
			ULONG result;
			hr = ULongMult(Tmp->BufferSizeInChars(), 2, &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, HeaderName.Length(), &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, ValueLength, &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, 16, &result);
			KInvariant(SUCCEEDED(hr));
			
			if (! Tmp->Resize(result))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, this, result, 0);
                return(status);
            }
        }

        Tmp->Concat(HeaderName);
        Tmp->Concat(KStringView(L": "));
        Tmp->ConcatAnsi(Value, ValueLength);
        Tmp->Concat(KStringView(L"\r\n"));

        if (Tmp->FreeSpace() == 0)
        {
            status = STATUS_INTERNAL_ERROR;
            KTraceFailedAsyncRequest(status, this, 0, 0);
            return(status);
        }
    }


    // Custom headers
    //
    USHORT Count = _PHttpRequest->Headers.UnknownHeaderCount;
    for (USHORT i = 0; i < Count; i++)
    {
        PCHAR HeaderName = (PCHAR) _PHttpRequest->Headers.pUnknownHeaders[i].pName;
        ULONG HeaderNameLength =  _PHttpRequest->Headers.pUnknownHeaders[i].NameLength;
        PCHAR Value = (PCHAR) _PHttpRequest->Headers.pUnknownHeaders[i].pRawValue;
        ULONG ValueLength =  _PHttpRequest->Headers.pUnknownHeaders[i].RawValueLength;

        KAssert(HeaderName);
        KAssert(HeaderNameLength);

        // Ensure space for the next header
        //
        if (Tmp->FreeSpace() < (HeaderNameLength + ValueLength + 16))
        {
			HRESULT hr;
			ULONG result;
			hr = ULongMult(Tmp->BufferSizeInChars(), 2, &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, HeaderNameLength, &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, ValueLength, &result);
			KInvariant(SUCCEEDED(hr));
			hr = ULongAdd(result, 16, &result);
			KInvariant(SUCCEEDED(hr));
			
			if (! Tmp->Resize(result))
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, this, result, 0);
                return(status);
            }
        }

        Tmp->ConcatAnsi(HeaderName, HeaderNameLength);
        Tmp->Concat(KStringView(L": "));
        Tmp->ConcatAnsi(Value, ValueLength);
        Tmp->Concat(KStringView(L"\r\n"));

        KAssert(Tmp->FreeSpace() > 0);
    }

    if (! Tmp->SetNullTerminator())
    {
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, this, 0, 0);
        return(status);
    }

    HeaderBlock = Tmp;
    return(STATUS_SUCCESS);
}

//
//   KHttpServerRequest::GetAllHeaders
//
//   Primarily reserved for debugging.
//
NTSTATUS
KHttpServerRequest::GetAllHeaders(
__out KHashTable<KWString, KString::SPtr> &Headers
    )
{
    NTSTATUS status;
    KAssert(_PHttpRequest);

    Headers.Initialize(
        ((ULONG)KHttpUtils::HttpHeaderRequestMaximum + _PHttpRequest->Headers.UnknownHeaderCount),
        K_DefaultHashFunction);

    // Standard headers
    //
    for (ULONG ix = 0; ix < KHttpUtils::HttpHeaderRequestMaximum; ix++)
    {
        PCHAR Value = (PCHAR)_PHttpRequest->Headers.KnownHeaders[ix].pRawValue;
        ULONG ValueLength = _PHttpRequest->Headers.KnownHeaders[ix].RawValueLength;

        if (Value == 0 || ValueLength == 0)
        {
            continue;
        }

        // Convert the abstract id to a string header name.
        KStringView HeaderName;
        if (!KHttpUtils::StdHeaderToString(static_cast<KHttpUtils::HttpHeaderType>(ix), HeaderName))
        {
            status = STATUS_INTERNAL_ERROR;
            KTraceFailedAsyncRequest(status, this, ix, 0);
            return(status);
        }

        // If here, we have a value.
        KWString HeaderWStr(GetThisAllocator());
        HeaderWStr = HeaderName;

        KString::SPtr HeaderValueStr;
        HeaderValueStr = KString::Create(GetThisAllocator(), ValueLength * 2);
        HeaderValueStr->ConcatAnsi(Value, ValueLength);

        Headers.Put(HeaderWStr, HeaderValueStr, 1);
    }

    // Custom headers
    //
    USHORT Count = _PHttpRequest->Headers.UnknownHeaderCount;
    for (USHORT i = 0; i < Count; i++)
    {
        PCHAR HeaderName = (PCHAR)_PHttpRequest->Headers.pUnknownHeaders[i].pName;
        ULONG HeaderNameLength = _PHttpRequest->Headers.pUnknownHeaders[i].NameLength;
        PCHAR Value = (PCHAR)_PHttpRequest->Headers.pUnknownHeaders[i].pRawValue;
        ULONG ValueLength = _PHttpRequest->Headers.pUnknownHeaders[i].RawValueLength;

        KAssert(HeaderName);
        KAssert(HeaderNameLength);

        KWString HeaderNameWStr(GetThisAllocator());
        KString::SPtr HeaderNameStr = KString::Create(GetThisAllocator(), HeaderNameLength * 2);
        HeaderNameStr->ConcatAnsi(HeaderName, HeaderNameLength);
        HeaderNameWStr = *HeaderNameStr;

        KString::SPtr HeaderValueStr;
        HeaderValueStr = KString::Create(GetThisAllocator(), ValueLength * 2);
        HeaderValueStr->ConcatAnsi(Value, ValueLength);

        Headers.Put(HeaderNameWStr, HeaderValueStr, 1);
    }

    return STATUS_SUCCESS;
}

//
//  KHttpServerRequest constructor
//
KHttpServerRequest::KHttpServerRequest(
    __in KSharedPtr<KHttpServer> Parent
    )
    : 
        _ResponseHeaders(GetThisAllocator()),
        _IsUpgraded(FALSE)
{
    NTSTATUS status;

    _Parent = Parent;
    _PHttpRequest = nullptr;

    _RequestEntityLength = 0;
    _RequestContentLengthHeader = 0;

    eState = eWaiting;

    _ResponseCode = KHttpUtils::InternalServerError;   // Default to this unless specifically set
    _ResponseContentCarrier = KHttpUtils::eNone;
    _ResponseContentLengthHeader = 0;
    _ResponsePUnknownHeaders = NULL;
    RtlZeroMemory(&_HttpResponse, sizeof(HTTP_RESPONSE));  //  Required by http.sys api
    RtlZeroMemory(&_ResponseChunk, sizeof(HTTP_DATA_CHUNK));  //  Required by http.sys api
    _ResponseCompleteCallback = 0;
    _ResponseReason = NULL;
    _hAccessToken = INVALID_HANDLE_VALUE;

    status = _ResponseHeaders.Status();
    
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
    }
}

//
//  KHttpServerRequest destructor
//
KHttpServerRequest::~KHttpServerRequest()
{
    if (_hAccessToken != INVALID_HANDLE_VALUE)
    {
        CloseHandle(_hAccessToken);
    }

    // _delete checks for NULL
    _delete(_ResponsePUnknownHeaders);
    _delete(_ResponseReason);
}

//
//  KHttpServerRequest::SetResponseHeader
//
NTSTATUS
KHttpServerRequest::SetResponseHeader(
    __in KStringView& HeaderId,
    __in KStringView& Value
    )
{
    NTSTATUS status;

    if (HeaderId.IsEmpty() || Value.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    HeaderPair::SPtr NewPair = _new(KTL_TAG_HTTP, GetThisAllocator()) HeaderPair();
    if (! NewPair)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
        return(status);
    }

    if (!NewPair->_Id.CopyFrom(HeaderId) ||
        !NewPair->_Value.CopyFrom(Value))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
        return(status);
    }

    status = _ResponseHeaders.Append(NewPair);

    if (status == STATUS_INSUFFICIENT_RESOURCES)
    {
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
    }

    return(status);
}

//
//  KHttpServerRequest::SetResponseHeader
//
NTSTATUS
KHttpServerRequest::SetResponseHeaderByCode(
    __in KHttpUtils::HttpHeaderType HeaderId,
    __in KStringView& Value
    )
{
    NTSTATUS status;

    if (HeaderId >= KHttpUtils::HttpHeaderRequestMaximum || Value.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    HeaderPair::SPtr NewPair = _new(KTL_TAG_HTTP, GetThisAllocator()) HeaderPair();
    if (! NewPair)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
        return(status);
    }

    if (! NewPair->_Value.CopyFrom(Value))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
        return(status);
    }

    NewPair->_HeaderCode = HeaderId;

    status = _ResponseHeaders.Append(NewPair);
    if (status == STATUS_INSUFFICIENT_RESOURCES)
    {
        KTraceOutOfMemory(0, status, this, sizeof(HeaderPair), 0);
    }

    return(status);
}

//
//  KHttpServerRequest::OnStart
//
VOID
KHttpServerRequest::OnStart()
{
    NTSTATUS status;

    eState = eReadHeaders;

    status = ReadHeaders(HTTP_NULL_ID);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
        Complete(status);
    }
}

//
//  TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::ReadHeaders(HTTP_OPAQUE_ID RequestId)
{
    NTSTATUS status;

    _PHttpRequest = (PHTTP_REQUEST_V2) _RequestStructBuffer->GetBuffer();

    ULONG res = HttpReceiveHttpRequest(
                    _Parent->_hRequestQueue,             // Req Queue
                    RequestId,                           // Req ID
                    0,                                   // Headers only! We will allocate for content later
                    (PHTTP_REQUEST) _RequestStructBuffer->GetBuffer(),
                    _RequestStructBuffer->QuerySize(),
                    NULL,
                    &_Overlapped
                    );

    if (res == ERROR_IO_PENDING || res == NO_ERROR)
    {
        return(STATUS_SUCCESS);
    }

    //
    // We should never see this except during shutdown.
    //
    _HttpError = res;
    status = STATUS_INTERNAL_ERROR;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
    return(status);
}

//
// KHttpServerRequest::GetUrl
//
// Retrieves the URL that the request was sent to.
// The returned Url has the segment portions escaped, the hostname or any characters following # or ? (query/fragments) are not escaped.
//
NTSTATUS
KHttpServerRequest::GetUrl(
    __out KUri::SPtr& Url
    )
{
    NTSTATUS Res;

    KAssert(_PHttpRequest);
    DWORD length = KHttpUtils::DefaultFieldLength;
    KDynString Scratch(GetThisAllocator(), KHttpUtils::DefaultFieldLength);

    Res = KNt::UrlEscape(_PHttpRequest->CookedUrl.pFullUrl, Scratch, &length, URL_ESCAPE_AS_UTF8);
    if (Res == STATUS_BUFFER_TOO_SMALL && length > Scratch.BufferSizeInChars())
    {
        // Resize the buffer and try again.
        Scratch.Resize(length);
        Res = KNt::UrlEscape(_PHttpRequest->CookedUrl.pFullUrl, Scratch, &length, URL_ESCAPE_AS_UTF8);
    }

    if (! NT_SUCCESS(Res))
    {
        return Res;
    }

    Scratch.MeasureThis();

    KStringView Tmp(Scratch);
    Res = KUri::Create(Tmp, GetThisAllocator(), Url);
    if (! NT_SUCCESS(Res))
    {
        KTraceOutOfMemory(0, Res, this, 0, 0);
        return Res;
    }

    return STATUS_SUCCESS;
}



//  StartRead
//
//  Posts an initial pending read for incoming requests.
//
NTSTATUS
KHttpServerRequest::StartRead(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase::CompletionCallback Completion
    )
{
    Start(ParentAsync, Completion);
    return STATUS_PENDING;
}

VOID
KHttpServerRequest::FailRequest(
    __in NTSTATUS Status,
    __in KHttpUtils::HttpResponseCode Code
    )
{
    KInvariant(! NT_SUCCESS(Status));

    if (_ASDRContext)
    {
        //
        // If the operation was started via an app async then complete it first
        //
        KInvariant(_HandlerAction != KHttpServer::HeaderHandlerAction::DeliverCertificateAndContent);

        KHttpServerRequest::AsyncResponseDataContext::SPtr asdr = _ASDRContext;
        _ASDRContext = nullptr;
        asdr->Complete(Status);
    }

    if (Code == KHttpUtils::HttpResponseCode::NoResponse)
    {
        //
        // If no response to the request is needed
        //
        Complete(Status);
    } else {
        //
        // Fire off a response to the request
        //
        SendResponse(Code);
    }
}


KHttpServerRequestImpl::~KHttpServerRequestImpl()
{
}

VOID
KHttpServerRequestImpl::IOCP_Completion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    IOCP_Completion_Impl(Error, BytesTransferred);
}

// IOCP_Completion
//
// Called when completion on the read occurs.  This could be called
// multiple times during the read process.
//
// TODO: factor and fix reentrancy/etc. issues
//
VOID
KHttpServerRequest::IOCP_Completion_Impl(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    NTSTATUS status;

//  KDbgCheckpointWData(1, "IOCP_Completion", Error, (ULONGLONG)this, (ULONGLONG)_ASDRContext.RawPtr(), BytesTransferred, eState);

    if ((Error == ERROR_OPERATION_ABORTED) &&
        (eState != eReadCertificate))
    {
        // Shutdown, etc.
        status = STATUS_REQUEST_ABORTED;

        _HttpError = Error;
        KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
        FailRequest(status, KHttpUtils::HttpResponseCode::NoResponse);
        return;
    }

    switch(eState)
    {
        #pragma region case_eReadHeaders
        case eReadHeaders:
        {
            //
            // Initial state of request where we receive the request structure
            // that include the headers
            //
            if (Error == ERROR_MORE_DATA)
            {
                //
                // Not enough room to read all of the headers so retry reading the request with a 
                // larger buffer
                //
                KInvariant(_RequestStructBuffer);
                _RequestStructBuffer = nullptr;
                HTTP_OPAQUE_ID requestId = _PHttpRequest->RequestId;

                status = KBuffer::Create(BytesTransferred, _RequestStructBuffer, GetThisAllocator());
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }

                status = ReadHeaders(requestId);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }

                return;
            }
            else
            {
                //
                // We have received an incoming request and so now gather up 
                // the headers and and prepare the URL
                //
                PCHAR Str = (PCHAR) _PHttpRequest->Headers.KnownHeaders[KHttpUtils::HttpHeaderContentLength].pRawValue;
                ULONG Len = _PHttpRequest->Headers.KnownHeaders[KHttpUtils::HttpHeaderContentLength].RawValueLength;

                if (Len && Str)
                {
                    KLocalString<32> Tmp;
                    Tmp.CopyFromAnsi(Str, Len);
                    Tmp.ToULONG(_RequestContentLengthHeader); // If this fails, it is still ok. We'll just have to allocate blindly.
                }

                //
                // Parse the URL - Get the suffix portion and see if there is a handler for that.
                //
                KUri::SPtr CandidateUrl;
                status = GetUrl(CandidateUrl);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }

                KDynUri Relative(GetThisAllocator());
                status = _Parent->_Url.GetSuffix(*CandidateUrl, Relative);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }

                // Move the contents of Relative to the request object for future use.
                //
                status = KUri::Create(Relative, GetThisAllocator(), _RelativeUrl);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }

                eState = eHeaderHandlerInvoked;

                //
                //  Prevent Complete() triggered by InvokeHeaderHandler (e.g. via SendResponse) from cleaning up the async.  This
                //  is a spot fix, which doesn't address the underlying problems (which will be addressed later via a partial rewrite)
                //
                //  BUG if this fails (due to cleanup racing)
                //
                AcquireActivities();

                //
                //  Release the acquired activity when we return from this function
                //
                KFinally([&]
                {
                    ReleaseActivities();
                });

                //
                //  Invoke the header handler to see how to handle this request
                //
                //  BUG: 3324466 - Improve KHttpServerRequest Quality
                //
                //  TODO: Rewrite this state machine for correctness.  Each call to an asynchronous http.sys API should be wrapped 
                //      in a child async, with state machine transitions occurring only within the request's apartment.  Currently,
                //      this logic is totally broken; as the user header handler may perform arbitrary work, including calling 
                //      SendResponse and advancing the request's state machine, this function may experience unexpected reentrancy
                //      and unexpected state transitions.
                //      Factoring out these different operations will have the added benefit of making the code more readable, 
                //      maintainable, and testable.  The tests should also be extended.
                //
                _Parent->InvokeHeaderHandler(this, _HandlerAction);

                if (_HandlerAction == KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead)
                {
                    //
                    // Handler specified that this is a multipart read and so the handler
                    // is required to start asyncs to read the data and the certificate.
                    // The async's started to read data, certificate and sending response
                    // moves the state machine. So the state machine's state(eState) need
                    // not be updated here.
                    //
                    return;

                } else if (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverCertificateAndContent) {
                    //
                    // Handler specified that it wanted the certificate to be read and cached 
                    // so move to read certificate state
                    // 
                    eState = eReadCertificate;

                    status = ReadCertificate();
                    if (! NT_SUCCESS(status))
                    {
                        //
                        // Failure to read certificate is not fatal
                        //
                        if (status != STATUS_NOT_SUPPORTED)
                        {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                        }

                        eState = eReadBody;

                        status = ReadEntityBody();
                        if (! NT_SUCCESS(status))
                        {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                            FailRequest(status, KHttpUtils::InternalServerError);
                        }
                    }

                    return;
                } else if (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverContent) {
                    if (_PHttpRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
                    {
                        //
                        // There is data associated with the request so move to the next state
                        // where all data is read
                        //
                        eState = eReadBody;

                        status = ReadEntityBody();
                        if (! NT_SUCCESS(status))
                        {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                            FailRequest(status, KHttpUtils::InternalServerError);
                        }

                        return;
                    } else {
                        //
                        // Otherwise there is just headers and so we can invoke the handler for the
                        // request
                        //

                        eState = eReadComplete;

                        _Parent->InvokeHandler(this);
                        return;
                    }
                } else {
                    //
                    // Must be one of these handler actions
                    //
                    KInvariant(FALSE);

                    status = STATUS_INTERNAL_ERROR;
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                    FailRequest(status, KHttpUtils::InternalServerError);

                    return;
                }
            }
        }
        #pragma endregion case_eReadHeaders

        #pragma region case_eReadCertificate
        case eReadCertificate:
        {
            if (Error == ERROR_MORE_DATA)
            {
                //
                // The bufffer wasn't large enough so try again
                //
                status = ReadCertificate();
                if (! NT_SUCCESS(status))
                {
                    if (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverCertificateAndContent)
                    {
                        //
                        // Failure to read certificate is not fatal
                        //
                        if (status != STATUS_NOT_SUPPORTED)
                        {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                        }

                        eState = eReadBody;

                        status = ReadEntityBody();
                        if (! NT_SUCCESS(status))
                        {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                            FailRequest(status, KHttpUtils::InternalServerError);
                        }
                    } else {
                        KInvariant((_HandlerAction == KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead) ||
                                   (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverContent));
                        KInvariant(_ASDRContext);
                        
                        if (status == STATUS_NOT_FOUND)
                        {
                            *_ASDRContext->_Certificate = nullptr;
                            _ClientCertificateInfoBuffer = nullptr;
                            _ASDRContext->Complete(status);
                        } else {
                            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                            FailRequest(status, KHttpUtils::InternalServerError);
                        }
                    }
                }

                return;
            }

            if ((_HandlerAction == KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead) ||
                (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverContent))
            {
                //
                // If reading the cert from one of these handler actions, it means that the
                // application must have specifically started the async to do it.
                //
                KInvariant(_ASDRContext);

                if (Error == ERROR_SUCCESS)
                {
                    *_ASDRContext->_Certificate = _ClientCertificateInfoBuffer;
                    _ClientCertificateInfoBuffer = nullptr;
                    status = STATUS_SUCCESS;
                } else if (Error == ERROR_NOT_FOUND) {
                    *_ASDRContext->_Certificate = nullptr;
                    _ClientCertificateInfoBuffer = nullptr;
                    status = STATUS_NOT_FOUND;
                } else  if (Error == ERROR_OPERATION_ABORTED) {
                    *_ASDRContext->_Certificate = nullptr;
                    _ClientCertificateInfoBuffer = nullptr;
                    status = STATUS_REQUEST_ABORTED;
                } else {
                    //
                    // Some other error
                    //
                    *_ASDRContext->_Certificate = nullptr;
                    _ClientCertificateInfoBuffer = nullptr;

                    status = STATUS_INTERNAL_ERROR;
                    _HttpError = Error;
                    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                }

                _ASDRContext->Complete(status);
                return;

            } else if (_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverCertificateAndContent) {
                if (Error == ERROR_NOT_FOUND)
                {
                    //
                    // No certificate available
                    //
                    _ClientCertificateInfoBuffer = nullptr;
                } else if (Error != NO_ERROR) {
                    //
                    // Although there was an error reading the certificate, we don't
                    // care enough for this handler to fail the request
                    //
                    _HttpError = Error;
                    KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, _HttpError, NULL);
                }

                if (_PHttpRequest->Flags & HTTP_REQUEST_FLAG_MORE_ENTITY_BODY_EXISTS)
                {
                    //
                    // There is data associated with the request so move to the next state
                    // where all data is read
                    //
                    eState = eReadBody;

                    status = ReadEntityBody();
                    if (! NT_SUCCESS(status))
                    {
                        KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                        FailRequest(status, KHttpUtils::InternalServerError);
                    }
                } else {
                    //
                    // Otherwise there is just headers and so we can invoke the handler for the
                    // request
                    //
                    eState = eReadComplete;

                    _Parent->InvokeHandler(this);
                }

                return;
            } else {
                //
                // Must be one of the above handler actions
                //
                KInvariant(FALSE);

                status = STATUS_INTERNAL_ERROR;
                KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                FailRequest(status, KHttpUtils::InternalServerError);

                return;
            }
        }
        #pragma endregion case_eReadCertificate

        #pragma region case_eReadBody
        case eReadBody:
        {
            KInvariant(_HandlerAction != KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);

            _RequestEntityLength += BytesTransferred;

            if (_RequestEntityLength > _Parent->GetMaximumEntityBodySize())
            {
                //
                // If the sender is sending too large of a buffer then 
                // send error response to the requester
                //
                KInvariant(_ResponseContentCarrier == KHttpUtils::eNone);
                status = STATUS_SUCCESS;
                KTraceFailedAsyncRequest(status, this, _RequestEntityLength, _Parent->GetMaximumEntityBodySize());
                FailRequest(status, KHttpUtils::HttpResponseCode::RequestEntityTooLarge);
                return;
            }
                                
            status = ReadEntityBody();
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
                FailRequest(status, KHttpUtils::InternalServerError);
            }

            return;
        }
        #pragma endregion case_eReadBody

        #pragma region case_eReadMultiPart
        case eReadMultiPart:
        {
            KInvariant(_HandlerAction == KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);

            //
            // On success, return the buffer read and complete the async
            //
            if (_ASDRContext->_ContentCarrier == KHttpUtils::ContentCarrierType::eKMemRef)
            {
                _ASDRContext->_ReadContent_KMemRef->_Param = BytesTransferred;
            }
            else if (_ASDRContext->_ContentCarrier == KHttpUtils::ContentCarrierType::eKBuffer)
            {
                //
                // Trim buffer to exact amount read
                //
                status = _ASDRContext->_ReadBuffer->SetSize(BytesTransferred, TRUE);
                if (! NT_SUCCESS(status))
                {
                    KTraceFailedAsyncRequest(status, this, _HttpError, BytesTransferred);
                    FailRequest(status, KHttpUtils::InternalServerError);
                    return;
                }
                *(_ASDRContext->_ReadContent) = _ASDRContext->_ReadBuffer;
                _ASDRContext->_ReadBuffer = nullptr;
            }
            else
            {
                KAssert(false);
            }

            KHttpServerRequest::AsyncResponseDataContext::SPtr asdr = _ASDRContext;
            _ASDRContext = nullptr;
            asdr->Complete(STATUS_SUCCESS);

            return;
        }
        #pragma endregion case_eReadMultiPart

        #pragma region case_eSendHeaders_&_eSendResponseData
        case eSendHeaders:
        case eSendResponseData:
        {
            KInvariant(_ASDRContext);
            KInvariant(_ResponseHeadersSent);

            if (Error == ERROR_SUCCESS)
            {
                KHttpServerRequest::AsyncResponseDataContext::SPtr asdr = _ASDRContext;
                KHttpServerRequest::SPtr r = this;

                asdr->Complete(STATUS_SUCCESS);
                if ((asdr->_Flags & HTTP_SEND_RESPONSE_FLAG_MORE_DATA) == 0)
                {
                    //
                    // if this is the last buffer of data to be sent then complete the
                    // HttpServerRequest
                    //
                    Complete(STATUS_SUCCESS);
                }
            } else {
                status = STATUS_INTERNAL_ERROR;
                _HttpError = Error;
                KTraceFailedAsyncRequest(status, this, _HttpError, eState);
                FailRequest(status, KHttpUtils::NoResponse);
            }

            return;
        }
        #pragma endregion case_eSendHeaders_&_eSendResponseData

        # pragma region case_eSendResponse
        case eSendResponse:
        {
            //
            // Note the result of a send
            //
            if (_ResponseCompleteCallback)
            {
                _ResponseCompleteCallback(this, Error);
            }

            Complete(STATUS_SUCCESS);
            return;
        }
        # pragma endregion case_eSendResponse
    }

    //
    // Should not get here.
    //
    KInvariant(FALSE);

    status = STATUS_INTERNAL_ERROR;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
    Complete(status);
}

//
// KHttpServerRequest::ReadCertificate
//
// Begins/Continues a multi-part certificate read. If this is a multipart read,
// then the size of the encoded cert is larger than the default size that we allocated.
// The actual size of the encoded cert is given in the header's
// CertEncodedSize field.
//
// Return Value:
//
//    STATUS_SUCCESS - Reading certificate has started, IOCP expected to callback
//
//    STATUS_NOT_FOUND - No certificate is available, IOCP not expected to callback
//
//    Any other status - Internal error occured, IOCP not expected to
//                       callback, caller should respond to client with HTTP 5xx error
//
// TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::ReadCertificate()
{
    NTSTATUS status;
    ULONG error;
    ULONG size;

    if (! IsAuthTypeSupported(KHttpUtils::HttpAuthType::AuthCertificate))
    {
        return(STATUS_NOT_SUPPORTED);
    }

    if (! _ClientCertificateInfoBuffer)
    {
        size = KHttpUtils::DefaultClientCertificateSize + sizeof(HTTP_SSL_CLIENT_CERT_INFO);

        status = KBuffer::Create(size, _ClientCertificateInfoBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, size, NULL);
            return(status);
        }
    } else {
        size = ((PHTTP_SSL_CLIENT_CERT_INFO)_ClientCertificateInfoBuffer->GetBuffer())->CertEncodedSize +
                                             sizeof(HTTP_SSL_CLIENT_CERT_INFO);
        //
        // Check if we need to reallocate.
        //
        if (_ClientCertificateInfoBuffer->QuerySize() < size)
        {
            status = _ClientCertificateInfoBuffer->SetSize(size, TRUE);
            if (! NT_SUCCESS(status))
            {
                KTraceFailedAsyncRequest(status, this, size, NULL);
                return(status);
            }
        }
    }
    
    error = HttpReceiveClientCertificate(
                                        _Parent->_hRequestQueue,
                                        _PHttpRequest->ConnectionId,
                                        0,
                                        (PHTTP_SSL_CLIENT_CERT_INFO)_ClientCertificateInfoBuffer->GetBuffer(),
                                        _ClientCertificateInfoBuffer->QuerySize(),
                                        NULL,
                                        &_Overlapped);

    if ((error == ERROR_SUCCESS) ||       // CONSIDER: Will IOCP fire for ERROR_SUCCESS ?
        (error == ERROR_IO_PENDING))
    {
        return(STATUS_SUCCESS);
    }

    //
    // Release the buffer if it is any other error.
    //
    _ClientCertificateInfoBuffer = nullptr;
    _HttpError = error;

    if (error == ERROR_NOT_FOUND)
    {
#if defined(PLATFORM_UNIX)
        KDbgPrintf("HttpServer Client certificate requested, but not found for url %s", Utf16To8(_PHttpRequest->CookedUrl.pFullUrl).c_str());
#else
        KDbgPrintf("HttpServer Client certificate requested, but not found for url %S", _PHttpRequest->CookedUrl.pFullUrl);
#endif
        return(STATUS_NOT_FOUND);
    }
    
    status = STATUS_UNSUCCESSFUL;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
    return(STATUS_UNSUCCESSFUL);
}

//
//  KHttpServerRequest::ReadEntityBody
//
//  Begins/continues a full entity body read
//
//  TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::ReadEntityBody()
{
    NTSTATUS status;
    ULONG error;
    PUCHAR ReceivingAddress = nullptr;
    ULONG  SizeRemaining = 0;

    KInvariant(_HandlerAction != KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);

    if (! _RequestEntityBuffer)
    {
        // If here, we have no receiving buffer.  If we have a content-length header,
        // we know what size to allocate.  If not, we use the default from the parent.
        //
        ULONG SizeToAlloc = 0;
        if (_RequestContentLengthHeader)
        {
            HRESULT hr;
            hr = ULongAdd(_RequestContentLengthHeader, 16, &SizeToAlloc);
            KInvariant(SUCCEEDED(hr));  // REVIEW: TyAdam, BharatN
        }
        else
        {
            SizeToAlloc = _Parent->GetDefaultEntityBodySize();
        }

        if (SizeToAlloc > _Parent->GetMaximumEntityBodySize())
        {
            //
            // Send error response to the requester
            //
            KInvariant(_ResponseContentCarrier == KHttpUtils::eNone);
            
            status = STATUS_SUCCESS;
            KTraceFailedAsyncRequest(status, this, KHttpUtils::HttpResponseCode::RequestEntityTooLarge, 0);
            SendResponse(KHttpUtils::HttpResponseCode::RequestEntityTooLarge,
                         NULL,                    // ReasonMessage
                         NULL);                   // Callback

            return(status);
        }
        
        status = KBuffer::Create(SizeToAlloc, _RequestEntityBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
            return(status);
        }
    }

    ReceivingAddress = PUCHAR(_RequestEntityBuffer->GetBuffer()) + _RequestEntityLength;
    SizeRemaining = _RequestEntityBuffer->QuerySize() - _RequestEntityLength;

    // This may be a multi-part read and we may need to reallocate.  Always
    // ensure we have at least the default left by the parent in cases
    // where the Content-Length wasn't set.
    //
    if (!_RequestContentLengthHeader && SizeRemaining < _Parent->GetDefaultEntityBodySize())
    {
        ULONG NewSize = _RequestEntityBuffer->QuerySize() * 2;

        status = _RequestEntityBuffer->SetSize(NewSize, TRUE);
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, NewSize, NULL);
            return(status);
        }

        ReceivingAddress = PUCHAR(_RequestEntityBuffer->GetBuffer()) + _RequestEntityLength;
        SizeRemaining = _RequestEntityBuffer->QuerySize() - _RequestEntityLength;
    }

    // Now try to read.  This may be asynchronously completed via IOCP or may return
    // immediately if there is content already to go.
    //
    // This API fails strangely if SizeRemaining is called with 0, even on end-of-file, so either
    // don't call it if the content is fully received, or make sure that the SizeRemaining
    // is nonzero.
    //
    KAssert(SizeRemaining > 0);
    KAssert(ReceivingAddress);

    error = HttpReceiveRequestEntityBody(
                                        _Parent->_hRequestQueue,
                                        _PHttpRequest->RequestId,
                                        HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER,
                                        ReceivingAddress,
                                        SizeRemaining,
                                        0,
                                        &_Overlapped
                                        );

    if (error == 0 || error == ERROR_IO_PENDING)
    {
        // Both of these have the same logical effect.  The only difference
        // is whether the IOCP completion is reentrant or via
        // a different thread.
        return(STATUS_SUCCESS);
    }

    if (error == ERROR_HANDLE_EOF)
    {
        eState = eReadComplete;
        _Parent->InvokeHandler(this);
        return(STATUS_SUCCESS);
    }

    //
    // Some other error
    //
    status = STATUS_INTERNAL_ERROR;
    _HttpError = error;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);

    return(status);
}

//
//  KHttpServerRequest::ReadRequestData
//
//  Reads data incoming for a request
//
//  TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::ReadRequestData(
    __in KMemRef& Mem
    )
{
    NTSTATUS status;
    ULONG error;

    KInvariant(_HandlerAction == KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);

    // Now try to read.  This may be asynchronously completed via IOCP or may return
    // immediately if there is content already to go.
    //
    // This API fails strangely if SizeRemaining is called with 0, even on end-of-file, so either
    // don't call it if the content is fully received, or make sure that the SizeRemaining
    // is nonzero.
    //
    eState = eReadMultiPart;

    error = HttpReceiveRequestEntityBody(
                                        _Parent->_hRequestQueue,
                                        _PHttpRequest->RequestId,
                                        HTTP_RECEIVE_REQUEST_ENTITY_BODY_FLAG_FILL_BUFFER,
                                        Mem._Address,
                                        Mem._Size,
                                        0,
                                        &_Overlapped
                                        );

    if (error == 0 || error == ERROR_IO_PENDING)
    {
        // Both of these have the same logical effect.  The only difference
        // is whether the IOCP completion is reentrant or via
        // a different thread.
        return(STATUS_SUCCESS);
    }

    if (error == ERROR_HANDLE_EOF)
    {
        //
        // No more request data to be read, go ahead and return the special status
        //
        eState = eReadComplete;
        return(STATUS_END_OF_FILE);
    }

    //
    // Some other error
    //
    status = STATUS_INTERNAL_ERROR;
    _HttpError = error;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);

    return(status);
}


NTSTATUS
KHttpServerRequest::GetRequestCertificate(
    __out PHTTP_SSL_CLIENT_CERT_INFO &CertInfo)
{
    NTSTATUS status = STATUS_NOT_FOUND;

    KInvariant(_HandlerAction == KHttpServer::HeaderHandlerAction::DeliverCertificateAndContent);

    if (_ClientCertificateInfoBuffer)
    {
        CertInfo = (PHTTP_SSL_CLIENT_CERT_INFO)_ClientCertificateInfoBuffer->GetBuffer();
        if ( (! CertInfo->pCertEncoded) || (CertInfo->CertFlags))
        {
#if defined(PLATFORM_UNIX)
            KDbgPrintf("Client certificate for url %s has CERT_CHAIN_POLICY_STATUS other than S_OK: - 0x%x", 
                Utf16To8(_PHttpRequest->CookedUrl.pFullUrl).c_str(), CertInfo->CertFlags);
#else
            KDbgPrintf("Client certificate for url %S has CERT_CHAIN_POLICY_STATUS other than S_OK: - 0x%x",
                _PHttpRequest->CookedUrl.pFullUrl, CertInfo->CertFlags);
#endif
        }
        return(STATUS_SUCCESS);
    }

    return(status);
}

// CONSIDER: Changing function signature to return a NTSTATUS or VOID and return
//           HTTP_AUTH_STATUS as an __out parameter
HTTP_AUTH_STATUS
KHttpServerRequest::GetRequestToken(
    __out HANDLE &HToken
    )
{
    HToken = INVALID_HANDLE_VALUE;

    if (_hAccessToken != INVALID_HANDLE_VALUE)
    {
        HToken = _hAccessToken;
        return HttpAuthStatusSuccess;
    }

    for (DWORD i = 0; i < _PHttpRequest->RequestInfoCount; i++)
    {
        PHTTP_REQUEST_INFO pInfo = _PHttpRequest->pRequestInfo + i;
        if (pInfo->InfoType != HttpRequestInfoTypeAuth)
        {
            continue;
        }

        PHTTP_REQUEST_AUTH_INFO pAuthInfo = (HTTP_REQUEST_AUTH_INFO *)pInfo->pInfo;
        if (!(_Parent->_WinHttpAuthType & pAuthInfo->AuthType))
        {
            continue;
        }

        if (pAuthInfo->AuthStatus != HttpAuthStatusSuccess)
        {
            return pAuthInfo->AuthStatus;
        }

        HToken = _hAccessToken = pAuthInfo->AccessToken;
        return HttpAuthStatusSuccess;
    }

    return HttpAuthStatusNotAuthenticated;
}

NTSTATUS
KHttpServerRequest::GetRemoteAddress(
    __out KString::SPtr &Value
    )
{
    if (!_PHttpRequest || !_PHttpRequest->Address.pRemoteAddress)
    {
        return STATUS_NOT_FOUND;
    }

    // The RTL api's below require a buffer atleast 46 characters long.
    //
    SOCKADDR* remoteAddr = _PHttpRequest->Address.pRemoteAddress;
    Value = KString::Create(GetThisAllocator(), INET6_ADDRSTRLEN);
    if (!Value)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (remoteAddr->sa_family == AF_INET)
    {
        SOCKADDR_IN* remoteAddr4 = (SOCKADDR_IN*)_PHttpRequest->Address.pRemoteAddress;
        RtlIpv4AddressToString(&remoteAddr4->sin_addr, *Value);
    }
    else if (remoteAddr->sa_family == AF_INET6)
    {
        SOCKADDR_IN6* remoteAddr6 = (SOCKADDR_IN6*)_PHttpRequest->Address.pRemoteAddress;
        RtlIpv6AddressToString(&remoteAddr6->sin6_addr, *Value);
    }
    else
    {
        return STATUS_INVALID_ADDRESS;
    }

    return STATUS_SUCCESS;
}



//
//  KHttpServerRequest::AnalyzeResponseHeaders
//
//  Computes the space requirement in bytes for the returned headers and how many
//  custom/unknown headers are present.
//
VOID
KHttpServerRequest::AnalyzeResponseHeaders(
    __out ULONG& AnsiBufSize,
    __out ULONG& UnknownHeaderCount
    )
{
    AnsiBufSize = 0;
    UnknownHeaderCount = 0;

    for (ULONG i = 0; i < _ResponseHeaders.Count(); i++)
    {
        HeaderPair::SPtr Current = _ResponseHeaders[i];
        if (Current->_HeaderCode != KHttpUtils::HttpHeader_INVALID)
        {
            // We are processing a standard header
            //
            AnsiBufSize +=  Current->_Value.Length();
        }
        else
        {
            UnknownHeaderCount++;
            AnsiBufSize +=  Current->_Value.Length();
            AnsiBufSize +=  Current->_Id.Length();
        }
    }
}

//
//  KHttpServerRequest::FormatResponse
//
NTSTATUS
KHttpServerRequest::FormatResponseData(
    __in KHttpUtils::ContentCarrierType ResponseContentCarrier,
    __in KBuffer::SPtr ResponseBuffer,
    __in KMemRef ResponseMemRef,
    __in KStringView ResponseStringView,
    __out USHORT& ChunkCount
    )
{
    // First send the response
    //
    if (ResponseContentCarrier == KHttpUtils::eNone)
    {
        ChunkCount = 0;
    } else {
        //
        // CONSIDER: Extend the api to allow setting multiple buffers/chunks so that
        //           scatter/gather will occur in kernel
        //
        ChunkCount = 1;
        switch (ResponseContentCarrier)
        {
            case KHttpUtils::eKMemRef:
                 _ResponseChunk.FromMemory.pBuffer = ResponseMemRef._Address;
                 _ResponseChunk.FromMemory.BufferLength = ResponseMemRef._Param;
                 break;

            case KHttpUtils::eKBuffer:
                 _ResponseChunk.FromMemory.pBuffer = ResponseBuffer->GetBuffer();
                 _ResponseChunk.FromMemory.BufferLength = ResponseBuffer->QuerySize();
                 break;

            case KHttpUtils::eKStringView:
                 _ResponseChunk.FromMemory.pBuffer = PWSTR(ResponseStringView);
                 _ResponseChunk.FromMemory.BufferLength = ResponseStringView.Length() * 2;
                 break;
        }

        _ResponseChunk.DataChunkType = HttpDataChunkFromMemory;
    }

    return(STATUS_SUCCESS);
}

//
//  KHttpServerRequest::FormatResponse
//
NTSTATUS
KHttpServerRequest::FormatResponse(
    __in KHttpUtils::HttpResponseCode Code,
    __in LPCSTR ReasonMessage
    )
{
    NTSTATUS status;

    _HttpResponse.StatusCode = (USHORT) Code;

    // Set up ANSI headers
    //
    //
    // We will proactively create a buffer and resize it as it becomes full.
    //

    // Compute the size of all the headers.
    //
    ULONG HeaderBufferSize = 0;
    ULONG UnknownHeaderCount = 0;

    AnalyzeResponseHeaders(HeaderBufferSize, UnknownHeaderCount);

    if (HeaderBufferSize)
    {
        // If here, the user has set up some headers.
        //
        status = KBuffer::Create(HeaderBufferSize, _ResponseHeaderBuffer, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, this, NULL, NULL);
            return(status);
        }

        // Allocate array for the custom headers
        //
        if (UnknownHeaderCount)
        {
            _ResponsePUnknownHeaders = _new(KTL_TAG_HTTP, GetThisAllocator()) HTTP_UNKNOWN_HEADER[UnknownHeaderCount];
            if (! _ResponsePUnknownHeaders)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceFailedAsyncRequest(status, this, NULL, NULL);
                return(status);
            }
            _HttpResponse.Headers.pUnknownHeaders = _ResponsePUnknownHeaders;
            _HttpResponse.Headers.UnknownHeaderCount = (USHORT) UnknownHeaderCount;
        }

        PUCHAR Cursor = (PUCHAR) _ResponseHeaderBuffer->GetBuffer();
        ULONG UnknownHeaderIndex = 0;
        for (ULONG i = 0; i < _ResponseHeaders.Count(); i++)
        {
            HeaderPair::SPtr Current = _ResponseHeaders[i];
            if (Current->_HeaderCode != KHttpUtils::HttpHeader_INVALID)
            {
                // We are processing a standard header
                //
                _HttpResponse.Headers.KnownHeaders[Current->_HeaderCode].RawValueLength = (USHORT) Current->_Value.Length();
                _HttpResponse.Headers.KnownHeaders[Current->_HeaderCode].pRawValue = (PCSTR) Cursor;
                Current->_Value.CopyToAnsi((CHAR *) Cursor, Current->_Value.Length());
                Cursor += Current->_Value.Length();
            }
            else
            {
                // A custom header
                //
                _HttpResponse.Headers.pUnknownHeaders[UnknownHeaderIndex].NameLength = (USHORT) Current->_Id.Length();
                _HttpResponse.Headers.pUnknownHeaders[UnknownHeaderIndex].RawValueLength = (USHORT) Current->_Value.Length();

                _HttpResponse.Headers.pUnknownHeaders[UnknownHeaderIndex].pName = (PCSTR)Cursor;
                Current->_Id.CopyToAnsi((CHAR *) Cursor, Current->_Id.Length());
                Cursor += Current->_Id.Length();

                _HttpResponse.Headers.pUnknownHeaders[UnknownHeaderIndex].pRawValue = (PCSTR) Cursor;
                Current->_Value.CopyToAnsi((CHAR *) Cursor, Current->_Value.Length());
                Cursor += Current->_Value.Length();
                UnknownHeaderIndex++;
            }
        }
    }

    // If there is a reason message, copy it.
    //
    if (ReasonMessage && *ReasonMessage)
    {
        ULONG ReasonLen = 0;
        ULONG NullTerminatedReasonLen = 0;
        LPCSTR Scanner = ReasonMessage;

        HRESULT hr;
        while (*Scanner++){
            // REVIEW: TyAdam, BharatN
            hr = ULongAdd(ReasonLen, 1, &ReasonLen);
            KInvariant(SUCCEEDED(hr));
        }  // Compute length

        // Reserve space for the terminating null character
        hr = ULongAdd(ReasonLen, 1, &NullTerminatedReasonLen);
        KInvariant(SUCCEEDED(hr));

        _ResponseReason = _new(KTL_TAG_HTTP, GetThisAllocator()) char[NullTerminatedReasonLen];

        if (_ResponseReason)
        {
            // If we failed to allocate or copy, then we just skip this,
            // as it is not that important to return a 'Reason' message.
            //
            StringCchCopyA(_ResponseReason, NullTerminatedReasonLen, ReasonMessage);

            // ReasonLen is the size of the string pointed to by pReason not including the terminating null.
            if (ReasonLen > MAXUSHORT)
            {
                status = K_STATUS_OUT_OF_BOUNDS;  //  http.sys uses USHORT for ReasonLength
                KTraceFailedAsyncRequest(status, this, ReasonLen, MAXUSHORT);
                return status;
            }

            _HttpResponse.ReasonLength = static_cast<USHORT>(ReasonLen);
            _HttpResponse.pReason = _ResponseReason;
        }
    }

    return(STATUS_SUCCESS);
}


//
//  KHttpServerRequest::DeliverResponse
//
//  TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::DeliverResponse(
)
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG error;

    error = HttpSendHttpResponse(
                                _Parent->_hRequestQueue,
                                _PHttpRequest->RequestId,
                                _HttpResponseFlags,
                                &_HttpResponse,
                                0,   // Cache policy
                                0,   // Bytes sent; not used in IOCP
                                0,   // Reserved
                                0,   // Reserved even more
                                &_Overlapped,
                                NULL  // Log data
                                );

    if ((error == ERROR_IO_PENDING) || (error == ERROR_SUCCESS))
    {
        return(status);
    }

    // If this fails, there are not documented error codes.
    // However, in practice all the sender needs to know is that it failed.
    // A client-side disconnect is the most likely reason.
    //
    _HttpError = error;

    status = STATUS_UNSUCCESSFUL;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
    return(status);
}

//
//  KHttpServerRequest::SendResponse
//
//  TODO: guard state changes etc.
//
VOID
KHttpServerRequest::SendResponse(
    __in KHttpUtils::HttpResponseCode Code,
    __in LPCSTR ReasonMessage,
    __in ResponseCompletion Callback
    )
{
    NTSTATUS status;
    USHORT chunkCount;

    eState = eSendResponse;

    _ResponseCompleteCallback = Callback;

    status = FormatResponseData(_ResponseContentCarrier, _ResponseBuffer, _ResponseMemRef, _ResponseStringView, chunkCount);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, NULL, NULL);
        // CONSIDER: The _ResponseCompleteCallback is not invoked for this error in the old code
        if (_ResponseCompleteCallback)
        {
            _ResponseCompleteCallback(this, status);
        }

        Complete(status);
        return;
    }

    if (chunkCount != 0)
    {
        _HttpResponse.EntityChunkCount = chunkCount;
        _HttpResponse.pEntityChunks = &_ResponseChunk;
    } else {
        _HttpResponse.EntityChunkCount = 0;
        _HttpResponse.pEntityChunks = NULL;
    }

    status = FormatResponse(Code,
                            ReasonMessage);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, NULL, NULL);
        // CONSIDER: The _ResponseCompleteCallback is not invoked for this error in the old code
        if (_ResponseCompleteCallback)
        {
            _ResponseCompleteCallback(this, status);
        }
        Complete(status);
        return;
    }

    //
    // Fire it asynchronously.
    //
    _HttpResponseFlags = 0;
    status = DeliverResponse();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
        if (_ResponseCompleteCallback)
        {
            _ResponseCompleteCallback(this, status);
        }
        Complete(status);
    }
}

//
//  KHttpServerRequest::DeliverResponse
//
//  TODO: wrap in a child async
//
NTSTATUS
KHttpServerRequest::SendResponseData(
    __in KHttpUtils::ContentCarrierType ResponseContentCarrier,
    __in KBuffer::SPtr ResponseBuffer,
    __in KMemRef ResponseMemRef,
    __in KStringView ResponseStringView,
    __in ULONG Flags
)
{
    NTSTATUS status;
    ULONG error;
    USHORT chunkCount;

    status = FormatResponseData(ResponseContentCarrier,
                                ResponseBuffer,
                                ResponseMemRef,
                                ResponseStringView,
                                chunkCount);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
        return(status);
    }

    error = HttpSendResponseEntityBody(
                                _Parent->_hRequestQueue,
                                _PHttpRequest->RequestId,
                                Flags,
                                chunkCount,   // Entity Chunk Count
                                chunkCount > 0 ? &_ResponseChunk : NULL,
                                NULL,   // Bytes sent; not used in IOCP
                                0,   // Reserved
                                0,   // Reserved even more
                                &_Overlapped,
                                NULL  // Log data
                                );

    if ((error == ERROR_IO_PENDING) || (error == ERROR_SUCCESS))
    {
        return(status);
    }

    // If this fails, there are not documented error codes.
    // However, in practice all the sender needs to know is that it failed.
    // A client-side disconnect is the most likely reason.
    //
    _HttpError = error;

    status = STATUS_UNSUCCESSFUL;
    KTraceFailedAsyncRequest(status, this, _HttpError, NULL);
    return(status);
}

//
// KHttpServerRequest::AsyncResponseDataContext
//
VOID
KHttpServerRequest::AsyncResponseDataContext::OnCompleted(
    )
{
    //
    // Break circular reference
    //
    _Request->_ASDRContext = nullptr;
    
    _Content_KBuffer = nullptr;
    _ReadBuffer = nullptr;
    _Request = nullptr;   

    _ReasonMessage = NULL;
}

NTSTATUS
KHttpServerRequest::AsyncResponseDataContext::GetReceiveBuffer(KMemRef& Mem)
{
    if (_ContentCarrier == KHttpUtils::ContentCarrierType::eKMemRef)
    {
        Mem = *_ReadContent_KMemRef;
        return STATUS_SUCCESS;
    }
    else if (_ContentCarrier == KHttpUtils::ContentCarrierType::eKBuffer)
    {

        NTSTATUS status = KBuffer::Create(_MaxBytesToRead, _ReadBuffer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            return status;
        }

        Mem = Ktl::Move(KMemRef(_ReadBuffer->QuerySize(), _ReadBuffer->GetBuffer()));
        return STATUS_SUCCESS;
    }

    KAssert(false);

    // should not get here
    return STATUS_UNSUCCESSFUL;
}

VOID
KHttpServerRequest::AsyncResponseDataContext::ReceiveRequestDataFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;
    KMemRef mem;

    status = GetReceiveBuffer(mem);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _Request, _MaxBytesToRead);
        Complete(status);
        return;
    }

    status = _Request->ReadRequestData(mem);
    if (! NT_SUCCESS(status))
    {
        if (status != STATUS_END_OF_FILE)
        {
            KTraceFailedAsyncRequest(status, this, _Request, NULL);
        }

        Complete(status);
    }
}

VOID 
KHttpServerRequest::AsyncResponseDataContext::ReceiveRequestCertificateFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    //
    // Establish a circular link here which is broken when this async completes
    //
    *_Certificate = nullptr;

    _Request->eState = KHttpServerRequest::eReadCertificate;
    status = _Request->ReadCertificate();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _Request->_HttpError, _Request);
        Complete(status);
    }
}


VOID 
KHttpServerRequest::AsyncResponseDataContext::SendResponseHeaderFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    _Request->eState = eSendHeaders;

    _Request->_ResponseHeadersSent = TRUE;

    status = _Request->FormatResponse(_Code,
                                      _ReasonMessage);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, NULL, NULL);
        Complete(status);
        return;
    }

    _Request->_HttpResponseFlags = _Flags;
    status = _Request->DeliverResponse();
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, NULL, NULL);
        _Request->FailRequest(status, KHttpUtils::HttpResponseCode::NoResponse);
        return;
    }
}

VOID 
KHttpServerRequest::AsyncResponseDataContext::SendResponseDataFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    _Request->eState = eSendResponseData;

    status = _Request->SendResponseData(_ContentCarrier, _Content_KBuffer, _Content_KMemRef, _Content_KStringView, _Flags);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, NULL, NULL);
        _Request->FailRequest(status, KHttpUtils::HttpResponseCode::NoResponse);
        return;
    }   
}

VOID
KHttpServerRequest::AsyncResponseDataContext::OnStart(
    )
{
    //
    // This establishes a circular reference between this async and the KHttpServerRequest. However
    // this circular reference is removed when this async is completed and thus should not cause
    // a memory leak.
    //
    _Request->_ASDRContext = this;

    switch(_State)
    {
        case ReceiveRequestData:
        {
            ReceiveRequestDataFSM(STATUS_SUCCESS);
            break;
        }

        case ReceiveRequestCertificate:
        {
            ReceiveRequestCertificateFSM(STATUS_SUCCESS);
            break;
        }

        case SendResponseHeader:
        {
            SendResponseHeaderFSM(STATUS_SUCCESS);
            break;
        }

        case SendResponseData:
        {
            SendResponseDataFSM(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
KHttpServerRequest::AsyncResponseDataContext::OnCancel(
    )
{
    KTraceCancelCalled(this, FALSE, FALSE, _State);
    BOOL bRes = CancelIoEx(
        _Request->_Parent->_hRequestQueue,
        &(_Request->_Overlapped));

    if (!bRes)
    {
        // Cancel attempt failed. Let the IO complete on its own via the IO completion callback.
        ULONG error = GetLastError();
        KTraceFailedAsyncRequest(error, this, NULL, NULL);
    }
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartSendResponseHeader(
    __in KHttpServerRequest& Request,
    __in KHttpUtils::HttpResponseCode Code,
    __in_opt LPSTR ReasonMessage,
    __in ULONG Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = SendResponseHeader;
    _Request = &Request;
    _Code = Code;
    _ReasonMessage = ReasonMessage;
    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartSendResponseDataContent(
    __in KHttpServerRequest& Request,                                      
    __in KBuffer::SPtr& Buffer,
    __in ULONG Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendResponseData;
    _Request = &Request;
    _Content_KBuffer = Buffer;

    if (Buffer)
    {
        _ContentCarrier = KHttpUtils::ContentCarrierType::eKBuffer;
    }
    else
    {
        //
        // For multipart sends, the last call to add entity body data, can just
        // send a empty body with the flags as 0. 
        //
        _ContentCarrier = KHttpUtils::ContentCarrierType::eNone;
    }

    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartSendResponseDataContent(
    __in KHttpServerRequest& Request,                                      
    __in KMemRef& Mem,
    __in ULONG Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendResponseData;
    _Request = &Request;
    _Content_KMemRef = Mem;

    if (Mem._Size != 0)
    {
        _ContentCarrier = KHttpUtils::ContentCarrierType::eKMemRef;
    }
    else
    {
        //
        // For multipart sends, the last call to add entity body data, can just
        // send a empty body with the flags as 0. 
        //
        _ContentCarrier = KHttpUtils::ContentCarrierType::eNone;
    }

    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartSendResponseDataContent(
    __in KHttpServerRequest& Request,                                      
    __in KStringView& StringContent,
    __in ULONG Flags,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendResponseData;
    _Request = &Request;
    _Content_KStringView = StringContent;

    if (StringContent.Length() != 0)
    {
        _ContentCarrier = KHttpUtils::ContentCarrierType::eKStringView;
    }
    else
    {
        //
        // For multipart sends, the last call to add entity body data, can just
        // send a empty body with the flags as 0. 
        //
        _ContentCarrier = KHttpUtils::ContentCarrierType::eNone;
    }

    _Flags = Flags;

    Start(ParentAsyncContext, CallbackPtr);
}
            
VOID
KHttpServerRequest::AsyncResponseDataContext::StartReceiveRequestData(
    __in KHttpServerRequest& Request, 
    __in ULONG MaxBytesToRead,
    __out KBuffer::SPtr& ReadContent,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = ReceiveRequestData;
    _Request = &Request;
    _MaxBytesToRead = MaxBytesToRead;
    _ReadContent = &ReadContent;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKBuffer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartReceiveRequestData(
    __in KHttpServerRequest& Request,
    __inout KMemRef *Mem,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = ReceiveRequestData;
    _Request = &Request;
    _MaxBytesToRead = Mem->_Size;
    _ReadContent_KMemRef = Mem;
    _ReadContent_KMemRef->_Param = 0;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKMemRef;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::StartReceiveRequestCertificate(
    __in KHttpServerRequest& Request,
    __out KBuffer::SPtr& Certificate,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = ReceiveRequestCertificate;
    _Request = &Request;
    _Certificate = &Certificate;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerRequest::AsyncResponseDataContext::Initialize(
    )
{
    _State = Unassigned;
    _Request = nullptr;
    _MaxBytesToRead = 0;
    _ReadContent = NULL;
    _ReadContent_KMemRef = nullptr;
    _Certificate = NULL;
    _Code = KHttpUtils::HttpResponseCode::Ok;
    _ReasonMessage = NULL;
    _Flags = 0;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eNone;
    _Content_KMemRef._Address = NULL;   
    _Content_KBuffer = nullptr;
    _ReadBuffer = nullptr;  
}

VOID
KHttpServerRequest::AsyncResponseDataContext::OnReuse(
    )
{
    Initialize();
}

KHttpServerRequest::AsyncResponseDataContext::AsyncResponseDataContext()
{
    Initialize();
}

KHttpServerRequest::AsyncResponseDataContext::~AsyncResponseDataContext()
{
}

NTSTATUS
KHttpServerRequest::AsyncResponseDataContext::Create(
    __out AsyncResponseDataContext::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    AsyncResponseDataContext::SPtr context;

    context = _new(AllocationTag, Allocator) AsyncResponseDataContext();
    if (! context)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return(status);
    }

    status = context->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Context.Attach(context.Detach());

    return(status);
}

#pragma endregion httpServerRequest


#pragma region khttpServer
//
//  KHttpServer::ReadCompletion
//
//  Called upon completion of each HTTP request-response.
//  This will create new pending read ops if we are not in shutdown mode.
//
//
VOID
KHttpServer::ReadCompletion(
   __in KAsyncContextBase* const Parent,
   __in KAsyncContextBase& Op
   )
{
    KInvariant(Parent == this);
    KAssert(IsInApartment());
    
    KHttpServerRequest& Req = static_cast<KHttpServerRequest&>(Op);
    if (!_Shutdown && !Req.IsUpgraded())  
    {
        //
        //  Create another op to keep things going if we are not in _Shutdown and the request has not been upgraded to a WebSocket connection.
        //  In the latter case, another op will be created when the WebSocket completes (and WebSocketCompletion is called)
        //
        PostRead();
    }
}

VOID
KHttpServer::WebSocketCompletion(
   __in KAsyncContextBase* const Parent,
   __in KAsyncContextBase& Op
   )
{
    UNREFERENCED_PARAMETER(Op);

    KInvariant(Parent == this);
    KAssert(IsInApartment());

    KDbgPrintf("KHttpServer::WebSocketCompletion called");
    ReleaseActivities(); // Release activities acquired in StartOpenWebSocket()

    if (!_Shutdown)
    {
        //
        //  Create another op to keep things going if we are not in _Shutdown
        //
        PostRead();
    }
}

VOID
KHttpServer::OnCompleted()
{
    // Continued here because all outstanding KHttpServerRequests are completed AND
    // this server instance has been closed
    UnbindIOCP();
    HttpRemoveUrlFromUrlGroup(_hUrlGroupId, NULL, HTTP_URL_FLAG_REMOVE_ALL);
    HttpCloseRequestQueue(_hRequestQueue);
    HttpCloseUrlGroup(_hUrlGroupId);
    HttpCloseServerSession(_hSessionId);
    HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
    _hRequestQueue = 0;
}



//  KHttpServer::BindIOCP
//
//  Turn on IOCP for this handle.
//
NTSTATUS
KHttpServer::BindIOCP()
{
    NTSTATUS Result = GetThisAllocator().GetKtlSystem().DefaultThreadPool().RegisterIoCompletionCallback(
        HANDLE(_hRequestQueue),
        KHttp_IOCP_Handler::_Raw_IOCP_Completion,
        nullptr,
        &_IoRegistrationContext
        );

    return Result;
}

// KHttpServer::UnbindIOCP
//
// Turn off IOCP.
//
VOID
KHttpServer::UnbindIOCP()
{
    if (_IoRegistrationContext)
    {
        GetThisAllocator().GetKtlSystem().DefaultThreadPool().UnregisterIoCompletionCallback(_IoRegistrationContext);
        _IoRegistrationContext = 0;
    }
}


//
//  KHttpServer::Create
//
NTSTATUS
KHttpServer::Create(
    __in  KAllocator& Allocator,
    __out KHttpServer::SPtr& Server,
    __in BOOLEAN AllowNonAdminServerCreation
    )
{
    NTSTATUS status;

    if (AllowNonAdminServerCreation == FALSE)
    {
        if (!IsUserAdmin())
        {
            return STATUS_ACCESS_DENIED;
        }
    }

    KHttpServer::SPtr Tmp = _new(KTL_TAG_HTTP, Allocator) KHttpServer();
    if (! Tmp)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, sizeof(KHttpServer), 0);
        return(status);
    }

    status = Tmp->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    Server = Tmp;
    return(STATUS_SUCCESS);
}


//
//  KHttpServer::KHttpServer
//
KHttpServer::KHttpServer()
    :  _Url(GetThisAllocator()), _OverrideHandlers(GetThisAllocator())
{
    NTSTATUS status;

    _hRequestQueue = 0;
    HTTP_SET_NULL_ID(&_hUrlGroupId);
    HTTP_SET_NULL_ID(&_hSessionId);
    _IoRegistrationContext = 0;
    _PrepostedRequests = 0;
    _Shutdown = FALSE;
    _HaltCode = 0;

    status = _Url.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _OverrideHandlers.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}


//
//  KHttpServer::~KHttpServer
//
KHttpServer::~KHttpServer()
{
    KDbgPrintf("KHttpServer destructing.");
}

//
//  KHttpServer::StartOpen
//
NTSTATUS
KHttpServer::StartOpen(
    __in KUriView& UrlPrefix,
    __in KHttpServer::RequestHandler Callback,
    __in ULONG PrepostedRequests,
    __in ULONG DefaultHeaderBufferSize,
    __in ULONG DefaultEntityBodySize,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase::OpenCompletionCallback Completion,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride,
    __in ULONG MaximumEntityBodySize
    )
{
    return StartOpen(
        UrlPrefix,
        Callback,
        NULL,                         // HeaderHandlerCallback
        PrepostedRequests,
        DefaultHeaderBufferSize,
        DefaultEntityBodySize,
        ParentAsync,
        Completion,
        GlobalContextOverride,
        KHttpUtils::HttpAuthType::AuthNone,
        MaximumEntityBodySize);
}

NTSTATUS
KHttpServer::StartOpen(
    __in KUriView& UrlPrefix,
    __in KHttpServer::RequestHandler Callback,
    __in ULONG PrepostedRequests,
    __in ULONG DefaultHeaderBufferSize,
    __in ULONG DefaultEntityBodySize,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase::OpenCompletionCallback Completion,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride,
    __in KHttpUtils::HttpAuthType AuthType,
    __in ULONG MaximumEntityBodySize
    )
{
    return StartOpen(
        UrlPrefix,
        Callback,
        NULL,                         // HeaderHandlerCallback
        PrepostedRequests,
        DefaultHeaderBufferSize,
        DefaultEntityBodySize,
        ParentAsync,
        Completion,
        GlobalContextOverride,
        AuthType,
        MaximumEntityBodySize);
}

NTSTATUS
KHttpServer::StartOpen(
    __in KUriView& UrlPrefix,
    __in RequestHandler DefaultCallback,
    __in RequestHeaderHandler DefaultHeaderCallback,
    __in ULONG PrepostedRequests,
    __in ULONG DefaultHeaderBufferSize,
    __in ULONG DefaultEntityBodySize,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncServiceBase::OpenCompletionCallback Completion,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride,
    __in KHttpUtils::HttpAuthType AuthType,
    __in ULONG MaximumEntityBodySize
    )
{
    NTSTATUS Res;

    // Test URL to ensure port explicitly present in URL, as required by HTTP.SYS
    //
    if (!UrlPrefix.IsValid() || UrlPrefix.GetPort() == 0)
    {
        return STATUS_INVALID_ADDRESS;
    }

    Res = _Url.Set(UrlPrefix);
    if (! NT_SUCCESS(Res))
    {
        return Res;
    }

    KInvariant(DefaultCallback);
    _DefaultHandler = DefaultCallback;

    if (DefaultHeaderCallback)
    {
        _DefaultHeaderHandler = DefaultHeaderCallback;
    } else {
        _DefaultHeaderHandler = KHttpServer::DefaultHeaderHandler;
    }

    _DefaultHeaderBufferSize = DefaultHeaderBufferSize;
    _DefaultEntityBodySize = DefaultEntityBodySize;
    _PrepostedRequests = PrepostedRequests;
    _AuthType = AuthType;
    _WinHttpAuthType = HttpRequestAuthTypeNone;

    if (MaximumEntityBodySize == 0)
    {
        _MaximumEntityBodySize = MAXULONG;
    } else {
        _MaximumEntityBodySize = MaximumEntityBodySize;
    }

    return KAsyncServiceBase::StartOpen(ParentAsync, Completion, GlobalContextOverride);
}

VOID KHttpServer::DefaultHeaderHandler(
    __in KHttpServerRequest::SPtr Request,
    __out HeaderHandlerAction& Action
    )
{
    UNREFERENCED_PARAMETER(Request);

    Action = DeliverCertificateAndContent;
}

NTSTATUS
KHttpServer::RegisterHandler(
    __in KUriView& SuffixUrl,
    __in BOOLEAN ExactMatch,
    __in RequestHandler Callback,
    __in_opt RequestHeaderHandler HeaderCallback
    )
{
    UNREFERENCED_PARAMETER(HeaderCallback);

    RequestHandler Existing;
    NTSTATUS Res = STATUS_SUCCESS;

    K_LOCK_BLOCK(_TableLock)
    {
        for (ULONG i = 0; i < _OverrideHandlers.Count(); i++)
        {
            HandlerRegistration& reg = _OverrideHandlers[i];
            if (reg._Suffix->Compare(SuffixUrl))
            {
                return STATUS_OBJECTID_EXISTS;
            }
        }

        HandlerRegistration Tmp;

        Tmp._ExactMatch = ExactMatch;
        Tmp._Handler = Callback;
        Tmp._HeaderHandler = HeaderCallback;

        Res = KUri::Create(SuffixUrl, GetThisAllocator(), Tmp._Suffix);
        if (!NT_SUCCESS(Res))
        {
            return Res;
        }

        Res = _OverrideHandlers.Append(Tmp);
    }
    return Res;
}

NTSTATUS
KHttpServer::ResolveHandler(
    __in BOOLEAN IsHeaderHandler,
    __in  KDynUri& Suffix,
    __out RequestHeaderHandler& HeaderCallback,
    __out RequestHandler& Callback
    )
{
    K_LOCK_BLOCK(_TableLock)
    {
        for (ULONG i = 0; i < _OverrideHandlers.Count(); i++)
        {
            HandlerRegistration& reg = _OverrideHandlers[i];

            if (reg._ExactMatch)
            {
                if (reg._Suffix->Compare(Suffix))
                {
                    if (IsHeaderHandler)
                    {
                        HeaderCallback = reg._HeaderHandler;
                    } else {
                        Callback = reg._Handler;
                    }

                    return STATUS_SUCCESS;
                }
            }
            else
            {
                if (reg._Suffix->IsPrefixFor(Suffix))
                {
                    if (IsHeaderHandler)
                    {
                        HeaderCallback = reg._HeaderHandler;
                    } else {
                        Callback = reg._Handler;
                    }
                    return STATUS_SUCCESS;
                }
            }
        }
    }

    return STATUS_NOT_FOUND;
}


NTSTATUS
KHttpServer::UnregisterHandler(
    __in KUriView& SuffixUrl
    )
{
    K_LOCK_BLOCK(_TableLock)
    {
        for (ULONG i = 0; i < _OverrideHandlers.Count(); i++)
        {
            HandlerRegistration& reg = _OverrideHandlers[i];
            if (reg._Suffix->Compare(SuffixUrl))
            {
                _OverrideHandlers.Remove(i);
                return STATUS_SUCCESS;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

VOID
KHttpServer::InvokeHandler(
    __in KHttpServerRequest::SPtr Req
    )
{
    NTSTATUS status;
    RequestHandler handler;
    RequestHeaderHandler headerHandler;

    KInvariant(Req->_RelativeUrl);

    // Try to find an override.
    //

    status = ResolveHandler(FALSE, *Req->_RelativeUrl, headerHandler, handler);
    if (NT_SUCCESS(status))
    {
        handler(Req);
        return;
    }

    // If none, then invoke the default handler.
    //
    _DefaultHandler(Req);
}

VOID
KHttpServer::InvokeHeaderHandler(
    __in KSharedPtr<KHttpServerRequest> Req,
    __out HeaderHandlerAction& Action
    )
{
    NTSTATUS status;
    RequestHandler handler;
    RequestHeaderHandler headerHandler;

    KInvariant(Req->_RelativeUrl);

    // Try to find an override.
    //

    status = ResolveHandler(TRUE, *Req->_RelativeUrl, headerHandler, handler);
    if (NT_SUCCESS(status) && (headerHandler))
    {
        headerHandler(Req, Action);
        return;
    }

    // If none, then invoke the default handler.
    //
    _DefaultHeaderHandler(Req, Action);
}


//
//  KHttpServer::OnServiceOpen
//
VOID
KHttpServer::OnServiceOpen()
{
    NTSTATUS Result = Initialize();
    if (!NT_SUCCESS(Result))
    {
        CompleteOpen(Result);
        return;
    }

    for (ULONG i = 0; i < _PrepostedRequests; i++)
    {
        NTSTATUS Res = PostRead();
        if (! NT_SUCCESS(Res))
        {
            KTraceFailedAsyncRequest(Res, this, i, _PrepostedRequests);
            Halt(Res);
        }
    }

    CompleteOpen(STATUS_SUCCESS);
}


//
//  KHttpServer::PostRead
//
//  Post an HTTP read request.
//
NTSTATUS
KHttpServer::PostRead()
{
    KHttpServerRequest::SPtr Req;
    NTSTATUS Result = KHttpServerRequest::Create(this, GetThisAllocator(), Req);
    if (! NT_SUCCESS(Result))
    {
        KTraceOutOfMemory(0, Result, this, sizeof(KHttpServerRequest), 0);
        return(Result);
    }

    Result = Req->Status();
    if (! NT_SUCCESS(Result))
    {
        KTraceFailedAsyncRequest(Result, this, 0, 0);
        return(Result);
    }

    Result = Req->StartRead(this, KAsyncContextBase::CompletionCallback(this, &KHttpServer::ReadCompletion));
    if (Result != STATUS_PENDING)
    {
        KTraceFailedAsyncRequest(Result, this, 0, 0);
        return STATUS_INTERNAL_ERROR;
    }
    return STATUS_SUCCESS;
}


//
// KHttpServer::IsUserAdmin
//
BOOLEAN
KHttpServer::IsUserAdmin()
{
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
    PSID AdministratorsGroup;

    // Initialize SID
    //
    if (!AllocateAndInitializeSid( &NtAuthority,
                               2,
                               SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS,
                               0, 0, 0, 0, 0, 0,
                               &AdministratorsGroup))
    {
        // Initializing SID Failed.
        return FALSE;
    }

    // Check whether the token is present in admin group.
    //
    BOOL IsInAdminGroup = FALSE;
    if (!CheckTokenMembership( NULL,
                           AdministratorsGroup,
                           &IsInAdminGroup))
    {
        // Error occurred.
        IsInAdminGroup = FALSE;
    }

    // Free SID and return.
    //
    FreeSid(AdministratorsGroup);

    return BOOLEAN(IsInAdminGroup);
}



// KHttpServer::Initialize
//
//
NTSTATUS
KHttpServer::Initialize()
{
    HTTPAPI_VERSION HttpApiVersion = HTTPAPI_VERSION_2;
    LONG Result = NO_ERROR;
    bool HttpInitialized = false;

    //
    // Initialize HTTP Server
    //
    Result = HttpInitialize(
                HttpApiVersion,
                HTTP_INITIALIZE_SERVER,    // Flags
                NULL                       // Reserved
                );

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

    HttpInitialized = true;

    //
    // Create a Server Session
    //
    Result = HttpCreateServerSession(
                HttpApiVersion,
                &_hSessionId,
                0);

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

    //
    // Set the authentication type that is required. 
    // NOTE: Negotiate tries kerberos first but falls back to NTLM when 
    // kerberos is not possible. In NTLM, server's identity cannot be verified,
    // by the client so it is upto the client to negotiate kerberos whereever possible.
    //
    if (_AuthType & KHttpUtils::HttpAuthType::AuthKerberos)
    {
        HTTP_SERVER_AUTHENTICATION_INFO AuthInfo;
        RtlZeroMemory(&AuthInfo, sizeof(HTTP_SERVER_AUTHENTICATION_INFO));  //  Required by http.sys api
        _WinHttpAuthType |= HttpRequestAuthTypeNegotiate;

        AuthInfo.Flags.Present = 1;
        AuthInfo.AuthSchemes = HTTP_AUTH_ENABLE_NEGOTIATE;
        AuthInfo.ReceiveMutualAuth = TRUE; // Mutual auth - send the server's creds to the client
        AuthInfo.ExFlags = HTTP_AUTH_EX_FLAG_CAPTURE_CREDENTIAL;  // Captures caller's creds and uses for kerberos auth.

        Result = HttpSetServerSessionProperty(
                    _hSessionId,
                    HttpServerExtendedAuthenticationProperty,
                    &AuthInfo,
                    sizeof(HTTP_SERVER_AUTHENTICATION_INFO));

        if (Result != NO_ERROR)
        {
            KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
            goto done;
        }

        KDbgPrintf("HttpServer session configured with auth Negotiate");
    }

    //
    // Create URL group
    //
    Result = HttpCreateUrlGroup(
                _hSessionId,
                &_hUrlGroupId,
                0);

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

    //
    // Create a Request Queue Handle
    //
    Result = HttpCreateRequestQueue(
                HttpApiVersion,
                NULL,
                NULL,
                0,
                &_hRequestQueue);

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

    Result = HttpAddUrlToUrlGroup(
                _hUrlGroupId,
                PWSTR(KStringView(_Url)),
                0,
                0);

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

#if defined(PLATFORM_UNIX)
    KDbgPrintf("HttpServer configured for URL %s", Utf16To8(PWSTR(KStringView(_Url))).c_str());
#else
    KDbgPrintf("HttpServer configured for URL %S", PWSTR(KStringView(_Url)));
#endif

    HTTP_BINDING_INFO BindingInfo;
    BindingInfo.Flags.Present = 1;
    BindingInfo.RequestQueueHandle = _hRequestQueue;

    Result = HttpSetUrlGroupProperty(
                _hUrlGroupId,
                HttpServerBindingProperty,
                &BindingInfo,
                sizeof(BindingInfo));

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
        goto done;
    }

    Result = BindIOCP();

    if (Result != NO_ERROR)
    {
        KTraceFailedAsyncRequest(STATUS_INTERNAL_ERROR, this, Result, 0);
    }

done:
    if (Result != NO_ERROR)
    {
#if defined(PLATFORM_UNIX)
        KDbgPrintf("HttpServer initialization for URL %s failed with error - 0x%x", Utf16To8(PWSTR(KStringView(_Url))).c_str(), Result);
#else
        KDbgPrintf("HttpServer initialization for URL %S failed with error - 0x%x", PWSTR(KStringView(_Url)), Result);
#endif

        if (Result == ERROR_ALREADY_EXISTS)
        {
            Result = STATUS_CONFLICTING_ADDRESSES;
        }
        else if (Result == ERROR_NOT_ENOUGH_MEMORY)
        {
            Result =  STATUS_INSUFFICIENT_RESOURCES;
        }
        else if (Result == ERROR_ACCESS_DENIED)
        {
            Result = STATUS_ACCESS_DENIED;
        }
        else
        {
            Result = STATUS_INTERNAL_ERROR;
        }

        if (!HTTP_IS_NULL_ID(&_hUrlGroupId))
        {
            HttpRemoveUrlFromUrlGroup(_hUrlGroupId, NULL, HTTP_URL_FLAG_REMOVE_ALL);
            HttpCloseUrlGroup(_hUrlGroupId);
        }

        if (_hRequestQueue != 0)
        {
            HttpShutdownRequestQueue(_hRequestQueue);
            HttpCloseRequestQueue(_hRequestQueue);
        }

        if (!HTTP_IS_NULL_ID(&_hSessionId))
        {
            HttpCloseServerSession(_hSessionId);
        }

        if (HttpInitialized)
        {
            HttpTerminate(HTTP_INITIALIZE_SERVER, NULL);
        }
    }

    return Result;
}


// KHttpServer::OnServiceClose
//
//
VOID
KHttpServer::OnServiceClose()
{
    _Shutdown = TRUE;
    HttpShutdownRequestQueue(_hRequestQueue);

    CompleteClose(STATUS_SUCCESS);
}

VOID
KHttpServer::Halt(
    __in NTSTATUS Reason
    )
{
    _HaltCode = Reason;
   // _DefaultHandler(nullptr);
}

#pragma endregion khttpServer

#if NTDDI_VERSION >= NTDDI_WIN8

KHttpServerWebSocket::KHttpServerWebSocket(
    __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveOperationQueue,
    __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& ReceiveRequestDequeueOperation,
    __in KAsyncQueue<WebSocketOperation>& SendOperationQueue,
    __in KAsyncQueue<WebSocketOperation>::DequeueOperation& SendRequestDequeueOperation,
    __in HttpReceiveEntityBodyOperation& TransportReceiveOperation,
    __in HttpSendEntityBodyOperation& TransportSendOperation,
    __in HttpSendResponseOperation& SendResponseOperation,
    __in KHttpServerSendCloseOperation& SendCloseOperation,
    __in KHttpUtils::CallbackAsync& InformOfClosureAsync,
    __in KHttpUtils::CallbackAsync& AbortAsync,
    __in CloseTransportOperation& CloseTransportOperation,
    __in KStringA& ActiveSubprotocol,
    __in KBuffer& RemoteCloseReasonBuffer
    ) : 
        KHttpWebSocket(ReceiveOperationQueue, SendOperationQueue, AbortAsync, ActiveSubprotocol),

        // args
        _ReceiveOperationProcessor(*this, ReceiveRequestDequeueOperation, TransportReceiveOperation),
        _SendRequestDequeueOperation(&SendRequestDequeueOperation),
        _TransportSendOperation(&TransportSendOperation),
        _SendResponseOperation(&SendResponseOperation),
        _SendCloseOperation(&SendCloseOperation),
        _InformOfClosureAsync(&InformOfClosureAsync),
        _CloseTransportOperation(&CloseTransportOperation),
        _RemoteCloseReasonBuffer(&RemoteCloseReasonBuffer),

        _RequestQueueHandle(nullptr),
        _WebSocketHandle(nullptr),
        _HttpRequest(nullptr),

        _HttpResponse(),
        _HttpResponseUnknownHeaders(nullptr),
        _ServerRequest(nullptr),
        _RequestHeaders(nullptr),
        _ResponseHeaders(nullptr),

        _CloseReceivedCallback(nullptr),
        _TransportReceiveBufferSize(0),
        _TransportSendBufferSize(0),
        _SupportedSubProtocolsString(nullptr),
        _ParentServer(nullptr),

        _PingTimer(nullptr),
        _UnsolicitedPongTimer(nullptr),
        _PongTimeoutTimer(nullptr),
        _PongTimeoutTimerIsActive(FALSE),

        _IsSendRequestQueueDeactivated(FALSE),
        
        _IsSendTruncated(FALSE),
        _IsHandshakeStarted(FALSE),
        _IsHandshakeComplete(FALSE),

        _IsCleanedUp(FALSE),

        _CloseSent(FALSE)
{
    LONGLONG now = (LONGLONG) KDateTime::Now();

    _LatestReceive = KDateTime(now);
    _TimerReceive = KDateTime(now - 1);
    _LatestReceivedPong = KDateTime(now);
    _TimerReceivedPong = KDateTime(now - 1);
    _LatestPing = KDateTime(now);
}

KHttpServerWebSocket::~KHttpServerWebSocket()
{
    _ResponseHeaders.Detach();

    if (_WebSocketHandle != nullptr)
    {
        //
        //  The WebSocket is destructing, so there can be no outstanding subops.  As all transport operations are
        //  represented each by a subop, it is guaranteed that there is no transport operation using the buffers
        //  owned by _WebSocketHandle
        //
        //  Calling delete during cleanup races buffer deletion with IOCP receive/send and transport closure.
        //
        WebSocketDeleteHandle(_WebSocketHandle);
    }
}

NTSTATUS
KHttpServerWebSocket::Create(
    __out KHttpServerWebSocket::SPtr& WebSocket,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    KHttpServerWebSocket::SPtr webSocket;

    WebSocket = nullptr;

    KAsyncQueue<WebSocketReceiveOperation>::SPtr receiveOperationQueue;
    status = KAsyncQueue<WebSocketReceiveOperation>::Create(
        AllocationTag,
        Allocator,
        FIELD_OFFSET(WebSocketReceiveOperation, _Links),
        receiveOperationQueue
        );
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation::SPtr receiveDequeueOperation;
    status = receiveOperationQueue->CreateDequeueOperation(receiveDequeueOperation);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpReceiveEntityBodyOperation::SPtr transportReceiveOperation;
    status = HttpReceiveEntityBodyOperation::Create(transportReceiveOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketOperation>::SPtr sendOperationQueue;
    status = KAsyncQueue<WebSocketOperation>::Create(
        AllocationTag,
        Allocator,
        FIELD_OFFSET(WebSocketOperation, _Links),
        sendOperationQueue
        );
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KAsyncQueue<WebSocketOperation>::DequeueOperation::SPtr sendDequeueOperation;
    status = sendOperationQueue->CreateDequeueOperation(sendDequeueOperation);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpSendEntityBodyOperation::SPtr transportSendOperation;
    status = HttpSendEntityBodyOperation::Create(transportSendOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    HttpSendResponseOperation::SPtr sendResponseOperation;
    status = HttpSendResponseOperation::Create(sendResponseOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpServerSendCloseOperation::SPtr sendCloseOperation;
    status = KHttpServerSendCloseOperation::Create(sendCloseOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpUtils::CallbackAsync::SPtr informOfClosureAsync;
    status = KHttpUtils::CallbackAsync::Create(informOfClosureAsync, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpUtils::CallbackAsync::SPtr abortAsync;
    status = KHttpUtils::CallbackAsync::Create(abortAsync, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    CloseTransportOperation::SPtr closeTransportOperation;
    status = CloseTransportOperation::Create(closeTransportOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }


    KStringA::SPtr selectedSubprotocol;
    status = KStringA::Create(selectedSubprotocol, Allocator, "", TRUE);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KBuffer::SPtr remoteCloseReasonBuffer;
    status = KBuffer::Create(MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES, remoteCloseReasonBuffer, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    webSocket = _new(AllocationTag, Allocator) KHttpServerWebSocket(
        *receiveOperationQueue,
        *receiveDequeueOperation,
        *sendOperationQueue,
        *sendDequeueOperation,
        *transportReceiveOperation,
        *transportSendOperation,
        *sendResponseOperation,
        *sendCloseOperation,
        *informOfClosureAsync,
        *abortAsync,
        *closeTransportOperation,
        *selectedSubprotocol,
        *remoteCloseReasonBuffer
        );
  
    if (! webSocket)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    if (! NT_SUCCESS(webSocket->Status()))
    {
        return webSocket->Status();
    }

    WebSocket = Ktl::Move(webSocket);

    return STATUS_SUCCESS;
}

class KHttpServerWebSocket::KHttpServerSendFragmentOperation : public SendFragmentOperation, public WebSocketSendFragmentOperation
{
    K_FORCE_SHARED(KHttpServerSendFragmentOperation);
    friend class KHttpServerWebSocket;

public:

    KDeclareSingleArgumentCreate(
        SendFragmentOperation,      //  Output SPtr type
        KHttpServerSendFragmentOperation,  //  Implementation type
        SendOperation,              //  Output arg name
        KTL_TAG_WEB_SOCKET,         //  default allocation tag
        KHttpServerWebSocket&,      //  Constructor arg type
        WebSocketContext            //  Constructor arg name
        );

    VOID
    DoSend(
        __in WEB_SOCKET_HANDLE WebSocketHandle
        );

    VOID
    StartSendFragment(
        __in KBuffer& Data,
        __in BOOLEAN IsFinalFragment,
        __in MessageContentType FragmentDataFormat,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
        __in ULONG const Offset = 0,
        __in ULONG const Size = ULONG_MAX
        ) override;

    VOID
    CompleteOperation(
        __in NTSTATUS Status
        ) override;

private:

    KHttpServerSendFragmentOperation(
        KHttpServerWebSocket& Context
        );

    //
    //  Given a MessageContentType and whether or not the frame is final, get the corresponding WEB_SOCKET_BUFFER_TYPE
    //
    static WEB_SOCKET_BUFFER_TYPE
    GetWebSocketBufferType(
        __in KWebSocket::MessageContentType const & ContentType,
        __in BOOLEAN const & IsFinalFragment
        );

    VOID OnStart() override;

    VOID OnCompleted() override;

private:

    WEB_SOCKET_BUFFER _WebSocketBuffer;
};

class KHttpServerWebSocket::KHttpServerSendMessageOperation : public SendMessageOperation, public WebSocketSendFragmentOperation
{
    K_FORCE_SHARED(KHttpServerSendMessageOperation);
    friend class KHttpServerWebSocket;

public:

    KDeclareSingleArgumentCreate(
        SendMessageOperation,      //  Output SPtr type
        KHttpServerSendMessageOperation,  //  Implementation type
        SendOperation,              //  Output arg name
        KTL_TAG_WEB_SOCKET,         //  default allocation tag
        KHttpServerWebSocket&,      //  Constructor arg type
        WebSocketContext            //  Constructor arg name
        );

    VOID
    DoSend(
        __in WEB_SOCKET_HANDLE WebSocketHandle
        );

    VOID
    StartSendMessage(
        __in KBuffer& Message,
        __in MessageContentType MessageDataFormat,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
        __in ULONG const Offset = 0,
        __in ULONG const Size = ULONG_MAX
        ) override;

    VOID
    CompleteOperation(
        __in NTSTATUS Status
        ) override;

private:

    KHttpServerSendMessageOperation(
        KHttpServerWebSocket& Context
        );

    //
    //  Given a MessageContentType, get the corresponding WEB_SOCKET_BUFFER_TYPE
    //
    static WEB_SOCKET_BUFFER_TYPE
    GetWebSocketBufferType(
        __in KWebSocket::MessageContentType const & ContentType
        );

    VOID OnStart() override;

    VOID OnCompleted() override;

private:

    WEB_SOCKET_BUFFER _WebSocketBuffer;
};

#pragma region KHttpServerWebSocket_ServiceOpen
NTSTATUS
KHttpServerWebSocket::StartOpenWebSocket(
    __in KHttpServerRequest& ServerRequest,
    __in WebSocketCloseReceivedCallback CloseReceivedCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncServiceBase::OpenCompletionCallback CallbackPtr,
    __in_opt ULONG TransportReceiveBufferSize,
    __in_opt ULONG TransportSendBufferSize,
    __in_opt KStringA::SPtr SupportedSubProtocolsString
    )
{
    K_LOCK_BLOCK(_StartOpenLock)
    {
        if (! CloseReceivedCallback)  //  Callback is required
        {
            return STATUS_INVALID_PARAMETER_2;
        }
    
        if (! ServerRequest._Parent->TryAcquireActivities())
        {
            //
            //  If the parent khttpserver is shutting down, fail to open
            //
            return K_STATUS_SHUTDOWN_PENDING;
        }
        //  Activity will be released when WebSocket closes

        if (GetConnectionStatus() == ConnectionStatus::Initialized)
        {
            _ServerRequest = &ServerRequest;
            _ParentServer = ServerRequest._Parent;
            _CloseReceivedCallback = CloseReceivedCallback;
            _TransportReceiveBufferSize = TransportReceiveBufferSize;
            _TransportSendBufferSize = TransportSendBufferSize;
            _SupportedSubProtocolsString = SupportedSubProtocolsString;

            _ConnectionStatus = ConnectionStatus::OpenStarted;

            //
            //  Activate the queues
            //
            {
                NTSTATUS status;

                status = _ReceiveRequestQueue->Activate(nullptr, nullptr);
                if (! NT_SUCCESS(status))
                {
                    _ConnectionStatus = ConnectionStatus::Error;
                    return status;
                }

                status = _SendRequestQueue->Activate(nullptr, nullptr);
                if (! NT_SUCCESS(status))
                {
                    _ConnectionStatus = ConnectionStatus::Error;

                    _ReceiveRequestQueue->Deactivate();
                    _IsReceiveRequestQueueDeactivated = TRUE;
                    _ReceiveRequestQueue->CancelAllEnqueued(
                         KAsyncQueue<WebSocketReceiveOperation>::DroppedItemCallback(this, &KHttpWebSocket::HandleDroppedReceiveRequest)
                         );

                    return status;
                }
            }
    
            return StartOpen(ParentAsyncContext, CallbackPtr);
        }
    }

    return STATUS_SHARING_VIOLATION;  //  StartOpenWebSocket has already been called
}

VOID
KHttpServerWebSocket::OnServiceOpen()
{
    KAssert(IsInApartment());

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        CompleteOpen(_FailureStatus);
        return;
    }


    NTSTATUS status = STATUS_SUCCESS;

    #pragma region initialize_timers
    //
    //  Initialize the timers
    //
    status = InitializeHandshakeTimers();
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "Failing WebSocket due to InitializeHandshakeTimers() returning failure status.", status, ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(status);
        return;
    }

    //
    //  Create the Ping timer and Pong Timeout timer if the constants are set
    //
    _PingTimer = nullptr;
    _PongTimeoutTimer = nullptr;
    if (GetTimingConstant(TimingConstant::PingQuietChannelPeriodMs) != TimingConstantValue::None)
    {
        KTimer::SPtr pingTimer;

        status = KTimer::Create(pingTimer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, this, NULL, NULL);
            FailWebSocket(status);
            return;
        }

        _PingTimer = Ktl::Move(pingTimer);

        //
        //  Only create the Pong Timeout Timer if pings will be issued and the pong timeout constant is set
        //
        if (GetTimingConstant(TimingConstant::PongTimeoutMs) != TimingConstantValue::None)
        {
            KTimer::SPtr pongTimeoutTimer;

            status = KTimer::Create(pongTimeoutTimer, GetThisAllocator(), GetThisAllocationTag());
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(0, status, this, NULL, NULL);
                FailWebSocket(status);
                return;
            }

            _PongTimeoutTimer = Ktl::Move(pongTimeoutTimer);
        }
    }
    
    //
    //  Create the Unsolicited Pong timer if the constant is set
    //
    _UnsolicitedPongTimer = nullptr;
    if (GetTimingConstant(TimingConstant::PongKeepalivePeriodMs) != TimingConstantValue::None)
    {
        KTimer::SPtr unsolicitedPongTimer;

        status = KTimer::Create(unsolicitedPongTimer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, this, NULL, NULL);
            FailWebSocket(status);
            return;
        }

        _UnsolicitedPongTimer = Ktl::Move(unsolicitedPongTimer);
    }

    //
    //  Start the Open Timeout timer if it was initialized by InitializeHandshakeTimers()
    //
    if (_OpenTimeoutTimer)
    {
        KAssert(GetTimingConstant(TimingConstant::OpenTimeoutMs) != TimingConstantValue::Invalid);
        KAssert(GetTimingConstant(TimingConstant::OpenTimeoutMs) != TimingConstantValue::None);

        _OpenTimeoutTimer->StartTimer(
            static_cast<ULONG>(GetTimingConstant(TimingConstant::OpenTimeoutMs)),
            this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::OpenTimeoutTimerCallback)
            );
    }
    #pragma endregion initialize_timers

    //
    //  Initialize WebSocket handle
    //
    KInvariant(!_IsCleanedUp);
    status = CreateHandle(_TransportReceiveBufferSize, _TransportSendBufferSize, *this, _WebSocketHandle);
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "Failing WebSocket due to CreateHandle returning failure status.", status, ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(status);
        return;
    }

    //
    //  Select the Subprotocol
    //
    {
        if (! _SupportedSubProtocolsString)
        {
            status = KStringA::Create(_SupportedSubProtocolsString, GetThisAllocator(), TRUE);
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(0, status, this, 0, 0);
                FailWebSocket(status);
                return;
            }
        }

        KAssert(_ActiveSubprotocol);
        status = SelectSubprotocol(*_SupportedSubProtocolsString, *_ServerRequest, *this, *_ActiveSubprotocol);
        if (! NT_SUCCESS(status))
        {
            KDbgCheckpointWData(0, "Failing WebSocket due to SelectSubprotocol returning failure status.", status, ULONG_PTR(this), 0, 0, 0);
            FailWebSocket(status);
            return;
        }
    }

    //
    //  Begin the server handshake in the websocket protocol component API, then use the returned response headers to generate the http response
    //
    {
        ULONG webSocketResponseHeadersCount;
        KInvariant(!_IsCleanedUp);
        status = BeginServerHandshake(*_ServerRequest, _WebSocketHandle, *_ActiveSubprotocol, *this, _RequestHeaders, _ResponseHeaders, webSocketResponseHeadersCount);
        if (! NT_SUCCESS(status))
        {
            KDbgCheckpointWData(0, "Failing WebSocket due to BeginServerHandshake returning failure status.", status, ULONG_PTR(this), 0, 0, 0);
            FailWebSocket(status);
            return;
        }
        _IsHandshakeStarted = TRUE;

        //
        //  Get the HTTP_RESPONSE structure to send to the client
        //
        status = GetHttpResponse(_ResponseHeaders, webSocketResponseHeadersCount, *this, _HttpResponse, _HttpResponseUnknownHeaders);
        if (! NT_SUCCESS(status))
        {
            KDbgCheckpointWData(0, "Failing WebSocket due to GetHttpResponse returning failure status.", status, ULONG_PTR(this), 0, 0, 0);
            FailWebSocket(status);
            return;
        }
    }
    
    //
    //  Capture the http.sys handles from the given server request
    //
    Capture(*_ServerRequest, _RequestQueueHandle, _HttpRequest);

    //
    //  Send the response asynchronously
    //
    _SendResponseOperation->StartSend(
        *this,
        KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::SendResponseCallback),
        _RequestQueueHandle,
        _HttpRequest->RequestId,
        _HttpResponse
        );

    //  Execution will continue in SendResponseCallback
    return;
}

VOID
KHttpServerWebSocket::Capture(
    __inout KHttpServerRequest& HttpServerRequest,
    __out HANDLE& RequestQueueHandle,
    __out PHTTP_REQUEST& HttpRequest
    )
{
    RequestQueueHandle = HttpServerRequest._Parent->_hRequestQueue;
    HttpRequest = HttpServerRequest._PHttpRequest;

    HttpServerRequest._IsUpgraded = TRUE;  //  This signals to the KHttpServer that the request completing does not mean the connection is terminated
    HttpServerRequest._PHttpRequest = nullptr;
}


NTSTATUS
KHttpServerWebSocket::CreateHandle(
    __in ULONG TransportReceiveBufferSize,
    __in ULONG TransportSendBufferSize,
    __in KAsyncContextBase& Context,
    __out WEB_SOCKET_HANDLE& WebSocketHandle
    )
{
    KAssert(Context.IsInApartment());

    NTSTATUS status = STATUS_SUCCESS;
    HRESULT hr = S_OK;
    WEB_SOCKET_HANDLE webSocketHandle;
   
    WebSocketHandle = NULL;

    //
    //  The WebSocket Protocol Component API requires that both the receive buffer size and send buffer size be at least
    //  256 bytes
    //
    if (TransportReceiveBufferSize < 256)
    {
        status = STATUS_INVALID_PARAMETER_1;
        KTraceFailedAsyncRequest(status, &Context, TransportReceiveBufferSize, NULL);
        return status;
    }
    
    if (TransportSendBufferSize < 256)
    {
        status = STATUS_INVALID_PARAMETER_2;
        KTraceFailedAsyncRequest(status, &Context, TransportSendBufferSize, NULL);
        return status;
    }

    WEB_SOCKET_PROPERTY properties[2] =
    {
        { WEB_SOCKET_RECEIVE_BUFFER_SIZE_PROPERTY_TYPE, &TransportReceiveBufferSize, sizeof TransportReceiveBufferSize },
        { WEB_SOCKET_SEND_BUFFER_SIZE_PROPERTY_TYPE, &TransportSendBufferSize, sizeof TransportSendBufferSize }
    };

    hr = WebSocketCreateServerHandle(properties, ARRAYSIZE(properties), &webSocketHandle);

    if (FAILED(hr))
    {
        status = K_STATUS_WEBSOCKET_API_CALL_FAILURE;
        KTraceFailedAsyncRequest(status, &Context, hr, NULL);
        return status;
    }

    WebSocketHandle = webSocketHandle;
    return STATUS_SUCCESS;
}

NTSTATUS
KHttpServerWebSocket::GetRequestedSubprotocols(
    __in KHttpServerRequest& ServerRequest,
    __in KAsyncContextBase& Context,
    __out KStringA::SPtr& RequestedSubprotocolsString
    )
{
    KAssert(Context.IsInApartment());

    NTSTATUS status;
    KString::SPtr requestedSubprotocolsWide;
    KStringA::SPtr requestedSubprotocolsAnsi;

    RequestedSubprotocolsString = nullptr;

    KStringView requiredHeader(WebSocketHeaders::Protocol);
    status = ServerRequest.GetRequestHeader(requiredHeader, requestedSubprotocolsWide);
    if (status == STATUS_NOT_FOUND || (NT_SUCCESS(status) && requestedSubprotocolsWide->Length() == 0))
    {
        //
        //  If the header is not present or its value is empty, output the empty string
        //
        status = KStringA::Create(requestedSubprotocolsAnsi, Context.GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, &Context, NULL, NULL);
            return status;
        }
    }
    else if (! NT_SUCCESS(status))
    {
        //
        //  Unknown error
        //
        KTraceFailedAsyncRequest(status, &Context, NULL, NULL);
        return status;
    }
    else if (requestedSubprotocolsWide->Length() > 0)
    {
        //
        //  The header value is nonempty, so copy it to and output a new ANSI string (KStringA) 
        //
        KUniquePtr<CHAR, ArrayDeleter<CHAR>> requestedSubprotocolsPCHAR;

        requestedSubprotocolsPCHAR = _newArray<CHAR>(Context.GetThisAllocationTag(), Context.GetThisAllocator(), requestedSubprotocolsWide->Length());
        if (requestedSubprotocolsPCHAR == nullptr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory(0, status, &Context, 0, 0);
            KTraceFailedAsyncRequest(status, &Context, NULL, NULL);
            return status;
        }

        BOOLEAN res = requestedSubprotocolsWide->CopyToAnsi(requestedSubprotocolsPCHAR, requestedSubprotocolsWide->Length());
        if (! res)
        {
            status = K_STATUS_WEBSOCKET_STANDARD_VIOLATION;  //  The header contained non-ASCII characters
            KTraceFailedAsyncRequest(status, &Context, NULL, NULL);
            return status;
        }

        status = KStringA::Create(
            requestedSubprotocolsAnsi,
            Context.GetThisAllocator(),
            KStringViewA(requestedSubprotocolsPCHAR, requestedSubprotocolsWide->Length(), requestedSubprotocolsWide->Length()),
            TRUE
            );
        if (! NT_SUCCESS(status))
        {
            KTraceFailedAsyncRequest(status, &Context, NULL, NULL);
            return status;
        }
    }

    RequestedSubprotocolsString = Ktl::Move(requestedSubprotocolsAnsi);
    return STATUS_SUCCESS;
}

NTSTATUS
KHttpServerWebSocket::SelectSubprotocol(
    __in KStringA& SupportedSubprotocolsString,
    __in KHttpServerRequest& ServerRequest,
    __in KAsyncContextBase& Context,
    __inout KStringA& SelectedSubprotocol
    )
{
    KAssert(Context.IsInApartment());

    NTSTATUS status;

    //
    //  Fetch the requested subprotocols string (comma-separated list) from the headers
    //
    KStringA::SPtr requestedSubprotocols;
    status = GetRequestedSubprotocols(ServerRequest, Context, requestedSubprotocols);
    if (! NT_SUCCESS(status))
    {
        return status;
    }

    //
    //  Use the standard string matching algorithm to select the subprotocol
    //
    KStringViewA selectedSubprotocolStringView;
    KWebSocket::SelectSubprotocol(SupportedSubprotocolsString, *requestedSubprotocols, selectedSubprotocolStringView);

    if (! SelectedSubprotocol.CopyFrom(selectedSubprotocolStringView, TRUE))
    {
        return STATUS_UNSUCCESSFUL;
    }
    KAssert(SelectedSubprotocol.IsNullTerminated());

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpServerWebSocket::BeginServerHandshake(
    __in KHttpServerRequest& ServerRequest,
    __in WEB_SOCKET_HANDLE& WebSocketHandle,
    __in KStringViewA& SelectedSubprotocol,
    __in KAsyncContextBase& Context,
    __out KUniquePtr<WEB_SOCKET_HTTP_HEADER, ArrayDeleter<WEB_SOCKET_HTTP_HEADER>>& RequestHeaders,
    __out_ecount(ResponseHeadersCount) KUniquePtr<WEB_SOCKET_HTTP_HEADER, FailFastIfNonNullDeleter<WEB_SOCKET_HTTP_HEADER>>& ResponseHeaders,
    __out ULONG& ResponseHeadersCount
    )
{
    KAssert(ServerRequest._PHttpRequest);
    KAssert(Context.IsInApartment());

    NTSTATUS status = STATUS_SUCCESS;
    PWEB_SOCKET_HTTP_HEADER requestHeaders;
    ULONG requestHeadersCount = 0;
    HRESULT hr = S_OK;

    ResponseHeaders = nullptr;
    ResponseHeadersCount = 0;

    //
    //  Count the headers and allocate the corresponding WEB_SOCKET_HTTP_HEADERs
    //
    for (USHORT i = 0; i < ARRAYSIZE(ServerRequest._PHttpRequest->Headers.KnownHeaders); i++)
    {
        if (ServerRequest ._PHttpRequest->Headers.KnownHeaders[i].RawValueLength > 0)
        {
            hr = ULongAdd(requestHeadersCount, 1, &requestHeadersCount);
            KInvariant(SUCCEEDED(hr));
        }
    }
    hr = ULongAdd(requestHeadersCount, ServerRequest._PHttpRequest->Headers.UnknownHeaderCount, &requestHeadersCount);
    KInvariant(SUCCEEDED(hr));
    KAssert(requestHeadersCount > 0);

    requestHeaders = _newArray<WEB_SOCKET_HTTP_HEADER>(Context.GetThisAllocationTag(), Context.GetThisAllocator(), requestHeadersCount);
    if (requestHeaders == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(NULL, status, &Context, NULL, NULL);
        return status;
    }

    //
    //  Copy the known headers' pointers
    //
    requestHeadersCount = 0;
    for (USHORT i = 0; i < ARRAYSIZE(ServerRequest._PHttpRequest->Headers.KnownHeaders); i++)
    {
        if (ServerRequest._PHttpRequest->Headers.KnownHeaders[i].RawValueLength > 0)
        {
            KStringViewA str;
            if (KHttpUtils::StdHeaderToStringA(static_cast<KHttpUtils::HttpHeaderType>(i), str))
            {
                requestHeaders[requestHeadersCount].pcName = (PCHAR) str;
                requestHeaders[requestHeadersCount].ulNameLength = str.Length();
                requestHeaders[requestHeadersCount].pcValue = const_cast<PCHAR>(ServerRequest._PHttpRequest->Headers.KnownHeaders[i].pRawValue);
                requestHeaders[requestHeadersCount].ulValueLength = ServerRequest._PHttpRequest->Headers.KnownHeaders[i].RawValueLength;

                requestHeadersCount++;
            }
            else 
            {
                status = STATUS_INTERNAL_ERROR;  //  Bug in StdHeaderToStringA - known header type not converted to string
                KTraceFailedAsyncRequest(status, &Context, i, NULL);
                return status;
            }
        }
    }

    //
    //  Copy the unknown headers' pointers
    //
    for (USHORT i = 0; i < ServerRequest._PHttpRequest->Headers.UnknownHeaderCount; i++)
    {
        requestHeaders[requestHeadersCount].pcName = const_cast<PCHAR>(ServerRequest._PHttpRequest->Headers.pUnknownHeaders[i].pName);
        requestHeaders[requestHeadersCount].ulNameLength = ServerRequest._PHttpRequest->Headers.pUnknownHeaders[i].NameLength;
        requestHeaders[requestHeadersCount].pcValue = const_cast<PCHAR>(ServerRequest._PHttpRequest->Headers.pUnknownHeaders[i].pRawValue);
        requestHeaders[requestHeadersCount].ulValueLength = ServerRequest._PHttpRequest->Headers.pUnknownHeaders[i].RawValueLength;

        requestHeadersCount++;
    }

    //
    //  Pass the request WEB_SOCKET_HTTP_HEADERs to WebSocketBeginServerHandshake, and output the returned WEB_SOCKET_HTTP_HEADERs
    //  which need to be sent in the response
    //
    PWEB_SOCKET_HTTP_HEADER responseHeaders;
    ULONG responseHeadersCount;

    KAssert(SelectedSubprotocol.IsEmpty() || SelectedSubprotocol.IsNullTerminated());
    KInvariant(!static_cast<KHttpServerWebSocket&>(Context)._IsCleanedUp);
    hr = WebSocketBeginServerHandshake(
        WebSocketHandle,
        SelectedSubprotocol.IsEmpty() ? nullptr : (PCSTR) SelectedSubprotocol,
        nullptr,
        0,
        requestHeaders,
        requestHeadersCount,
        &responseHeaders,
        &responseHeadersCount
        );

    if (FAILED(hr))
    {
        status = K_STATUS_WEBSOCKET_API_CALL_FAILURE;
        KTraceFailedAsyncRequest(status, &Context, hr, NULL);
        return status;
    }

    RequestHeaders = requestHeaders;
    ResponseHeaders = responseHeaders;
    ResponseHeadersCount = responseHeadersCount;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpServerWebSocket::GetHttpResponse(
    __in KUniquePtr<WEB_SOCKET_HTTP_HEADER, FailFastIfNonNullDeleter<WEB_SOCKET_HTTP_HEADER>>& ResponseHeaders,
    __in ULONG ResponseHeaderCount,
    __in KAsyncContextBase& Context,
    __out HTTP_RESPONSE& HttpResponse,
    __out KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>>& HttpResponseUnknownHeaders
    )
{
    KAssert(Context.IsInApartment());

    HTTP_RESPONSE response;
    NTSTATUS status;
    USHORT unknownHeaderCount = 0;
    KHttpUtils::HttpHeaderType headerType;

    RtlZeroMemory(&response, sizeof response);  //  Required by http.sys api
    HttpResponseUnknownHeaders = nullptr;

    response.StatusCode = KHttpUtils::HttpResponseCode::Upgrade;

    if (ResponseHeaderCount > MAXUSHORT)
    {
        status = STATUS_INVALID_PARAMETER_2;
        KTraceFailedAsyncRequest(status, &Context, ResponseHeaderCount, MAXUSHORT);
        return status;
    }

    //
    //  Set the known headers and count the unknown headers
    //
	HRESULT hr;
    for (ULONG i = 0; i < ResponseHeaderCount; i++)
    {
        KStringViewA headerString(ResponseHeaders[i].pcName, ResponseHeaders[i].ulNameLength, ResponseHeaders[i].ulNameLength);
        if (KHttpUtils::StringAToStdHeader(headerString, headerType)
            && headerType != KHttpUtils::HttpHeaderType::HttpHeaderConnection // http.sys won't send the Connection header if set as a known header
            )
        {
            if (ResponseHeaders[i].ulValueLength > MAXUSHORT)
            {
                status = K_STATUS_OUT_OF_BOUNDS;  //  Interop problem - http.sys uses USHORT for header value length
                KTraceFailedAsyncRequest(status, &Context, ResponseHeaders[i].ulValueLength, MAXUSHORT);
                return status;
            }

            response.Headers.KnownHeaders[headerType].pRawValue = ResponseHeaders[i].pcValue;
            response.Headers.KnownHeaders[headerType].RawValueLength = static_cast<USHORT>(ResponseHeaders[i].ulValueLength);
        }
        else
        {
            hr = UShortAdd(unknownHeaderCount, 1, &unknownHeaderCount);
			KInvariant(SUCCEEDED(hr));
        }
    }

    //
    //  Allocate the unknown headers
    //
    HttpResponseUnknownHeaders = _newArray<HTTP_UNKNOWN_HEADER>(Context.GetThisAllocationTag(), Context.GetThisAllocator(), unknownHeaderCount);
    if (! HttpResponseUnknownHeaders)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, &Context, 0, 0);
        return status;
    }

    response.Headers.UnknownHeaderCount = unknownHeaderCount;
    response.Headers.pUnknownHeaders = HttpResponseUnknownHeaders;

    //
    //  Set the unknown headers
    //
    USHORT unknownHeaderIndex = 0;
    for (ULONG i = 0; i < ResponseHeaderCount; i++)
    {
        KStringViewA headerString(ResponseHeaders[i].pcName, ResponseHeaders[i].ulNameLength, ResponseHeaders[i].ulNameLength);
        if (! KHttpUtils::StringAToStdHeader(headerString, headerType)
            || headerType == KHttpUtils::HttpHeaderType::HttpHeaderConnection // send the Connection header as an unknown header
            )
        {
            if (ResponseHeaders[i].ulNameLength > MAXUSHORT)
            {
                status = K_STATUS_OUT_OF_BOUNDS;  //  Interop problem - http.sys uses USHORT for header name length
                KTraceFailedAsyncRequest(status, &Context, ResponseHeaders[i].ulNameLength, MAXUSHORT);
                return status;
            }
            
            if (ResponseHeaders[i].ulValueLength > MAXUSHORT)
            {
                status = K_STATUS_OUT_OF_BOUNDS;  //  Interop problem - http.sys uses USHORT for header value length
                KTraceFailedAsyncRequest(status, &Context, ResponseHeaders[i].ulValueLength, MAXUSHORT);
                return status;
            }

            response.Headers.pUnknownHeaders[unknownHeaderIndex].pName = ResponseHeaders[i].pcName;
            response.Headers.pUnknownHeaders[unknownHeaderIndex].NameLength = static_cast<USHORT>(ResponseHeaders[i].ulNameLength);
            response.Headers.pUnknownHeaders[unknownHeaderIndex].pRawValue = ResponseHeaders[i].pcValue;
            response.Headers.pUnknownHeaders[unknownHeaderIndex].RawValueLength = static_cast<USHORT>(ResponseHeaders[i].ulValueLength);

            unknownHeaderIndex++;
        }
    }

    KAssert(unknownHeaderCount == unknownHeaderIndex);

    HttpResponse = response;
    return STATUS_SUCCESS;
}

KDefineDefaultCreate(KHttpServerWebSocket::HttpSendResponseOperation, SendResponseOperation);

KHttpServerWebSocket::HttpSendResponseOperation::HttpSendResponseOperation()
    :
        _Overlapped(this)
{
}

KHttpServerWebSocket::HttpSendResponseOperation::~HttpSendResponseOperation()
{
}

VOID
KHttpServerWebSocket::HttpSendResponseOperation::StartSend(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HANDLE const & RequestQueueHandle,
    __in HTTP_REQUEST_ID const & HttpRequestId,
    __in HTTP_RESPONSE & HttpResponse
)
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    ULONG error = HttpSendHttpResponse(
        RequestQueueHandle,
        HttpRequestId,
        HTTP_SEND_RESPONSE_FLAG_OPAQUE | HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA | HTTP_SEND_RESPONSE_FLAG_MORE_DATA,
        &HttpResponse,
        nullptr,   // Cache policy
        nullptr,   // Bytes sent; not used in IOCP
        nullptr,   // Reserved
        0,         // Reserved even more
        &_Overlapped,
        nullptr    // Log data
        );

    if (error != ERROR_IO_PENDING && error != ERROR_SUCCESS)
    {
        //
        // If this fails, there are not documented error codes.
        // However, in practice all the sender needs to know is that it failed.
        // A client-side disconnect is the most likely reason.
        //
        NTSTATUS status = K_STATUS_HTTP_API_CALL_FAILURE;
        KTraceFailedAsyncRequest(status, this, error, 0);
        Complete(status);
        return;
    }

    //  Execution continues in the operation's IOCP Completion handler
}

VOID
KHttpServerWebSocket::HttpSendResponseOperation::OnStart()
{
    //  No-op
}

VOID
KHttpServerWebSocket::HttpSendResponseOperation::IOCP_Completion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    if (Error != ERROR_SUCCESS)
    {
        NTSTATUS status = K_STATUS_IOCP_ERROR;
        KTraceFailedAsyncRequest(status, this, Error, BytesTransferred);
        Complete(status);
        return;
    }

    Complete(STATUS_SUCCESS);
}

VOID
KHttpServerWebSocket::SendResponseCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendResponseOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    //
    //  Should only be called when sending the response during the open procedure; failure e.g. due to timeout or abort may have already happened
    //
    if (GetConnectionStatus() != ConnectionStatus::OpenStarted)
    {
        KInvariant(GetConnectionStatus() == ConnectionStatus::Error);
        KDbgCheckpointWData(0, "Returning from KHttpServerWebSocket::SendResponseCallback due to failure already occurring.", _FailureStatus, ULONG_PTR(this), 0, 0, 0);
        return;
    }

    KAssert(! _IsHandshakeComplete);
    KAssert(_WebSocketHandle != nullptr);
    KInvariant(!_IsCleanedUp);
    HRESULT hr = WebSocketEndServerHandshake(_WebSocketHandle);
    if (FAILED(hr))
    {
        KDbgCheckpointWData(0, "Call to WebSocketEndServerHandshake failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(this), hr, 0, 0);
        FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
        return;
    }
    _IsHandshakeComplete = TRUE;

    //
    //  Now that the handshake is complete, we are no longer interested in the request headers and the response headers allocated by the external FSM
    //
    _RequestHeaders = nullptr;
    _ResponseHeaders.Detach();  

    NTSTATUS status;

    //
    //  Clean up resources which are unneeded once the WebSocket handshake has completed
    //
    {
        //
        //  Complete the captured KHttpServerRequest and allow it to destruct.  Note that this will not
        //  trigger a new read to be posted by the KHttpServer; this will be delayed until the WebSocket completes.
        //
        KAssert(_ServerRequest->IsUpgraded());
        _ServerRequest->Complete(STATUS_SUCCESS);
        _ServerRequest = nullptr;

        _HttpResponseUnknownHeaders = nullptr;  //  Allow the unknown headers to be deleted
        _HttpResponse.Headers.pUnknownHeaders = nullptr;  //  Clean up dangling pointer
    }
    
    status = _SendResponseOperation->Status();
    _SendResponseOperation = nullptr;  //  Allow to destruct, as no longer needed
    if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "Failing WebSocket due to SendResponseOperation completing with a failure status.", status, ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(status);
        return;
    }
    
    
    if (_OpenTimeoutTimer)
    {
        _OpenTimeoutTimer->Cancel();
    }
    _ConnectionStatus = ConnectionStatus::Open;
    CompleteOpen(STATUS_SUCCESS);

    //
    //  Start ping/pong timers
    //
    {
        if (_PingTimer)
        {
            //
            //  Ensure that if _LatestReceive is set in TransportReceiveCallback within this same timer tick, it gets set to a value different than _TimerReceive
            //
            _LatestReceive = KDateTime(((LONGLONG) KDateTime::Now()) - 1);
            _TimerReceive = _LatestReceive;

            _PingTimer->StartTimer(
                static_cast<ULONG>(GetTimingConstant(TimingConstant::PingQuietChannelPeriodMs)),
                this,
                KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::PingTimerCallback)
                );
        }

        if (_UnsolicitedPongTimer)
        {

            _UnsolicitedPongTimer->StartTimer(
                static_cast<ULONG>(GetTimingConstant(TimingConstant::PongKeepalivePeriodMs)),
                this,
                KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::UnsolicitedPongTimerCallback)
                );
        }
    }

    //
    //  Start processing on the send and receive channels
    //
    _SendRequestDequeueOperation->StartDequeue(
        this,
        KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::SendRequestDequeueCallback)
        );
    _ReceiveOperationProcessor.FSM();
}
#pragma endregion KHttpServerWebSocket_ServiceOpen


#pragma region KHttpServerWebSocket_ServiceClose
VOID
KHttpServerWebSocket::OnServiceClose()
{
    KAssert(IsInApartment());
    KDbgCheckpointWData(0, "KHttpServerWebSocket::OnServiceClose() called.", _FailureStatus, ULONG_PTR(this), 0, 0, 0);

    ConnectionStatus connectionStatus = GetConnectionStatus();
    if (connectionStatus == ConnectionStatus::Open)
    {
        _ConnectionStatus = ConnectionStatus::CloseInitiated;
    }
    else if (connectionStatus == ConnectionStatus::CloseReceived)
    {
        _ConnectionStatus = ConnectionStatus::CloseCompleting;
    }
    else
    {
        KDbgCheckpointWData(
            0,
            "Calling CompleteClose and returning from KHttpServerWebSocket::OnServiceClose() due to WebSocket already having failed and cleaned up.",
            _FailureStatus,
            ULONG_PTR(this),
            0,
            0,
            0
            );
        KInvariant(connectionStatus == ConnectionStatus::Error);
        CompleteClose(_FailureStatus);
        return;
    }

    //
    //  Start the Close Timeout timer if the Close timeout has been specified
    //
    if (_CloseTimeoutTimer)
    {
        TimingConstantValue closeTimeout = GetTimingConstant(TimingConstant::CloseTimeoutMs);
        KAssert(closeTimeout != TimingConstantValue::None);

        KDbgCheckpointWData(0, "Starting the server close timer.", STATUS_SUCCESS, ULONG_PTR(this), static_cast<ULONG>(closeTimeout), 0, 0);
        _CloseTimeoutTimer->StartTimer(
            static_cast<ULONG>(closeTimeout),
            this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::CloseTimeoutTimerCallback)
            );
    }
    
    //
    //  Enqueue the close operation.  When it is dequeued, the send frame will be sent and the send channel closed.
    //
    KInvariant(NT_SUCCESS(_SendRequestQueue->Enqueue(*_SendCloseOperation)));

    RunSendActionQueue();
}

VOID
KHttpServerWebSocket::CleanupWebSocket(
    __in NTSTATUS Status
    )
{
    KAssert(IsInApartment());
    KDbgCheckpointWData(0, "Cleaning up server WebSocket.", Status, ULONG_PTR(this), 0, 0, 0);

    KInvariant(KInterlockedSetFlag(_IsCleanedUp));

    //
    //  Deactivate the queues and drop all enqueued items
    //
    if (! _IsReceiveRequestQueueDeactivated)
    {
        _ReceiveRequestQueue->Deactivate();
        _IsReceiveRequestQueueDeactivated = TRUE;
    }
    _ReceiveRequestQueue->CancelAllEnqueued(
        KAsyncQueue<WebSocketReceiveOperation>::DroppedItemCallback(this, &KHttpWebSocket::HandleDroppedReceiveRequest)
        );
    KAssert(_ReceiveRequestQueue->GetNumberOfQueuedItems() == 0);
    
    if (! _IsSendRequestQueueDeactivated)
    {
        _SendRequestQueue->Deactivate();
        _IsSendRequestQueueDeactivated = TRUE;
    }
    _SendRequestQueue->CancelAllEnqueued(
        KAsyncQueue<WebSocketOperation>::DroppedItemCallback(this, &KHttpWebSocket::HandleDroppedSendRequest)
        );
    KAssert(_SendRequestQueue->GetNumberOfQueuedItems() == 0);


    if (_ReceiveOperationProcessor._CurrentOperation)
    {
        _ReceiveOperationProcessor._CurrentOperation->CompleteOperation(Status);
        _ReceiveOperationProcessor._CurrentOperation.Detach();
    }

    //
    //  Cancel all the timers
    //
    if (_PingTimer)
    {
        _PingTimer->Cancel();
    }
    
    if (_UnsolicitedPongTimer)
    {
        _UnsolicitedPongTimer->Cancel();
    }

    if (_PongTimeoutTimer)
    {
        _PongTimeoutTimer->Cancel();
    }

    if (_OpenTimeoutTimer)
    {
        _OpenTimeoutTimer->Cancel();
    }

    if (_CloseTimeoutTimer)
    {
        _CloseTimeoutTimer->Cancel();
    }

    //
    //  Fire off graceful or rude transport closure, depending on whether we are closing normally or due to failure (respectively)
    //
    if (_HttpRequest) // Failure may have occurred before the transport was captured
    {
        _CloseTransportOperation->StartCloseTransport(
            *_ParentServer,
            GetConnectionStatus() != ConnectionStatus::Error,
            _RequestQueueHandle,
            _HttpRequest->RequestId
            );
    }

    //
    //  Clean up the resources held by the WebSocket Protocol Component API if they had been initialized (Failure may have occurred
    //  prior to their initialization)
    //
    if (_WebSocketHandle != nullptr)  
    {
        //
        //  If the websocket failed while a transport request is outstanding, we need to complete the corresponding action before
        //  flushing the queues.  When the transport operation calls back and notices that the WebSocket has cleaned up, it MUST NOT
        //  call WebSocketCompleteAction
        //
        if (_TransportSendOperation->IsActive())
        {
            KDbgCheckpointWData(
                0,
                "Calling WebSocketCompleteAction for the transport send action from KHttpServer::CleanupWebSocket.",
                _FailureStatus,
                ULONG_PTR(this),
                ULONG_PTR(_TransportSendOperation.RawPtr()),
                ULONG_PTR(_TransportSendOperation->GetActionContext()),
                0
                );
            KAssert(_TransportSendOperation->GetActionContext() != nullptr);
            WebSocketCompleteAction(_WebSocketHandle, _TransportSendOperation->GetActionContext(), 0);
            _TransportSendOperation->ClearActionContext();
        }

        if (_ReceiveOperationProcessor._TransportReceiveOperation->IsActive())
        {
            KDbgCheckpointWData(
                0,
                "Calling WebSocketCompleteAction for the transport receive action from KHttpServer::CleanupWebSocket.",
                _FailureStatus,
                ULONG_PTR(this),
                ULONG_PTR(_ReceiveOperationProcessor._TransportReceiveOperation.RawPtr()),
                ULONG_PTR(_ReceiveOperationProcessor._TransportReceiveOperation->GetActionContext()),
                0
                );
            KAssert(_ReceiveOperationProcessor._TransportReceiveOperation->GetActionContext() != nullptr);
            WebSocketCompleteAction(_WebSocketHandle, _ReceiveOperationProcessor._TransportReceiveOperation->GetActionContext(), 0);
            _ReceiveOperationProcessor._TransportReceiveOperation->ClearActionContext();
        }

        if (_IsHandshakeStarted && !_IsHandshakeComplete)
        {
            WebSocketEndServerHandshake(_WebSocketHandle);
            _IsHandshakeComplete = TRUE;
        }

        WebSocketAbortHandle(_WebSocketHandle);
        FlushActions();
    }

    //
    //  Complete and release the KHttpServerRequest if failure occurred before it was released normally
    //
    if (_ServerRequest)
    {
        KAssert(GetConnectionStatus() == ConnectionStatus::Error);
        KAssert(_ServerRequest->IsUpgraded() || _HttpRequest == nullptr);  //  Either the request was upgraded, or failure occurred before capture
        _ServerRequest->Complete(Status);
        _ServerRequest = nullptr;
    }

    _IsSendChannelClosed = TRUE;
    _IsReceiveChannelClosed = TRUE;
    
    //
    //  Inform the KHttpServer that the WebSocket connection is closed, so that it can post another read
    //
    KDbgCheckpointWData(
        0,
        "Starting the InformOfClosureAsync to inform the KHttpServer of KWebSocket closure.",
        _FailureStatus,
        ULONG_PTR(this),
        ULONG_PTR(_ParentServer.RawPtr()),
        0,
        0
        );
    _InformOfClosureAsync->Start(
        _ParentServer.RawPtr(),
        KAsyncContextBase::CompletionCallback(_ParentServer.RawPtr(), &KHttpServer::WebSocketCompletion)
        );
}

VOID
KHttpServerWebSocket::FlushActions()
{
    KAssert(IsInApartment());

    WEB_SOCKET_BUFFER buffer;
    ULONG bufferCount;
    WEB_SOCKET_ACTION action;
    WEB_SOCKET_BUFFER_TYPE bufferType;
    PVOID applicationContext;
    PVOID actionContext;
    HRESULT hr;

    do
    {
        bufferCount = 1;

        hr = WebSocketGetAction(
            _WebSocketHandle,
            WEB_SOCKET_ALL_ACTION_QUEUE,
            &buffer,
            &bufferCount,
            &action,
            &bufferType,
            &applicationContext,
            &actionContext
            );

        if (hr != S_OK)
        {
            KDbgCheckpointWData(0, "An error occurred while flushing actions.", STATUS_UNSUCCESSFUL, ULONG_PTR(this), hr, 0, 0);
            break;
        }

        if ((action == WEB_SOCKET_SEND_TO_NETWORK_ACTION
                || action == WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION
            ) 
            && applicationContext != nullptr
            )
        {
            KDbgCheckpointWData(
                0,
                "Canceling an enqueued send operation in FlushActions()",
                STATUS_SUCCESS,
                ULONG_PTR(this),
                ULONG_PTR(applicationContext),
                0,
                0
                );
            WebSocketOperation& op = *static_cast<WebSocketOperation*>(applicationContext);
            op.CompleteOperation(STATUS_CANCELLED);
        }

        KAssert((action == WEB_SOCKET_NO_ACTION && actionContext == nullptr) || actionContext != nullptr);
        WebSocketCompleteAction(_WebSocketHandle, actionContext, 0);
        actionContext = nullptr;
    } while (action != WEB_SOCKET_NO_ACTION);
}

KDefineDefaultCreate(KHttpServerWebSocket::CloseTransportOperation, CloseOperation);

KHttpServerWebSocket::CloseTransportOperation::CloseTransportOperation()
    :
        _Overlapped(this)
{
}

KHttpServerWebSocket::CloseTransportOperation::~CloseTransportOperation()
{
}

VOID
KHttpServerWebSocket::CloseTransportOperation::StartCloseTransport(
    __in KHttpServer& ParentServer,
    __in BOOLEAN IsGracefulClose,
    __in HANDLE RequestQueueHandle,
    __in HTTP_REQUEST_ID RequestId
    )
{
    KDbgCheckpointWData(0, "Closing the server transport.", STATUS_SUCCESS, 0, ULONG_PTR(&ParentServer), IsGracefulClose, RequestId);

    //
    //  Fire and forget, but don't allow parent server to complete while this is outstanding (so that IO registration context remains valid)
    //
    Start(&ParentServer, nullptr);  

    ULONG error;
    //
    //  Close the transport connection rudely with HttpCancelHttpRequest if the WebSocket has failed, otherwise signal that the connection may be
    //  disconnected once all data has been sent by calling HttpSendResponseEntityBody with the DISCONNECT flag
    //
    if (IsGracefulClose)
    {
        error = HttpSendResponseEntityBody(
            RequestQueueHandle,
            RequestId,
            HTTP_SEND_RESPONSE_FLAG_DISCONNECT,
            0,
            nullptr,
            nullptr,
            nullptr,
            0,
            &_Overlapped,
            NULL
            );
    }
    else
    {
        error = HttpCancelHttpRequest(RequestQueueHandle, RequestId, &_Overlapped);
    }

    if (error != ERROR_IO_PENDING && error != ERROR_SUCCESS)
    {
        KTraceFailedAsyncRequest(K_STATUS_HTTP_API_CALL_FAILURE, this, error, RequestId);
        Complete(K_STATUS_HTTP_API_CALL_FAILURE);
        return;
    }

    //  Execution continues in the operation's IOCP Completion handler
}

VOID
KHttpServerWebSocket::CloseTransportOperation::OnStart()
{
    //  No-op
}


VOID
KHttpServerWebSocket::CloseTransportOperation::IOCP_Completion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    if (Error != ERROR_SUCCESS)
    {
        KTraceFailedAsyncRequest(K_STATUS_IOCP_ERROR, this, Error, BytesTransferred);
        Complete(K_STATUS_IOCP_ERROR);
        return;
    }

    Complete(STATUS_SUCCESS);
}
#pragma endregion KHttpServerWebSocket_ServiceClose


#pragma region KHttpServerWebSocket_Receive
KHttpServerWebSocket::ReceiveOperationProcessor::ReceiveOperationProcessor(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& RequestDequeueOperation,
    __in HttpReceiveEntityBodyOperation& TransportReceiveOperation
    ) : 
        _ParentWebSocket(ParentWebSocket),
        _RequestDequeueOperation(&RequestDequeueOperation),
        _TransportReceiveOperation(&TransportReceiveOperation),

        _Buffer(),
        _BufferCount(0),
        _BufferType(),
        _Action(WEB_SOCKET_NO_ACTION),
        _ActionContext(nullptr),
        _BytesReceived(0),

        _DataOffset(_Buffer.Data.ulBufferLength),
        _CurrentFrameType(FrameType::Invalid),
        _CurrentOperation(nullptr)
{
}

VOID 
KHttpServerWebSocket::ReceiveOperationProcessor::FSM()
{
    KAssert(_ParentWebSocket.IsInApartment());
    KAssert(! _ParentWebSocket._IsReceiveChannelClosed);
    KAssert(_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Open
        || _ParentWebSocket.GetConnectionStatus() == ConnectionStatus::CloseInitiated
        );

    NTSTATUS status;
    HRESULT hr = S_OK;
    while (true)
    {
        //
        //  Process any available user data
        //
        while (_DataOffset < _Buffer.Data.ulBufferLength)  //  Not all the current data has been consumed
        {
            if (! _CurrentOperation)  //  Get the next user receive operation from the queue
            {
                _RequestDequeueOperation->StartDequeue(
                    &_ParentWebSocket,
                    KAsyncContextBase::CompletionCallback(this, &ReceiveOperationProcessor::RequestDequeueCallback)
                    );
                return;
            }
            
            ULONG dataConsumed;
            BOOLEAN receiveMoreData;
            

            status = _CurrentOperation->HandleReceive(
                _Buffer.Data.pbBuffer + _DataOffset,
                _Buffer.Data.ulBufferLength - _DataOffset,
                FrameTypeToMessageContentType(_CurrentFrameType),
                KWebSocket::IsFinalFragment(_CurrentFrameType),
                dataConsumed,
                receiveMoreData
                );

            if (! NT_SUCCESS(status))
            {
                KDbgCheckpointWData(
                    0,
                    "Current receive operation returned failure status from HandleReceive.",
                    status,
                    ULONG_PTR(this),
                    ULONG_PTR(_CurrentOperation),
                    0,
                    0
                    );
                _ParentWebSocket.FailWebSocket(status);
                return;
            }

            _DataOffset += dataConsumed;
            KInvariant(_DataOffset <= _Buffer.Data.ulBufferLength);

            if (! receiveMoreData)
            {
                _CurrentOperation.Detach();
            }

            //
            //  Complete the action when the data has been processed
            //
            if (_DataOffset == _Buffer.Data.ulBufferLength)
            {
                KAssert(_ActionContext != nullptr);
                KInvariant(!_ParentWebSocket._IsCleanedUp);
                WebSocketCompleteAction(_ParentWebSocket._WebSocketHandle, _ActionContext, 0);
                _ActionContext = nullptr;
            }
        }

        //
        //  Process the next action in the WebSocket Protocol Component API receive queue
        //
        {
            _BufferCount = 1;

            KInvariant(!_ParentWebSocket._IsCleanedUp);
            hr = WebSocketGetAction(
                _ParentWebSocket._WebSocketHandle,
                WEB_SOCKET_RECEIVE_ACTION_QUEUE,
                &_Buffer,
                &_BufferCount,
                &_Action,
                &_BufferType,
                NULL,
                &_ActionContext
                );

            if (FAILED(hr))
            {
                KDbgCheckpointWData(
                    0,
                    "Call to WebSocketGetAction failed.",
                    K_STATUS_WEBSOCKET_API_CALL_FAILURE,
                    ULONG_PTR(&_ParentWebSocket),
                    hr,
                    ULONG_PTR(this),
                    0
                    );
                _ParentWebSocket.FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
                return;
            }

            KAssert(_Action == WEB_SOCKET_NO_ACTION
                || _Action == WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION
                || _Action == WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION
                );

            _DataOffset = _Buffer.Data.ulBufferLength;  //  Default to "no user data available" for next iteration

            switch (_Action)
            {
            case WEB_SOCKET_NO_ACTION:

                //
                //  If the queue is empty, all currently received data must have been processed, so issue another receive
                //

                KAssert(_ActionContext == nullptr);
                KInvariant(!_ParentWebSocket._IsCleanedUp);
                WebSocketCompleteAction(_ParentWebSocket._WebSocketHandle, _ActionContext, 0);

                KInvariant(!_ParentWebSocket._IsCleanedUp);
                hr = WebSocketReceive(_ParentWebSocket._WebSocketHandle, NULL, nullptr);
                if (FAILED(hr))
                {
                    KDbgCheckpointWData(
                        0,
                        "Call to WebSocketReceive failed.",
                        K_STATUS_WEBSOCKET_API_CALL_FAILURE,
                        ULONG_PTR(&_ParentWebSocket),
                        hr,
                        ULONG_PTR(this),
                        0
                        );
                    _ParentWebSocket.FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
                    return;
                }

                break;

            case WEB_SOCKET_RECEIVE_FROM_NETWORK_ACTION:

                //
                //  Asynchronously receive more data and callback to this FSM.
                //
                _TransportReceiveOperation->StartReceive(
                    _ParentWebSocket,
                    KAsyncContextBase::CompletionCallback(this, &ReceiveOperationProcessor::TransportReceiveCallback),
                    _ParentWebSocket._RequestQueueHandle,
                    _ParentWebSocket._HttpRequest->RequestId,
                    _Buffer.Data.pbBuffer,
                    _Buffer.Data.ulBufferLength,
                    _ActionContext
                    );

                //  Action will be completed in the transport receive callback
                return;

            case WEB_SOCKET_INDICATE_RECEIVE_COMPLETE_ACTION:

                _CurrentFrameType = WebSocketBufferTypeToFragmentType(_BufferType);
                KAssert(_CurrentFrameType != FrameType::Invalid 
                    && _CurrentFrameType != FrameType::Ping  //  According to WebSocket Protocol Component API docs, Ping is not visible on receive
                    );  

                switch (_CurrentFrameType)
                {

                case FrameType::Pong:

                    KAssert(_ActionContext != nullptr);
                    KInvariant(!_ParentWebSocket._IsCleanedUp);
                    WebSocketCompleteAction(_ParentWebSocket._WebSocketHandle, _ActionContext, 0);
                    _ActionContext = nullptr;

                    _ParentWebSocket._LatestReceivedPong = KDateTime::Now();
                    KAssert(_ParentWebSocket._LatestReceivedPong != _ParentWebSocket._TimerReceivedPong);

                    KDbgCheckpointWData(
                        0,
                        "PONG received.",
                        STATUS_SUCCESS,
                        ULONG_PTR(&_ParentWebSocket),
                        LONGLONG(_ParentWebSocket._LatestReceivedPong),
                        LONGLONG(_ParentWebSocket._TimerReceivedPong),
                        0
                        );

                    break;

                case FrameType::Close:
                    
                    //
                    //  If the remote close status is longer than the largest allowed control frame payload, the standard has been violated.
                    //
                    if (_Buffer.CloseStatus.ulReasonLength > MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES)
                    {
                        KDbgCheckpointWData(
                            0,
                            "Close reason exceeds maximum control frame payload size; WebSocket protocol violation.",
                            K_STATUS_WEBSOCKET_STANDARD_VIOLATION,
                            ULONG_PTR(&_ParentWebSocket),
                            ULONG_PTR(this),
                            _Buffer.CloseStatus.ulReasonLength,
                            MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES
                            );

                        _ParentWebSocket.FailWebSocket(
                            K_STATUS_WEBSOCKET_STANDARD_VIOLATION,
                            _Buffer.CloseStatus.ulReasonLength,
                            MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES
                            );
                        return;
                    }

                    KInvariant(_ParentWebSocket._RemoteCloseReasonBuffer->QuerySize() >= MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES);
                    KInvariant(_Buffer.CloseStatus.ulReasonLength <= MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES);
                    KMemCpySafe(
                        _ParentWebSocket._RemoteCloseReasonBuffer->GetBuffer(),
                        _ParentWebSocket._RemoteCloseReasonBuffer->QuerySize(),
                        _Buffer.CloseStatus.pbReason,
                        _Buffer.CloseStatus.ulReasonLength
                        );

                    status = _ParentWebSocket._RemoteCloseReason->MapBackingBuffer(
                        *_ParentWebSocket._RemoteCloseReasonBuffer,
                        _Buffer.CloseStatus.ulReasonLength,
                        0,
                        _Buffer.CloseStatus.ulReasonLength
                        );
                    KAssert(NT_SUCCESS(status));

                    _ParentWebSocket._RemoteCloseStatusCode = static_cast<CloseStatusCode>(_Buffer.CloseStatus.usStatus);

                    if (_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Open)
                    {
                        _ParentWebSocket._ConnectionStatus = ConnectionStatus::CloseReceived;
                    }
                    else  // _ParentWebSocket.GetConnectionStatus() == ConnectionStatus::CloseInitiated
                    {
                        _ParentWebSocket._ConnectionStatus = ConnectionStatus::CloseCompleting;
                    }

                    //
                    //  Inform the WebSocket user that a close has been received
                    //
                    KAssert(_ParentWebSocket._CloseReceivedCallback);
                    _ParentWebSocket._CloseReceivedCallback(_ParentWebSocket);

                    KAssert(_ActionContext != nullptr);
                    KInvariant(!_ParentWebSocket._IsCleanedUp);
                    WebSocketCompleteAction(_ParentWebSocket._WebSocketHandle, _ActionContext, 0);
                    _ActionContext = nullptr;

                    //
                    //  Deactivate the receive queue, canceling any queued receives (as no more frames will be received)
                    //
                    _ParentWebSocket._ReceiveRequestQueue->Deactivate();
                    _ParentWebSocket._IsReceiveRequestQueueDeactivated = TRUE;
                    _ParentWebSocket._ReceiveRequestQueue->CancelAllEnqueued(
                        KAsyncQueue<WebSocketReceiveOperation>::DroppedItemCallback(&_ParentWebSocket, &KHttpWebSocket::HandleDroppedReceiveRequest)
                    );

                    //
                    //  Inform the parent WebSocket that the receive processor has completed successfully
                    //
                    _ParentWebSocket.ReceiveProcessorCompleted(); 

                    return;
                    
                default: // Data frame, so user data ready to be processed

                    _DataOffset = 0;
                    //  Action will be completed after the data is processed
                }

                break;

            default:

                KInvariant(FALSE);  //  According to WebSocket Protocol Component API contract, should NEVER happen
            }
        }

        //
        //  Processing a receive action may have silently added an action to the send action queue, e.g. a PONG so it is necessary
        //  to stimulate the send FSM in case it is idle
        //
        _ParentWebSocket.RunSendActionQueue();  
    }
}

VOID
KHttpServerWebSocket::ReceiveOperationProcessor::RequestDequeueCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(_ParentWebSocket.IsInApartment());
    KAssert(Parent == &_ParentWebSocket);
    KAssert(&CompletingOperation == _RequestDequeueOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    NTSTATUS status = _RequestDequeueOperation->Status();
    if (status == K_STATUS_SHUTDOWN_PENDING)  //  The queue is being deactivated, not necessarily due to failure
    {
        KDbgCheckpointWData(0, "Receive request dequeue failed due to queue shutting down.", status, ULONG_PTR(&_ParentWebSocket), 0, 0, 0);
        return;
    }
    else if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "Receive request dequeue failed.", status, ULONG_PTR(&_ParentWebSocket), 0, 0, 0);
        _ParentWebSocket.FailWebSocket(status);
        return;
    }

    KDbgCheckpointWData(0, "Receive request dequeued.", status, ULONG_PTR(&_ParentWebSocket), 0, 0, 0);
    _CurrentOperation = &_RequestDequeueOperation->GetDequeuedItem();

    if (_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Error)
    {
        KDbgCheckpointWData(0, "Aborting the dequeued receive request as the WebSocket has failed.", _ParentWebSocket._FailureStatus, ULONG_PTR(&_ParentWebSocket), 0, 0, 0);
        _CurrentOperation->CompleteOperation(STATUS_REQUEST_ABORTED);
        _CurrentOperation.Detach();
        return;
    }

    _RequestDequeueOperation->Reuse();
    
    FSM();
}

KDefineDefaultCreate(KHttpServerWebSocket::HttpReceiveEntityBodyOperation, ReceiveEntityBodyOperation);

KHttpServerWebSocket::HttpReceiveEntityBodyOperation::HttpReceiveEntityBodyOperation()
    :
        _BytesReceived(0),
        _ActionContext(nullptr),
        _IsActive(FALSE),

        _Overlapped(this)
{
}

KHttpServerWebSocket::HttpReceiveEntityBodyOperation::~HttpReceiveEntityBodyOperation()
{
}

ULONG
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::GetBytesReceived()
{
    return _BytesReceived;
}

VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::StartReceive(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HANDLE const & RequestQueueHandle,
    __in HTTP_REQUEST_ID const & HttpRequestId,
    __out_bcount(BufferSizeInBytes) PBYTE const & Buffer,
    __in ULONG const & BufferSizeInBytes,
    __in PVOID const & ActionContext
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    _ActionContext = ActionContext;

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    ULONG error = HttpReceiveRequestEntityBody(
        RequestQueueHandle,
        HttpRequestId,
        0,
        Buffer,
        BufferSizeInBytes,
        nullptr,
        &_Overlapped
        );

    if (error != NO_ERROR && error != ERROR_IO_PENDING)
    {
        NTSTATUS status = K_STATUS_HTTP_API_CALL_FAILURE;
        KTraceFailedAsyncRequest(status, this, error, HttpRequestId);
        Complete(status);
        return;
    }

    //  Execution continues in the operation's IOCP Completion handler
}

VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::OnStart()
{
    //  No-op
}

VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::OnReuse()
{
    _BytesReceived = 0;
}

VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::IOCP_Completion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    NTSTATUS status;

    if (Error == ERROR_OPERATION_ABORTED)
    {
        status = STATUS_REQUEST_ABORTED;

        KDbgCheckpointWData(
            0,
            "HttpReceiveEntityBody called back with ERROR_OPERATION_ABORTED.  Completing the operation with STATUS_REQUEST_ABORTED.",
            status,
            0,
            ULONG_PTR(this),
            Error,
            BytesTransferred
            );
        KTraceFailedAsyncRequest(status, this, Error, BytesTransferred);
        Complete(status);
        return;
    }

    if (FAILED(HRESULT_FROM_WIN32(Error)))  //  Some other error
    {
        status = K_STATUS_IOCP_ERROR;

        KDbgCheckpointWData(
            0,
            "HttpReceiveEntityBody called back with an error status.  Completing the operation with K_STATUS_IOCP_ERROR.",
            status,
            0,
            ULONG_PTR(this),
            Error,
            BytesTransferred
            );
        KTraceFailedAsyncRequest(status, this, Error, BytesTransferred);
        Complete(status);
        return;
    }

    status = STATUS_SUCCESS;
    KDbgCheckpointWData(
        0,
        "HttpReceiveEntityBody called back with with a success status.",
        status,
        0,
        ULONG_PTR(this),
        Error,
        BytesTransferred
        );
    _BytesReceived = BytesTransferred;
    Complete(STATUS_SUCCESS);
}


BOOLEAN
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::SetActive(
    __in BOOLEAN IsActive
)
{
    _IsActive = IsActive;
}

PVOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::GetActionContext()
{
    return _ActionContext;
}


VOID
KHttpServerWebSocket::HttpReceiveEntityBodyOperation::ClearActionContext()
{
    _ActionContext = nullptr;
}

VOID
KHttpServerWebSocket::ReceiveOperationProcessor::TransportReceiveCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(_ParentWebSocket.IsInApartment());
    KAssert(Parent == &_ParentWebSocket);
    KAssert(&CompletingOperation == _TransportReceiveOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_TransportReceiveOperation->IsActive());
    _TransportReceiveOperation->SetActive(FALSE);

    if (_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Error)
    {
        KDbgCheckpointWData(
            0,
            "WebSocket has already failed.  WebSocketCompleteAction has already been called.",
            _ParentWebSocket._FailureStatus,
            ULONG_PTR(Parent),
            ULONG_PTR(this),
            ULONG_PTR(_TransportReceiveOperation.RawPtr()),
            _TransportReceiveOperation->Status()
            );
        KAssert(! _CurrentOperation);
        return;
    }
    KAssert(! _ParentWebSocket.IsStateTerminal());

    _BytesReceived = _TransportReceiveOperation->GetBytesReceived();

    KAssert(_ActionContext != nullptr);
    KInvariant(!_ParentWebSocket._IsCleanedUp);
    WebSocketCompleteAction(_ParentWebSocket._WebSocketHandle, _ActionContext, _BytesReceived);
    _ActionContext = nullptr;

    if (! NT_SUCCESS(_TransportReceiveOperation->Status()))
    {
        KDbgCheckpointWData(
            0,
            "Transport receive operation failed.",
            _TransportReceiveOperation->Status(),
            ULONG_PTR(&_ParentWebSocket),
            ULONG_PTR(this),
            ULONG_PTR(_TransportReceiveOperation.RawPtr()),
            _BytesReceived
            );
        _ParentWebSocket.FailWebSocket(_TransportReceiveOperation->Status(), ULONG_PTR(_TransportReceiveOperation.RawPtr()), _BytesReceived);
        return;
    }

    KDbgCheckpointWData(
        0,
        "Transport receive operation succeeded.",
        _TransportReceiveOperation->Status(),
        ULONG_PTR(&_ParentWebSocket),
        ULONG_PTR(this),
        ULONG_PTR(_TransportReceiveOperation.RawPtr()),
        _BytesReceived
        );

    _ParentWebSocket._LatestReceive = KDateTime::Now();
    KAssert(_ParentWebSocket._LatestReceive != _ParentWebSocket._TimerReceive);

    _TransportReceiveOperation->Reuse();
    FSM();
}
    
KWebSocket::FrameType
KHttpServerWebSocket::ReceiveOperationProcessor::WebSocketBufferTypeToFragmentType(
    __in WEB_SOCKET_BUFFER_TYPE const & WebSocketBufferType
    )
{
    switch (WebSocketBufferType)
    {
    case WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
        return FrameType::TextFinalFragment;

    case WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
        return FrameType::TextContinuationFragment;

    case WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
        return FrameType::BinaryFinalFragment;

    case WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
        return FrameType::BinaryContinuationFragment;

    case WEB_SOCKET_CLOSE_BUFFER_TYPE:
        return FrameType::Close;

    case WEB_SOCKET_PING_PONG_BUFFER_TYPE:
    case WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE:
        return FrameType::Pong;

    default:
        return FrameType::Invalid;
    }
}
#pragma endregion KHttpServerWebSocket_Receive


#pragma region KHttpServerWebSocket_Send
VOID
KHttpServerWebSocket::RunSendActionQueue()
{
    KAssert(IsInApartment());

    HRESULT hr = S_OK;
    PVOID applicationContext = nullptr;
    WEB_SOCKET_BUFFER buffer;
    ULONG bufferCount;
    WEB_SOCKET_ACTION action;
    WEB_SOCKET_BUFFER_TYPE bufferType;
    PVOID actionContext;

    //
    //  Process the next action in the WebSocket Protocol Component API send queue
    //
    do
    {
        if (_CloseSent)
        {
            return;  //  Do not run the queue if the close frame has finished sending; the other side cannot be expected to be listening.
        }


        bufferCount = 1;

        KInvariant(! _IsCleanedUp);
        hr = WebSocketGetAction(
            _WebSocketHandle,
            WEB_SOCKET_SEND_ACTION_QUEUE,
            &buffer,
            &bufferCount,
            &action,
            &bufferType,
            &applicationContext,
            &actionContext
            );

        if (FAILED(hr))
        {
            KDbgCheckpointWData(0, "Call to WebSocketGetAction failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(this), hr, 0, 0);
            FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
            return;
        }

        KAssert(action == WEB_SOCKET_SEND_TO_NETWORK_ACTION
            || action == WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION
            || action == WEB_SOCKET_NO_ACTION
            );

        switch (action)
        {

        case WEB_SOCKET_SEND_TO_NETWORK_ACTION:

            KAssert(! _TransportSendOperation->IsActive());
            _TransportSendOperation->StartSend(
                *this,
                KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::TransportSendCallback),
                _RequestQueueHandle,
                _HttpRequest->RequestId,
                buffer.Data.pbBuffer,
                buffer.Data.ulBufferLength,
                actionContext
                );

            //  Action will be completed in the transport send callback

            return;

        case WEB_SOCKET_INDICATE_SEND_COMPLETE_ACTION:

            if (applicationContext != nullptr) 
            {
                WebSocketOperation& op = *static_cast<WebSocketOperation*>(applicationContext);
                if (_IsSendTruncated)
                {
                    KDbgCheckpointWData(0, "Truncated transport send.", K_STATUS_TRUNCATED_TRANSPORT_SEND, ULONG_PTR(this), 0, 0, 0);
                    KAssert(actionContext != nullptr);
                    KInvariant(!_IsCleanedUp);
                    WebSocketCompleteAction(_WebSocketHandle, actionContext, 0);
                    actionContext = nullptr;

                    op.CompleteOperation(K_STATUS_TRUNCATED_TRANSPORT_SEND);
                    FailWebSocket(K_STATUS_TRUNCATED_TRANSPORT_SEND);
                    return;
                }

                if (op.GetOperationType() == UserOperationType::CloseSend)
                {
                    KAssert(! _CloseSent);
                    _CloseSent = TRUE;
                }

                op.CompleteOperation(STATUS_SUCCESS);
            }

            KAssert(actionContext != nullptr);
            KInvariant(!_IsCleanedUp);
            WebSocketCompleteAction(_WebSocketHandle, actionContext, 0);
            actionContext = nullptr;

            break;

        case WEB_SOCKET_NO_ACTION:

            KAssert(actionContext == nullptr);
            KInvariant(!_IsCleanedUp);
            WebSocketCompleteAction(_WebSocketHandle, actionContext, 0);

            break;

        default:

            KInvariant(FALSE);  //  According to WebSocket Protocol Component API contract, should NEVER happen
        }
    } while (action != WEB_SOCKET_NO_ACTION);
}


VOID
KHttpServerWebSocket::SendRequestDequeueCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendRequestDequeueOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    NTSTATUS status;

    status = _SendRequestDequeueOperation->Status();
    if (status == K_STATUS_SHUTDOWN_PENDING)  //  The queue is being deactivated, not necessarily due to failure
    {
        return;
    }
    else if (! NT_SUCCESS(status))
    {
        KDbgCheckpointWData(0, "SendRequestDequeueOperation completed with failure status.", status, ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(status);
        return;
    }

    WebSocketOperation& op = _SendRequestDequeueOperation->GetDequeuedItem();

    //
    //  If the WebSocket has failed, complete the operation
    //
    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        op.CompleteOperation(STATUS_REQUEST_ABORTED);
        return;
    }

    if (op.GetOperationType() == UserOperationType::FragmentSend)
    {
        KHttpServerSendFragmentOperation& sendFragmentOperation = static_cast<KHttpServerSendFragmentOperation&>(op);

        KInvariant(!_IsCleanedUp);
        sendFragmentOperation.DoSend(_WebSocketHandle);
        if (GetConnectionStatus() == ConnectionStatus::Error)
        {
            sendFragmentOperation.CompleteOperation(STATUS_UNSUCCESSFUL);
            return;
        }

        //
        //  Start next dequeue
        //
        _SendRequestDequeueOperation->Reuse();
        _SendRequestDequeueOperation->StartDequeue(
            this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::SendRequestDequeueCallback)
            );
        
        return;
    }
    else if (op.GetOperationType() == UserOperationType::MessageSend)
    {
        KHttpServerSendMessageOperation& sendMessageOperation = static_cast<KHttpServerSendMessageOperation&>(op);

        KInvariant(!_IsCleanedUp);
        sendMessageOperation.DoSend(_WebSocketHandle);
        if (GetConnectionStatus() == ConnectionStatus::Error)
        {
            sendMessageOperation.CompleteOperation(STATUS_UNSUCCESSFUL);
            return;
        } 

        //
        //  Start next dequeue
        //
        _SendRequestDequeueOperation->Reuse();
        _SendRequestDequeueOperation->StartDequeue(
            this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::SendRequestDequeueCallback)
            );

        return;
    }
    else
    {
        KInvariant(op.GetOperationType() == UserOperationType::CloseSend);
        KHttpServerSendCloseOperation& sendCloseOperation = static_cast<KHttpServerSendCloseOperation&>(op);

        _SendRequestQueue->Deactivate();
        _IsSendRequestQueueDeactivated = TRUE;
        _SendRequestQueue->CancelAllEnqueued(KAsyncQueue<WebSocketOperation>::DroppedItemCallback(this, &KHttpWebSocket::HandleDroppedSendRequest));

        KInvariant(!_IsCleanedUp);
        sendCloseOperation.StartSendClose(
            *this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::SendCloseCallback),
            _WebSocketHandle
            );
        if (GetConnectionStatus() == ConnectionStatus::Error)
        {
            sendCloseOperation.CompleteOperation(STATUS_UNSUCCESSFUL);
            return;
        }

        return;
    }
}

KDefineDefaultCreate(KHttpServerWebSocket::HttpSendEntityBodyOperation, SendEntityBodyOperation);

KHttpServerWebSocket::HttpSendEntityBodyOperation::HttpSendEntityBodyOperation(
    )
    :
        _BytesSent(0),
        _ActionContext(nullptr),
        _IsActive(FALSE),

        _Overlapped(this)

{
    RtlZeroMemory(&_Chunk, sizeof _Chunk);  //  Required by http.sys api
}

KHttpServerWebSocket::HttpSendEntityBodyOperation::~HttpSendEntityBodyOperation()
{
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::StartSend(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HANDLE RequestQueueHandle,
    __in HTTP_REQUEST_ID HttpRequestId,
    __in_bcount(BufferSizeInBytes) PBYTE Buffer,
    __in ULONG BufferSizeInBytes,
    __in_opt PVOID ActionContext
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    _Chunk.DataChunkType = HttpDataChunkFromMemory;
    _Chunk.FromMemory.pBuffer = Buffer;
    _Chunk.FromMemory.BufferLength = BufferSizeInBytes;

    _ActionContext = ActionContext;

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    //  We use one outstanding transport send, so buffering will increase performance
    ULONG flags = HTTP_SEND_RESPONSE_FLAG_BUFFER_DATA | HTTP_SEND_RESPONSE_FLAG_MORE_DATA;  

    ULONG error = HttpSendResponseEntityBody(
        RequestQueueHandle,
        HttpRequestId,
        flags,
        1,
        &_Chunk,
        nullptr,
        0,
        0,
        &_Overlapped,
        nullptr
        );

    if (error != NO_ERROR && error != ERROR_IO_PENDING)
    {
        NTSTATUS status = K_STATUS_HTTP_API_CALL_FAILURE;
        KTraceFailedAsyncRequest(status, this, error, HttpRequestId);
        Complete(status);
        return;
    }

    //  Execution continues in the operation's IOCP Completion handler
}

ULONG
KHttpServerWebSocket::HttpSendEntityBodyOperation::GetBufferSizeInBytes()
{
    return _Chunk.FromMemory.BufferLength;
}

ULONG
KHttpServerWebSocket::HttpSendEntityBodyOperation::GetBytesSent()
{
    return _BytesSent;
}

PVOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::GetActionContext()
{
    return _ActionContext;
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::ClearActionContext()
{
    _ActionContext = nullptr;
}

BOOLEAN
KHttpServerWebSocket::HttpSendEntityBodyOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::SetActive(
    __in BOOLEAN IsActive
)
{
    _IsActive = IsActive;
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::OnStart()
{
    //  No-op
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::IOCP_Completion(
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    NTSTATUS status;

    if (Error == ERROR_OPERATION_ABORTED)
    {
        status = STATUS_REQUEST_ABORTED;
        KTraceFailedAsyncRequest(status, this, Error, 0);
        KAssert(_BytesSent == 0);
        Complete(status);
        return;
    }

    if (FAILED(HRESULT_FROM_WIN32(Error)))  //  Some other error
    {
        status = K_STATUS_IOCP_ERROR;
        KTraceFailedAsyncRequest(status, this, Error, 0);
        KAssert(_BytesSent == 0);
        Complete(status);
        return;
    }

    _BytesSent = BytesTransferred;
    Complete(STATUS_SUCCESS);
}

VOID
KHttpServerWebSocket::HttpSendEntityBodyOperation::OnReuse()
{
    _ActionContext = nullptr;
    _BytesSent = 0;
    RtlZeroMemory(&_Chunk, sizeof _Chunk);  //  Required by http.sys api
}


VOID
KHttpServerWebSocket::TransportSendCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _TransportSendOperation.RawPtr());
    KAssert(IsOpenCompleted() && GetConnectionStatus() != ConnectionStatus::Closed);

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_TransportSendOperation->IsActive());
    _TransportSendOperation->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;  //  If the WebSocket has already been cleaned up, WebSocketCompleteAction has already been called
    }
    KAssert(! IsStateTerminal());

    KAssert(_TransportSendOperation->GetActionContext() != nullptr);
    KInvariant(!_IsCleanedUp);
    WebSocketCompleteAction(_WebSocketHandle, _TransportSendOperation->GetActionContext(), _TransportSendOperation->GetBytesSent());
    _TransportSendOperation->ClearActionContext();

    if (! NT_SUCCESS(_TransportSendOperation->Status()))
    {
        KDbgCheckpointWData(
            0,
            "Transport send operation failed.",
            _TransportSendOperation->Status(),
            ULONG_PTR(this),
            ULONG_PTR(_TransportSendOperation.RawPtr()),
            _TransportSendOperation->GetBytesSent(),
            _TransportSendOperation->GetBufferSizeInBytes()
            );
        FailWebSocket(_TransportSendOperation->Status());
        return;
    }

    if (_TransportSendOperation->GetBytesSent() != _TransportSendOperation->GetBufferSizeInBytes())
    {
        KDbgCheckpointWData(
            0,
            "Truncated transport send.",
            K_STATUS_TRUNCATED_TRANSPORT_SEND,
            ULONG_PTR(this),
            ULONG_PTR(_TransportSendOperation.RawPtr()),
            _TransportSendOperation->GetBytesSent(),
            _TransportSendOperation->GetBufferSizeInBytes()
            );

        //
        //  The operation and WebSocket need to be failed, but this must be delayed until the send action queue
        //  returns the context (user operation)
        //
        _IsSendTruncated = TRUE; 
    }
    else
    {
        KDbgCheckpointWData(
            0,
            "Transport send operation succeeded.",
            STATUS_SUCCESS,
            ULONG_PTR(this),
            ULONG_PTR(_TransportSendOperation.RawPtr()),
            _TransportSendOperation->GetBytesSent(),
            _TransportSendOperation->GetBufferSizeInBytes()
            );
    }
    
    _TransportSendOperation->Reuse();
    RunSendActionQueue();
}

KDefineDefaultCreate(KHttpServerWebSocket::KHttpServerSendCloseOperation, SendCloseOperation);

KHttpServerWebSocket::KHttpServerSendCloseOperation::KHttpServerSendCloseOperation()
    :
        WebSocketSendCloseOperation()
{
}

KHttpServerWebSocket::KHttpServerSendCloseOperation::~KHttpServerSendCloseOperation()
{
}

VOID
KHttpServerWebSocket::KHttpServerSendCloseOperation::OnStart()
{
    //  No-op
}

VOID
KHttpServerWebSocket::KHttpServerSendCloseOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID
KHttpServerWebSocket::KHttpServerSendCloseOperation::StartSendClose(
    __in KHttpServerWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in WEB_SOCKET_HANDLE WebSocketHandle
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    WEB_SOCKET_CLOSE_STATUS closeStatus;
    if (ParentWebSocket.GetRemoteWebSocketCloseStatusCode() == KWebSocket::CloseStatusCode::InvalidStatusCode)
    {
        closeStatus = static_cast<_WEB_SOCKET_CLOSE_STATUS>(ParentWebSocket.GetLocalWebSocketCloseStatusCode());
    }
    else
    {
        closeStatus = static_cast<_WEB_SOCKET_CLOSE_STATUS>(ParentWebSocket.GetRemoteWebSocketCloseStatusCode());
    }

    if (! ParentWebSocket.GetLocalCloseReason() || ParentWebSocket.GetLocalCloseReason()->IsEmpty())
    {
        _WebSocketBuffer.CloseStatus.pbReason = NULL;
        _WebSocketBuffer.CloseStatus.ulReasonLength = 0;
        _WebSocketBuffer.CloseStatus.usStatus = static_cast<USHORT>(closeStatus);
    }
    else
    {
        _WebSocketBuffer.CloseStatus.pbReason = static_cast<PBYTE>((PVOID) *ParentWebSocket.GetLocalCloseReason());
        _WebSocketBuffer.CloseStatus.ulReasonLength = __min(ParentWebSocket.GetLocalCloseReason()->LengthInBytes(), WEB_SOCKET_MAX_CLOSE_REASON_LENGTH);
        _WebSocketBuffer.CloseStatus.usStatus = static_cast<USHORT>(closeStatus);
    }

    HRESULT hr;
    hr = WebSocketSend(
        WebSocketHandle,
        WEB_SOCKET_BUFFER_TYPE::WEB_SOCKET_CLOSE_BUFFER_TYPE,
        &_WebSocketBuffer,
        static_cast<WebSocketOperation*>(this)
        );

    if (FAILED(hr))
    {
        KDbgCheckpointWData(
            0,
            "Call to WebSocketSend failed.",
            K_STATUS_WEBSOCKET_API_CALL_FAILURE,
            ULONG_PTR(&ParentWebSocket),
            ULONG_PTR(this),
            hr,
            0
            );
        Complete(K_STATUS_WEBSOCKET_API_CALL_FAILURE);
        ParentWebSocket.FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
        return;
    }

    ParentWebSocket.RunSendActionQueue();
}

VOID
KHttpServerWebSocket::SendCloseCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendCloseOperation.RawPtr());
    KAssert(IsCloseStarted() || GetConnectionStatus() == ConnectionStatus::Error);

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }

    if (! NT_SUCCESS(_SendCloseOperation->Status()))
    {
        KDbgCheckpointWData(
            0,
            "SendCloseOperation completed with error status.",
            _SendCloseOperation->Status(),
            ULONG_PTR(this),
            ULONG_PTR(_SendCloseOperation.RawPtr()),
            0,
            0
            );
        FailWebSocket(_SendCloseOperation->Status());
        return;
    }

    SendChannelCompleted();  // Inform WebSocket that the send processor has completed successfully
}

#pragma region KHttpServerSendFragmentOperation
NTSTATUS
KHttpServerWebSocket::CreateSendFragmentOperation(
    __out SendFragmentOperation::SPtr& SendOperation
    ) 
{
    return KHttpServerSendFragmentOperation::Create(SendOperation, *this, GetThisAllocator(), GetThisAllocationTag());
}

KDefineSingleArgumentCreate(KWebSocket::SendFragmentOperation, KHttpServerWebSocket::KHttpServerSendFragmentOperation, SendOperation, KHttpServerWebSocket&, WebSocketContext);

KHttpServerWebSocket::KHttpServerSendFragmentOperation::KHttpServerSendFragmentOperation(
    __in KHttpServerWebSocket& WebSocketContext
    )
    :
        WebSocketSendFragmentOperation(WebSocketContext, UserOperationType::FragmentSend)
{
}

KHttpServerWebSocket::KHttpServerSendFragmentOperation::~KHttpServerSendFragmentOperation()
{
}

VOID
KHttpServerWebSocket::KHttpServerSendFragmentOperation::StartSendFragment(
    __in KBuffer& Data,
    __in BOOLEAN IsFinalFragment,
    __in MessageContentType FragmentDataFormat,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
    __in ULONG const Offset,
    __in ULONG const Size
    )
{
    _Buffer = KBuffer::SPtr(&Data);
    _IsFinalFragment = IsFinalFragment;
    _MessageContentType = FragmentDataFormat;
    _Offset = Offset;
    _BufferSizeInBytes = __min(Size, _Buffer->QuerySize() - _Offset);

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerWebSocket::KHttpServerSendFragmentOperation::OnStart()
{
    NTSTATUS status;

    status = static_cast<KHttpServerWebSocket&>(*_Context)._SendRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

VOID
KHttpServerWebSocket::KHttpServerSendFragmentOperation::DoSend(
    __in WEB_SOCKET_HANDLE WebSocketHandle
    )
{
    KAssert(_Context->IsInApartment());

    KHttpServerWebSocket& Parent = static_cast<KHttpServerWebSocket&>(*_Context);

    _WebSocketBuffer.Data.pbBuffer = GetData();
    _WebSocketBuffer.Data.ulBufferLength = GetDataLength();

    WEB_SOCKET_BUFFER_TYPE bufferType = GetWebSocketBufferType(
        GetContentType(), 
        IsFinalFragment()
        );

    if (bufferType == -1)
    {
        KDbgCheckpointWData(0, "Unknown buffer type.", K_STATUS_OBJECT_UNKNOWN_TYPE, ULONG_PTR(&Parent), ULONG_PTR(this), bufferType, 0);
        Parent.FailWebSocket(K_STATUS_OBJECT_UNKNOWN_TYPE, bufferType);
        return;
    }

    HRESULT hr;
    hr = WebSocketSend(
        WebSocketHandle,
        bufferType,
        &_WebSocketBuffer,
        static_cast<WebSocketOperation*>(this)
        );

    if (FAILED(hr))
    {
        KDbgCheckpointWData(0, "Call to WebSocketSend failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(&Parent), ULONG_PTR(this), hr, 0);
        Parent.FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
        return;
    }

    Parent.RunSendActionQueue();
}

VOID
KHttpServerWebSocket::KHttpServerSendFragmentOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID KHttpServerWebSocket::KHttpServerSendFragmentOperation::OnCompleted()
{
    _Buffer = nullptr;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}

WEB_SOCKET_BUFFER_TYPE
KHttpServerWebSocket::KHttpServerSendFragmentOperation::GetWebSocketBufferType(
    __in MessageContentType const & ContentType,
    __in BOOLEAN const & IsFinalFragment
    )
{
    switch (ContentType)
    {
    case MessageContentType::Binary:
        return IsFinalFragment ? WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE : WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
    case MessageContentType::Text:
        return IsFinalFragment ? WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE : WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
    default:
        return static_cast<WEB_SOCKET_BUFFER_TYPE>(-1);
    }
}
#pragma endregion KHttpServerSendFragmentOperation

#pragma region KHttpServerSendMessageOperation
NTSTATUS
KHttpServerWebSocket::CreateSendMessageOperation(
    __out SendMessageOperation::SPtr& SendOperation
    ) 
{
    return KHttpServerSendMessageOperation::Create(SendOperation, *this, GetThisAllocator(), GetThisAllocationTag());
}

KDefineSingleArgumentCreate(KWebSocket::SendMessageOperation, KHttpServerWebSocket::KHttpServerSendMessageOperation, SendOperation, KHttpServerWebSocket&, WebSocketContext);

KHttpServerWebSocket::KHttpServerSendMessageOperation::KHttpServerSendMessageOperation(
    __in KHttpServerWebSocket& WebSocketContext
    )
    :
        WebSocketSendFragmentOperation(WebSocketContext, UserOperationType::MessageSend)
{
}

KHttpServerWebSocket::KHttpServerSendMessageOperation::~KHttpServerSendMessageOperation()
{
}

VOID
KHttpServerWebSocket::KHttpServerSendMessageOperation::StartSendMessage(
    __in KBuffer& Data,
    __in MessageContentType MessageDataFormat,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr,
    __in ULONG const Offset,
    __in ULONG const Size
    )
{
    _Buffer = KBuffer::SPtr(&Data);
    _MessageContentType = MessageDataFormat;
    _Offset = Offset;
    _BufferSizeInBytes = __min(Size, _Buffer->QuerySize() - _Offset);

    _IsFinalFragment = TRUE;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpServerWebSocket::KHttpServerSendMessageOperation::OnStart()
{
    NTSTATUS status;

    status = static_cast<KHttpServerWebSocket&>(*_Context)._SendRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

VOID
KHttpServerWebSocket::KHttpServerSendMessageOperation::DoSend(
    __in WEB_SOCKET_HANDLE WebSocketHandle
    )
{
    KAssert(_Context->IsInApartment());

    KHttpServerWebSocket& Parent = static_cast<KHttpServerWebSocket&>(*_Context);

    _WebSocketBuffer.Data.pbBuffer = GetData();
    _WebSocketBuffer.Data.ulBufferLength = GetDataLength();

    WEB_SOCKET_BUFFER_TYPE bufferType = GetWebSocketBufferType(
        GetContentType()
        );

    if (bufferType == -1)
    {
        KDbgCheckpointWData(0, "Unknown buffer type.", K_STATUS_OBJECT_UNKNOWN_TYPE, ULONG_PTR(&Parent), ULONG_PTR(this), bufferType, 0);
        Parent.FailWebSocket(K_STATUS_OBJECT_UNKNOWN_TYPE, bufferType);
        return;
    }

    HRESULT hr;
    hr = WebSocketSend(
        WebSocketHandle,
        bufferType,
        &_WebSocketBuffer,
        static_cast<WebSocketOperation*>(this)
        );

    if (FAILED(hr))
    {
        KDbgCheckpointWData(0, "Call to WebSocketSend failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(&Parent), ULONG_PTR(this), hr, 0);
        Parent.FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
        return;
    }

    Parent.RunSendActionQueue();
}

VOID
KHttpServerWebSocket::KHttpServerSendMessageOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID KHttpServerWebSocket::KHttpServerSendMessageOperation::OnCompleted()
{
    _Buffer = nullptr;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}

WEB_SOCKET_BUFFER_TYPE
KHttpServerWebSocket::KHttpServerSendMessageOperation::GetWebSocketBufferType(
    __in MessageContentType const & ContentType
    )
{
    return KHttpServerWebSocket::KHttpServerSendFragmentOperation::GetWebSocketBufferType(ContentType, TRUE);
}
#pragma endregion KHttpServerSendMessageOperation
#pragma endregion KHttpServerWebSocket_Send


#pragma region KHttpServerWebSocket_TimerCallbacks
VOID
KHttpServerWebSocket::PingTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KInvariant(IsInApartment());
    KInvariant(Parent == this);
    KInvariant(&CompletingOperation == _PingTimer.RawPtr());
    KInvariant(IsOpenCompleted());

    if (_PingTimer->Status() == STATUS_CANCELLED || IsStateTerminal() || _CloseSent)
    {
        KDbgCheckpointWData(0, "Disabling Ping timer.", _PingTimer->Status(), ULONG_PTR(this), IsStateTerminal(), _CloseSent, 0);
        return;
    }

    if (! NT_SUCCESS(_PingTimer->Status()))
    {
        KDbgCheckpointWData(0, "Ping timer failed.", _PingTimer->Status(), ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(_PingTimer->Status());
        return;
    }

    ULONG pingPeriod = static_cast<ULONG>(GetTimingConstant(TimingConstant::PingQuietChannelPeriodMs));
    ULONG toWait;
    if (_LatestReceive == _TimerReceive)
    {
        //
        //  Only send a new PING if we are not waiting for a PONG for the current PING, i.e.:
        //      - There is no pong timeout timer enabled
        //      - The pong timer is inactive
        //      - The pong timer is active and a pong has been received since it started
        //
        if (!_PongTimeoutTimer || (_PongTimeoutTimer && (!_PongTimeoutTimerIsActive || (_PongTimeoutTimerIsActive && (_LatestReceivedPong > _TimerReceivedPong)))))
        {
            //
            //  No receive has occurred since the timer started, so a new Ping should be sent
            //
            _LatestPing = KDateTime::Now();
            if (_PongTimeoutTimer && !_PongTimeoutTimerIsActive)
            {
                //
                //  Ensure that if _LatestReceivedPong is set in ReceiveOperationProcessor::FSM() within this same timer tick, it gets set to a value different than _TimerReceivedPong
                //
                _LatestReceivedPong = KDateTime(((LONGLONG) _LatestReceivedPong) - 1);
                _TimerReceivedPong = _LatestReceivedPong;

                KAssert(!_PongTimeoutTimerIsActive);
                _PongTimeoutTimerIsActive = TRUE;
                _PongTimeoutTimer->Reuse();
                _PongTimeoutTimer->StartTimer(
                    static_cast<ULONG>(GetTimingConstant(TimingConstant::PongTimeoutMs)),
                    this,
                    KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::PongTimeoutTimerCallback)
                    );
            }

            KInvariant(!_IsCleanedUp);
            HRESULT hr;
            hr = WebSocketSend(
                _WebSocketHandle,
                WEB_SOCKET_BUFFER_TYPE::WEB_SOCKET_PING_PONG_BUFFER_TYPE,
                NULL,
                NULL
                );

            if (FAILED(hr))
            {
                KDbgCheckpointWData(0, "Call to WebSocketSend failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(this), hr, 0, 0);
                FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
                return;
            }

            RunSendActionQueue();
        }
        

        //
        //  Wait the full ping period
        //
        toWait = pingPeriod;
    }
    else
    {
        //
        //  A receive has occurred, so wait the remainder of the PingQuietChannelPeriodMs
        //
        KDuration diff = KDateTime::Now() - _LatestReceive;
        toWait = diff.Milliseconds() >= pingPeriod ? pingPeriod : pingPeriod - static_cast<ULONG>(diff.Milliseconds());
    }

    //
    //  Ensure that if _LatestReceive is set in TransportReceiveCallback within this same timer tick, it gets set to a value different than _TimerReceive
    //
    _LatestReceive = KDateTime(((LONGLONG) _LatestReceive) - 1);
    _TimerReceive = _LatestReceive;

    _PingTimer->Reuse();
    _PingTimer->StartTimer(
        toWait,
        this,
        KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::PingTimerCallback)
        );
}

VOID
KHttpServerWebSocket::UnsolicitedPongTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KInvariant(IsInApartment());
    KInvariant(Parent == this);
    KInvariant(&CompletingOperation == _UnsolicitedPongTimer.RawPtr());
    KInvariant(IsOpenCompleted());

    if (_UnsolicitedPongTimer->Status() == STATUS_CANCELLED || IsStateTerminal() || _CloseSent)
    {
        KDbgCheckpointWData(0, "Disabling Unsolicited Pong timer.", _UnsolicitedPongTimer->Status(), ULONG_PTR(this), IsStateTerminal(), _CloseSent, 0);
        return;
    }

    if (! NT_SUCCESS(_UnsolicitedPongTimer->Status()))
    {
        KDbgCheckpointWData(0, "Unsolicited Pong timer failed.", _UnsolicitedPongTimer->Status(), ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(_UnsolicitedPongTimer->Status());
        return;
    }

    KInvariant(!_IsCleanedUp);
    HRESULT hr;
    hr = WebSocketSend(
        _WebSocketHandle,
        WEB_SOCKET_BUFFER_TYPE::WEB_SOCKET_UNSOLICITED_PONG_BUFFER_TYPE,
        NULL,
        NULL
        );

    if (FAILED(hr))
    {
        KDbgCheckpointWData(0, "Call to WebSocketSend failed.", K_STATUS_WEBSOCKET_API_CALL_FAILURE, ULONG_PTR(this), hr, 0, 0);
        FailWebSocket(K_STATUS_WEBSOCKET_API_CALL_FAILURE, hr);
        return;
    }

    _UnsolicitedPongTimer->Reuse();
    _UnsolicitedPongTimer->StartTimer(
        static_cast<ULONG>(GetTimingConstant(TimingConstant::PongKeepalivePeriodMs)),
        this,
        KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::UnsolicitedPongTimerCallback)
        );

    RunSendActionQueue();
}

VOID
KHttpServerWebSocket::PongTimeoutTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _PongTimeoutTimer.RawPtr());
    KInvariant(IsOpenCompleted());
    KInvariant(_PongTimeoutTimerIsActive);

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (_PongTimeoutTimer->Status() == STATUS_CANCELLED || IsStateTerminal() || _CloseSent)
    {
        KDbgCheckpointWData(0, "Disabling Pong Timeout timer.", _PongTimeoutTimer->Status(), ULONG_PTR(this), IsStateTerminal(), _CloseSent, 0);
        _PongTimeoutTimerIsActive = FALSE;
        return;
    }

    if (! NT_SUCCESS(_PongTimeoutTimer->Status()))
    {
        KDbgCheckpointWData(0, "Pong Timeout timer failed.", _PongTimeoutTimer->Status(), ULONG_PTR(this), 0, 0, 0);
        FailWebSocket(_PongTimeoutTimer->Status());
        return;
    }
    
    if (_IsReceiveChannelClosed)
    {
        //
        //  The WebSocket will not be receiving any more data (a Ping or Close cannot be received) as the receive channel
        //  is closed, so disable the timer
        //
        KAssert(GetConnectionStatus() == ConnectionStatus::CloseReceived
            || GetConnectionStatus() == ConnectionStatus::CloseCompleting
            );

        _PongTimeoutTimerIsActive = FALSE;
        return;
    }

    if (_LatestReceivedPong == _TimerReceivedPong)
    {
        KDbgCheckpointWData(
            0,
            "Failed to receive a PONG or CLOSE within the pong timeout (no pong or close received since timer started).",
            K_STATUS_PROTOCOL_TIMEOUT,
            ULONG_PTR(this),
            _LatestReceivedPong,
            _TimerReceivedPong,
            static_cast<ULONG>(GetTimingConstant(TimingConstant::PongTimeoutMs))
            );
        //
        //  A CLOSE frame has not been received based on the connection status, and a PONG frame has not been received
        //  since the timer was started, so fail the WebSocket.
        //
        FailWebSocket(K_STATUS_PROTOCOL_TIMEOUT, _LatestReceivedPong, _TimerReceivedPong);
        return;
    }

    if (_LatestPing > _LatestReceivedPong)
    {
        //
        //  Another Ping has been sent since the last pong received, so fail the WebSocket or re-set the timer as appropriate
        //
        ULONG pongTimeout = static_cast<ULONG>(GetTimingConstant(TimingConstant::PongTimeoutMs));
        KDateTime now(KDateTime::Now());
        KDuration diff = now - _LatestPing;
        KAssert(diff.Milliseconds() <= ULONG_MAX);
        if (diff.Milliseconds() >= pongTimeout)
        {
            KDbgCheckpointWData(
                0,
                "Failed to receive a PONG or CLOSE within the pong timeout.",
                K_STATUS_PROTOCOL_TIMEOUT,
                ULONG_PTR(this),
                _LatestPing,
                now,
                pongTimeout
                );
            FailWebSocket(K_STATUS_PROTOCOL_TIMEOUT, diff.Milliseconds(), pongTimeout);
            return;
        }

        //
        //  Ensure that if _LatestReceivedPong is set in ReceiveOperationProcessor::FSM() within this same timer tick, it gets set to a value different than _TimerReceivedPong
        //
        _LatestReceivedPong = KDateTime(((LONGLONG) _LatestReceivedPong) - 1);
        _TimerReceivedPong = _LatestReceivedPong;

        _PongTimeoutTimer->Reuse();
        _PongTimeoutTimer->StartTimer(
            pongTimeout - static_cast<ULONG>(diff.Milliseconds()),
            this,
            KAsyncContextBase::CompletionCallback(this, &KHttpServerWebSocket::PongTimeoutTimerCallback)
            );

        return;
    }
    
    //
    //  There has not been another ping since the last received pong, so let the next ping start this timer
    //
    _PongTimeoutTimer->Reuse();
    _PongTimeoutTimerIsActive = FALSE;
    return;
}
#pragma endregion KHttpServerWebSocket_TimerCallbacks

#endif  //  NTDDI_VERSION >= NTDDI_WIN8

#endif  //  KTL_USER_MODE

// kernel proxy code would go here, when/if we need it
