/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    khttp.h

    Description:
      Kernel Tempate Library (KTL): HTTP Package

      Describes KTL-based HTTP Client and HTTP Server.

      This is a user-mode wrapper for WinHTTP (client) and HTTP.SYS (server).

      User-mode only at this time.

    History:
      raymcc          3-May-2012         Initial version.

--*/

#pragma once

#if KTL_USER_MODE

#include <KAsyncService.h>

#if NTDDI_VERSION >= NTDDI_WIN8
#include <websocket.h>
#endif

//
//  Use InterlockedCompareExchange to set the value of the given flag from FALSE to TRUE.
//
//  Return true IFF the given flag is atomically modified from FALSE to TRUE
//
BOOLEAN
KInterlockedSetFlag(
    __in volatile ULONG& Flag
    );

//
//  Abstract IOCP handler.  KHttpServer binds IOCP callbacks to the raw IOCP completion, which then
//  delegates to the handler instance pointed to by the overlapped structure.
//
class KHttp_IOCP_Handler
{
public:

    static VOID
    _Raw_IOCP_Completion(
        __in_opt VOID* Context,
        __inout OVERLAPPED* Overlapped,
        __in ULONG Error,
        __in ULONG BytesTransferred
        );

    virtual VOID
    IOCP_Completion(
        __in ULONG Error,
        __in ULONG BytesTransferred
        ) = 0;

    struct Overlapped : OVERLAPPED
    {
        KHttp_IOCP_Handler* _Handler;

        Overlapped(
            __in_opt KHttp_IOCP_Handler* const Handler = nullptr
            )
        {
            Internal = 0;
            InternalHigh = 0;
            Pointer = 0;
            hEvent = 0;

            _Handler = Handler;
        }
    };
};


// Standard HTTP status codes and header values
//
// RFC 2616
//
// http://www.w3.org/Protocols/rfc2616/rfc2616.html
//
namespace KHttpUtils
{
    const ULONG MaxHostName    =  512;

    // DefaultFieldLength: represents the default length in characters for each header field (HeaderName : HeaderValue) or URI length
    // In Windows, default value of HTTP.SYS registry setting MaxFieldLength = 16384 Bytes = 8192 characters
    // We will start with 2048 and Underlying implemenation will resize the buffers if length of the received URI/ header is greater than this default value.
    //
    const ULONG DefaultFieldLength   = 2048;
    const ULONG DefaultClientCertificateSize = 2048;
    const int DefaultResolveTimeoutInMilliSeconds = 0;      // INFINITE - default value of WinHttp
    const int DefaultConnectTimeoutInMilliSeconds = 60000;  // 60s - default value of WinHttp
    const int DefaultSendTimeoutInMilliSeconds = 30000;     // 30s - default value of WinHttp
    const int DefaultReceiveTimeoutInMilliSeconds = 30000;     // 30s - default value of WinHttp

    namespace HttpHeaderName
    {
        KDualStringDefine(CacheControl,      "Cache-Control");
        KDualStringDefine(Connection,        "Connection");
        KDualStringDefine(Date,              "Date");
        KDualStringDefine(KeepAlive,         "Keep-Alive");
        KDualStringDefine(Pragma,            "Pragma");
        KDualStringDefine(Trailer,           "Trailer");
        KDualStringDefine(TransferEncoding,  "Transfer-Encoding");
        KDualStringDefine(Upgrade,           "Upgrade");
        KDualStringDefine(Via,               "Via");
        KDualStringDefine(Warning,           "Warning");

        KDualStringDefine(Allow,             "Allow");
        KDualStringDefine(ContentLength,     "Content-Length");
        KDualStringDefine(ContentType,       "Content-Type");
        KDualStringDefine(ContentEncoding,   "Content-Encoding");
        KDualStringDefine(ContentLanguage,   "Content-Language");
        KDualStringDefine(ContentLocation,   "Content-Location");
        KDualStringDefine(ContentMd5,        "Content-MD5");
        KDualStringDefine(ContentRange,      "Content-Range");
        KDualStringDefine(Expires,           "Expires");
        KDualStringDefine(LastModified,      "Last-Modified");

        KDualStringDefine(Accept,            "Accept");
        KDualStringDefine(AcceptCharset,     "Accept-Charset");
        KDualStringDefine(AcceptEncoding,    "Accept-Encoding");
        KDualStringDefine(AcceptLanguage,    "Accept-Language");
        KDualStringDefine(Authorization,     "Authorization");
        KDualStringDefine(Cookie,            "Cookie");
        KDualStringDefine(Expect,            "Expect");
        KDualStringDefine(From,              "From");
        KDualStringDefine(Host,              "Host");
        KDualStringDefine(IfMatch,           "If-Match");

        KDualStringDefine(IfModifiedSince,   "If-Modified-Since");
        KDualStringDefine(IfNoneMatch,       "If-None-Match");
        KDualStringDefine(IfRange,           "If-Range");
        KDualStringDefine(IfUnmodifiedSince, "If-Unmodified-Since");
        KDualStringDefine(MaxForwards,       "Max-Forwards");
        KDualStringDefine(ProxyAuthorization,"Proxy-Authorization");
        KDualStringDefine(Referer,           "Referer");
        KDualStringDefine(Range,             "Range");
        KDualStringDefine(TE,                "TE");
        KDualStringDefine(Translate,         "Translate");

        KDualStringDefine(UserAgent,         "User-Agent");

        KDualStringDefine(RequestMaximum,    "Request-Maximum");

        // Response Headers

        KDualStringDefine(AcceptRanges,      "Accept-Ranges");
        KDualStringDefine(Age,               "Age");
        KDualStringDefine(ETag,              "ETag");
        KDualStringDefine(Location,          "Location");
        KDualStringDefine(ProxyAuthenticate, "Proxy-Authenticate");
        KDualStringDefine(RetryAfter,        "Retry-After");
        KDualStringDefine(Server,            "Server");
        KDualStringDefine(SetCookie,         "Set-Cookie");
        KDualStringDefine(Vary,              "Vary");
        KDualStringDefine(WwwAuthenticate,   "WWW-Authenticate");

        KDualStringDefine(ResponseMaximum,   "Response-Maximum");
        KDualStringDefine(Maximum,           "Maximum");
    };

    //
    // Values that are used to specify known, standard HTTP verbs
    //
    namespace HttpVerb
    {
        KDualStringDefine(OPTIONS,    "OPTIONS");
        KDualStringDefine(GET,        "GET");
        KDualStringDefine(HEAD,       "HEAD");
        KDualStringDefine(POST,       "POST");
        KDualStringDefine(PUT,        "PUT");
        KDualStringDefine(DEL,        "DELETE");
        KDualStringDefine(TRACE,      "TRACE");
        KDualStringDefine(CONNECT,    "CONNECT");
        KDualStringDefine(TRACK,      "TRACK");
        KDualStringDefine(MOVE,       "MOVE");
        KDualStringDefine(COPY,       "COPY");
        KDualStringDefine(PROPFIND,   "PROPFIND");
        KDualStringDefine(PROPPATCH,  "PROPPATCH");
        KDualStringDefine(MKCOL,      "MKCOL");
        KDualStringDefine(LOCK,       "LOCK");
        KDualStringDefine(UNLOCK,     "UNLOCK");
        KDualStringDefine(SEARCH,     "SEARCH");
    };

    // Indicates the type of HTTP operation.
    //
    //
    enum OperationType : ULONG
    {
        eUnassigned   = 0,
        eHttpGet      = 1,
        eHttpPost     = 2,
        eHttpPut      = 3,
        eHttpDelete   = 4,
        eHttpOptions  = 5,
        eHttpHead     = 6,
        eHttpTrace    = 7,
        eHttpConnect  = 8,
        eHttpTrack    = 9,
        eHttpMove     = 10,
        eHttpCopy     = 11,
        eHttpPropFind = 12,
        eHttpPropPatch = 13,
        eHttpMkcol    = 14,
        eHttpSearch   = 15,
        eHttpLock     = 16,
        eHttpUnlock   = 17
    };

    // Used to indicate the type of data object
    // used for read/write of content.
    //
    // eNone is only legal with a GET from the client side request or
    // an empty status-code-only response from the server.
    //
    //
    enum ContentCarrierType : ULONG
    {
        eNone         = 0,
        eKBuffer      = 1,
        eKMemRef      = 2,
        eKStringView  = 3
    };

    // Common HTTP status codes.  SUCCESS == Ok == 200
    //
    enum HttpResponseCode : ULONG
    {
        NoResponse = 0,                // Not a valid HTTP code, but used internally

        Upgrade                = 101,
        Ok                     = 200,
        Found                  = 302,
        BadRequest             = 400,
        Unauthorized           = 401,
        Forbidden              = 403,
        NotFound               = 404,
        Conflict               = 409,
        Gone                   = 410,
        ContentLengthRequired  = 411,
        RequestEntityTooLarge  = 413,
        UriTooLong             = 414,
        InternalServerError    = 500,
        NotImplemented         = 501,
        BadGateway             = 502,
        ServiceUnavailable     = 503,
        GatewayTimeout         = 504,
        InsufficientResources  = 507
    };

    // Standardized headers from RFC 2616
    //
    // This is a copy from http.h in order to allow kernel-mode compiling.
    //
    enum HttpHeaderType : ULONG
    {
        // General headers

        HttpHeaderCacheControl          = 0,    // general-header [section 4.5]
        HttpHeaderConnection            = 1,    // general-header [section 4.5]
        HttpHeaderDate                  = 2,    // general-header [section 4.5]
        HttpHeaderKeepAlive             = 3,    // general-header [not in rfc]
        HttpHeaderPragma                = 4,    // general-header [section 4.5]
        HttpHeaderTrailer               = 5,    // general-header [section 4.5]
        HttpHeaderTransferEncoding      = 6,    // general-header [section 4.5]
        HttpHeaderUpgrade               = 7,    // general-header [section 4.5]
        HttpHeaderVia                   = 8,    // general-header [section 4.5]
        HttpHeaderWarning               = 9,    // general-header [section 4.5]

        HttpHeaderAllow                 = 10,   // entity-header  [section 7.1]
        HttpHeaderContentLength         = 11,   // entity-header  [section 7.1]
        HttpHeaderContentType           = 12,   // entity-header  [section 7.1]
        HttpHeaderContentEncoding       = 13,   // entity-header  [section 7.1]
        HttpHeaderContentLanguage       = 14,   // entity-header  [section 7.1]
        HttpHeaderContentLocation       = 15,   // entity-header  [section 7.1]
        HttpHeaderContentMd5            = 16,   // entity-header  [section 7.1]
        HttpHeaderContentRange          = 17,   // entity-header  [section 7.1]
        HttpHeaderExpires               = 18,   // entity-header  [section 7.1]
        HttpHeaderLastModified          = 19,   // entity-header  [section 7.1]

        // Request Headers
        HttpHeaderAccept                = 20,   // request-header [section 5.3]
        HttpHeaderAcceptCharset         = 21,   // request-header [section 5.3]
        HttpHeaderAcceptEncoding        = 22,   // request-header [section 5.3]
        HttpHeaderAcceptLanguage        = 23,   // request-header [section 5.3]
        HttpHeaderAuthorization         = 24,   // request-header [section 5.3]
        HttpHeaderCookie                = 25,   // request-header [not in rfc]
        HttpHeaderExpect                = 26,   // request-header [section 5.3]
        HttpHeaderFrom                  = 27,   // request-header [section 5.3]
        HttpHeaderHost                  = 28,   // request-header [section 5.3]
        HttpHeaderIfMatch               = 29,   // request-header [section 5.3]

        HttpHeaderIfModifiedSince       = 30,   // request-header [section 5.3]
        HttpHeaderIfNoneMatch           = 31,   // request-header [section 5.3]
        HttpHeaderIfRange               = 32,   // request-header [section 5.3]
        HttpHeaderIfUnmodifiedSince     = 33,   // request-header [section 5.3]
        HttpHeaderMaxForwards           = 34,   // request-header [section 5.3]
        HttpHeaderProxyAuthorization    = 35,   // request-header [section 5.3]
        HttpHeaderReferer               = 36,   // request-header [section 5.3]
        HttpHeaderRange                 = 37,   // request-header [section 5.3]
        HttpHeaderTe                    = 38,   // request-header [section 5.3]
        HttpHeaderTranslate             = 39,   // request-header [webDAV, not in rfc 2518]

        HttpHeaderUserAgent             = 40,   // request-header [section 5.3]

        HttpHeaderRequestMaximum        = 41,


        // Response Headers

        HttpHeaderAcceptRanges          = 20,   // response-header [section 6.2]
        HttpHeaderAge                   = 21,   // response-header [section 6.2]
        HttpHeaderEtag                  = 22,   // response-header [section 6.2]
        HttpHeaderLocation              = 23,   // response-header [section 6.2]
        HttpHeaderProxyAuthenticate     = 24,   // response-header [section 6.2]
        HttpHeaderRetryAfter            = 25,   // response-header [section 6.2]
        HttpHeaderServer                = 26,   // response-header [section 6.2]
        HttpHeaderSetCookie             = 27,   // response-header [not in rfc]
        HttpHeaderVary                  = 28,   // response-header [section 6.2]
        HttpHeaderWwwAuthenticate       = 29,   // response-header [section 6.2]

        HttpHeaderResponseMaximum       = 30,
        HttpHeaderMaximum               = 41,

        HttpHeader_INVALID              = 0xFFFFFFFF
    };

    // Authentication schemes supported
    //
    // AuthCertificate indicates that the http server should retrieve the client certificate
    // when it receives a request. HttpServer can run as admin and non-admin, so the server SSL
    // certificate configuration for 'AuthCertificate' should be by an admin out of band for
    // the specific ip:port combination.
    //
    enum HttpAuthType : ULONG
    {
        AuthNone        = 0,
        AuthCertificate = 1,
        AuthKerberos    = 2,
        AuthInvalid     = 0xFFFFFFFF
    };

    // Scheme for the URI
    //
    namespace HttpSchemes
    {
        WCHAR const * const Insecure  =  L"http";
        WCHAR const * const Secure    =  L"https";
    };

    // OpToString
    //
    // Converts the operation type to a string representation.
    // Returns FALSE if the Op is not a legal value.
    //
    _Success_(return == TRUE)
    BOOLEAN
    OpToString(
        __in  OperationType Op,
        __out KStringView& Str
        );

    // StdHeaderToStringA
    //
    // Converts the header code to a (narrow) string representation.
    // Returns a reference to an internal read-only constant string.
    //
    // Returns FALSE if the Header value has no string representation.
    //
    _Success_(return == TRUE)
    BOOLEAN
    StdHeaderToStringA(
        __in  HttpHeaderType Header,
        __out KStringViewA& HeaderString
        );

    // StdHeaderToString
    //
    // Converts the header code to a (wide) string representation.
    // Returns a reference to an internal read-only constant string.
    //
    // Returns FALSE if the Header value has no string representation.
    //
    _Success_(return == TRUE)
    BOOLEAN
    StdHeaderToString(
        __in  HttpHeaderType Header,
        __out KStringView& HeaderString
        );


    // StringToStdHeader
    //
    // Converts a (narrow) string header value to one of the standardized codes.
    // Returns FALSE if there is no code for the specified header string.
    //
    _Success_(return == TRUE)
    BOOLEAN
    StringAToStdHeader(
        __in  KStringViewA& HeaderString,
        __out HttpHeaderType& Header
        );

    // StringToStdHeader
    //
    // Converts a (wide) string header value to one of the standardized codes.
    // Returns FALSE if there is no code for the specified header string.
    //
    _Success_(return == TRUE)
    BOOLEAN
    StringToStdHeader(
        __in  KStringView& HeaderString,
        __out HttpHeaderType& Header
        );

    // StandardCodeToWinHttpCode
    //
    // Converts a server-side HTTP.SYS header code to a WinHTTP header code.
    // Returns FALSE if there is no equivalent, since the two systems are not 100% symmetric.
    //
    _Success_(return == TRUE)
    BOOLEAN
    StandardCodeToWinHttpCode(
        __in  HttpHeaderType HeaderCode,
        __out ULONG& WinHttpCode
        );

    // WinHttpCodeToStandardCode
    //
    // Converts a WinHTTP header code to a server-side HTTP.SYS header code.
    // Returns FALSE if there is no equivalent, since the two systems are not 100% symmetric.
    //
    _Success_(return == TRUE)
    BOOLEAN
    WinHttpCodeToStandardCode(
        __in   ULONG Code,
        __out  HttpHeaderType& HeaderCode
        );

    //
    //  Used to callback into the parent's apartment to perform some work
    //
    //  Actual work is done in the completion callback; this async is essentially a no-op/dummy async
    //
    //  TODO: move to a more general location, as this is not http-specific
    //
    class CallbackAsync : public KAsyncContextBase
    {
        K_FORCE_SHARED(CallbackAsync);

    public:

        KDeclareDefaultCreate(
            CallbackAsync,  //  Type created and output
            Async,          //  Output arg name
            KTL_TAG_HTTP    //  default allocation tag
            );

        using KAsyncContextBase::Start;
    };
};

class KHttpCliRequest;

//
//  KHttpClient
//
//  An Http client object from which individual requests are created.
//
class KHttpClient : public KObject<KHttpClient>, public KShared<KHttpClient>
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpClient);
    friend class KHttpCliRequest;

public:
    // Create
    //
    // Creates an instance of a reusable Http client.  A background work item is created to interact with
    // the WinHttp startup mechanism, so the object may not be read to use immediately.  However, the startup
    // will complete (or fail) relatively quickly.
    //
    // In practice, the caller should go ahead and begin creating and executing client requests, which will test the startup
    // status as part of their own operation and wait appropriate for it to complete/fail if it is not yet finished.
    //
    // Parameters:
    //      UserAgent       A user-defined "User-Agent" field.  This typically identifies the vendor & component,i.e.,
    //                      "Microsoft-Ktl-Foo"
    //      Alloc           Allocator
    //      Client          Receives the KHttpClient smart pointer.
    //
    // Result:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    static NTSTATUS
    Create(
        __in  KStringView& UserAgent,
        __in  KAllocator& Alloc,
        __out KHttpClient::SPtr& Client
        );

    //
    // CreateRequest
    //
    // Creates a new request. One request must be created for each separate HTTP operation.
    //
    // Proper use of the single part request:
    //
    // 1. Call KHttpCliRequest::SetTarget() to configure the target URI and the HTTP operation.
    //
    // 2. Optionally call KHttpCliRequest::AddHeader to configure any HTTP headers
    //
    // 3. Optionally call KHttpCliRequest::SetContent to configure the data to be sent with the request
    //
    // 4. Call KHttpClieRequest::Execute() to send the request. When the response is ready the KHttpCliRequest is completed.
    //
    NTSTATUS
    CreateRequest(
        __out KSharedPtr<KHttpCliRequest>& Request,
        __in int resolveTimeoutInMilliSeconds = KHttpUtils::DefaultResolveTimeoutInMilliSeconds,
        __in int connectTimeoutInMilliSeconds = KHttpUtils::DefaultConnectTimeoutInMilliSeconds,
        __in int sendTimeoutInMilliSeconds = KHttpUtils::DefaultSendTimeoutInMilliSeconds,
        __in int receiveTimeoutInMilliSeconds = KHttpUtils::DefaultReceiveTimeoutInMilliSeconds
        );

    NTSTATUS
    CreateRequest(
        __out KSharedPtr<KHttpCliRequest>& Request,
        __in KAllocator &Alloc,
        __in int resolveTimeoutInMilliSeconds = KHttpUtils::DefaultResolveTimeoutInMilliSeconds,
        __in int connectTimeoutInMilliSeconds = KHttpUtils::DefaultConnectTimeoutInMilliSeconds,
        __in int sendTimeoutInMilliSeconds = KHttpUtils::DefaultSendTimeoutInMilliSeconds,
        __in int receiveTimeoutInMilliSeconds = KHttpUtils::DefaultReceiveTimeoutInMilliSeconds
        );

    //
    // CreateMultipartRequest
    //
    // Creates a new request. One request must be created for each
    // separate HTTP operation. This request is used for multipart
    // messages. Multipart messages should use 
    // AsyncRequestDataContext asyncs to send the headers, content
    // and to complete the request.
    //
    // Proper use of a multipart request is as follows:
    // 
    // 1. Call KHttpCliRequest::SetTarget() to configure the target URI and the HTTP operation
    //
    // 2. Optionally call KHttpCliRequest::AddHeader to configure any HTTP headers
    //
    // 3. Use AsyncRequestDataContext::StartSendRequestHeaders to perform request validation and send headers
    //
    // 4. Multiple contents can be sent using AsyncRequestDataContext::StartSendRequestDataContent.
    //
    // 5. Finish sending the request using AsyncRequestDataContext::StartSendEndRequest.
    //
    // When the response is ready the KHttpCliRequest is completed.
    //
    NTSTATUS
    CreateMultiPartRequest(
        __out KSharedPtr<KHttpCliRequest>& Request,
        __in int resolveTimeoutInMilliSeconds = KHttpUtils::DefaultResolveTimeoutInMilliSeconds,
        __in int connectTimeoutInMilliSeconds = KHttpUtils::DefaultConnectTimeoutInMilliSeconds,
        __in int sendTimeoutInMilliSeconds = KHttpUtils::DefaultSendTimeoutInMilliSeconds,
        __in int receiveTimeoutInMilliSeconds = KHttpUtils::DefaultReceiveTimeoutInMilliSeconds
        );

    NTSTATUS
    CreateMultiPartRequest(
        __out KSharedPtr<KHttpCliRequest>& Request,
        __in KAllocator &Alloc,
        __in int resolveTimeoutInMilliSeconds = KHttpUtils::DefaultResolveTimeoutInMilliSeconds,
        __in int connectTimeoutInMilliSeconds = KHttpUtils::DefaultConnectTimeoutInMilliSeconds,
        __in int sendTimeoutInMilliSeconds = KHttpUtils::DefaultSendTimeoutInMilliSeconds,
        __in int receiveTimeoutInMilliSeconds = KHttpUtils::DefaultReceiveTimeoutInMilliSeconds
        );

    NTSTATUS
    StartupStatus()
    {
        return _StartupStatus;
    }

private:
    HINTERNET
    GetHandle()
    {
        return _hSession;
    }

    NTSTATUS
    WaitStartup()
    {
        if (_StartupStatus == STATUS_PENDING)
        {
            _StartupEvent.WaitUntilSet();
        }
        return _StartupStatus;
    }

    VOID
    BackgroundInitialize(
        __in KHttpClient::SPtr Obj
        );

    VOID
    _BackgroundInitialize();

    static VOID CALLBACK
    _WinHttpCallback(
        __in  HINTERNET hInternet,
        __in  DWORD_PTR dwContext,
        __in  DWORD dwInternetStatus,
        __in  LPVOID lpvStatusInformation,
        __in  DWORD dwStatusInformationLength
        );

    // Data members
    //
    HINTERNET   _hSession;
    NTSTATUS    _StartupStatus;
    KEvent      _StartupEvent;
    KDynString  _UserAgent;
    ULONG       _WinHttpError;
};


class KHttpClientWebSocket;

//  KHttpCliRequest
//
//  Defines a standard HTTP Request
//
//  For Get operations, call SetTarget() and then Execute().
//  For Post or other verbs, SetContent() is typically required
//  along with setting Content-Type to indicate what is being sent.
//  Other headers may be required, depending on the target.
//
class KHttpCliRequest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpCliRequest);
    friend class KHttpClient;
    friend class KHttpClientWebSocket;    //  Allow KHttpClientWebSocket to upgrade a request into a WebSocket
    friend class KHttpHeaderList;  //  Allow KHttpHeaderList to access KHttpCliRequest::HeaderPair

public:
    //  SetTarget
    //
    //  Sets the URL and operation of this particular Http request.
    //
    //  Parameters:
    //     Ep           The network endpoint containing the URL.
    //     Op           Get, Post., etc.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //      STATUS_INVALID_ADDRESS
    //
    NTSTATUS
    SetTarget(
        __in  KNetworkEndpoint::SPtr Ep,
        __in  KHttpUtils::OperationType Op
        );

    //  SetTarget
    //
    //  Sets the URL and operation of this particular Http request.
    //
    //  Parameters:
    //     Ep           The network endpoint containing the URL.
    //     OpStr        Operation type string such as Get, Post or any custom types etc.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //      STATUS_INVALID_ADDRESS
    //
    NTSTATUS
    SetTarget(
        __in  KNetworkEndpoint::SPtr Ep,
        __in  KStringView& OpStr
        );

    //  SetTarget (overload)
    //
    //  Sets the URL and operation of this particular Http request.
    //
    //  Parameters:
    //     Url          The URL
    //     Op           Get, Post., etc.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //
    NTSTATUS
    SetTarget(
        __in  KUriView& Url,
        __in  KHttpUtils::OperationType Op
        );


    // Note on headers
    //
    // For operations other than Get, Content-Type and Content-Length should be set.
    // For all operations, Accept should be set to indicate the expected media type of the response.
    //

    // AddHeader
    //
    // Appends the specified header value to the header block.
    // Duplicates are not checked for performance reasons (WinHTTP behavior).
    //
    // Parameters:
    //      HeaderId        The HTTP header, such as "Content-Type"
    //      Value           The value of the header.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND

    NTSTATUS
    AddHeader(
        __in KStringView& HeaderId,
        __in KStringView& Value
        );

    // AddHeaderByCode
    //
    // An alternate way of specificying standard headers.  For common
    // headers, this is preferable as it avoids spelling errors in headers
    // and associated bugs.
    //
    // If the header is not standardized, then STATUS_OBJECT_NAME_NOT_FOUND
    // will be returned and AddHeader() will have to be used.
    //
    // Duplicates are not checked for performance reasons (WinHTTP behavior).
    //
    //
    // Parameters:
    //      HeaderCode          A standardized header by its code value.
    //      Value               The header value
    //
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    AddHeaderByCode(
        __in KHttpUtils::HttpHeaderType HeaderCode,
        __in KStringView& Value
        );


    //
    // Set the headers for this request to those specified in the supplied KHttpHeaderList
    //
    NTSTATUS
    SetHeaders(
        __in KHttpHeaderList const & KHttpHeaderList
        );

    //
    // Set the headers. The headers set arent interpreted, so this should be properly formatted
    // by the caller. This is usually used when the caller has a blob of headers that he wants to 
    // be directly set on the request.
    //
    NTSTATUS
    SetHeaders(
        __in KString::SPtr const &HeadersBlob
        );

    // SetContent
    //
    // Used for Post, etc. The content must match the media type
    // in the Content-Type header.
    //
    // Content-Length will be automatically set to the size of this
    // buffer.
    //
    //
    NTSTATUS
    SetContent(
        __in KBuffer::SPtr& Buffer
        );

    // SetContent (overload)
    //
    // This is intended for cases where the input content buffer
    // already exists in some form besides a KBuffer.
    //
    // The caller must ensure the buffer is Valid until the operation completes.
    // The _Param value in <Mem> indicates the size of the transfer and the
    // Content-Length header will be set to that value.
    //
    // Content-Length will be automatically set to the size of this
    // buffer.
    //
    NTSTATUS
    SetContent(
        __in KMemRef& Mem
        );


    // SetContent (overload)
    //
    // This is intended for cases where the input content buffer
    // already exists in some form besides a KBuffer.
    //
    // The caller must ensure the buffer is valid until the operation completes.
    //  
    // This will set Content-Length to the length (in bytes) of the string.
    //
    //
    NTSTATUS
    SetContent(
        __in KStringView& StringContent
        );

    // SetClientCertificate
    //
    // Sets the client certificate context.
    // As part of this, KTL will call CertDuplicateCertificateContext on the provided certificate context 
    // so this certificate context must be independently released by the caller.
    //
    NTSTATUS
    SetClientCertificate(
            __in PCCERT_CONTEXT CertContext
        );

    // ServerCertValidationHandler
    // Handler that verifies the remote Secure Sockets Layer (SSL) certificate used for authentication. 
    //
    // Parameter:
    // PCCERT_CONTEXT : Pointer to the CERT_CONTEXT structure.
    // KTL will take care of freeing the PCCERT_CONTEXT once the validation callback has executed.
    // Returns:
    // TRUE: Server certificate validation succeeded. Proceed with the request.
    // FALSE: Server certificate validation failed. Fail the request.
    //
    typedef KDelegate<BOOLEAN(__in PCCERT_CONTEXT pCertContext)> ServerCertValidationHandler;

    // SetServerCertValidationHandler
    //
    // Parameter: ServerCertValidationHandler
    // Sets a handler/callback function to validate the server certificate.
    //
    NTSTATUS
    SetServerCertValidationHandler(
        __in ServerCertValidationHandler Handler
        );

    // SetAllowRedirects
    //
    // Parameter: BOOLEAN
    // Allow/disable automatic redirection when sending requests with WinHttpSendRequest
    // By default this is enabled
    NTSTATUS
    SetAllowRedirects(
        __in BOOLEAN AllowRedirects
        );

    // SetEnableCookies
    //
    // Parameter: BOOLEAN
    // Automatic addition of cookie headers to requests
    // By default this is enabled
    NTSTATUS
    SetEnableCookies(
        __in BOOLEAN EnableCookies
        );

    // SetAutoLogonForAllRequests
    //
    // Parameter: BOOLEAN
    // TRUE: An authenticated log on using the default credentials is performed for all requests.
    // By default autologon is enabled only for requests on the local intranet(any server on the proxy bypass list in the current proxy configuration.)
    // Note: This can be used in conjunction with SetClientCertificate. Depending on the Server's auth policy, either one or both windows auth and certificate based auth will be used. 
    NTSTATUS
    SetAutoLogonForAllRequests(
        __in BOOLEAN EnableAutoLogonForAllRequests
        );

    // GetWinHttpError
    //
    // Returns the WinHttpError code if an error occurs.
    //
    ULONG
    GetWinHttpError()
    {
        return _WinHttpError;
    }


    // Response object
    //
    // This is object type which houses the response to the operation.
    //
    class Response : public KShared<Response>, public KObject<Response>
    {
        K_FORCE_SHARED_WITH_INHERITANCE(Response);
        friend class KHttpCliRequest;
        friend class KHttpClient;
        static const ULONG DefaultResponseBufSize = 8192;

        Response(BOOLEAN IsMultiPart);

    public:
        // GetHttpResponseCode
        //
        // Returns the HTTP response code for this request.
        //
        // Note that 200 (HttpResponseCode enumeration) is the success code.
        //
        ULONG
        GetHttpResponseCode()
        {
            return _HttpStatusCode;
        }

        // GetHttpResponseStatusText
        //
        // Retrieves the additional text returned by the server on the response line.
        //
        // Parameters:
        //      Value           Receives the status text string.
        //
        NTSTATUS
        GetHttpResponseStatusText(
            __out KString::SPtr& Value
            );

        // GetHeader
        //
        // Retrieves a specific header.  This can be used for both
        // standardized and custom headers.
        //
        // Parameters:
        //      HeaderName      The HTTP header name.
        //      Value           Receives the header string.
        //
        // Return values:
        //      STATUS_SUCCESS
        //      STATUS_OBJECT_NAME_NOT_FOUND
        //      STATUS_INSUFFICIENT_RESOURCES
        //
        NTSTATUS
        GetHeader(
            __in  LPCWSTR  HeaderName,
            __out KString::SPtr& Value
            );

        // GetHeaderByCode
        //
        // Retrieves a specific header using the standardized codes.
        // This tends to eliminate spelling errors in standard header names.
        //
        // Parameters:
        //      HeaderCode      The HTTP header code; works only for standard headers.
        //      Value           Receives the header string.
        //
        //
        // Return values:
        //      STATUS_SUCCESS
        //      STATUS_OBJECT_NAME_NOT_FOUND
        //      STATUS_INSUFFICIENT_RESOURCES
        //
        NTSTATUS
        GetHeaderByCode(
            __in  KHttpUtils::HttpHeaderType HeaderCode,
            __out KString::SPtr& Value
            );

        // GetContentLengthHeader
        //
        // Returns the value of the Content-Length header.
        // This may be absent (zero) according to the rules of HTTP and yet
        // there still may be content.
        //
        // To retrieve the actual content read, use GetEntityLength().
        //
        //
        ULONG
        GetContentLengthHeader()
        {
            return _ContentLengthHeader;
        }

        // GetEntityBodyLength()
        //
        // Returns the number of bytes actually read as content.  This value
        // should be identical to the Content-Length (if that was present).
        //
        // If Content-Length is absent, there is no way to verify that this value constitutes the
        // full response, except by other external means, such as end-markers in the
        // content itself, or the fact that it is a parsable XML document, etc.
        //
        ULONG
        GetEntityBodyLength()
        {
            return _EntityBodyLength;
        }

        // GetContent
        //
        // The caller must know how to interpret the buffer by examining the
        // Content-Type header.
        //
        // Parameters:
        //      Buf          Receives the internal buffer containing the content.
        //
        NTSTATUS
        GetContent(
            __out KBuffer::SPtr& Buf
            );


        // GetAllHeaders
        //
        // Diagnostic function to dump as a single string
        // all the headers that came back.
        //
        NTSTATUS
        GetAllHeaders(
            __out KString::SPtr& Headers
            );

        // GetWinHttpError
        //
        // Returns the WinHttpError code if an error occurs.
        //
        ULONG
        GetWinHttpError()
        {
            return _WinHttpError;
        }
        
    private:
        VOID
        IncEntityBodyLength(ULONG Count)
        {
            _EntityBodyLength += Count;
        }

        NTSTATUS
        ProcessStdHeaders();

        BOOLEAN
        ReallocateResponseBuffer(
            __in ULONG Size
            );

        // GetHeaderByWinHttpCode
        //
        // Retrieves a specific header using the WinHttp Query info flags.
        //
        // Parameters:
        //      WinHttpCode     The WinHttp query flag used by WinHttpQueryHeaders.
        //      Value           Receives the header string.
        //
        //
        // Return values:
        //      STATUS_SUCCESS
        //      STATUS_OBJECT_NAME_NOT_FOUND
        //      STATUS_INSUFFICIENT_RESOURCES
        //
        NTSTATUS
        GetHeaderByWinHttpCode(
            __in DWORD WinHttpCode,
            __out KString::SPtr& Value
            );

        KBuffer::SPtr         _Content;
        HINTERNET             _hRequest;
        ULONG                 _ContentLengthHeader; // Preferred but not required
        ULONG                 _EntityBodyLength;    // Bytes actually read
        ULONG                 _HttpStatusCode;
        BOOLEAN               _IsMultiPart;
        ULONG                 _WinHttpError;
    };


    // Execute
    //
    // Executes the request. The Response object is immediately returned,
    // although there is no readable content from it until the operation
    // completes.
    //
    // Since many servers do not return a Content-Length header, the
    // receiving buffer size is guesswork.  The default size is 8K, but
    // if the response is larger, then reallocations will have to occur
    // as the response is read if it exceeds this size.
    //
    // If the caller knows the approximate size of the response, then
    // it is advantageous to use a more suitable value than 8192.
    //
    // If the Content-Length header is present in the response, it will be used
    // instead so as to make allocations more efficient.
    //
    // If the request was created using CreateMultipartRequest then
    // AsyncRequestDataContext should be used instead
    //
    // Parameters:
    //      Response            Receives the response object, which contains
    //                          no information until the completion callback occurs.
    //      ParentContext       Async parent
    //      Callback            Async completion callback.
    //      DefaultResponseBufSize     See notes above.
    //
    // Return values:
    //      STATUS_PENDING      Success
    //
    NTSTATUS
    Execute(
        __out    Response::SPtr&  Response,
        __in_opt KAsyncContextBase* const ParentContext = nullptr,
        __in_opt KAsyncContextBase::CompletionCallback Callback = 0,
        __in     ULONG DefaultResponseBufSize = 8192
        );

    //
    // Writes data to an executing request. Used internally by the AsyncSendRequestData async only
    //
    NTSTATUS
    WriteData(
        __in KHttpUtils::ContentCarrierType ContentType,
        __in KMemRef& ContentKMemRef,
        __in KStringView& ContentKStringView,
        __in_opt KBuffer* const ContentKBuffer = nullptr
    );

    //
    // Signifies the end of a multipart request. Used internally by the AsyncSendRequestData async only
    //
    NTSTATUS
    EndRequest(
        );

    //
    // Reads data from the response to a request.  Used internally by the AsyncSendRequestData async only
    //
    NTSTATUS
    ReadData(
    );

    //
    // This class implements an async that allows sending multipart
    // messages. It can only be used with KHttpCliRequest created using
    // CreateMultipartRequest.
    //
    class AsyncRequestDataContext : public KAsyncContextBase
    {
        K_FORCE_SHARED(AsyncRequestDataContext);

        friend KHttpCliRequest;

        public:
            //
            // Create an AsyncRequestDataContext
            //
            static
            NTSTATUS
            Create(__out AsyncRequestDataContext::SPtr& Context,
                    __in KAllocator& Allocator,
                    __in ULONG AllocationTag
            );


            //
            // Starts an async to send request headers only. Internally the async will
            // also start the KHttpRequest async which itself is completed when the request
            // is fully completed, typically after StartEndRequest is called. This async 
            // is completed once the headers have been successfully sent.
            //
            // Request is the request on which to send the data
            //
            // Response returns with a pointer to the response object. If Response is != nullptr then
            //    the response object will complete after the last data is read. If Response is == nullptr
            //    then the request was not started and the caller should not expect RequestCallback to be
            //    invoked.
            //
            // RequestParentContext is the parent context for the KHttpRequest
            //
            // RequestCallback is the async completion callback invoked whern the KHttpRequest completes
            //
            // ParentAsyncContext - Supplies an optional parent async context for this async
            //
            // CallbackPtr - Supplies an optional callback delegate to be notified
            //               when a notification event for this async occurs
            //
            VOID
            StartSendRequestHeader(
                __in KHttpCliRequest& Request,   
                __in ULONG ContentLength,
                __out KHttpCliRequest::Response::SPtr&  Response,
                __in_opt KAsyncContextBase* const RequestParentContext,
                __in_opt KAsyncContextBase::CompletionCallback RequestCallback,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
                );

            //
            // Starts an async to send request data
            //
            // Request is the request on which to send the data
            //
            //  Buffer          The KBuffer containing the content to transmit.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendRequestDataContent(
                __in KHttpCliRequest& Request,                                      
                __in KBuffer::SPtr& Buffer,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to send request data
            //
            // Request is the request on which to send the data
            //
            //  Mem         The _Param of the KMemRef struct is used to indicate the amount to transmit.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendRequestDataContent(
                __in KHttpCliRequest& Request,                                      
                __in KMemRef& Mem,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to send request data
            //
            // Request is the request on which to send the data
            //
            //  StringContent       A string to be sent back.  Useful for HTML/XML/text responses.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendRequestDataContent(
                __in KHttpCliRequest& Request,                                      
                __in KStringView& StringContent,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);
            
            //
            // Starts an async to complete the request send to the
            // server
            //
            // Request is the request on which to send the data
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartEndRequest(
                __in KHttpCliRequest& Request,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to receive the response content data.
            // NOTE: this async may only be called after StartEndRequest has completed successfully.
            //
            // Request is the request on which to send the data
            //
            // MaxBytesToRead is the maximum number of bytes that will be returned
            //     by the read.
            // 
            // ReadContent returns with a buffer containing the content
            //     data received
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartReceiveResponseData(
                __in KHttpCliRequest& Request, 
                __in ULONG MaxBytesToRead,
                __out KBuffer::SPtr& ReadContent,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to receive the response content data.
            // NOTE: this async may only be called after StartEndRequest has completed successfully.
            //
            // Request is the request on which to send the data
            //
            // Mem supplies the address and size of buffer to which the data will be read into
            //     Size indicates the max data size that will be attempted to
            //     be read.
            //     On completion, the _Param in the memref will be set to the actual bytes read
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartReceiveResponseData(
                __in KHttpCliRequest& Request,
                __inout KMemRef *Mem,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async that completes when the connection to the server is disconnected.
            // NOTE: this can be called at anytime during the request to terminate the connection.
            //
            // Request is the request which is to be disconnected.
            //
            // Status indicates the status with which the multipart and the parent request should
            //     completed with
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartDisconnect(
                __in KHttpCliRequest& Request,
                __in NTSTATUS Status,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            using KAsyncContextBase::Complete;

            VOID
            CompleteRead(
                __in NTSTATUS Status
                );

            ULONG
            GetContentLength()
            {
                return(_ContentLength);
            }

            ULONG
            GetMaxBytesToRead()
            {
                return(_MaxBytesToRead);
            }

            NTSTATUS
            GetOrAllocateReadBuffer(
                __in ULONG Size,
                __out PVOID* Address
                );

            private:
                enum { Unassigned,
                       SendRequestHeader,
                       SendRequestData, 
                       EndRequest, 
                       ReceiveResponseData,
                       Disconnect,
                       Completed, 
                       CompletedWithError } _State;

                VOID
                Initialize(
                );

                VOID 
                SendRequestDataFSM(
                    __in NTSTATUS Status
                    );

                VOID 
                SendRequestHeaderFSM(
                    __in NTSTATUS Status
                    );

                VOID 
                EndRequestFSM(
                    __in NTSTATUS Status
                    );

                VOID 
                ReceiveResponseDataFSM(
                    __in NTSTATUS Status
                    );

                VOID
                DisconnectFromServerFSM(
                    __in NTSTATUS Status
                    );

            protected:
                VOID
                OnStart(
                    ) override ;

                VOID
                OnReuse(
                    ) override ;

                VOID
                OnCompleted(
                    ) override ;

                VOID
                OnCancel(
                    ) override ;

            private:
                //
                // General members
                //
                
                //
                // Parameters to api
                //
                KHttpCliRequest::SPtr _Request;                                      

                // SendRequestHeaders
                ULONG _ContentLength;
                KHttpCliRequest::Response::SPtr*  _Response;
                KAsyncContextBase::SPtr _RequestParentContext;
                KAsyncContextBase::CompletionCallback _RequestCallback;

                KHttpUtils::ContentCarrierType _ContentCarrier;

                // SendRequestData
                KMemRef _Content_KMemRef;
                KBuffer::SPtr _Content_KBuffer;
                KStringView _Content_KStringView;

                //
                // ReceiveResponseData
                //
                ULONG _MaxBytesToRead;
                KBuffer::SPtr* _ReadContent;
                KMemRef* _ReadContent_KMemRef;

                //
                // Members needed for functionality
                //
                KBuffer::SPtr _ReadBuffer;
    };

protected:

    VOID
    FailRequest(
        __in NTSTATUS Status
        );

private:

    VOID
    OnStart() override;

    KHttpCliRequest(
        __in KHttpClient::SPtr Session,
        __in BOOLEAN IsMultiPart
        );

    VOID
    SetWinHttpError(ULONG Err)
    {
        _WinHttpError = Err;
    }

    VOID
    Deferred_Continue();

    BOOLEAN
    ValidateServerCertificate();

    VOID 
    CancelRequest();

    struct HeaderPair : public KShared<HeaderPair>, public KObject<HeaderPair>
    {
        typedef KSharedPtr<HeaderPair> SPtr;

        HeaderPair()
            : _Id(GetThisAllocator()), _Value(GetThisAllocator())
            {
            }

        static NTSTATUS
        AddHeader(
            __in KStringView& HeaderId,
            __in KStringView& Value,
            __in KArray<HeaderPair::SPtr>& Headers,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_HTTP,
            __in PVOID ContextPointer = nullptr,
            __in ULONGLONG AdditionalErrorTrace0 = 0,
            __in ULONGLONG AdditionalErrorTrace1 = 0
            );

        static NTSTATUS
        AddHeaderByCode(
            __in KHttpUtils::HttpHeaderType HeaderCode,
            __in KStringView& Value,
            __in KArray<HeaderPair::SPtr>& Headers,
            __in KAllocator& Allocator,
            __in ULONG AllocationTag = KTL_TAG_HTTP,
            __in PVOID ContextPointer = nullptr,
            __in ULONGLONG AdditionalErrorTrace0 = 0,
            __in ULONGLONG AdditionalErrorTrace1 = 0
            );

        KDynString _Id;
        KDynString _Value;
    };

    // Data members
    //
    KArray<HeaderPair::SPtr>              _Headers;
    KString::SPtr                         _HeadersBlob;
    KHttpClient::SPtr                     _Parent;
    KUri::SPtr                            _Url;
    KString::SPtr                         _Op;
    ULONG                                 _ContentLengthHeader;
    HINTERNET                             _hRequest;
    HINTERNET                             _hConnection;
    ULONG                                 _WinHttpError;
    KDeferredCall<VOID(VOID)>             _Continue;
    BOOLEAN                               _AllowRedirects;
    BOOLEAN                               _EnableCookies;
    BOOLEAN                               _EnableAutoLogonForAllRequests;
    KHttpUtils::ContentCarrierType        _ContentCarrier;
    KMemRef                               _Content_KMemRef;
    KBuffer::SPtr                         _Content_KBuffer;
    KStringView                           _Content_KStringView;
    Response::SPtr                        _Response;

    //
    // Fields used for SSL requests
    //
    BOOLEAN                               _IsSecure;
    PCCERT_CONTEXT                        _ClientCert;
    ServerCertValidationHandler           _ServerCertValidationHandler;

    //
    // Fields used for multipart requests
    //
    BOOLEAN                               _IsMultiPart;
    AsyncRequestDataContext::SPtr     _ASDRContext;

    //
    // WinHttp request timeout values
    //
    int                                 _resolveTimeoutInMilliSeconds;
    int                                 _connectTimeoutInMilliSeconds;
    int                                 _sendTimeoutInMilliSeconds;
    int                                 _receiveTimeoutInMilliSeconds;
};


//
//  Class providing a way to pass a header block constructed in the builder style 
//  as a single parameter.
//
//  A potential refactor might be to use this under the hood within KHttpCliRequest
//  rather than using an array
//
class KHttpHeaderList : public KShared<KHttpHeaderList>, public KObject<KHttpHeaderList>
{
    K_FORCE_SHARED(KHttpHeaderList);
    friend class KHttpCliRequest;  //  Allow KHttpCliRequest to access _Headers array when setting headers
    friend class KHttpClientWebSocket;  //  "

public:

    static NTSTATUS
    Create(
        __out KHttpHeaderList::SPtr& KHttpHeaderList,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __in ULONG ReserveSize = 16
        );

    // AddHeader
    //
    // Appends the specified header value to the header block.
    // Duplicates are not checked for performance reasons (WinHTTP behavior).
    //
    // Parameters:
    //      HeaderId        The HTTP header, such as "Content-Type"
    //      Value           The value of the header.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    NTSTATUS
    AddHeader(
        __in KStringView& HeaderId,
        __in KStringView& Value
        );

    // AddHeaderByCode
    //
    // An alternate way of specificying standard headers.  For common
    // headers, this is preferable as it avoids spelling errors in headers
    // and associated bugs.
    //
    // If the header is not standardized, then STATUS_OBJECT_NAME_NOT_FOUND
    // will be returned and AddHeader() will have to be used.
    //
    // Duplicates are not checked for performance reasons (WinHTTP behavior).
    //
    //
    // Parameters:
    //      HeaderCode          A standardized header by its code value.
    //      Value               The header value
    //
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    AddHeaderByCode(
        __in KHttpUtils::HttpHeaderType HeaderCode,
        __in KStringView& Value
        );

private:

    KHttpHeaderList(
        __in KAllocator& Allocator,
        __in ULONG ReserveSize = 16
        );

    KArray<KHttpCliRequest::HeaderPair::SPtr> _Headers;
};


//
// **************************** SERVER *****************************************
//

class KHttpServerRequest;
class KHttpServer;


//
//  KHttpServer
//
//  The root HTTP client object.
//
class KHttpServer : public KAsyncServiceBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServer);
    friend class KHttpServerRequest;
    friend class KHttpServerWebSocket;

public:
    //  RequestHandler (KDelegate)
    //
    //  This is the callback to which each client request is dispatched.  The Request object
    //  is already in a started state.  The user will query the request for inbound data
    //  and set the response data, and then Send the response. Completion is hidden within the
    //  implementation.  The receiver may hold the request and spawn other async operations
    //  and complete this at any later time, within typical HTTP client timeouts.
    //
    //  Parameters:
    //          Request             The request being served from http.sys
    //
    typedef KDelegate<VOID(__in KSharedPtr<KHttpServerRequest> Request)> RequestHandler;

    //  RequestHeadersHandler (KDelegate)
    //
    typedef enum
    {
        Undefined = 0,

        //
        // This action instructs that the application will perform one
        // or more reads of the content data using
        // AsyncReceiveRequestContext. The RequestHandler is not
        // invoked.
        //
        ApplicationMultiPartRead = 1,

        //
        // This action instructs the engine to read all content data
        // and any certificate and then invoke the RequestHandler. This
        // is the default for any request that does not supply a header 
        // handler.
        //
        DeliverCertificateAndContent = 2,

        //
        // This action instructs the engine to read all content data
        // but not the certificate and then invoke the RequestHandler.
        //
        DeliverContent = 3
    } HeaderHandlerAction;
    
    //  This is the callback to which each client request is dispatched
    //  when the headers are completely read. The callback can
    //  determine which action to take on return from this handler. 
    //
    //  Parameters:
    //          Request             The request being served from
    //                              http.sys
    //
    //          Action              Returns with the action to be taken
    //                              on the request.
    //
    typedef KDelegate<VOID(__in KSharedPtr<KHttpServerRequest> Request, __out HeaderHandlerAction& Action)> RequestHeaderHandler;

    static VOID 
    DefaultHeaderHandler(
        __in KSharedPtr<KHttpServerRequest> Request,
        __out HeaderHandlerAction& Action
    );

    // Create
    //
    // Creates an inactive server object. This must be activated.
    //
    // Parameters:
    //
    //      AllowNonAdminServerCreation     By default, the HTTP Server API only allows administrators
    //                                      to register for receiving requests to a URL prefix. When running
    //                                      as a non admin user, the URL prefix should be reserved on behalf
    //                                      of that user by the admin. So the default behavior of the Ktl Http
    //                                      server is to reject requests for the server object creation if the
    //                                      current user is non admin. This parameter is an explicit override
    //                                      to that behavior to support cases where the url namespace reservations
    //                                      have been done already.
    //
    static NTSTATUS
    Create(
        __in  KAllocator& Allocator,
        __out KHttpServer::SPtr& Server,
        __in BOOLEAN AllowNonAdminServerCreation = FALSE
        );


    // StartOpen
    //
    // Starts up the server.  The caller must also subsequently call RegisterUrl at least
    // once to intialize a URL for listening.
    //
    // Parameters:
    //      UrlPrefix               The URL to listen on.  The port MUST be included in the URL.
    //                              This acts as a prefix.  All URLs beginning with this will be accepted.
    //
    //      DefaultCallback         The default delegate to invoke as each new request arrives for processing.  This will be used
    //                              for all requests which are not specifically handled by dedicated handlers createad by
    //                              RegisterHandler.
    //
    //      DefaultHeaderCallback   The optional default delegate to invoke as each new request header arrives for processing.  
    //                              If specified it is used for all
    //                              requests which are not specifically
    //                              handled by a dedicated handlers
    //                              registered by
    //                              RegisterHandler.
    //
    //      PrepostedRequests       How many reads to prepost.  New ones are auto-created as the old ones complete, so this
    //                              represents the maximum concurrent servicing of requests.
    //
    //      DefaultHeaderBufferSize The preallocation size to use for header receiver buffers.
    //      DefaultEntityBodySize   The preallocation size to use for receiving entity bodies if no Content-Length is present.
    //                              This should be set to a size large enough to receive typical requests for the server type
    //                              that is being exposed.  Messages larger than this will be successfully received, but
    //                              perf time will be lost doing reallocation if the incoming message is larger than this.
    //
    //      Parent                  Parent async context
    //      Completion              The completion routine for open completion.
    //      GlobalContextOverride   Global context
    //
    //      MaximumEntityBodySize   The maximum size of an entity body
    //                              that the server will accept. If 0 there is no limit. It is
    //                              recommended that the caller set a reasonable value for this
    //                              to avoid DOS attacks by agents that may send huge payloads
    //                              in an effort to exaust memory.
    //
    // Return value:
    //      STATUS_INVALID_ADDRESS  If the URL is syntactically bad or missing the port value.
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_PENDING          On success.
    //
    //
    NTSTATUS
    StartOpen(
        __in KUriView& UrlPrefix,
        __in RequestHandler DefaultCallback,
        __in ULONG PrepostedRequests,
        __in ULONG DefaultHeaderBufferSize = 8192,
        __in ULONG DefaultEntityBodySize = 8192,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in KAsyncServiceBase::OpenCompletionCallback Completion = 0,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr,
        __in ULONG MaximumEntityBodySize = 0
        );

    NTSTATUS
    StartOpen(
        __in KUriView& UrlPrefix,
        __in RequestHandler DefaultCallback,
        __in ULONG PrepostedRequests,
        __in ULONG DefaultHeaderBufferSize = 8192,
        __in ULONG DefaultEntityBodySize = 8192,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in KAsyncServiceBase::OpenCompletionCallback Completion = 0,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr,
        __in KHttpUtils::HttpAuthType authInfo = KHttpUtils::HttpAuthType::AuthNone,
        __in ULONG MaximumEntityBodySize = 0
        );

    NTSTATUS
    StartOpen(
        __in KUriView& UrlPrefix,
        __in RequestHandler DefaultCallback,
        __in RequestHeaderHandler DefaultHeaderCallback,
        __in ULONG PrepostedRequests,
        __in ULONG DefaultHeaderBufferSize = 8192,
        __in ULONG DefaultEntityBodySize = 8192,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in KAsyncServiceBase::OpenCompletionCallback Completion = 0,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr,
        __in KHttpUtils::HttpAuthType authInfo = KHttpUtils::HttpAuthType::AuthNone,
        __in ULONG MaximumEntityBodySize = 0
        );



    // StartClose
    //
    // Requests a shutdown the server.
    // This returns immediately and shutdown is complete only after async Completion.
    //
    // Parameters:
    //      Parent                  Parent async context
    //      Completion              The completion routine for open completion.
    //
    // Return value:
    //      STATUS_PENDING          On success.
    //
    using KAsyncServiceBase::StartClose;


    // RegisterHander
    //
    // Registers an override handler. Any request that does not match a handler registered by this function
    // will be handled by the default handler used in Open()
    //
    // Parameters:
    //      Suffix              The relative URL to use for this handler.  This is a suffix to the UrlPrefix used in Activate.
    //
    //      ExactMatch          If TRUE, only URIs which precisely match the Url will be routed to the handler.
    //                          If FALSE, any request which  has a prefix match with the Url will be routed to the handler.
    //      Callback            The delegate which will receive the request.  The same callback can be used for multiple
    //                          Urls if necessary.
    //
    //      HeaderCallback      Optional callback invoked when headers are received. If a request for this URL needs
    //                          to be a multi-part request (ie, receive or send multiple parts) then header callback is needed
    //                          and the header callback should return KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead
    //
    NTSTATUS
    RegisterHandler(
        __in KUriView& SuffixUrl,
        __in BOOLEAN ExactMatch,
        __in RequestHandler Callback,
        __in_opt RequestHeaderHandler HeaderCallback = NULL
        );

    // UnregisterHandler
    //
    // Unregisters a previously registered handler.
    //
    NTSTATUS
    UnregisterHandler(
        __in KUriView& SuffixUrl
        );

    // ResolveHandler
    //
    // Returns the handler that would be used for the URL suffix.  This takes into account the 'exact match' semantics
    // of the registered handlers.
    //
    NTSTATUS
    ResolveHandler(
        __in BOOLEAN IsHeaderHandler,
        __in  KDynUri& Suffix,
        __out RequestHeaderHandler& HeaderCallback,
        __out RequestHandler& Callback
        );

    // InvokeHandler
    //
    VOID
    InvokeHandler(
        __in KSharedPtr<KHttpServerRequest> Req
        );

    // InvokeHeaderHandler
    //
    VOID
    InvokeHeaderHandler(
        __in KSharedPtr<KHttpServerRequest> Req,
        __out HeaderHandlerAction& Action
        );

protected:

    virtual NTSTATUS
    PostRead();

    virtual VOID
    ReadCompletion(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

private:

    ULONG GetDefaultHeaderBufferSize()
    {
        return _DefaultHeaderBufferSize;
    }

    ULONG GetDefaultEntityBodySize()
    {
        return _DefaultEntityBodySize;
    }

    ULONG GetMaximumEntityBodySize()
    {
        return _MaximumEntityBodySize;
    }

    static BOOLEAN
    IsUserAdmin();

    NTSTATUS
    Initialize();

    NTSTATUS
    BindIOCP();

    VOID
    UnbindIOCP();

    virtual VOID
    OnServiceOpen() override;

    virtual VOID
    OnServiceClose() override;

    virtual VOID
    OnCompleted() override;

    VOID
    Halt(
        __in NTSTATUS Reason
        );

    virtual VOID
    WebSocketCompletion(
        __in KAsyncContextBase* const Parent,
       __in KAsyncContextBase& Op
       );

    struct HandlerRegistration
    {
        KUri::SPtr     _Suffix;
        BOOLEAN        _ExactMatch;
        RequestHandler _Handler;
        RequestHeaderHandler _HeaderHandler;
    };

    // Data members
    //
    ULONG           _HaltCode;
    HANDLE          _hRequestQueue;
    HTTP_SERVER_SESSION_ID _hSessionId;
    HTTP_URL_GROUP_ID  _hUrlGroupId;
    KDynUri         _Url;
    RequestHandler  _DefaultHandler;
    RequestHeaderHandler  _DefaultHeaderHandler;

    KSpinLock                   _TableLock;
    KArray<HandlerRegistration> _OverrideHandlers;

    PVOID           _IoRegistrationContext;
    BOOLEAN         _Shutdown;
    ULONG           _PrepostedRequests;
    ULONG           _DefaultHeaderBufferSize;
    ULONG           _DefaultEntityBodySize;
    ULONG           _MaximumEntityBodySize;
    KHttpUtils::HttpAuthType _AuthType;
    ULONG _WinHttpAuthType;
};


class KHttpServerWebSocket;

// KHttpServerRequest
//
// Models an incoming HTTP Server request.  The user receives these
// via a Request callback registered for each URL.  The user will query
// the incoming data using the Get* methods and set the response data
// using the Set* methods and then Complete() the request.
//
// Completing without setting all the correct fields will result
// in an error in the completion callback and the client will receive
// an HTTP 500 Internal Server Error in those cases.
//
class KHttpServerRequest : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServerRequest);
    friend class KHttpServer;
    friend class KDeferredJump<VOID()>;
    friend class KHttpServerWebSocket;  //  Allow KHttpServerWebSocket to upgrade a request to a WebSocket

protected:

    KHttpServerRequest(
    __in KSharedPtr<KHttpServer> Parent
    );

    NTSTATUS Initialize();

public:

    // GetOpType
    //
    // Returns the operation type.
    //
    KHttpUtils::OperationType
    GetOpType();

    // GetOpString
    //
    // Returns the operation type string (HTTP Verb)
    //
    BOOLEAN 
    GetOpString(
        __out KString::SPtr& OpStr
        );

    // GetUrl
    //
    // Returns the parsed URL.
    //
    NTSTATUS
    GetUrl(
        __out KUri::SPtr& Url
        );


    // GetRelativeUrl
    //
    // Gets the suffix, i.e., the portion of the URL that is not part of the overall server prefix URL
    // This may be empty (but not null) if the URL used in the request matches the server prefix exactly (the default 'page' or HTTP address).
    //
    KUri::SPtr
    GetRelativeUrl()
    {
        return _RelativeUrl;
    }


    // GetRequestHeaderByCode
    //
    // Retrieves a standardized header
    //
    // Parameters:
    //      HeaderId            The header to be retrieved.
    //      KStringView         Returns a read-only reference to internal memory
    //
    // Return values:
    //      STATUS_NOT_FOUND if the header was not present in the request.
    //      STATUS_SUCCESS
    //
    NTSTATUS
    GetRequestHeaderByCode(
        __in  KHttpUtils::HttpHeaderType HeaderId,
        __out KString::SPtr& Value
        );

    // GetRequestHeader
    //
    // Used to retrieve any header. Primarily intended for custom headers for which
    // there is no standard code.
    //
    // Parameters:
    //      HeaderName          The header to be retrieved.
    //      KStringView         Returns a read-only reference to internal memory
    //
    // Return values:
    //      STATUS_NOT_FOUND if the header was not present in the request.
    //      STATUS_SUCCESS
    //
    NTSTATUS
    GetRequestHeader(
        __in  KStringView& HeaderName,
        __out KString::SPtr& Value
        );

    // GetAllHeaders
    //
    // Primarily for debugging.  Returns all the headers in the request into a single string, CR/LF separated.
    //
    NTSTATUS
    GetAllHeaders(
        __out KString::SPtr& HeaderBlock
        );

    NTSTATUS
    GetAllHeaders(
        __out KHashTable<KWString, KString::SPtr> &Headers
        );

    // GetRequestContentLengthHeader
    //
    // Used to retrieve the Content-Length header value in integral form.
    // Since this header is not required, the caller may have to use GetRequestEntityLength.
    //
    // Parameters:
    //      Length      Receives the Content-Length value if it was specified and TRUE is returned,.
    //
    // Return value:
    //      TRUE if the Content-Length header was present and returned.
    //      FALSE if the Content-Length header was absent.
    //
    BOOLEAN
    GetRequestContentLengthHeader(
        __out ULONG& Length
        );

    // GetRequestEntityLength
    //
    // Returns the number of bytes actually transmitted by the client.
    // If Content-Length was not specified, then this value and the content
    // itself must be verified by some external mechanism.
    //
    // This may return zero if the client sent 0 bytes of content and did a
    // headers-only operation.
    //
    ULONG
    GetRequestEntityBodyLength()
    {
        KInvariant(_HandlerAction != KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);
        return _RequestEntityLength;
    }

    // GetRequestCertificate
    //
    // Returns the certinfo if the auth scheme is specified as cert.
    // OnSuccess - the certInfo contains the certificate context. This certificate
    // context is freed when the http request goes out of scope.
    // OnFailure - corresponding NTSTATUS.
    //
    NTSTATUS
    GetRequestCertificate(
        __out PHTTP_SSL_CLIENT_CERT_INFO &CcertInfo
        );

    // GetRequestToken
    //
    // Returns the clients token if the auth scheme is kerberos.
    //
    HTTP_AUTH_STATUS
    GetRequestToken(
        __out HANDLE &HToken
        );

    // GetRequestAuthType
    //
    // Returns the authentication type configured on the http server.
    //
    KHttpUtils::HttpAuthType
    GetRequestAuthType()
    {
        return _Parent->_AuthType;
    }

    // GetRemoteAddress
    //
    // Returns the remote IP address associated with this http request.
    //
    NTSTATUS
    GetRemoteAddress(
        __out KString::SPtr& Value
        );

    // GetRequestContent
    //
    // Gets the content uploaded by the client, if any.
    // The type of data can be interpreted by examining the Content-Type header.
    //
    // Note that the Content-Length header may not have been set by the client.
    // In such cases, this contains what was actually received, but its length will have to be
    // validated independently.
    //
    // Parameters:
    //   Content            Receives the internal buffer containing any entity-body content.
    //
    // Return value:
    //  STATUS_SUCCESS
    //  STATUS_INSUFFICIENT_RESOURCES if content was uploaded, but there was not enough memory to capture it.
    //  STATUS_NOT_FOUND if no content was uploaded.
    //
    NTSTATUS
    GetRequestContent(
        __out KBuffer::SPtr& Content
        )
    {
        KInvariant(_HandlerAction != KHttpServer::HeaderHandlerAction::ApplicationMultiPartRead);

        if (_RequestEntityBuffer)
        {
            _RequestEntityBuffer->SetSize(_RequestEntityLength);
            Content = _RequestEntityBuffer;
            return STATUS_SUCCESS;
        }
        return STATUS_NOT_FOUND;
    }

    // SetResponseHeaderByCode
    //
    // Sets a response header value.  This works only with standardized headers
    // and avoids spelling mistakes.
    //
    //
    // Parameters:
    //      HeaderId    The code of the standardized header.
    //      Value       The value of the header. This is copied, so the source
    //                  may immediately go out of scope.
    //
    // Return value:
    //      STATUS_NOT_FOUND if the header value was not a standard one.
    //      STATUS_INSUFFICIENT_RESOURCES
    //      STATUS_SUCCESS
    //
    NTSTATUS
    SetResponseHeaderByCode(
        __in KHttpUtils::HttpHeaderType HeaderId,
        __in KStringView& Value
        );

    // SetResponseHeader
    //
    // Used to set any header, but primarily intended for customized headers.
    // Consider using SetResponseHeaderByCode when the header is a standard one.
    //
    // Parameters:
    //      HeaderId        The header name.
    //      Value           The value of the header. This is copied, so the value
    //                      may immediately go out of scope.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    SetResponseHeader(
        __in KStringView& HeaderId,
        __in KStringView& Value
        );

    // SetResponseContent
    //
    // Sets the content to be transmitted in the response.
    //
    // Note that this api should only be used if the caller intends to use the SendResponse() api
    //       to send the response to the client. Callers that intend to use AsyncSendResponseData async
    //       to send the response in pieces should not use this api.
    //
    // Parameters:
    //      Buffer          The KBuffer containing the content to transmit.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_ALRREADY_COMMITTED if one of the other Set* methods was called first.
    //
    NTSTATUS
    SetResponseContent(
        __in KBuffer::SPtr& Buffer
        )
    {
        _ResponseContentCarrier = KHttpUtils::eKBuffer;
        _ResponseBuffer = Buffer;
        return STATUS_SUCCESS;
    }

    // SetResponseContent (overload)
    //
    // Sets the content to be transmitted. The caller must ensure the memory is reachable until
    // the completion callback has been invoked.
    //
    // Note that this api should only be used if the caller intends to use the SendResponse() api
    //       to send the response to the client. Callers that intend to use AsyncSendResponseData async
    //       to send the response in pieces should not use this api.
    //
    // Parameters:
    //      Mem         The _Param of the KMemRef struct is used to indicate the amount to transmit.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_ALRREADY_COMMITTED if one of the other Set* methods was called first.
    //
    NTSTATUS
    SetResponseContent(
        __in KMemRef& Mem
        )
    {
        _ResponseContentCarrier = KHttpUtils::eKMemRef;
        _ResponseMemRef = Mem;
        return STATUS_SUCCESS;
    }

    // SetResponseContent (overload)
    //
    // Sets the content to be transmitted. The caller must ensure the memory is reachable until
    // the completion callback has been invoked.
    //
    // Note that this api should only be used if the caller intends to use the SendResponse() api
    //       to send the response to the client. Callers that intend to use AsyncSendResponseData async
    //       to send the response in pieces should not use this api.
    //
    // Parameters:
    //      StringContent       A string to be sent back.  Useful for HTML/XML/text responses.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_ALRREADY_COMMITTED if one of the other Set* methods was called first.
    //
    NTSTATUS
    SetResponseContent(
        __in KStringView& StringContent
        )
    {
        _ResponseContentCarrier = KHttpUtils::eKStringView;
        _ResponseStringView = StringContent;
        return STATUS_SUCCESS;
    }



    //  SendResponse
    //
    //  Initiates the sending of the response.  If a callback is supplied, then it will be invoked when sending
    //  is done but before async Completion, which is hidden from the user.
    //
    //  If no callback is supplied, then the model for response is fire-and-forget.  In such cases, only
    //  KBuffer::SPtr should be used if any content is sent back to the client.  If KStringView/KMemRef
    //  is used to deliver content, the callback should be non-null so that the user can know when these
    //  buffers are no longer required.
    //
    //  Parameters:
    //          Code                A HTTP status code.  Usually KHttpUtils::Ok (200) on success
    //          ReasonMessage       In cases where an error is sent back, this is a user-readable message indicating
    //                              the reason the request failed. This is an ANSI string and is immedately copied if used,
    //                              so the source can go out of scope after the call completes.
    //          CompletionCallback  An optional callback that will be invoked when the transmission to the client
    //                              has been completed so that any resources associated with the returned content can be released/closed.
    //
    typedef KDelegate<VOID(__in KSharedPtr<KHttpServerRequest> Request, NTSTATUS FinalStatus)> ResponseCompletion;

    VOID
    SendResponse(
        __in KHttpUtils::HttpResponseCode Code,
        __in LPCSTR ReasonMessage = 0,
        __in ResponseCompletion CompletionCallback = 0
        );

    //
    // This will return any windows error code returned from HTTP apis
    //
    ULONG GetHttpError()
    {
        return(_HttpError);
    }

    //
    // This async is used when the request is received by a KHttpServer
    // that has been passed to the RequestHeaderHandler and that
    // returns ApplicationMultiPartRead as the Action.  In this mode
    // the request data is read and response data sent by repeated calls 
    // using this async. Note that the async can be used
    // with any KHttpServerRequest.
    //
    // This async is used to send a response to a request. It can be
    // used as an alternate to the SendResponse() method. In this mode
    // the server receives data using multiple invocations of StartReceiveRequestData().
    // The server then responds with the headers using
    // StartSendResponseHeaders and then with multiple StartSendResponseContent
    // if there is content to return. The last async that denotes the
    // end of the response should have a flag NOT set to
    // HTTP_SEND_RESPONSE_FLAG_MORE_DATA. That is, if the flag is set
    // then it is expected than another async will be sent. Note that the async can be used
    // with any KHttpServerRequest.
    //
    class AsyncResponseDataContext: public KAsyncContextBase
    {
        friend KHttpServerRequest;

        K_FORCE_SHARED(AsyncResponseDataContext);

        public:
            //
            // Create an AsyncSendResponseContext
            //
            static
            NTSTATUS
            Create(
                __out AsyncResponseDataContext::SPtr& Context,
                __in KAllocator& Allocator,
                __in ULONG AllocationTag
                );

            //
            // Starts an async to receive the request content data
            //
            // Request is the KHttpRequest for which the content is to
            //     be received.
            //
            // MaxBytesToRead is the maximum number of bytes to return
            //
            // Content returns with a buffer containing the content
            //     data received
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartReceiveRequestData(
                __in KHttpServerRequest& Request,
                __in ULONG MaxBytesToRead,
                __out KBuffer::SPtr& Content,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to receive the request content data
            //
            // Request is the KHttpRequest for which the content is to
            //     be received.
            //
            // Mem supplies the address and size of the buffer into which the data will
            //     be read into. Size indicates the max data size that will be attempted to
            //     be read.
            //     On completion, the _Param in the memref will be set to the actual bytes read.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartReceiveRequestData(
                __in KHttpServerRequest& Request,
                __inout KMemRef *Mem,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Starts an async to receive the request certificate
            //
            // Request is the KHttpRequest for which the content is to
            //     be received.
            //
            // Certificate returns with a buffer containing the certificate data
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartReceiveRequestCertificate(
                __in KHttpServerRequest& Request,
                __out KBuffer::SPtr& Certificate,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Start async that sends the response headers. Note that headers are
            // set by using SetResponseHeaders() before starting this async.
            //
            // Request has the KHttpRequest for which the response
            //     headers are to be sent. Actual header information to
            //     send is maintained in the Request object and
            //     configured using SetResponseHeader() and
            //     SetResponseHeaderByCode()
            //
            // Code is a HTTP status code.  Usually KHttpUtils::Ok (200) on success
            //
            // ReasonMessage is in cases where an error is sent back, this is a user-readable message indicating
            //               the reason the request failed. This is an ANSI string and is not copied if used,
            //               so the source can not go out of scope until the completion callback is invoked.
            //
            //  Flags are flags that are passed on to the
            //      HttpSendHttpResponse(). Valid flags are:
            //
            //        HTTP_SEND_RESPONSE_FLAG_DISCONNECT 
            //          The network connection should be disconnected after sending this response,
            //          overriding any persistent connection features
            //          associated with the version of HTTP in use.
            //          Applications should use this flag to indicate
            //          the end of the entity in cases where neither
            //          content length nor chunked encoding is used.
            //
            //        HTTP_SEND_RESPONSE_FLAG_MORE_DATA 
            //          Additional entity body data for this response
            //          is sent by the application through one or more
            //          subsequent calls to HttpSendResponseEntityBody.
            //          The last call then sets this flag to zero.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendResponseHeader(
                __in KHttpServerRequest& Request,
                __in KHttpUtils::HttpResponseCode Code,
                __in_opt LPSTR ReasonMessage,
                __in ULONG Flags,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            //
            // Start async that sends the response content
            //  
            //  Buffer          The KBuffer containing the content to transmit.
            //
            //  Flags are flags that are passed on to the
            //      HttpSendHttpResponse(). Valid flags are:
            //
            //        HTTP_SEND_RESPONSE_FLAG_DISCONNECT 
            //          The network connection should be disconnected after sending this response,
            //          overriding any persistent connection features
            //          associated with the version of HTTP in use.
            //          Applications should use this flag to indicate
            //          the end of the entity in cases where neither
            //          content length nor chunked encoding is used.
            //
            //        HTTP_SEND_RESPONSE_FLAG_MORE_DATA 
            //          Additional entity body data for this response
            //          is sent by the application through one or more
            //          subsequent calls to HttpSendResponseEntityBody.
            //          The last call then sets this flag to zero.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendResponseDataContent(
                __in KHttpServerRequest& Request,
                __in KBuffer::SPtr& Buffer,
                __in ULONG Flags,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);


            //
            // Start async that sends the response content
            //  
            //  Mem         The _Param of the KMemRef struct is used to indicate the amount to transmit.
            //
            //  Flags are flags that are passed on to the
            //      HttpSendHttpResponse(). Valid flags are:
            //
            //        HTTP_SEND_RESPONSE_FLAG_DISCONNECT 
            //          The network connection should be disconnected after sending this response,
            //          overriding any persistent connection features
            //          associated with the version of HTTP in use.
            //          Applications should use this flag to indicate
            //          the end of the entity in cases where neither
            //          content length nor chunked encoding is used.
            //
            //        HTTP_SEND_RESPONSE_FLAG_MORE_DATA 
            //          Additional entity body data for this response
            //          is sent by the application through one or more
            //          subsequent calls to HttpSendResponseEntityBody.
            //          The last call then sets this flag to zero.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendResponseDataContent(
                __in KHttpServerRequest& Request,
                __in KMemRef& Mem,
                __in ULONG Flags,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

            //
            // Start async that sends the response content
            //  
            //  StringContent       A string to be sent back.  Useful for HTML/XML/text responses.
            //
            //  Flags are flags that are passed on to the
            //      HttpSendHttpResponse(). Valid flags are:
            //
            //        HTTP_SEND_RESPONSE_FLAG_DISCONNECT 
            //          The network connection should be disconnected after sending this response,
            //          overriding any persistent connection features
            //          associated with the version of HTTP in use.
            //          Applications should use this flag to indicate
            //          the end of the entity in cases where neither
            //          content length nor chunked encoding is used.
            //
            //        HTTP_SEND_RESPONSE_FLAG_MORE_DATA 
            //          Additional entity body data for this response
            //          is sent by the application through one or more
            //          subsequent calls to HttpSendResponseEntityBody.
            //          The last call then sets this flag to zero.
            //
            //  ParentAsyncContext - Supplies an optional parent async context.
            //
            //  CallbackPtr - Supplies an optional callback delegate to be notified
            //      when a notification event occurs
            //
            VOID
            StartSendResponseDataContent(
                __in KHttpServerRequest& Request,
                __in KStringView& StringContent,
                __in ULONG Flags,
                __in_opt KAsyncContextBase* const ParentAsyncContext,
                __in_opt KAsyncContextBase::CompletionCallback CallbackPtr);

        private:
            enum { Unassigned,
                    ReceiveRequestData,
                    ReceiveRequestCertificate,
                    SendResponseHeader,
                    SendResponseData, 
                    Completed, CompletedWithError } _State;

            VOID
            Initialize(
                );

            VOID 
            ReceiveRequestDataFSM(
                __in NTSTATUS Status
                );

            VOID 
            ReceiveRequestCertificateFSM(
                __in NTSTATUS Status
                );

            VOID 
            SendResponseDataFSM(
                __in NTSTATUS Status
                );

            VOID 
            SendResponseHeaderFSM(
                __in NTSTATUS Status
                );

            NTSTATUS
            GetReceiveBuffer(__out KMemRef& Mem);

        protected:
            VOID
            OnStart(
                ) override ;

            VOID
            OnReuse(
                ) override ;

            VOID
            OnCompleted(
                ) override ;

            VOID
            OnCancel(
                ) override ;

        private:

            //
            // General members
            //
                
            //
            // Parameters to api
            //
            KHttpServerRequest::SPtr _Request;

            KHttpUtils::ContentCarrierType _ContentCarrier;

            //
            // ReceiveRequestData
            //
            ULONG _MaxBytesToRead;
            KBuffer::SPtr* _ReadContent;
            KMemRef* _ReadContent_KMemRef;

            //
            // ReceiveRequestCertificate
            //
            KBuffer::SPtr* _Certificate;            

            // SendResponseHeaders
            KHttpUtils::HttpResponseCode _Code;
            LPSTR _ReasonMessage;
            ULONG _Flags;

            // SendResponseData
            KMemRef _Content_KMemRef;
            KBuffer::SPtr _Content_KBuffer;
            KStringView _Content_KStringView;

            //
            // Members needed for functionality
            //
            KBuffer::SPtr _ReadBuffer;
    };
    
protected:

    static NTSTATUS
    Create(
        __in   KSharedPtr<KHttpServer> Parent,
        __in   KAllocator& Allocator,
        __out  KHttpServerRequest::SPtr& NewRequest
        );

    BOOLEAN
    IsUpgraded()
    {
        return _IsUpgraded;
    }

    virtual VOID
    OnStart() override;

    NTSTATUS
    FormatResponse(
        __in KHttpUtils::HttpResponseCode Code,
        __in LPCSTR ReasonMessage
        );

    NTSTATUS
    FormatResponseData(
        __in KHttpUtils::ContentCarrierType ResponseContentCarrier,
        __in KBuffer::SPtr ResponseBuffer,
        __in KMemRef ResponseMemRef,
        __in KStringView ResponseStringView,
        __out USHORT& ChunkCount
        );

    NTSTATUS
    DeliverResponse(
    );

    NTSTATUS
    SendResponseData(
        __in KHttpUtils::ContentCarrierType ResponseContentCarrier,
        __in KBuffer::SPtr ResponseBuffer,
        __in KMemRef ResponseMemRef,
        __in KStringView ResponseStringView,
        __in ULONG Flags
    );

    VOID
    FailRequest(
        __in NTSTATUS Status,
        __in KHttpUtils::HttpResponseCode Code
    );

    NTSTATUS
    StartRead(
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in KAsyncContextBase::CompletionCallback Completion = 0
        );

    VOID
    ReadCompletion(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
        );

    NTSTATUS
    ReadHeaders(HTTP_OPAQUE_ID RequestId);

    NTSTATUS
    ReadEntityBody();

    NTSTATUS
    ReadCertificate();

    NTSTATUS
    ReadRequestData(
        __in KMemRef& Mem
    );

    VOID
    IOCP_Completion_Impl(
        __in ULONG Error,
        __in ULONG BytesTransferred
        );

    BOOL
    IsAuthTypeSupported(KHttpUtils::HttpAuthType authType)
    {
        return _Parent->_AuthType & authType;
    }

    VOID
    AnalyzeResponseHeaders(
        __out ULONG& AnsiBufSize,
        __out ULONG& UnknownHeaderCount
        );

    struct HeaderPair : KShared<HeaderPair>, KObject<HeaderPair>
    {
        typedef KSharedPtr<HeaderPair> SPtr;

        HeaderPair()
            : _Id(GetThisAllocator()), _Value(GetThisAllocator())
            {
                _HeaderCode = KHttpUtils::HttpHeader_INVALID;
            }

        KDynString _Id;
        ULONG      _HeaderCode;
        KDynString _Value;
    };

    // Data members
    //
    KSharedPtr<KHttpServer>            _Parent;
    enum { eWaiting = 0, eReadHeaders, eHeaderHandlerInvoked, eReadBody, eReadCertificate, eReadMultiPart, eReadComplete, 
           eSendHeaders, eSendResponseData, eSendResponse } eState;

    KHttpServer::HeaderHandlerAction _HandlerAction;
    AsyncResponseDataContext::SPtr _ASDRContext;
    ULONG _HttpError;
    KHttp_IOCP_Handler::Overlapped _Overlapped;
    BOOLEAN _IsUpgraded;  //  Whether or not this request has been upgraded to a WebSocket

    // Request related
    //
    KBuffer::SPtr                      _RequestStructBuffer;
    KBuffer::SPtr                      _RequestEntityBuffer;
    ULONG                              _RequestEntityLength;
    ULONG                              _RequestContentLengthHeader;
    PHTTP_REQUEST_V2                   _PHttpRequest;
    KUri::SPtr                         _RelativeUrl;
    KBuffer::SPtr                      _ClientCertificateInfoBuffer;
    HANDLE                             _hAccessToken;

    // Response Related
    //
    KArray<HeaderPair::SPtr>           _ResponseHeaders;
    KBuffer::SPtr                      _ResponseHeaderBuffer;
    KHttpUtils::HttpResponseCode       _ResponseCode;

    BOOLEAN                            _ResponseHeadersSent;

    KHttpUtils::ContentCarrierType     _ResponseContentCarrier;
    KBuffer::SPtr                      _ResponseBuffer;
    KMemRef                            _ResponseMemRef;
    KStringView                        _ResponseStringView;
    ULONG                              _ResponseContentLengthHeader;

    ULONG                              _HttpResponseFlags;
    HTTP_RESPONSE                      _HttpResponse;
    HTTP_DATA_CHUNK                    _ResponseChunk;
    PHTTP_UNKNOWN_HEADER               _ResponsePUnknownHeaders;
    ResponseCompletion                 _ResponseCompleteCallback;
    LPSTR                              _ResponseReason;
};

#else
class KHttpServer : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServer);
public:
};
#endif  //  #if KTL_USER_MODE


#if NTDDI_VERSION >= NTDDI_WIN8 && KTL_USER_MODE

class KHttpServerWebSocket;
class KHttpClientWebSocket;

//
//  Base class for KHttpServerWebSocket and KHttpClientWebSocket
//
class KHttpWebSocket : public KWebSocket
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpWebSocket);
    friend class KHttpServerWebSocket;
    friend class KHttpClientWebSocket;

public:

    KStringViewA
    GetActiveSubProtocol() override;

    VOID
    Abort() override;

protected:

    //
    //  Transition to Error state and clean up the WebSocket's resources
    //
    VOID
    FailWebSocket(
        __in NTSTATUS Status = STATUS_UNSUCCESSFUL,
        __in_opt ULONGLONG D1 = 0,
        __in_opt ULONGLONG D2 = 0
        );

    //
    //  Clean up resources held by the WebSocket
    //
    virtual VOID
    CleanupWebSocket(
        __in NTSTATUS Status
        ) = 0;

    //
    //  Handle a receive request dropped from the receive queue (due to the queue being shut down)
    //
    VOID
    HandleDroppedReceiveRequest(
        __in KAsyncQueue<WebSocketReceiveOperation>&,
        __in WebSocketReceiveOperation& DroppedItem
        );

    //
    //  Handle a send request dropped from the send queue (due to the queue being shut down)
    //
    VOID
    HandleDroppedSendRequest(
        __in KAsyncQueue<WebSocketOperation>& DeactivatingQueue,
        __in WebSocketOperation& DroppedItem
        );

    //
    //  Called when the receive processor completes due to receiving a CLOSE frame.
    //
    VOID
    ReceiveProcessorCompleted();

    //
    //  Called when the send processor completes due to sending a CLOSE frame.
    //
    VOID
    SendChannelCompleted();

    //
    //  Allocate the Open and/or Close handshake timers if appropriate
    //
    NTSTATUS
    InitializeHandshakeTimers();

    VOID
    OpenTimeoutTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    CloseTimeoutTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

private:

    VOID
    AbortAsyncCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    //
    //  Transition to the Closed state, complete the close procedure, and clean up the WebSocket.
    //  Will only called once, either from SendChannelCompleted() or ReceiveProcessorCompleted(),
    //  depending on whether the send or receive channel completes first, respectively.
    //
    VOID
    DoClose();

protected:

    KHttpWebSocket(
        __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveRequestQueue,
        __in KAsyncQueue<WebSocketOperation>& SendRequestQueue,
        __in KHttpUtils::CallbackAsync& AbortAsync,
        __in KStringA& ActiveSubprotocol
        );

protected:

    BOOLEAN _IsReceiveChannelClosed;
    BOOLEAN _IsSendChannelClosed;
    KStringA::SPtr _ActiveSubprotocol;

    //
    //  Allocated on-demand during the open handshake, if the corresponding timing constants were set
    //
    KTimer::SPtr _OpenTimeoutTimer;  //  Used by the WebSocket to time the Open procedure
    KTimer::SPtr _CloseTimeoutTimer;  //  Used by the WebSocket to time the Close procedure

    //
    //  Set when the WebSocket fails, and is returned as the status for post-failure service completion
    //
    NTSTATUS _FailureStatus;

private:

    KHttpUtils::CallbackAsync::SPtr _AbortAsync;
    volatile ULONG _IsAbortStarted;
};



//
//  Client-side derivation of KWebSocket, which uses the WinHTTP WebSocket API
//
class KHttpClientWebSocket : public KHttpWebSocket
{
    K_FORCE_SHARED(KHttpClientWebSocket);

public:

    //
    //  Create a KHttpClientWebSocket instance
    //
    static NTSTATUS
    Create(
        __out KHttpClientWebSocket::SPtr& WebSocket,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_WEB_SOCKET
        );

    // ServerCertValidationHandler
    // Handler that verifies the remote Secure Sockets Layer (SSL) certificate used for authentication. 
    //
    // Parameter:
    // PCCERT_CONTEXT : Pointer to the CERT_CONTEXT structure.
    // KTL will take care of freeing the PCCERT_CONTEXT once the validation callback has executed.
    // Returns:
    // TRUE: Server certificate validation succeeded. Proceed with the request.
    // FALSE: Server certificate validation failed. Fail the request.
    //
    typedef KDelegate<BOOLEAN(__in PCCERT_CONTEXT pCertContext)> ServerCertValidationHandler;

    //
    //  Initiate and complete the WebSocket handshake.
    //
    //  Parameters:
    //      TargetURI – The target host to connect to, e.g. wss://localhost:443/resource
    //
    //      CloseReceivedCallback – A callback to be invoked whenever the WebSocket receives a
    //          CLOSE frame.  A typical impelementation of this callback will be to halt sending
    //          any further data and call StartCloseWebSocket to complete the close procedure.
    //
    //      TransportReceiveBufferSize – An optional size (in bytes) of the transport receive
    //          buffer.  Must be at least the maximum control frame payload size (125).  Defaults
    //          to the value used by KHttpServerWebSocket (4096).
    //
    //      RequestedSubProtocolsString – An optional comma-separated list of subprotocol names 
    //          in order of preference.  The server will select one or zero of these.  
    //          GetActiveSubProtocol() may be called after the Open procedure completes successfully
    //          to query which subprotocol was selected.
    //
    //      Proxy – An optional name of the proxy server to use when making the WebSocket 
    //          connection.  The client can pass nullptr to use the default proxy if one is
    //          configured, or an empty string to specify that no proxy should be used.
    //
    //      AdditionalHeaders – An optional list of HTTP headers to be passed with the client
    //          handshake request.  Client authentication headers, for example, could be
    //          specified here.
    //
    //      CertContext - The client certificate context.
    //          As part of this, KTL will call CertDuplicateCertificateContext on the provided certificate context.
    //          So this certificate context must be independently released by the caller.
    //
    //      Handler - ServerCertValidationHandler
    //          Sets a handler/callback function to validate the server certificate.
    //
    //      AllowRedirects - Allow/disable automatic redirection when sending requests with WinHttpSendRequest
    //
    //      EnableCookies - Automatic addition of cookie headers to requests
    NTSTATUS
    StartOpenWebSocket(
        __in KUri& TargetURI,
        __in WebSocketCloseReceivedCallback CloseReceivedCallback,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncServiceBase::OpenCompletionCallback CallbackPtr,
        __in_opt ULONG TransportReceiveBufferSize = 4096L,
        __in_opt KStringA::SPtr RequestedSubProtocols = nullptr,
        __in_opt KString::SPtr Proxy = nullptr,
        __in_opt KHttpHeaderList* const AdditionalHeaders = nullptr,
        __in_opt KString::SPtr const &AdditionalHeadersBlob = nullptr,
        __in_opt PCCERT_CONTEXT CertContext = nullptr,
        __in_opt ServerCertValidationHandler Handler = nullptr,
        __in_opt BOOLEAN AllowRedirects = TRUE,
        __in_opt BOOLEAN EnableCookies = TRUE,
        __in_opt BOOLEAN EnableAutoLogonForAllRequests = FALSE
        );

    //
    //  WinHttp does not support/expose PING or PONG frames directly, so the Ping Quiet Channel and
    //  Pong Timeout timing constants are unsupported.
    //
    NTSTATUS
    SetTimingConstant(
        __in TimingConstant Constant,
        __in TimingConstantValue Value
        ) override;

    NTSTATUS
    CreateSendFragmentOperation(
        __out SendFragmentOperation::SPtr& SendOperation
        ) override;

    NTSTATUS
    CreateSendMessageOperation(
        __out SendMessageOperation::SPtr& SendOperation
        ) override;

    //
    //  In the case of failure due to a WinHttp error, will return the WinHttp-returned error code
    //
    ULONG
    GetWinhttpError();

    // GetHttpResponseCode
    //
    // Returns the HTTP response code for this request.
    //
    // Note that 200 (HttpResponseCode enumeration) is the success code.
    //
    ULONG
    GetHttpResponseCode()
    {
        return _HttpStatusCode;
    }

    // GetHttpResponseHeader
    //
    // Retrieves a specific header.  This can be used for both
    // standardized and custom headers.
    //
    // Parameters:
    //      HeaderName      The HTTP header name.
    //      Value           Receives the header string.
    //
    // Return values:
    //      STATUS_SUCCESS
    //      STATUS_OBJECT_NAME_NOT_FOUND
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    GetHttpResponseHeader(
        __in  LPCWSTR  HeaderName,
        __out KString::SPtr& Value
    );

private:

    class KHttpClientSendFragmentOperation;
    class KHttpClientSendMessageOperation;

    //
    //  Send the close frame by calling WinHttpWebSocketShutdown
    //
    class KHttpClientSendCloseOperation : public KAsyncContextBase, public WebSocketSendCloseOperation
    {
        K_FORCE_SHARED(KHttpClientSendCloseOperation);
        friend class KHttpClientWebSocket;

    public:

        KDeclareDefaultCreate(KHttpClientSendCloseOperation, SendCloseOperation, KTL_TAG_WEB_SOCKET);

        VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) override;

        ULONG
        GetWinhttpError();

        VOID
        StartSendClose(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HINTERNET WinhttpWebSocketHandle,
            __in CloseStatusCode const CloseStatusCode,
            __in_opt KSharedBufferStringA::SPtr CloseReason
            );

        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    private:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    private:

        BOOLEAN _IsActive;
        ULONG _WinhttpError;
    };

    //
    //  Wrap a call to WinhttpWebSocketSend
    //
    class WinhttpWebSocketSendOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED(WinhttpWebSocketSendOperation);
        friend class KHttpClientWebSocket;

    public:

        KDeclareDefaultCreate(WinhttpWebSocketSendOperation, WebSocketSendOperation, KTL_TAG_WEB_SOCKET);

        VOID
        StartSend(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HINTERNET WinhttpWebSocketHandle,
            __in WINHTTP_WEB_SOCKET_BUFFER_TYPE BufferType,
            __in_bcount(BufferLength) PVOID Buffer,
            __in ULONG BufferLength
            );

        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

        ULONG
        GetWinhttpError();

        ULONG
        GetBytesSent();

    private:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    private:

        ULONG _BytesSent;
        BOOLEAN _IsActive;
        ULONG _WinhttpError;
    };

    //
    //  Wrap a call to WinhttpWebSocketReceive
    //
    class WinhttpWebSocketReceiveOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED(WinhttpWebSocketReceiveOperation);
        friend class KHttpClientWebSocket;

    public:

        KDeclareDefaultCreate(WinhttpWebSocketReceiveOperation, WebSocketReceiveOperation, KTL_TAG_WEB_SOCKET);

        VOID
        StartReceive(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HINTERNET WinhttpWebSocketHandle,
            __out_bcount(BufferLength) PVOID Buffer,
            __in ULONG BufferLength
            );

        ULONG
        GetBytesReceived();

        FrameType
        GetFrameType();

        ULONG
        GetWinhttpError();

    private:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    private:

        FrameType _FrameType;
        ULONG _BytesReceived;

        ULONG _WinhttpError;
    };

    //
    //  Begin the client handshake by sending the http request
    //
    class WinhttpSendRequestOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED(WinhttpSendRequestOperation);
        friend class KHttpClientWebSocket;

    public:

        KDeclareDefaultCreate(WinhttpSendRequestOperation, SendRequestOperation, KTL_TAG_WEB_SOCKET);

        VOID
        StartSendRequest(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HINTERNET RequestHandle
            );

        ULONG
        GetWinhttpError();

        //
        //  Whether or not this async is active, as observed by the parent web socket.  Set to true when StartSend(...) is called,
        //  and set to FALSE once the callback is received by the WebSocket.
        //
        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    private:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    private:

        BOOLEAN _IsActive;
        ULONG _WinhttpError;
    };

    //
    //  Complete the client handshake by receiving the http response
    //
    class WinhttpReceiveResponseOperation : public KAsyncContextBase
    {
        K_FORCE_SHARED(WinhttpReceiveResponseOperation);
        friend class KHttpClientWebSocket;

    public:

        KDeclareDefaultCreate(WinhttpReceiveResponseOperation, ReceiveResponseOperation, KTL_TAG_WEB_SOCKET);

        VOID
        StartReceiveResponse(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HINTERNET RequestHandle
            );

        ULONG
        GetWinhttpError();

        //
        //  Whether or not this async is active, as observed by the parent web socket.  Set to true when StartReceive(...) is called,
        //  and set to FALSE once the callback is received by the WebSocket.
        //
        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    private:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    private:

        BOOLEAN _IsActive;
        ULONG _WinhttpError;
    };

    //
    //  Parses incoming data and feeds it to user requests
    //
    class ReceiveOperationProcessor
    {
        K_DENY_COPY(ReceiveOperationProcessor);
        friend class KHttpClientWebSocket;

    public:

        ReceiveOperationProcessor(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& RequestDequeueOperation,
            __in WinhttpWebSocketReceiveOperation& WebSocketReceiveOperation
            );

        VOID
        FSM();

    private:

        VOID
        RequestDequeueCallback(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            );

        VOID
        WebSocketReceiveCallback(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            );

    private:

        //
        //  Set at construct-time
        //
        KHttpClientWebSocket& _ParentWebSocket;
        KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation::SPtr _RequestDequeueOperation;
        WinhttpWebSocketReceiveOperation::SPtr _WebSocketReceiveOperation;

        KBuffer::SPtr _TransportReceiveBuffer;
        ULONG _DataOffset;
        KUniquePtr<WebSocketReceiveOperation, FailFastIfNonNullDeleter<WebSocketReceiveOperation>> _CurrentOperation;
        MessageContentType _ReceivedContentType;
        BOOLEAN _ReceivedFragmentIsFinal;
        ULONG _BytesReceived;
    };

    //
    //  Execute winhttp open logic (which is synchronous) on a work item thread.
    //
    //  Failure and cleanup (e.g. due to Abort()) may happen concurrently with this async,
    //  so the cleanup and this async's completion must coordinate the releasing of some
    //  resources (e.g. the Winhttp handles)
    //
    class DeferredServiceOpenAsync : public KAsyncContextBase
    {
        K_FORCE_SHARED(DeferredServiceOpenAsync);

    public:

        KDeclareDefaultCreate(DeferredServiceOpenAsync, Async, KTL_TAG_WEB_SOCKET);

        VOID
        StartDeferredServiceOpen(
            __in KHttpClientWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback OpenCallbackPtr
            );

        ULONG
        GetWinhttpError();

        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    private:

        VOID
        DeferredServiceOpen();

        VOID
        OnStart() override;

        VOID
        OnCompleted() override;

    private:

        //
        //  Set at construct-time
        //
        KDeferredCall<VOID(VOID)> _DeferredServiceOpen;
        ULONG _WinhttpError;
        BOOLEAN _IsActive;

        //
        //  Set in StartDeferredServiceOpen
        //
        KHttpClientWebSocket::SPtr _ParentWebSocket;
        KAsyncContextBase::CompletionCallback _OpenCallbackPtr;
    };

protected:

    using KAsyncContextBase::AcquireActivities;

private:

    KHttpClientWebSocket(
        __in KAsyncQueue<WebSocketReceiveOperation>& ReceiveOperationQueue,
        __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& ReceiveRequestDequeueOperation,
        __in KAsyncQueue<WebSocketOperation>& SendOperationQueue,
        __in KAsyncQueue<WebSocketOperation>::DequeueOperation& SendRequestDequeueOperation,
        __in WinhttpWebSocketReceiveOperation& WebSocketReceiveOperation,
        __in DeferredServiceOpenAsync& ServiceOpenAsync,
        __in WinhttpWebSocketSendOperation& WebSocketSendOperation,
        __in WinhttpSendRequestOperation& SendRequestOperation,
        __in WinhttpReceiveResponseOperation& ReceiveResponseOperation,
        __in KHttpClientSendCloseOperation& SendCloseOperation,
        __in KHttpUtils::CallbackAsync& AbortAsync,
        __in KStringA& ActiveSubprotocol,
        __in KBuffer& RemoteCloseReasonBuffer
        );

    VOID
    OnServiceOpen() override;

    VOID
    DeferredServiceOpen();

    VOID
    OnServiceClose() override;

    static NTSTATUS
    CreateRequest(
        __out HINTERNET& WinhttpRequestHandle,
        __out ULONG& WinhttpError,
        __in HINTERNET WinhttpConnectionHandle,
        __in KHttpUtils::OperationType OperationType,
        __in KUriView Uri,
        __in_opt KStringA::SPtr RequestedSubprotocols,
        __in_opt KArray<KHttpCliRequest::HeaderPair::SPtr>* Headers,
        __in_opt KString::SPtr& HeadersBlob,
        __in_opt PCCERT_CONTEXT CertContext = nullptr,
        __in_opt ServerCertValidationHandler Handler = nullptr,
        __in_opt BOOLEAN AllowRedirects = TRUE,
        __in_opt BOOLEAN EnableCookies = TRUE,
        __in_opt BOOLEAN EnableAutoLogonForAllRequests = FALSE
        );

    static WINHTTP_WEB_SOCKET_BUFFER_TYPE
    GetWebSocketBufferType(
        __in MessageContentType const & ContentType,
        __in BOOLEAN const & IsFinalFragment
        );

    static FrameType
    WebSocketBufferTypeToFrameType(
        __in WINHTTP_WEB_SOCKET_BUFFER_TYPE BufferType
        );

    VOID
    CleanupWebSocket(
        __in NTSTATUS Status
        ) override;

    //
    //  Close the WinHttp handles.  Executed only once either during cleanup
    //  or by the deferred service open's completion (if cleanup occurred concurrently with 
    //  the deferred service open).
    //
    VOID
    CloseHandles();

    BOOLEAN
    ValidateServerCertificate();


    static VOID CALLBACK
    WinHttpCallback(
        __in  HINTERNET hInternet,
        __in  ULONG_PTR dwContext,
        __in  ULONG dwInternetStatus,
        __in  LPVOID lpvStatusInformation,
        __in  ULONG dwStatusInformationLength
        );

    VOID
    SendRequestDequeueCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    WebSocketSendCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    SendHttpRequestCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    ReceiveHttpResponseCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    SendCloseCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    DeferredServiceOpenCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

private:

    //
    //  Set at construct-time
    //
    ReceiveOperationProcessor _ReceiveOperationProcessor;
    DeferredServiceOpenAsync::SPtr _DeferredServiceOpenAsync;
    WinhttpSendRequestOperation::SPtr _SendRequestOperation;
    WinhttpReceiveResponseOperation::SPtr _ReceiveResponseOperation;
    KAsyncQueue<WebSocketOperation>::DequeueOperation::SPtr _SendRequestDequeueOperation;
    KBuffer::SPtr _RemoteCloseReasonBuffer;
    WinhttpWebSocketSendOperation::SPtr _WebSocketSendOperation;
    KHttpClientSendCloseOperation::SPtr _SendCloseOperation;

    //
    //  Set during StartOpenWebSocket()
    //
    KUri::SPtr _TargetURI;
    ULONG _TransportReceiveBufferSize;
    KStringA::SPtr _RequestedSubprotocols;
    KString::SPtr _Proxy;
    KHttpHeaderList::SPtr _AdditionalHeaders;
    KString::SPtr _AdditionalHeadersBlob;
    BOOLEAN _Secure;
    WebSocketCloseReceivedCallback _CloseReceivedCallback;
    PCCERT_CONTEXT _ClientCert;
    ServerCertValidationHandler _ServerCertValidationHandler;
    BOOLEAN _AllowRedirects;
    BOOLEAN _EnableCookies;
    BOOLEAN _EnableAutoLogonForAllRequests;

    //
    //  Winhttp Handles (set during service open)
    //
    HINTERNET _WinhttpWebSocketHandle;
    HINTERNET _WinhttpRequestHandle;
    HINTERNET _WinhttpConnectionHandle;
    HINTERNET _WinhttpSessionHandle;

    //
    //  Set to TRUE when the respective queues are deactivated.  Must be checked within the apartment before calling
    //  Deactivate() to prevent multiple calls to Deactivate()
    //
    BOOLEAN _IsReceiveRequestQueueDeactivated;
    BOOLEAN _IsSendRequestQueueDeactivated;

    KUniquePtr<WebSocketSendFragmentOperation, FailFastIfNonNullDeleter<WebSocketSendFragmentOperation>> _CurrentSendOperation;

    ULONG _WinhttpError;
    ULONG _HttpStatusCode;
};


//
//  Server-side derivation of KWebSocket, which uses HTTP Server API (http.sys)
//
class KHttpServerWebSocket : public KHttpWebSocket
{
    K_FORCE_SHARED_WITH_INHERITANCE(KHttpServerWebSocket);

public:

    //
    //  Create a KHttpServerWebSocket instance
    //
    static NTSTATUS
    Create(
        __out KHttpServerWebSocket::SPtr& WebSocket,
        __in KAllocator& Allocator,
        __in ULONG AllocationTag = KTL_TAG_WEB_SOCKET
        );

    //
    //  Upgrade the HTTP session underlying the given request to a WebSocket session.  Calling
    //  this function irrevocably binds the underlying session to a WebSocket; further
    //  attempts to use the given request should be expected to fail.
    //
    //  Parameters:
    //      ServerRequest – The KHttpServerRequest to be upgraded into a WebSocket session
    //
    //      CloseReceivedCallback – A callback to be invoked whenever the WebSocket receives a
    //          CLOSE frame.  A typical impelementation of this callback will be to halt sending
    //          any further data and call StartCloseWebSocket to complete the close procedure.
    //
    //      TransportReceiveBufferSize – An optional size (in bytes) of the transport receive
    //          buffer.  Must be at least 256 bytes.  Default is that specified by the
    //          WebSocket Protocol Component API (4096). 
    //
    //      TransportSendBufferSize – An optional size (in bytes) of the transport send
    //          buffer.  Must be at least 256 bytes.  Default is that specified by the
    //          WebSocket Protocol Component API (4096). 
    //
    //      SupportedSubProtocolsString – An optional comma-separated list of subprotocol names in order of preference.
    //          The first of which (if any) that matches any subprotocol sent in the client’s
    //          handshake will be selected.  GetActiveSubProtocol() may be called after 
    //          the Open procedure completes successfully to query which subprotocol was
    //          selected.
    //
    NTSTATUS
    StartOpenWebSocket(
        __in KHttpServerRequest& ServerRequest,
        __in WebSocketCloseReceivedCallback CloseReceivedCallback,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncServiceBase::OpenCompletionCallback CallbackPtr,
        __in_opt ULONG TransportReceiveBufferSize = 4096L,
        __in_opt ULONG TransportSendBufferSize = 4096L,
        __in_opt KStringA::SPtr SupportedSubProtocolsString = nullptr
        );

    NTSTATUS
    CreateSendFragmentOperation(
        __out SendFragmentOperation::SPtr& SendOperation
        ) override;

    NTSTATUS
    CreateSendMessageOperation(
        __out SendMessageOperation::SPtr& SendOperation
        ) override;

protected:

    class SendFragmentOperationImpl;
    class SendMessageOperationImpl;

    class KHttpServerSendFragmentOperation;
    class KHttpServerSendMessageOperation;

    //
    //  Asynchronously send the http response (server handshake)
    //
    class HttpSendResponseOperation : public KAsyncContextBase, public KHttp_IOCP_Handler
    {
        K_FORCE_SHARED_WITH_INHERITANCE(HttpSendResponseOperation);

    public:

        KDeclareDefaultCreate(HttpSendResponseOperation, SendResponseOperation, KTL_TAG_WEB_SOCKET);

        virtual VOID
        StartSend(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE const & RequestQueueHandle,
            __in HTTP_REQUEST_ID const & HttpRequestId,
            __in HTTP_RESPONSE& HttpResponse
            );

        //
        //  IOCP completion callback for sending the response
        //
        VOID
        IOCP_Completion(
            __in ULONG Error,
            __in ULONG BytesTransferred
            ) override;

    private:

        VOID
        OnStart() override;

    private:

        KHttp_IOCP_Handler::Overlapped _Overlapped;
    };

    //
    //  Asynchronously receive a partial http entity body (a portion of the WebSocket stream)
    //
    class HttpReceiveEntityBodyOperation : public KAsyncContextBase, public KHttp_IOCP_Handler
    {
        K_FORCE_SHARED_WITH_INHERITANCE(HttpReceiveEntityBodyOperation);

    public:

        KDeclareDefaultCreate(HttpReceiveEntityBodyOperation, ReceiveEntityBodyOperation, KTL_TAG_WEB_SOCKET);

        virtual VOID
        StartReceive(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE const & RequestQueueHandle,
            __in HTTP_REQUEST_ID const & HttpRequestId,
            __out_bcount(BufferSizeInBytes) PBYTE const & Buffer,
            __in ULONG const & BufferSizeInBytes,
            __in PVOID const & ActionContext
            );

        //
        //  IOCP completion callback for websocket receives
        //
        VOID
        IOCP_Completion(
            __in ULONG Error,
            __in ULONG BytesTransferred
            ) override;

        ULONG
        GetBytesReceived();

        PVOID
        GetActionContext();

        VOID
        ClearActionContext();

        //
        //  Whether or not this async is active, as observed by the parent web socket.  Set to true when StartReceive(...) is called,
        //  and set to FALSE once the callback is received by the WebSocket.
        //
        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    protected:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    protected:

        ULONG _BytesReceived;
        PVOID _ActionContext;
        BOOLEAN _IsActive;

        KHttp_IOCP_Handler::Overlapped _Overlapped;
    };

    //
    //  Parses incoming data and feeds it to user requests
    //
    class ReceiveOperationProcessor
    {
        K_DENY_COPY(ReceiveOperationProcessor);
        friend class KHttpServerWebSocket;

    public:

        ReceiveOperationProcessor(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& RequestDequeueOperation,
            __in HttpReceiveEntityBodyOperation& TransportReceiveOperation
            );

        VOID
        FSM();

    private:

        VOID
        RequestDequeueCallback(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            );

        VOID
        TransportReceiveCallback(
            __in_opt KAsyncContextBase* const Parent,
            __in KAsyncContextBase& CompletingOperation
            );

        //
        //  Given a WEB_SOCKET_BUFFER_TYPE, get the corresponding FrameType enum value
        //
        static FrameType
        WebSocketBufferTypeToFragmentType(
            __in WEB_SOCKET_BUFFER_TYPE const & WebSocketBufferType
            );

    private:

        //
        //  Set at construct time
        //
        KHttpServerWebSocket& _ParentWebSocket;
        KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation::SPtr _RequestDequeueOperation;
        HttpReceiveEntityBodyOperation::SPtr _TransportReceiveOperation;

        //
        //  WebSocket Protocol Component API variables
        //
        WEB_SOCKET_BUFFER _Buffer;
        ULONG _BufferCount;
        WEB_SOCKET_BUFFER_TYPE _BufferType;
        WEB_SOCKET_ACTION _Action;
        PVOID _ActionContext;
        ULONG _BytesReceived;

        ULONG _DataOffset;
        FrameType _CurrentFrameType;
        KUniquePtr<WebSocketReceiveOperation, FailFastIfNonNullDeleter<WebSocketReceiveOperation>> _CurrentOperation;
    };

    //
    //  Asynchronously send a partial http entity body (a portion of the WebSocket stream)
    //
    class HttpSendEntityBodyOperation : public KAsyncContextBase, public KHttp_IOCP_Handler
    {
        K_FORCE_SHARED_WITH_INHERITANCE(HttpSendEntityBodyOperation);

    public:

        KDeclareDefaultCreate(HttpSendEntityBodyOperation, SendEntityBodyOperation, KTL_TAG_WEB_SOCKET);

        virtual VOID
        StartSend(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in HANDLE RequestQueueHandle,
            __in HTTP_REQUEST_ID HttpRequestId,
            __in_bcount(BufferSizeInBytes) PBYTE Buffer,
            __in ULONG BufferSizeInBytes,
            __in PVOID ActionContext
            );

        ULONG
        GetBufferSizeInBytes();

        ULONG
        GetBytesSent();

        PVOID
        GetActionContext();

        VOID
        ClearActionContext();

        //
        //  IOCP completion callback for websocket sends
        //
        VOID
        IOCP_Completion(
            __in ULONG Error,
            __in ULONG BytesTransferred
            ) override;

        //
        //  Whether or not this async is active, as observed by the parent web socket.  Set to true when StartSend(...) is called,
        //  and set to FALSE once the callback is received by the WebSocket.
        //
        BOOLEAN
        IsActive();

        VOID
        SetActive(
            __in BOOLEAN IsActive
            );

    protected:

        VOID
        OnStart() override;

        VOID
        OnReuse() override;

    protected:

        //
        //  Set/observable externally
        //
        ULONG _BytesSent;
        PVOID _ActionContext;
        BOOLEAN _IsActive;

        HTTP_DATA_CHUNK _Chunk;
        KHttp_IOCP_Handler::Overlapped _Overlapped;
    };

    //
    //  Send the close frame
    //
    class KHttpServerSendCloseOperation : public KAsyncContextBase, public WebSocketSendCloseOperation 
    {
        K_FORCE_SHARED(KHttpServerSendCloseOperation);
        friend class KHttpServerWebSocket;

    public:

        KDeclareDefaultCreate(KHttpServerSendCloseOperation, SendCloseOperation, KTL_TAG_WEB_SOCKET);

        VOID
        StartSendClose(
            __in KHttpServerWebSocket& ParentWebSocket,
            __in KAsyncContextBase::CompletionCallback CallbackPtr,
            __in WEB_SOCKET_HANDLE WebSocketHandle
            );

        VOID
        CompleteOperation(
            __in NTSTATUS Status
            ) override;

    private:

        VOID
        OnStart() override;

        WEB_SOCKET_BUFFER _WebSocketBuffer;
    };

    //
    //  Asynchronously close (gracefully or rudely) the transport in a fire-and-forget fashion
    //
    class CloseTransportOperation : public KAsyncContextBase, public KHttp_IOCP_Handler
    {
        K_FORCE_SHARED_WITH_INHERITANCE(CloseTransportOperation);

    public:

        KDeclareDefaultCreate(CloseTransportOperation, CloseOperation, KTL_TAG_WEB_SOCKET);

        virtual VOID
        StartCloseTransport(
            __in KHttpServer& ParentServer,
            __in BOOLEAN IsGracefulClose,
            __in HANDLE RequestQueueHandle,
            __in HTTP_REQUEST_ID RequestId
            );

    private:

        VOID
        OnStart() override;

        VOID
        IOCP_Completion(
            __in ULONG Error,
            __in ULONG BytesTransferred
            ) override;

    private:

        KHttp_IOCP_Handler::Overlapped _Overlapped;
    };

protected:

    KHttpServerWebSocket(
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
        );

    //
    //  Capture the http request queue handle and http request owned by the given KHttpServerRequest.  After this has been called,
    //  the KHttpServerRequest is no longer valid (its reference to the http request is set to nullptr, so attempting to use
    //  the KHttpServerRequest may result in AV)
    //
    static VOID
    Capture(
        __inout KHttpServerRequest& HttpServerRequest,
        __out HANDLE& RequestQueueHandle,
        __out PHTTP_REQUEST& HttpRequest
        );

    //
    //  Create the WebSocket Protocol Component API handle
    //
    static NTSTATUS
    CreateHandle(
        __in ULONG _TransportReceiveBufferSize,
        __in ULONG _TransportSendBufferSize,
        __in KAsyncContextBase& Context,
        __out WEB_SOCKET_HANDLE& WebSocketHandle
        );

    //
    //  Retrieve the client-requested subprotocols
    //
    static NTSTATUS
    GetRequestedSubprotocols(
        __in KHttpServerRequest& ServerRequest,
        __in KAsyncContextBase& Context,
        __out KStringA::SPtr& RequestedSubprotocolsString
        );

    //
    //  Select the subprotocol to be used for this WebSocket session
    //
    static NTSTATUS
    SelectSubprotocol(
        __in KStringA& SupportedSubprotocolsString,
        __in KHttpServerRequest& ServerRequest,
        __in KAsyncContextBase& Context,
        __inout KStringA& SelectedSubprotocol
        );

    //
    //  Begin the server handshake in the WebSocket Protocol Component API, and obtain the headers to be sent
    //  in the http response
    //
    static NTSTATUS
    BeginServerHandshake(
        __in KHttpServerRequest& ServerRequest,
        __in WEB_SOCKET_HANDLE& WebSocketHandle,
        __in KStringViewA& SelectedSubprotocol,
        __in KAsyncContextBase& Context,
        __out KUniquePtr<WEB_SOCKET_HTTP_HEADER, ArrayDeleter<WEB_SOCKET_HTTP_HEADER>>& RequestHeaders,
        __out KUniquePtr<WEB_SOCKET_HTTP_HEADER, FailFastIfNonNullDeleter<WEB_SOCKET_HTTP_HEADER>>& ResponseHeaders,
        __out ULONG& ResponseHeadersCount
        );

    //
    //  Convert the WebSocket Protocol Component API response headers to their Http Server API (http.sys) equivalent,
    //  and obtain the http response to send to the client
    //
    static NTSTATUS
    GetHttpResponse(
        __in KUniquePtr<WEB_SOCKET_HTTP_HEADER, FailFastIfNonNullDeleter<WEB_SOCKET_HTTP_HEADER>>& ResponseHeaders,
        __in ULONG ResponseHeaderCount,
        __in KAsyncContextBase& Context,
        __out HTTP_RESPONSE& HttpResponse,
        __out KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>>& HttpResponseUnknownHeaders
        );

    VOID
    OnServiceOpen() override;

    VOID 
    OnServiceClose() override;

    VOID
    CleanupWebSocket(
        __in NTSTATUS Status
        ) override;

    //
    //  Drain the WebSocket Protocol Component action queues to release the resources it holds.
    //
    VOID
    FlushActions();

    VOID
    SendResponseCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    PingTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    UnsolicitedPongTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    PongTimeoutTimerCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    SendRequestDequeueCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    TransportSendCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    VOID
    SendCloseCallback(
        __in_opt KAsyncContextBase* const Parent,
        __in KAsyncContextBase& CompletingOperation
        );

    //
    //  Pump the WebSocket Protocol Component send queue
    //
    VOID
    RunSendActionQueue();

protected:

    //
    //  Set at create time
    //
    ReceiveOperationProcessor _ReceiveOperationProcessor;
    KAsyncQueue<WebSocketOperation>::DequeueOperation::SPtr _SendRequestDequeueOperation;
    HttpSendEntityBodyOperation::SPtr _TransportSendOperation;
    HttpSendResponseOperation::SPtr _SendResponseOperation;
    KHttpServerSendCloseOperation::SPtr _SendCloseOperation;
    KHttpUtils::CallbackAsync::SPtr _InformOfClosureAsync;
    CloseTransportOperation::SPtr _CloseTransportOperation;
    KBuffer::SPtr _RemoteCloseReasonBuffer;

    //
    //  Handles
    //
    HANDLE _RequestQueueHandle;
    WEB_SOCKET_HANDLE _WebSocketHandle;
    PHTTP_REQUEST _HttpRequest;

    //
    //  Vars used during service-open
    //
    HTTP_RESPONSE _HttpResponse;
    KUniquePtr<HTTP_UNKNOWN_HEADER, ArrayDeleter<HTTP_UNKNOWN_HEADER>> _HttpResponseUnknownHeaders;
    KHttpServerRequest::SPtr _ServerRequest;  //  Temporarily held during handshake
    KUniquePtr<WEB_SOCKET_HTTP_HEADER, ArrayDeleter<WEB_SOCKET_HTTP_HEADER>> _RequestHeaders;
    KUniquePtr<WEB_SOCKET_HTTP_HEADER, FailFastIfNonNullDeleter<WEB_SOCKET_HTTP_HEADER>> _ResponseHeaders;

    //
    //  Set during StartOpenWebSocket()
    //
    WebSocketCloseReceivedCallback _CloseReceivedCallback;
    ULONG _TransportReceiveBufferSize;
    ULONG _TransportSendBufferSize;
    KStringA::SPtr _SupportedSubProtocolsString;
    KHttpServer::SPtr _ParentServer;  //  Reference and activities are held during WebSocket active lifetime

    //
    //  Allocated on-demand during the open handshake, if the corresponding timing constants were set
    //
    KTimer::SPtr _PingTimer;
    KTimer::SPtr _UnsolicitedPongTimer;
    KTimer::SPtr _PongTimeoutTimer;

    BOOLEAN _PongTimeoutTimerIsActive;  //  TRUE while the pong timeout timer has not yet completed, FALSE otherwise.

    //
    //  Time values set at various points during WebSocket operation, which control when timers should be started
    //  and what action should be taken when they call back
    //
    KDateTime _LatestReceive;  //  Set whenever a transport receive occurs
    KDateTime _TimerReceive;  //  Set to the current value of _LatestReceive when the PingTimer starts
    KDateTime _LatestReceivedPong;  //  Set whenever a Pong frame is received.
    KDateTime _TimerReceivedPong;  //  Set to the current value of _LatestPong when the PongTimeoutTimer starts
    KDateTime _LatestPing;  //  Set whenever a ping is scheduled (whenever _ShouldSendPing is set to true)

    //
    //  Set to TRUE when the respective queues are deactivated.  Must be checked within the apartment before calling
    //  Deactivate() to prevent multiple calls to Deactivate()
    //
    BOOLEAN _IsSendRequestQueueDeactivated;

    //
    //  State flags used to ensure certain code is only executed once
    //
    BOOLEAN _IsSendTruncated;
    BOOLEAN _IsHandshakeStarted;
    BOOLEAN _IsHandshakeComplete;

    volatile ULONG _IsCleanedUp;

    BOOLEAN _CloseSent;  //  Stop running the send queue when this is set, to avoid sending ping/pong/etc. after the close has been sent
};

#endif





