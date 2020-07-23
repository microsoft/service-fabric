/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    khttp.cpp

    Description:
      Kernel Tempate Library (KTL): HTTP Package

      Describes KTL-based HTTP Client and HTTP Server

    History:
      raymcc          3-Mar-2012         Initial version.

    Known issues:
      [1] HTTP.SYS filtering the fragment portion of URI
      [2] Test the Transfer-Encoding header on both ends
      [3] Excessively long header value check (cookies from BING, etc.)
--*/

#include <ktl.h>
#include <ktrace.h>


#if KTL_USER_MODE

//
//  _Raw_IOCP_Completion
//
//  Called by the system as HTTP reads complete.
//  This jumps to the appropriate request object and process the IOCP op completion there.
//
VOID
KHttp_IOCP_Handler::_Raw_IOCP_Completion(
    __in_opt VOID* Context,
    __inout OVERLAPPED* Overlapped,
    __in ULONG Error,
    __in ULONG BytesTransferred
    )
{
    UNREFERENCED_PARAMETER(Context);

    //
    // The Overlapped object is actually the derivative KHttp_IOCP_Handler::Overlapped.
    // So, we cast to that and jump back into a member function for this instance.
    //
    KHttp_IOCP_Handler::Overlapped* overlapped = static_cast<KHttp_IOCP_Handler::Overlapped*>(Overlapped);
    if (overlapped->_Handler)
    {
        overlapped->_Handler->IOCP_Completion(Error, BytesTransferred);
    }
}

FAILABLE
KHttpHeaderList::KHttpHeaderList(
    __in KAllocator& Allocator,
    __in ULONG ReserveSize
    )
    :
        _Headers(Allocator, ReserveSize)
{
    NTSTATUS status;

    status = _Headers.Status();

    if(! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }
}

KHttpHeaderList::~KHttpHeaderList()
{
}

NTSTATUS
KHttpHeaderList::Create(
    __out KHttpHeaderList::SPtr& List,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in ULONG ReserveSize
    )
{
    NTSTATUS status;
    KHttpHeaderList::SPtr headerList;

    List = nullptr;

    headerList = _new(AllocationTag, Allocator) KHttpHeaderList(Allocator, ReserveSize);
    if (! headerList)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, nullptr, 0, 0);
        return status;
    }
    if (! NT_SUCCESS(headerList->Status()))
    {
        return headerList->Status();
    }

    List = Ktl::Move(headerList);
    return STATUS_SUCCESS;
}

KDefineDefaultCreate(KHttpUtils::CallbackAsync, Async);

KHttpUtils::CallbackAsync::CallbackAsync()
{
}

KHttpUtils::CallbackAsync::~CallbackAsync()
{
}

//
//  KHttpUtils::StandardCodeToWinHttpCode
//
BOOLEAN
KHttpUtils::StandardCodeToWinHttpCode(
    __in  HttpHeaderType HeaderCode,
    __out ULONG& WinHttpCode
    )
{
    switch (HeaderCode)
    {
        case HttpHeaderContentEncoding:
            WinHttpCode = WINHTTP_QUERY_CONTENT_ENCODING;
            return TRUE;

        case HttpHeaderContentLength:
            WinHttpCode = WINHTTP_QUERY_CONTENT_LENGTH;
            return TRUE;

        case HttpHeaderContentType:
            WinHttpCode = WINHTTP_QUERY_CONTENT_TYPE;
            return TRUE;

        case HttpHeaderWarning:
            WinHttpCode = WINHTTP_QUERY_WARNING;
            return TRUE;

        case HttpHeaderDate:
            WinHttpCode = WINHTTP_QUERY_DATE;
            return TRUE;

        case HttpHeaderFrom:
            WinHttpCode = WINHTTP_QUERY_FROM;
            return TRUE;

        case HttpHeaderHost:
            WinHttpCode = WINHTTP_QUERY_HOST;
            return TRUE;

        case HttpHeaderUserAgent:
            WinHttpCode = WINHTTP_QUERY_USER_AGENT;
            return TRUE;



/*
        These are problematic in terms of efficienty.
        WinHTTP uses non-overlapping
        codes betwen request/response header values,
        but HTTP.SYS does use overlapping codes between
        the request and response. This means that the mapping
        is not symmetric.

        Since the non-coded version can retrieve the headers
        anyway, this is a TBD and probaby doesn't impact
        KTL use for the most part.

        A possible solution is to switch to the WinHTTP code
        system as the master system.

        case HttpHeaderAccept:
            WinHttpCode = WINHTTP_QUERY_ACCEPT;
            return TRUE;

        case HttpHeaderAcceptCharset:
            WinHttpCode = WINHTTP_QUERY_ACCEPT_CHARSET;
            return TRUE;

        case HttpHeaderAcceptEncoding:
            WinHttpCode = WINHTTP_QUERY_ACCEPT_ENCODING;
            return TRUE;

        case HttpHeaderAcceptLanguage:
            WinHttpCode = WINHTTP_QUERY_ACCEPT_LANGUAGE;
            return TRUE;

        case HttpHeaderAcceptRanges:
            WinHttpCode = WINHTTP_QUERY_ACCEPT_RANGES;
            return TRUE;

        case HttpHeaderAge:
            WinHttpCode = WINHTTP_QUERY_AGE;
            return TRUE;

        case HttpHeaderAllow:
            WinHttpCode = WINHTTP_QUERY_ALLOW;
            return TRUE;

        case HttpHeaderAuthorization:
            WinHttpCode = WINHTTP_QUERY_AUTHORIZATION;
            break;

        case HttpHeaderCacheControl:
            WinHttpCode = WINHTTP_QUERY_CACHE_CONTROL;
            break;

        case HttpHeaderConnection:
            WinHttpCode = WINHTTP_QUERY_CONNECTION;
            break;

        case HttpHeaderContentLanguage:
            WinHttpCode = WINHTTP_QUERY_CONTENT_LANGUAGE;
            return TRUE;

        case HttpHeaderContentLocation:
            WinHttpCode = WINHTTP_QUERY_CONTENT_LOCATION;
            return TRUE;

        case HttpHeaderContentMd5:
            WinHttpCode = WINHTTP_QUERY_CONTENT_MD5;
            return TRUE;

        case HttpHeaderContentRange:
            WinHttpCode = WINHTTP_QUERY_CONTENT_RANGE;
            return TRUE;

        case HttpHeaderCookie:
            WinHttpCode = WINHTTP_QUERY_COOKIE;
            return TRUE;

        case HttpHeaderEtag:
            WinHttpCode = WINHTTP_QUERY_ETAG;
            return TRUE;

        case HttpHeaderExpect:
            WinHttpCode = WINHTTP_QUERY_EXPECT;
            return TRUE;

        case HttpHeaderExpires:
            WinHttpCode = WINHTTP_QUERY_EXPIRES;
            return TRUE;

        case HttpHeaderIfMatch:
            WinHttpCode = WINHTTP_QUERY_IF_MATCH;
            return TRUE;

        case HttpHeaderIfModifiedSince:
            WinHttpCode = WINHTTP_QUERY_IF_MODIFIED_SINCE;
            return TRUE;

        case HttpHeaderIfNoneMatch:
            WinHttpCode = WINHTTP_QUERY_IF_NONE_MATCH;
            return TRUE;

        case HttpHeaderRange:
            WinHttpCode = WINHTTP_QUERY_IF_RANGE;
            return TRUE;

        case HttpHeaderIfUnmodifiedSince:
            WinHttpCode = WINHTTP_QUERY_IF_UNMODIFIED_SINCE;
            return TRUE;

        case HttpHeaderLastModified:
            WinHttpCode = WINHTTP_QUERY_LAST_MODIFIED;
            return TRUE;

        case HttpHeaderLocation:
            WinHttpCode = WINHTTP_QUERY_LOCATION;
            return TRUE;

        case HttpHeaderMaxForwards:
            WinHttpCode = WINHTTP_QUERY_MAX_FORWARDS;
            return TRUE;

        case HttpHeaderPragma:
            WinHttpCode = WINHTTP_QUERY_PRAGMA;
            return TRUE;

        case HttpHeaderProxyAuthenticate:
            WinHttpCode = WINHTTP_QUERY_PROXY_AUTHENTICATE;
            return TRUE;

        case HttpHeaderAuthorization:
            WinHttpCode = WINHTTP_QUERY_PROXY_AUTHORIZATION;
            return TRUE;

        case HttpHeaderRange:
            WinHttpCode = WINHTTP_QUERY_RANGE;
            return TRUE;

        case HttpHeaderReferer:
            WinHttpCode = WINHTTP_QUERY_REFERER;
            return TRUE;

        case HttpHeaderRetryAfter:
            WinHttpCode = WINHTTP_QUERY_RETRY_AFTER;
            return TRUE;

        case HttpHeaderSetCookie:
            WinHttpCode = WINHTTP_QUERY_SET_COOKIE;
            return TRUE;

        case HttpHeaderTransferEncoding:
            WinHttpCode = WINHTTP_QUERY_TRANSFER_ENCODING;
            return TRUE;

        case HttpHeaderUpgrade:
            WinHttpCode = WINHTTP_QUERY_UPGRADE;
            return TRUE;

        case HttpHeaderVary:
            WinHttpCode = WINHTTP_QUERY_VARY;
            return TRUE;

        case HttpHeaderVia:
            WinHttpCode = WINHTTP_QUERY_VIA;
            return TRUE;

        case HttpHeaderWwwAuthenticate:
            WinHttpCode = WINHTTP_QUERY_WWW_AUTHENTICATE;
            return TRUE;
*/

    }

    return FALSE;
}

BOOLEAN
KHttpUtils::WinHttpCodeToStandardCode(
    __in   ULONG Code,
    __out  HttpHeaderType& HeaderCode
    )
{
    switch (Code)
    {
        case WINHTTP_QUERY_ACCEPT:
            HeaderCode = HttpHeaderAccept;
            return TRUE;

        case WINHTTP_QUERY_ACCEPT_CHARSET:
            HeaderCode = HttpHeaderAcceptCharset;
            return TRUE;

        case WINHTTP_QUERY_ACCEPT_ENCODING:
            HeaderCode = HttpHeaderAcceptEncoding;
            return TRUE;

        case WINHTTP_QUERY_ACCEPT_LANGUAGE:
            HeaderCode = HttpHeaderAcceptLanguage;
            return TRUE;

        case WINHTTP_QUERY_ACCEPT_RANGES:
            HeaderCode = HttpHeaderAcceptRanges;
            return TRUE;

        case WINHTTP_QUERY_AGE:
            HeaderCode = HttpHeaderAge;
            return TRUE;

        case WINHTTP_QUERY_ALLOW:
            HeaderCode = HttpHeaderAllow;
            return TRUE;

        case WINHTTP_QUERY_AUTHORIZATION:
            HeaderCode = HttpHeaderAuthorization;
            break;

        case WINHTTP_QUERY_CACHE_CONTROL:
            HeaderCode = HttpHeaderCacheControl;
            break;

        case WINHTTP_QUERY_CONNECTION:
            HeaderCode = HttpHeaderConnection;
            break;

        case WINHTTP_QUERY_CONTENT_ENCODING:
            HeaderCode = HttpHeaderContentEncoding;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_LANGUAGE:
            HeaderCode = HttpHeaderContentLanguage;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_LENGTH:
            HeaderCode = HttpHeaderContentLength;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_LOCATION:
            HeaderCode = HttpHeaderContentLocation;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_MD5:
            HeaderCode = HttpHeaderContentMd5;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_RANGE:
            HeaderCode = HttpHeaderContentRange;
            return TRUE;

        case WINHTTP_QUERY_CONTENT_TYPE:
            HeaderCode = HttpHeaderContentType;
            return TRUE;

        case WINHTTP_QUERY_COOKIE:
            HeaderCode = HttpHeaderCookie;
            return TRUE;

        case WINHTTP_QUERY_DATE:
            HeaderCode = HttpHeaderDate;
            return TRUE;

        case WINHTTP_QUERY_ETAG:
            HeaderCode = HttpHeaderEtag;
            return TRUE;


        case WINHTTP_QUERY_EXPECT:
            HeaderCode = HttpHeaderExpect;
            return TRUE;

        case WINHTTP_QUERY_EXPIRES:
            HeaderCode = HttpHeaderExpires;
            return TRUE;


        case WINHTTP_QUERY_FROM:
            HeaderCode = HttpHeaderFrom;
            return TRUE;

        case WINHTTP_QUERY_HOST:
            HeaderCode = HttpHeaderHost;
            return TRUE;

        case WINHTTP_QUERY_IF_MATCH:
            HeaderCode = HttpHeaderIfMatch;
            return TRUE;

        case WINHTTP_QUERY_IF_MODIFIED_SINCE:
            HeaderCode = HttpHeaderIfModifiedSince;
            return TRUE;

        case WINHTTP_QUERY_IF_NONE_MATCH:
            HeaderCode = HttpHeaderIfNoneMatch;
            return TRUE;

        case WINHTTP_QUERY_IF_RANGE:
            HeaderCode = HttpHeaderRange;
            return TRUE;

        case WINHTTP_QUERY_IF_UNMODIFIED_SINCE:
            HeaderCode = HttpHeaderIfUnmodifiedSince;
            return TRUE;

        case WINHTTP_QUERY_LAST_MODIFIED:
            HeaderCode = HttpHeaderLastModified;
            return TRUE;

        case WINHTTP_QUERY_LOCATION:
            HeaderCode = HttpHeaderLocation;
            return TRUE;

        case WINHTTP_QUERY_MAX_FORWARDS:
            HeaderCode = HttpHeaderMaxForwards;
            return TRUE;

        case WINHTTP_QUERY_PRAGMA:
            HeaderCode = HttpHeaderPragma;
            return TRUE;

        case WINHTTP_QUERY_PROXY_AUTHENTICATE:
            HeaderCode = HttpHeaderProxyAuthenticate;
            return TRUE;

        case WINHTTP_QUERY_PROXY_AUTHORIZATION:
            HeaderCode = HttpHeaderAuthorization;
            return TRUE;

        case WINHTTP_QUERY_RANGE:
            HeaderCode = HttpHeaderRange;
            return TRUE;

        case WINHTTP_QUERY_REFERER:
            HeaderCode = HttpHeaderReferer;
            return TRUE;

        case WINHTTP_QUERY_RETRY_AFTER:
            HeaderCode = HttpHeaderRetryAfter;
            return TRUE;

        case WINHTTP_QUERY_SET_COOKIE:
            HeaderCode = HttpHeaderSetCookie;
            return TRUE;

        case WINHTTP_QUERY_TRANSFER_ENCODING:
            HeaderCode = HttpHeaderTransferEncoding;
            return TRUE;

        case WINHTTP_QUERY_UPGRADE:
            HeaderCode = HttpHeaderUpgrade;
            return TRUE;

        case WINHTTP_QUERY_USER_AGENT:
            HeaderCode = HttpHeaderUserAgent;
            return TRUE;

        case WINHTTP_QUERY_VARY:
            HeaderCode = HttpHeaderVary;
            return TRUE;

        case WINHTTP_QUERY_VIA:
            HeaderCode = HttpHeaderVia;
            return TRUE;

        case WINHTTP_QUERY_WARNING:
            HeaderCode = HttpHeaderWarning;
            return TRUE;

        case WINHTTP_QUERY_WWW_AUTHENTICATE:
            HeaderCode = HttpHeaderWwwAuthenticate;
            return TRUE;

    }

    return FALSE;
}

BOOLEAN
KHttpUtils::StdHeaderToStringA(
    __in HttpHeaderType Header,
    __out KStringViewA& HeaderStringA
    )
{
    LPCSTR Result = "<null>";

    using namespace HttpHeaderName;

    switch (Header)
    {
        case HttpHeaderCacheControl:        Result = CacheControlA; break;
        case HttpHeaderConnection:          Result = ConnectionA; break;
        case HttpHeaderDate:                Result = DateA; break;
        case HttpHeaderKeepAlive:           Result = KeepAliveA; break;
        case HttpHeaderPragma:              Result = PragmaA; break;
        case HttpHeaderTrailer:             Result = TrailerA; break;
        case HttpHeaderTransferEncoding:    Result = TransferEncodingA; break;
        case HttpHeaderUpgrade:             Result = UpgradeA; break;
        case HttpHeaderVia:                 Result = ViaA; break;
        case HttpHeaderWarning:             Result = WarningA; break;
        case HttpHeaderAllow:               Result = AllowA; break;
        case HttpHeaderContentLength:       Result = ContentLengthA; break;
        case HttpHeaderContentType:         Result = ContentTypeA; break;
        case HttpHeaderContentEncoding:     Result = ContentEncodingA; break;
        case HttpHeaderContentLanguage:     Result = ContentLanguageA; break;
        case HttpHeaderContentLocation:     Result = ContentLocationA; break;
        case HttpHeaderContentMd5:          Result = ContentMd5A; break;
        case HttpHeaderContentRange:        Result = ContentRangeA; break;
        case HttpHeaderExpires:             Result = ExpiresA; break;
        case HttpHeaderLastModified:        Result = LastModifiedA; break;
        case HttpHeaderAccept:              Result = AcceptA; break;
        case HttpHeaderAcceptCharset:       Result = AcceptCharsetA; break;
        case HttpHeaderAcceptEncoding:      Result = AcceptEncodingA; break;
        case HttpHeaderAcceptLanguage:      Result = AcceptLanguageA; break;
        case HttpHeaderAuthorization:       Result = AuthorizationA; break;
        case HttpHeaderCookie:              Result = CookieA; break;
        case HttpHeaderExpect:              Result = ExpectA; break;
        case HttpHeaderFrom:                Result = FromA; break;
        case HttpHeaderHost:                Result = HostA; break;
        case HttpHeaderIfMatch:             Result = IfMatchA; break;
        case HttpHeaderIfModifiedSince:     Result = IfModifiedSinceA; break;
        case HttpHeaderIfNoneMatch:         Result = IfNoneMatchA; break;
        case HttpHeaderIfRange:             Result = IfRangeA; break;
        case HttpHeaderIfUnmodifiedSince:   Result = IfUnmodifiedSinceA; break;
        case HttpHeaderMaxForwards:         Result = MaxForwardsA; break;
        case HttpHeaderProxyAuthorization:  Result = ProxyAuthorizationA; break;
        case HttpHeaderReferer:             Result = RefererA; break;
        case HttpHeaderRange:               Result = RangeA; break;
        case HttpHeaderTe:                  Result = TEA; break;
        case HttpHeaderTranslate:           Result = TranslateA; break;
        case HttpHeaderUserAgent:           Result = UserAgentA; break;
        case HttpHeaderRequestMaximum:      Result = RequestMaximumA; break;

        default:
            return FALSE;
    }

    HeaderStringA = Result;
    return TRUE;
}

//
//  KHttpUtils::StdHeaderToString
//
BOOLEAN
KHttpUtils::StdHeaderToString(
    __in HttpHeaderType Header,
    __out KStringView& HeaderString
    )
{
    LPCWSTR Result = L"<null>";

    using namespace HttpHeaderName;

    switch (Header)
    {
        case HttpHeaderCacheControl:        Result = CacheControl; break;
        case HttpHeaderConnection:          Result = Connection; break;
        case HttpHeaderDate:                Result = Date; break;
        case HttpHeaderKeepAlive:           Result = KeepAlive; break;
        case HttpHeaderPragma:              Result = Pragma; break;
        case HttpHeaderTrailer:             Result = Trailer; break;
        case HttpHeaderTransferEncoding:    Result = TransferEncoding; break;
        case HttpHeaderUpgrade:             Result = HttpHeaderName::Upgrade; break;
        case HttpHeaderVia:                 Result = Via; break;
        case HttpHeaderWarning:             Result = Warning; break;
        case HttpHeaderAllow:               Result = Allow; break;
        case HttpHeaderContentLength:       Result = ContentLength; break;
        case HttpHeaderContentType:         Result = ContentType; break;
        case HttpHeaderContentEncoding:     Result = ContentEncoding; break;
        case HttpHeaderContentLanguage:     Result = ContentLanguage; break;
        case HttpHeaderContentLocation:     Result = ContentLocation; break;
        case HttpHeaderContentMd5:          Result = ContentMd5; break;
        case HttpHeaderContentRange:        Result = ContentRange; break;
        case HttpHeaderExpires:             Result = Expires; break;
        case HttpHeaderLastModified:        Result = LastModified; break;
        case HttpHeaderAccept:              Result = Accept; break;
        case HttpHeaderAcceptCharset:       Result = AcceptCharset; break;
        case HttpHeaderAcceptEncoding:      Result = AcceptEncoding; break;
        case HttpHeaderAcceptLanguage:      Result = AcceptLanguage; break;
        case HttpHeaderAuthorization:       Result = Authorization; break;
        case HttpHeaderCookie:              Result = Cookie; break;
        case HttpHeaderExpect:              Result = Expect; break;
        case HttpHeaderFrom:                Result = From; break;
        case HttpHeaderHost:                Result = Host; break;
        case HttpHeaderIfMatch:             Result = IfMatch; break;
        case HttpHeaderIfModifiedSince:     Result = IfModifiedSince; break;
        case HttpHeaderIfNoneMatch:         Result = IfNoneMatch; break;
        case HttpHeaderIfRange:             Result = IfRange; break;
        case HttpHeaderIfUnmodifiedSince:   Result = IfUnmodifiedSince; break;
        case HttpHeaderMaxForwards:         Result = MaxForwards; break;
        case HttpHeaderProxyAuthorization:  Result = ProxyAuthorization; break;
        case HttpHeaderReferer:             Result = Referer; break;
        case HttpHeaderRange:               Result = Range; break;
        case HttpHeaderTe:                  Result = TE; break;
        case HttpHeaderTranslate:           Result = Translate; break;
        case HttpHeaderUserAgent:           Result = UserAgent; break;
        case HttpHeaderRequestMaximum:      Result = RequestMaximum; break;

        default:
            return FALSE;
    }

    HeaderString = Result;
    return TRUE;
}


BOOLEAN
KHttpUtils::StringAToStdHeader(
    __in  KStringViewA& HeaderString,
    __out HttpHeaderType& Header
    )
{
    using namespace HttpHeaderName;
    Header = HttpHeader_INVALID;

    if (HeaderString.CompareNoCase(KStringViewA(CacheControlA)) == 0)
    {
        Header = HttpHeaderCacheControl;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ConnectionA)) == 0)
    {
        Header = HttpHeaderConnection;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(DateA)) == 0)
    {
        Header = HttpHeaderDate;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(KeepAliveA)) == 0)
    {
        Header = HttpHeaderKeepAlive;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(PragmaA)) == 0)
    {
        Header = HttpHeaderPragma;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(TrailerA)) == 0)
    {
        Header = HttpHeaderTrailer;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(TransferEncodingA)) == 0)
    {
        Header = HttpHeaderTransferEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(UpgradeA)) == 0)
    {
        Header = HttpHeaderUpgrade;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ViaA)) == 0)
    {
        Header = HttpHeaderVia;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(WarningA)) == 0)
    {
        Header = HttpHeaderWarning;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AllowA)) == 0)
    {
        Header = HttpHeaderAllow;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentLengthA)) == 0)
    {
        Header = HttpHeaderContentLength;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentTypeA)) == 0)
    {
        Header = HttpHeaderContentType;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentEncodingA)) == 0)
    {
        Header = HttpHeaderContentEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentLanguageA)) == 0)
    {
        Header = HttpHeaderContentLanguage;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentLocationA)) == 0)
    {
        Header = HttpHeaderContentLocation;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentMd5A)) == 0)
    {
        Header = HttpHeaderContentMd5;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ContentRangeA)) == 0)
    {
        Header = HttpHeaderContentRange;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ExpiresA)) == 0)
    {
        Header = HttpHeaderExpires;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(LastModifiedA)) == 0)
    {
        Header = HttpHeaderLastModified;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AcceptA)) == 0)
    {
        Header = HttpHeaderAccept;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AcceptCharsetA)) == 0)
    {
        Header = HttpHeaderAcceptCharset;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AcceptEncodingA)) == 0)
    {
        Header = HttpHeaderAcceptEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AcceptLanguageA)) == 0)
    {
        Header = HttpHeaderAcceptLanguage;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AuthorizationA)) == 0)
    {
        Header = HttpHeaderAuthorization;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(CookieA)) == 0)
    {
        Header = HttpHeaderCookie;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ExpectA)) == 0)
    {
        Header = HttpHeaderExpect;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(FromA)) == 0)
    {
        Header = HttpHeaderFrom;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(HostA)) == 0)
    {
        Header = HttpHeaderHost;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(IfMatchA)) == 0)
    {
        Header = HttpHeaderIfMatch;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(IfModifiedSinceA)) == 0)
    {
        Header = HttpHeaderIfModifiedSince;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(IfNoneMatchA)) == 0)
    {
        Header = HttpHeaderIfNoneMatch;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(IfRangeA)) == 0)
    {
        Header = HttpHeaderIfRange;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(IfUnmodifiedSinceA)) == 0)
    {
        Header = HttpHeaderIfUnmodifiedSince;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(MaxForwardsA)) == 0)
    {
        Header = HttpHeaderMaxForwards;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ProxyAuthorizationA)) == 0)
    {
        Header = HttpHeaderProxyAuthorization;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(RefererA)) == 0)
    {
        Header = HttpHeaderReferer;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(RangeA)) == 0)
    {
        Header = HttpHeaderRange;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(TEA)) == 0)
    {
        Header = HttpHeaderTe;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(TranslateA)) == 0)
    {
        Header = HttpHeaderTranslate;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(UserAgentA)) == 0)
    {
        Header = HttpHeaderUserAgent;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(RequestMaximumA)) == 0)
    {
        Header = HttpHeaderRequestMaximum;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AcceptRangesA)) == 0)
    {
        Header = HttpHeaderAcceptRanges;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(AgeA)) == 0)
    {
        Header = HttpHeaderAge;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ETagA)) == 0)
    {
        Header = HttpHeaderEtag;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(LocationA)) == 0)
    {
        Header = HttpHeaderLocation;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ProxyAuthenticateA)) == 0)
    {
        Header = HttpHeaderProxyAuthenticate;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(RetryAfterA)) == 0)
    {
        Header = HttpHeaderRetryAfter;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ServerA)) == 0)
    {
        Header = HttpHeaderServer;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(SetCookieA)) == 0)
    {
        Header = HttpHeaderSetCookie;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(VaryA)) == 0)
    {
        Header = HttpHeaderVary;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(WwwAuthenticateA)) == 0)
    {
        Header = HttpHeaderWwwAuthenticate;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(ResponseMaximumA)) == 0)
    {
        Header = HttpHeaderResponseMaximum;
    }

    else if (HeaderString.CompareNoCase(KStringViewA(MaximumA)) == 0)
    {
        Header = HttpHeaderMaximum;
    }

    else
    {
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
KHttpUtils::StringToStdHeader(
    __in  KStringView& HeaderString,
    __out HttpHeaderType& Header
    )
{
    using namespace HttpHeaderName;
    Header = HttpHeader_INVALID;

    if (HeaderString.CompareNoCase(KStringView(CacheControl)) == 0)
    {
        Header = HttpHeaderCacheControl;
    }

    else if (HeaderString.CompareNoCase(KStringView(Connection)) == 0)
    {
        Header = HttpHeaderConnection;
    }

    else if (HeaderString.CompareNoCase(KStringView(Date)) == 0)
    {
        Header = HttpHeaderDate;
    }

    else if (HeaderString.CompareNoCase(KStringView(KeepAlive)) == 0)
    {
        Header = HttpHeaderKeepAlive;
    }

    else if (HeaderString.CompareNoCase(KStringView(Pragma)) == 0)
    {
        Header = HttpHeaderPragma;
    }

    else if (HeaderString.CompareNoCase(KStringView(Trailer)) == 0)
    {
        Header = HttpHeaderTrailer;
    }

    else if (HeaderString.CompareNoCase(KStringView(TransferEncoding)) == 0)
    {
        Header = HttpHeaderTransferEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringView(HttpHeaderName::Upgrade)) == 0)
    {
        Header = HttpHeaderUpgrade;
    }

    else if (HeaderString.CompareNoCase(KStringView(Via)) == 0)
    {
        Header = HttpHeaderVia;
    }

    else if (HeaderString.CompareNoCase(KStringView(Warning)) == 0)
    {
        Header = HttpHeaderWarning;
    }

    else if (HeaderString.CompareNoCase(KStringView(Allow)) == 0)
    {
        Header = HttpHeaderAllow;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentLength)) == 0)
    {
        Header = HttpHeaderContentLength;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentType)) == 0)
    {
        Header = HttpHeaderContentType;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentEncoding)) == 0)
    {
        Header = HttpHeaderContentEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentLanguage)) == 0)
    {
        Header = HttpHeaderContentLanguage;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentLocation)) == 0)
    {
        Header = HttpHeaderContentLocation;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentMd5)) == 0)
    {
        Header = HttpHeaderContentMd5;
    }

    else if (HeaderString.CompareNoCase(KStringView(ContentRange)) == 0)
    {
        Header = HttpHeaderContentRange;
    }

    else if (HeaderString.CompareNoCase(KStringView(Expires)) == 0)
    {
        Header = HttpHeaderExpires;
    }

    else if (HeaderString.CompareNoCase(KStringView(LastModified)) == 0)
    {
        Header = HttpHeaderLastModified;
    }

    else if (HeaderString.CompareNoCase(KStringView(Accept)) == 0)
    {
        Header = HttpHeaderAccept;
    }

    else if (HeaderString.CompareNoCase(KStringView(AcceptCharset)) == 0)
    {
        Header = HttpHeaderAcceptCharset;
    }

    else if (HeaderString.CompareNoCase(KStringView(AcceptEncoding)) == 0)
    {
        Header = HttpHeaderAcceptEncoding;
    }

    else if (HeaderString.CompareNoCase(KStringView(AcceptLanguage)) == 0)
    {
        Header = HttpHeaderAcceptLanguage;
    }

    else if (HeaderString.CompareNoCase(KStringView(Authorization)) == 0)
    {
        Header = HttpHeaderAuthorization;
    }

    else if (HeaderString.CompareNoCase(KStringView(Cookie)) == 0)
    {
        Header = HttpHeaderCookie;
    }

    else if (HeaderString.CompareNoCase(KStringView(Expect)) == 0)
    {
        Header = HttpHeaderExpect;
    }

    else if (HeaderString.CompareNoCase(KStringView(From)) == 0)
    {
        Header = HttpHeaderFrom;
    }

    else if (HeaderString.CompareNoCase(KStringView(Host)) == 0)
    {
        Header = HttpHeaderHost;
    }

    else if (HeaderString.CompareNoCase(KStringView(IfMatch)) == 0)
    {
        Header = HttpHeaderIfMatch;
    }

    else if (HeaderString.CompareNoCase(KStringView(IfModifiedSince)) == 0)
    {
        Header = HttpHeaderIfModifiedSince;
    }

    else if (HeaderString.CompareNoCase(KStringView(IfNoneMatch)) == 0)
    {
        Header = HttpHeaderIfNoneMatch;
    }

    else if (HeaderString.CompareNoCase(KStringView(IfRange)) == 0)
    {
        Header = HttpHeaderIfRange;
    }

    else if (HeaderString.CompareNoCase(KStringView(IfUnmodifiedSince)) == 0)
    {
        Header = HttpHeaderIfUnmodifiedSince;
    }

    else if (HeaderString.CompareNoCase(KStringView(MaxForwards)) == 0)
    {
        Header = HttpHeaderMaxForwards;
    }

    else if (HeaderString.CompareNoCase(KStringView(ProxyAuthorization)) == 0)
    {
        Header = HttpHeaderProxyAuthorization;
    }

    else if (HeaderString.CompareNoCase(KStringView(Referer)) == 0)
    {
        Header = HttpHeaderReferer;
    }

    else if (HeaderString.CompareNoCase(KStringView(Range)) == 0)
    {
        Header = HttpHeaderRange;
    }

    else if (HeaderString.CompareNoCase(KStringView(TE)) == 0)
    {
        Header = HttpHeaderTe;
    }

    else if (HeaderString.CompareNoCase(KStringView(Translate)) == 0)
    {
        Header = HttpHeaderTranslate;
    }

    else if (HeaderString.CompareNoCase(KStringView(UserAgent)) == 0)
    {
        Header = HttpHeaderUserAgent;
    }

    else if (HeaderString.CompareNoCase(KStringView(RequestMaximum)) == 0)
    {
        Header = HttpHeaderRequestMaximum;
    }

    else if (HeaderString.CompareNoCase(KStringView(AcceptRanges)) == 0)
    {
        Header = HttpHeaderAcceptRanges;
    }

    else if (HeaderString.CompareNoCase(KStringView(Age)) == 0)
    {
        Header = HttpHeaderAge;
    }

    else if (HeaderString.CompareNoCase(KStringView(ETag)) == 0)
    {
        Header = HttpHeaderEtag;
    }

    else if (HeaderString.CompareNoCase(KStringView(Location)) == 0)
    {
        Header = HttpHeaderLocation;
    }

    else if (HeaderString.CompareNoCase(KStringView(ProxyAuthenticate)) == 0)
    {
        Header = HttpHeaderProxyAuthenticate;
    }

    else if (HeaderString.CompareNoCase(KStringView(RetryAfter)) == 0)
    {
        Header = HttpHeaderRetryAfter;
    }

    else if (HeaderString.CompareNoCase(KStringView(Server)) == 0)
    {
        Header = HttpHeaderServer;
    }

    else if (HeaderString.CompareNoCase(KStringView(SetCookie)) == 0)
    {
        Header = HttpHeaderSetCookie;
    }

    else if (HeaderString.CompareNoCase(KStringView(Vary)) == 0)
    {
        Header = HttpHeaderVary;
    }

    else if (HeaderString.CompareNoCase(KStringView(WwwAuthenticate)) == 0)
    {
        Header = HttpHeaderWwwAuthenticate;
    }

    else if (HeaderString.CompareNoCase(KStringView(ResponseMaximum)) == 0)
    {
        Header = HttpHeaderResponseMaximum;
    }

    else if (HeaderString.CompareNoCase(KStringView(Maximum)) == 0)
    {
        Header = HttpHeaderMaximum;
    }

    else
    {
        return FALSE;
    }

    return TRUE;
}


//
// KHttpUtils::OpToString
//
BOOLEAN
KHttpUtils::OpToString(
    __in KHttpUtils::OperationType Op,
    __out KStringView& OpStr
    )
{
    LPCWSTR Str = 0;

    switch (Op)
    {
       case eHttpGet:       Str = KHttpUtils::HttpVerb::GET; break;
       case eHttpPost:      Str = KHttpUtils::HttpVerb::POST; break;
       case eHttpPut:       Str = KHttpUtils::HttpVerb::PUT; break;
       case eHttpDelete:    Str = KHttpUtils::HttpVerb::DEL; break;
       case eHttpOptions:   Str = KHttpUtils::HttpVerb::OPTIONS; break;
       case eHttpHead:      Str = KHttpUtils::HttpVerb::HEAD; break;
       case eHttpTrace:     Str = KHttpUtils::HttpVerb::TRACE; break;
       case eHttpConnect:   Str = KHttpUtils::HttpVerb::CONNECT; break;
       case eHttpTrack:     Str = KHttpUtils::HttpVerb::TRACK; break;
       case eHttpMove:      Str = KHttpUtils::HttpVerb::MOVE; break;
       case eHttpCopy:      Str = KHttpUtils::HttpVerb::COPY; break;
       case eHttpPropFind:  Str = KHttpUtils::HttpVerb::PROPFIND; break;
       case eHttpPropPatch: Str = KHttpUtils::HttpVerb::PROPPATCH; break;
       case eHttpMkcol:     Str = KHttpUtils::HttpVerb::MKCOL; break;
       case eHttpSearch:    Str = KHttpUtils::HttpVerb::SEARCH; break;
       case eHttpLock:      Str = KHttpUtils::HttpVerb::LOCK; break;
       case eHttpUnlock:    Str = KHttpUtils::HttpVerb::UNLOCK; break;
       case eUnassigned:
       default:
           Str = L""; return FALSE;
    };


    OpStr = Str;
    return TRUE;
}


#if NTDDI_VERSION >= NTDDI_WIN8

KHttpWebSocket::KHttpWebSocket(
    __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveRequestQueue,
    __in KAsyncQueue<WebSocketOperation>& SendRequestQueue,
    __in KHttpUtils::CallbackAsync& AbortAsync,
    __in KStringA& ActiveSubprotocol
    )
    :
        KWebSocket(ReceiveRequestQueue, SendRequestQueue),

        _AbortAsync(&AbortAsync),
        _ActiveSubprotocol(&ActiveSubprotocol),

        _IsSendChannelClosed(FALSE),
        _IsReceiveChannelClosed(FALSE),
        _OpenTimeoutTimer(nullptr),
        _CloseTimeoutTimer(nullptr),
        _IsAbortStarted(FALSE),
        _FailureStatus(STATUS_SUCCESS)
{
}

KHttpWebSocket::~KHttpWebSocket()
{
}

KStringViewA
KHttpWebSocket::GetActiveSubProtocol()
{
    return *_ActiveSubprotocol;
}

VOID
KHttpWebSocket::FailWebSocket( 
    __in NTSTATUS Status,
    __in_opt ULONGLONG D1,
    __in_opt ULONGLONG D2
    )
{
    KAssert(! NT_SUCCESS(Status));
    KAssert(IsInApartment());

    if (! IsStateTerminal())
    {
        _FailureStatus = Status;

        KTraceFailedAsyncRequest(_FailureStatus, this, D1, D2);

        KAssert(_ConnectionStatus != ConnectionStatus::Initialized);
        CompleteOpen(_FailureStatus);

        if (IsCloseStarted())
        {
            KDbgCheckpointWData(
                0,
                "Completing the service close due to failure.",
                _FailureStatus,
                ULONG_PTR(this),
                static_cast<SHORT>(_ConnectionStatus),
                D1,
                D2
                );
            CompleteClose(_FailureStatus);
        }

        _ConnectionStatus = ConnectionStatus::Error;

        CleanupWebSocket(_FailureStatus);
    }
    KDbgCheckpointWData(
        0,
        "Ignoring call to FailWebSocket as state is already terminal.",
        Status,
        ULONG_PTR(this),
        static_cast<SHORT>(_ConnectionStatus),
        D1,
        D2
        );
}

VOID
KHttpWebSocket::Abort()
{
    if (TryAcquireActivities())
    {
        KFinally([&]()
        {
            ReleaseActivities();
        });

        //
        //  If the service has completed the open procedure and we win race to set _IsAbortStarted to true,
        //  begin the Abort async which will execute the abort procedure inside the WebSocket's apartment
        //
        if (KInterlockedSetFlag(_IsAbortStarted))
        {
            _AbortAsync->Start(this, KAsyncContextBase::CompletionCallback(this, &KHttpWebSocket::AbortAsyncCallback));
        }
    }
}

VOID
KHttpWebSocket::HandleDroppedReceiveRequest(
    __in KAsyncQueue<WebSocketReceiveOperation>&,
    __in WebSocketReceiveOperation& DroppedItem
    )
{
    KAssert(IsInApartment());

    //
    //  Receive requests may be dropped during failure, or when a close frame is received.  Use STATUS_CANCELLED
    //  to signal the latter, so that the WebSocket user can distinguish between failure and the remote connection
    //  starting the close procedure.
    //
    NTSTATUS status = GetConnectionStatus() == ConnectionStatus::Error ? STATUS_REQUEST_ABORTED : STATUS_CANCELLED;
    DroppedItem.CompleteOperation(status);
}

VOID
KHttpWebSocket::HandleDroppedSendRequest(
    __in KAsyncQueue<WebSocketOperation>&,
    __in WebSocketOperation& DroppedItem
    )
{
    KAssert(IsInApartment());

    //
    //  Receive requests may be dropped during failure, or when a close frame is received.  Use STATUS_CANCELLED
    //  to signal the latter, so that the WebSocket user can distinguish between failure and the remote connection
    //  starting the close procedure.
    //
    NTSTATUS status = GetConnectionStatus() == ConnectionStatus::Error ? STATUS_REQUEST_ABORTED : STATUS_CANCELLED;
    DroppedItem.CompleteOperation(status);
}

VOID
KHttpWebSocket::ReceiveProcessorCompleted()
{
    KAssert(IsInApartment());
    KInvariant((GetConnectionStatus() == ConnectionStatus::CloseReceived && !_IsSendChannelClosed)
        || GetConnectionStatus() == ConnectionStatus::CloseCompleting);
    KInvariant(! _IsReceiveChannelClosed);

    _IsReceiveChannelClosed = TRUE;

    if (_IsSendChannelClosed)
    {
        DoClose();
    }
}

VOID
KHttpWebSocket::SendChannelCompleted()
{
    KAssert(IsInApartment());
    KInvariant((GetConnectionStatus() == ConnectionStatus::CloseInitiated && !_IsReceiveChannelClosed)
        || GetConnectionStatus() == ConnectionStatus::CloseCompleting);
    KInvariant(! _IsSendChannelClosed);

    _IsSendChannelClosed = TRUE;

    if (_IsReceiveChannelClosed)
    {
        DoClose();
    }
}

NTSTATUS
KHttpWebSocket::InitializeHandshakeTimers()
{
    KAssert(IsInApartment());

    NTSTATUS status;

    //
    //  Create the Close Timeout timer if the constant is set
    //
    _CloseTimeoutTimer = nullptr;
    if (GetTimingConstant(TimingConstant::CloseTimeoutMs) != TimingConstantValue::None)
    {
        KTimer::SPtr closeTimeoutTimer;

        status = KTimer::Create(closeTimeoutTimer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
            return status;
        }

        _CloseTimeoutTimer = Ktl::Move(closeTimeoutTimer);
    }

    //
    //  Create the Open Timeout timer if the constant is set
    //
    _OpenTimeoutTimer = nullptr;
    TimingConstantValue openTimeout = GetTimingConstant(TimingConstant::OpenTimeoutMs);
    if (openTimeout != TimingConstantValue::None)
    {
        KTimer::SPtr openTimeoutTimer;

        status = KTimer::Create(openTimeoutTimer, GetThisAllocator(), GetThisAllocationTag());
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
            return status;
        }

        _OpenTimeoutTimer = Ktl::Move(openTimeoutTimer);
    }

    return STATUS_SUCCESS;
}

VOID
KHttpWebSocket::OpenTimeoutTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _OpenTimeoutTimer.RawPtr());
    KAssert(_ConnectionStatus != ConnectionStatus::Initialized);  //  This timer is started once the open procedure has started

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (_OpenTimeoutTimer->Status() == STATUS_CANCELLED || IsStateTerminal())
    {
        return;
    }

    if (! NT_SUCCESS(_OpenTimeoutTimer->Status()))
    {
        KDbgErrorWData(0, "Open timeout timer failed.", _OpenTimeoutTimer->Status(), ULONG_PTR(this), static_cast<SHORT>(_ConnectionStatus), 0, 0);
        FailWebSocket(_OpenTimeoutTimer->Status());
        return;
    }
    
    if (! IsOpenCompleted()) 
    {
        KDbgCheckpointWData(
            0,
            "Service open did not complete within open timeout duration: failing WebSocket.",
            K_STATUS_PROTOCOL_TIMEOUT,
            ULONG_PTR(this),
            static_cast<SHORT>(_ConnectionStatus),
            0,
            0
            );

        FailWebSocket(K_STATUS_PROTOCOL_TIMEOUT, STATUS_TIMEOUT);
        return;
    }

    return;
}

VOID
KHttpWebSocket::CloseTimeoutTimerCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _CloseTimeoutTimer.RawPtr());
    KAssert(IsCloseStarted() || IsStateTerminal()); // This timer is started once the close procedure is initiated

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (_CloseTimeoutTimer->Status() == STATUS_CANCELLED)
    {
        return;
    }

    if (! NT_SUCCESS(_CloseTimeoutTimer->Status()))
    {
        KDbgErrorWData(0, "Close timeout timer failed.", _CloseTimeoutTimer->Status(), ULONG_PTR(this), static_cast<SHORT>(_ConnectionStatus), 0, 0);
        FailWebSocket(_CloseTimeoutTimer->Status());
        return;
    }

    if (_ConnectionStatus != ConnectionStatus::Closed)
    {
        KDbgCheckpointWData(
            0,
            "Service close did not complete within close timeout duration: failing WebSocket.",
            K_STATUS_PROTOCOL_TIMEOUT,
            ULONG_PTR(this),
            static_cast<SHORT>(_ConnectionStatus),
            0,
            0
            );

        FailWebSocket(K_STATUS_PROTOCOL_TIMEOUT, STATUS_TIMEOUT);
        return;
    }

    return;
}

VOID
KHttpWebSocket::AbortAsyncCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _AbortAsync.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (! IsStateTerminal())
    {
        KDbgCheckpointWData(
            0,
            "Aborting websocket via FailWebSocket as state is non-terminal.",
            STATUS_REQUEST_ABORTED,
            ULONG_PTR(this),
            static_cast<SHORT>(_ConnectionStatus),
            0,
            0
            );
        FailWebSocket(STATUS_REQUEST_ABORTED);
        return;
    }
    KDbgCheckpointWData(
        0,
        "Ignoring Abort as state is terminal.",
        STATUS_REQUEST_ABORTED,
        ULONG_PTR(this),
        static_cast<SHORT>(_ConnectionStatus),
        0,
        0
        );
}

VOID
KHttpWebSocket::DoClose()
{
    KAssert(IsInApartment());
    KAssert(GetConnectionStatus() == ConnectionStatus::CloseCompleting);  //  Must only be called once

    KDbgCheckpointWData(0, "Completing service close.", STATUS_SUCCESS, ULONG_PTR(this), static_cast<SHORT>(_ConnectionStatus), 0, 0);

    _ConnectionStatus = ConnectionStatus::Closed;

    KInvariant(CompleteClose(STATUS_SUCCESS));

    CleanupWebSocket(STATUS_SUCCESS);
}
#endif  //  #if NTDDI_VERSION >= NTDDI_WIN8

#endif  //  # if KTL_USER_MODE

// kernel proxy code would go here, when/if we need it
