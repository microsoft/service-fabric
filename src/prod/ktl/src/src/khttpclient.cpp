/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    khttpclient.cpp

    Description:
      Kernel Tempate Library (KTL): HTTP Package

      Describes KTL-based HTTP Client

    History:
      raymcc          3-Mar-2012         Initial version.
      alanwar         1-Nov-2013         Added multipart client support, tracing
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

#pragma region KHttpClient
//  KHttpClient constructor
//
KHttpClient::KHttpClient()
    : _UserAgent(GetThisAllocator())
{
    _hSession = 0;
    _StartupStatus = STATUS_PENDING;
    _WinHttpError = 0;
}


//
//  KHttpClient::Create
//
//  Creates the client
//
NTSTATUS
KHttpClient::Create(
    __in  KStringView& UserAgent,
    __in  KAllocator& Alloc,
    __out KHttpClient::SPtr& Client
    )
{
    NTSTATUS status;
    KHttpClient::SPtr client;

    Client = nullptr;
    if (UserAgent.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    client = _new (KTL_TAG_HTTP, Alloc) KHttpClient();
    if (! client)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return(status);
    }

    if (! client->_UserAgent.CopyFrom(UserAgent, TRUE))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return(status);
    }

    KHttpClient* p = client.RawPtr();

    KDeferredJump<VOID(KHttpClient::SPtr)> Initialize(*p, &KHttpClient::BackgroundInitialize, Alloc);
    status = Initialize(client);   // We send 'this' in to ensure a ref count throughout the initialization
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, NULL, 0, 0);
        return(status);
    }

    Client = client;
    return STATUS_SUCCESS;
}


//
//
//  KHttpClient::BackgrounInitialize
//
//  Called as a work item
//
VOID
KHttpClient::BackgroundInitialize(
    __in KHttpClient::SPtr Obj
    )
{
    Obj->_BackgroundInitialize();
}

VOID
KHttpClient::_BackgroundInitialize()
{
    ULONG error;

    // _StartupStatus is still STATUS_PENDING on entry
    //

    KFinally([&](){ _StartupEvent.SetEvent(); });

    // This call is documented as being synchronous.
    //
    //
    _hSession = WinHttpOpen(LPCWSTR(_UserAgent),
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS,
                          WINHTTP_FLAG_ASYNC
                          );

    if (! _hSession)
    {
        error = GetLastError();
        _WinHttpError = error;

        if (error == ERROR_NOT_ENOUGH_MEMORY)
        {
            _StartupStatus = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory(0, _StartupStatus, this, _WinHttpError, 0);
            return;
        }

        _StartupStatus = STATUS_UNSUCCESSFUL;
        KTraceFailedAsyncRequest(_StartupStatus, NULL, _WinHttpError, 0);
        return;
    }

    // Set the async callback
    //
    WINHTTP_STATUS_CALLBACK result = WinHttpSetStatusCallback(_hSession, _WinHttpCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
    if (result == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        WinHttpCloseHandle(_hSession);
        _hSession = 0;

        _WinHttpError = GetLastError();
        _StartupStatus = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(_StartupStatus, NULL, _WinHttpError, 0);
        return;
    }

    _StartupStatus = STATUS_SUCCESS;
}


//
//  KHttpClient::~KHttpClient
//
KHttpClient::~KHttpClient()
{
    if (_hSession)
    {
        WinHttpCloseHandle(_hSession);
        _hSession = 0;
    }
}


//
// KHttpClient::_WinHTTPCallback
//
// WinHTTP async callback.  This is called by the WinHTTP mechanism, not KTL async.
//
VOID CALLBACK
KHttpClient::_WinHttpCallback(
    __in  HINTERNET hInternet,
    __in  DWORD_PTR dwContext,
    __in  DWORD dwInternetStatus,
    __in  LPVOID lpvStatusInformation,
    __in  DWORD dwStatusInformationLength
    )
{
    UNREFERENCED_PARAMETER(hInternet);
 
    NTSTATUS status = STATUS_SUCCESS;
    ULONG error;

    // Most of these cases are not used, but are useful for debugging.
    //
    switch (dwInternetStatus)
    {
        case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
            // An error occurred while sending an HTTP request. The lpvStatusInformation parameter contains a pointer to a WINHTTP_ASYNC_RESULT structure,
            // of which the dwResult member indicates the return value of the function and any error codes that apply.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;

                NTSTATUS status1;
                WINHTTP_ASYNC_RESULT* WinHttpRes = (WINHTTP_ASYNC_RESULT *) lpvStatusInformation;

                // If the request was cancelled due to invalid certificate, the error will already be set, don't override it.
                if (Request->_WinHttpError != ERROR_WINHTTP_SECURE_INVALID_CERT)
                {
                    Request->SetWinHttpError(WinHttpRes->dwError);
                }

                if (WinHttpRes->dwError == 12007) // ERROR_INTERNET_NAME_NOT_RESOLVED; can't include the file that contains this definition
                {
                    status1 = STATUS_INVALID_ADDRESS;
                }
                else
                {
                    status1 = STATUS_INTERNAL_ERROR;
                }

                if (Request->_IsMultiPart)
                {
                    KInvariant(Request->_ASDRContext);
                    Request->_ASDRContext->Complete(status1);
                }

                KTraceFailedAsyncRequest(status1, Request, Request->_WinHttpError, Request->_IsMultiPart);
                Request->Complete(status1);
            }
            break;

        case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
            // The request completed successfully. The lpvStatusInformation parameter is NULL.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;

                if (Request->_IsMultiPart)
                {
                    //
                    // For multipart messages, the headers are sent and now the data needs to be sent.
                    // Complete the SendRequestHeaders async so that the caller can start sending the data
                    //
                    KInvariant(Request->_ASDRContext);
                    Request->_ASDRContext->Complete(STATUS_SUCCESS);
                } else {
                    //
                    // For single part messages, all of the data has been transmitted and so move on to the 
                    // next step of receiving the response
                    //
                    BOOL bRes = WinHttpReceiveResponse(Request->_hRequest, NULL);
                    if (! bRes)
                    {
                        error = GetLastError();
                        status = STATUS_INTERNAL_ERROR;
                        Request->SetWinHttpError(error);

                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->Complete(status);
                        return;
                    }
                }
            }
            break;

        case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
            // Data was successfully written to the server. The lpvStatusInformation parameter 
            // contains a pointer to a DWORD that indicates the number of bytes written.
            // When used by WinHttpWebSocketSend, the lpvStatusInformation parameter contains a pointer to a
            //  WINHTTP_WEB_SOCKET_STATUS structure, and the
            //  dwStatusInformationLength parameter indicates the size
            //  of lpvStatusInformation.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;

                //
                // Only for multipart messages, writing data is completed.
                // Complete the SendRequestHeaders async so that the caller can start sending the data
                //
                KInvariant(Request->_IsMultiPart);
                KInvariant(Request->_ASDRContext);
                Request->_ASDRContext->Complete(STATUS_SUCCESS);
            }
            break;

            
        case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
            // The response header has been received and is available with WinHttpQueryHeaders. The lpvStatusInformation parameter is NULL.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;
                NTSTATUS status1;

                status1 = Request->_Response->ProcessStdHeaders();
                if (! NT_SUCCESS(status1))
                {
                    if (Request->_IsMultiPart)
                    {
                        KInvariant(Request->_ASDRContext);
                        Request->_ASDRContext->Complete(status1);
                    }

                    KTraceFailedAsyncRequest(status1, Request, Request->_WinHttpError, Request->_IsMultiPart);
                    Request->Complete(status1);
                    return;
                }

                if (Request->_Response->_IsMultiPart)
                {
                    //
                    // For multipart read, we complete the request StartEndRequest async so the caller
                    // can know it is safe to read the response
                    //
                    KInvariant(Request->_ASDRContext);
                    Request->_ASDRContext->Complete(STATUS_SUCCESS);
                    return;
                } else {
                    //
                    // For single part, setup to read in all of the data
                    //
                    BOOL bRes = WinHttpQueryDataAvailable(Request->_hRequest, NULL);
                    if (! bRes)
                    {
                        error = GetLastError();
                        status1 = STATUS_INTERNAL_ERROR;
                        Request->SetWinHttpError(error);

                        KTraceFailedAsyncRequest(status1, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->Complete(status1);
                        return;
                    }
                }
            }
            break;


        case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
            // Data is available to be retrieved with WinHttpReadData.
            // The lpvStatusInformation parameter points to a DWORD that contains the number of bytes of data available.
            // The dwStatusInformationLength parameter itself is 4 (the size of a DWORD).
            {
                ULONG ToRead = *PULONG(lpvStatusInformation);
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;
                KHttpCliRequest::Response::SPtr Resp = Request->_Response;

                if (ToRead == 0)
                {
                    if (Resp->_IsMultiPart)
                    {
                        //
                        // If this is a multipart read then complete the ReadResponseData async to indicate
                        // that no more data is available
                        //
                        KInvariant(Request->_ASDRContext);
                        Request->_ASDRContext->CompleteRead(STATUS_SUCCESS);
                    }

                    //
                    // CONSIDER: MSDN http://msdn.microsoft.com/en-us/library/windows/desktop/aa384101(v=vs.85).aspx
                    //           does not recommend using the result from WinHttpQueryDataAvailable() to determine if the 
                    //           end of response has been reached.
                    //
                    Request->Complete(STATUS_SUCCESS);
                    return;
                }

                //
                // Remember how much WinHttp tells us to read
                //
                if (Resp->_IsMultiPart)
                {
                    //
                    // For multipart read, call WinHttpReadData to retrieve data
                    //
                    KInvariant(Request->_ASDRContext);

                    if (ToRead > Request->_ASDRContext->GetMaxBytesToRead())
                    {
                        //
                        // Limit amount read to the max size wanted by the caller
                        //
                        ToRead = Request->_ASDRContext->GetMaxBytesToRead();
                    }

                    PVOID Address;
                    status = Request->_ASDRContext->GetOrAllocateReadBuffer(ToRead, &Address);
                    if (! NT_SUCCESS(status))
                    {
                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->_ASDRContext->Complete(status);
                        Request->Complete(status);
                        return;
                    }

                    BOOL bRes = WinHttpReadData(Request->_hRequest, Address, ToRead, NULL);
                    if (! bRes)
                    {
                        status = STATUS_INTERNAL_ERROR;
                        Request->SetWinHttpError(GetLastError());

                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->_ASDRContext->Complete(status);
                        Request->Complete(status);
                        return;
                    }
                } else {
                    //
                    // For single part read, keep reading into the content buffer
                    //
                    if (! Request->_Response->_Content)
                    {
                        status = STATUS_INTERNAL_ERROR;
                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->Complete(status);
                        return;
                    }

                    // If here, we have some reading to to.   Because we may not know the Content-Length up front
                    // we may have to reallocate the buffer on the fly.
                    //
                    //
                    LONG RemainingSpaceInBuf = LONG(Resp->_Content->QuerySize() - Resp->_EntityBodyLength);
                    if (RemainingSpaceInBuf < LONG(ToRead))
                    {
                        // This only executes if there was no Content-Length header.
                        //
                        ULONG NewSize = Resp->_Content->QuerySize() * 2;  // Double the buffer each time
                        BOOL Res = Request->_Response->ReallocateResponseBuffer(NewSize);
                        if (! Res)
                        {
                            status = STATUS_INSUFFICIENT_RESOURCES;
                            KTraceFailedAsyncRequest(status, Request, NewSize, NULL);
                            Request->Complete(status);
                            return;
                        }
                    }

                    // Now we can read content
                    //
                    LPBYTE ReceivingAddress = (LPBYTE) Resp->_Content->GetBuffer();
                    ReceivingAddress += Resp->_EntityBodyLength;
                    BOOL bRes = WinHttpReadData(Request->_hRequest, ReceivingAddress, ToRead, NULL);
                    if (! bRes)
                    {
                        error = GetLastError();
                        status = STATUS_INTERNAL_ERROR;
                        Request->SetWinHttpError(error);

                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->Complete(status);
                        return;
                    }
                }
            }
            break;

        case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
            // Data was successfully read from the server. The lpvStatusInformation parameter contains 
            // a pointer to the buffer specified in the call to WinHttpReadData. The 
            // dwStatusInformationLength parameter contains the number of bytes read.
            // When used by WinHttpWebSocketReceive, the lpvStatusInformation parameter contains a pointer 
            // to a WINHTTP_WEB_SOCKET_STATUS structure, and the dwStatusInformationLength parameter 
            // indicates the size of lpvStatusInformation.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *) dwContext;

                if (Request->_IsMultiPart)
                {
                    //
                    // For multipart read complete the async
                    //
                    KInvariant(Request->_ASDRContext);
                    Request->_ASDRContext->CompleteRead(STATUS_SUCCESS);
                } else {
                    Request->_Response->IncEntityBodyLength(dwStatusInformationLength);
                    BOOL bRes = WinHttpQueryDataAvailable(Request->_hRequest, NULL);
                    if (! bRes)
                    {
                        error = GetLastError();
                        status = STATUS_INTERNAL_ERROR;
                        Request->SetWinHttpError(error);

                        KTraceFailedAsyncRequest(status, Request, Request->_WinHttpError, Request->_IsMultiPart);
                        Request->Complete(status);
                        return;
                    }
                }
            }
            break;

        case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
            // Connecting to the server. The lpvStatusInformation parameter contains a pointer to an LPWSTR that indicates the IP address of the server in dotted notation. 
            // This flag has been deprecated.
            {
                KHttpCliRequest* Request = (KHttpCliRequest *)dwContext;

                // If request is over HTTPS and server certificate validation handler exists, call it now and decide whether to fail/succeed the request
                if (Request->_IsSecure
                    && Request->_ServerCertValidationHandler != NULL)
                {
                    BOOLEAN bRet = Request->ValidateServerCertificate();

                    // If certificate handler asks to fail the request, do so.
                    if (bRet == FALSE)
                    {
                        Request->SetWinHttpError(ERROR_WINHTTP_SECURE_INVALID_CERT);

                        // Cancel/Terminate the in-progress request by closing the request handle.
                        Request->CancelRequest();

                        return;
                    }
                }
            }
            break;

#if 0
        //
        // Leave cases below for completeness
        //

        case WINHTTP_CALLBACK_STATUS_HANDLE_CREATED:
            // An HINTERNET handle has been created. The lpvStatusInformation parameter contains a pointer to the HINTERNET handle.
            break;

        case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
            // This handle value has been terminated. The lpvStatusInformation parameter contains a pointer to the HINTERNET handle. 
            // There will be no more callbacks for this handle.
            break;

        case WINHTTP_CALLBACK_STATUS_INTERMEDIATE_RESPONSE:
            // Received an intermediate (100 level) status code message from the server. The lpvStatusInformation parameter 
            // contains a pointer to a DWORD that indicates the status code.
            break;


        case WINHTTP_CALLBACK_STATUS_REDIRECT:
            // An HTTP request is about to automatically redirect the request. The lpvStatusInformation parameter contains a
            // pointer to an LPWSTR indicating the new URL. At this point, the application can read any data returned by the
            // server with the redirect response and can query the response headers. It can also cancel the operation by closing the handle.
            break;

        case WINHTTP_CALLBACK_STATUS_SECURE_FAILURE:
            // One or more errors were encountered while retrieving a Secure Sockets Layer (SSL) certificate from the server.
            // The lpvStatusInformation parameter contains a flag. For more information, see the description for lpvStatusInformation.
            break;


        case WINHTTP_CALLBACK_STATUS_GETPROXYFORURL_COMPLETE:
            // The operation initiated by a call to WinHttpGetProxyForUrlEx is complete. Data is available to be retrieved with WinHttpReadData.
            break;

        case WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE:
            // The connection was successfully closed via a call to WinHttpWebSocketClose.
            break;


        // DEPRECATED CALLBACK FLAGS
        //
        // DO NOT USE THESE; they are here for debugging.
        //
        case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
            // Successfully sent the information request to the server. The lpvStatusInformation parameter
            // contains a pointer to a DWORD indicating the number of bytes sent. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_RESOLVING_NAME:
            // Looking up the IP address of a server name. The lpvStatusInformation parameter contains a pointer to the server name being resolved. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_RESPONSE_RECEIVED:
            // Successfully received a response from the server. The lpvStatusInformation parameter contains a pointer to a DWORD indicating the number of bytes received. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_CLOSING_CONNECTION:
            // Closing the connection to the server. The lpvStatusInformation parameter is NULL. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_CONNECTED_TO_SERVER:
            //  Successfully connected to the server. The lpvStatusInformation parameter contains a pointer to an LPWSTR that indicates the IP address of the server in dotted notation. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_CONNECTING_TO_SERVER:
            //  Connecting to the server. The lpvStatusInformation parameter contains a pointer to an LPWSTR that indicates the IP address of the server in dotted notation. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_CONNECTION_CLOSED:
            // Successfully closed the connection to the server. The lpvStatusInformation parameter is NULL. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_NAME_RESOLVED:
            // Successfully found the IP address of the server. The lpvStatusInformation parameter contains a pointer to a LPWSTR that indicates the name that was resolved. This flag has been deprecated.
            break;

        case WINHTTP_CALLBACK_STATUS_RECEIVING_RESPONSE:
            // Waiting for the server to respond to a request. The lpvStatusInformation parameter is NULL. This flag has been deprecated.
            break;
#endif

        default:
            // Unknown case
            break;
    }
}

BOOLEAN
KHttpCliRequest::ValidateServerCertificate()
{
    PCERT_CONTEXT pCertContext = NULL; // TODO-kavyako: Test this
    DWORD certContextSize = sizeof(pCertContext);
    BOOLEAN isValid = FALSE;

    // Obtain the server certificate details:
    if (!WinHttpQueryOption(_hRequest, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &pCertContext, &certContextSize))
    {
        // Failed to get server certificate.
        return FALSE;
    }

    // Call the user provided handler to validate cert.
    isValid = _ServerCertValidationHandler(pCertContext);

    // Free the PCCERT_CONTEXT pointer that is filled into the buffer.
    CertFreeCertificateContext(pCertContext);

    return isValid;
}

//
// KHttpClient::CreateRequest
//
NTSTATUS
KHttpClient::CreateRequest(
    __out KHttpCliRequest::SPtr& Request,
    __in int resolveTimeoutInMilliSeconds,
    __in int connectTimeoutInMilliSeconds,
    __in int sendTimeoutInMilliSeconds,
    __in int receiveTimeoutInMilliSeconds
    )
{
    return CreateRequest(
            Request, 
            GetThisAllocator(),
            resolveTimeoutInMilliSeconds,
            connectTimeoutInMilliSeconds,
            sendTimeoutInMilliSeconds,
            receiveTimeoutInMilliSeconds);
}

NTSTATUS
KHttpClient::CreateRequest(
    __out KHttpCliRequest::SPtr& Request,
    __in KAllocator &Alloc,
    __in int resolveTimeoutInMilliSeconds,
    __in int connectTimeoutInMilliSeconds,
    __in int sendTimeoutInMilliSeconds,
    __in int receiveTimeoutInMilliSeconds
    )
{
    NTSTATUS status;
    KHttpCliRequest::SPtr request;

    request = _new (KTL_TAG_HTTP, Alloc) KHttpCliRequest(this, FALSE);
    if (! request)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 0, 0);
        return(status);
    }

    status = request->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    request->_resolveTimeoutInMilliSeconds = resolveTimeoutInMilliSeconds;
    request->_connectTimeoutInMilliSeconds = connectTimeoutInMilliSeconds;
    request->_sendTimeoutInMilliSeconds = sendTimeoutInMilliSeconds;
    request->_receiveTimeoutInMilliSeconds = receiveTimeoutInMilliSeconds;

    Request.Attach(request.Detach());

    return(status);
}

NTSTATUS
KHttpClient::CreateMultiPartRequest(
    __out KHttpCliRequest::SPtr& Request,
    __in int resolveTimeoutInMilliSeconds,
    __in int connectTimeoutInMilliSeconds,
    __in int sendTimeoutInMilliSeconds,
    __in int receiveTimeoutInMilliSeconds
    )
{
    return CreateMultiPartRequest(
            Request, 
            GetThisAllocator(),
            resolveTimeoutInMilliSeconds,
            connectTimeoutInMilliSeconds,
            sendTimeoutInMilliSeconds,
            receiveTimeoutInMilliSeconds);
}

NTSTATUS
KHttpClient::CreateMultiPartRequest(
    __out KHttpCliRequest::SPtr& Request,
    __in KAllocator &Alloc,
    __in int resolveTimeoutInMilliSeconds,
    __in int connectTimeoutInMilliSeconds,
    __in int sendTimeoutInMilliSeconds,
    __in int receiveTimeoutInMilliSeconds
    )
{
    NTSTATUS status;
    KHttpCliRequest::SPtr request;

    request = _new (KTL_TAG_HTTP, Alloc) KHttpCliRequest(this, TRUE);
    if (! request)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, 0, 0);
        return(status);
    }

    status = request->Status();
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    request->_resolveTimeoutInMilliSeconds = resolveTimeoutInMilliSeconds;
    request->_connectTimeoutInMilliSeconds = connectTimeoutInMilliSeconds;
    request->_sendTimeoutInMilliSeconds = sendTimeoutInMilliSeconds;
    request->_receiveTimeoutInMilliSeconds = receiveTimeoutInMilliSeconds;

    Request.Attach(request.Detach());

    return(status);
}
#pragma endregion KHttpClient

#pragma region KHttpCliRequest
//
//  KHttpCliRequest constructor
//
KHttpCliRequest::KHttpCliRequest(
    __in KHttpClient::SPtr Parent,
    __in BOOLEAN IsMultiPart
    )
    : _Headers(GetThisAllocator()),
      _Continue(GetThisAllocator().GetKtlSystem())
{
    NTSTATUS status;

    status = _Headers.Status();
    if (! NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _Parent = Parent;
    _ContentLengthHeader = 0;
    _ContentCarrier = KHttpUtils::eNone;
    _hRequest = 0;
    _hConnection = 0;
    _WinHttpError = 0;
    _IsMultiPart = IsMultiPart;
    _IsSecure = FALSE;
    _ClientCert = NULL;
    _AllowRedirects = TRUE;
    _EnableCookies = TRUE;
    _EnableAutoLogonForAllRequests = FALSE;
}

//
//  KHttpCliRequest destructor
//
KHttpCliRequest::~KHttpCliRequest()
{
    if (_ClientCert)
    {
        CertFreeCertificateContext(_ClientCert);
        _ClientCert = NULL;
    }
    if (_hRequest)
    {
        WinHttpCloseHandle(_hRequest);
        _hRequest = NULL;
    }
    if (_hConnection)
    {
        WinHttpCloseHandle(_hConnection);
        _hConnection = NULL;
    }
}

//
// As per msdn: An application can terminate an in-progress synchronous or asynchronous request by closing the HINTERNET request handle using WinHttpCloseHandle.
// 
VOID KHttpCliRequest::CancelRequest()
{
    if (_hRequest)
    {
        WinHttpCloseHandle(_hRequest);
        _hRequest = NULL;
    }
}

//
//  KHttpCliRequest::Response constructor
//
KHttpCliRequest::Response::Response(
    __in BOOLEAN IsMultiPart
    )
{
    _IsMultiPart = IsMultiPart;
    _ContentLengthHeader = 0;
    _EntityBodyLength = 0;
    _HttpStatusCode = 0;
    _hRequest = 0;
}


//
//  KHttpCliRequest::Response::ProcessResponseHeaders
//
//  Extracts the HTTP response code and the content-length
//
NTSTATUS
KHttpCliRequest::Response::ProcessStdHeaders()
{
    NTSTATUS status;
    ULONG error;
    KLocalString<128> buffer;
    DWORD dwIndex;
    DWORD dwSize;
    BOOL bRes;

    // Get the final status code
    //
    dwIndex = WINHTTP_NO_HEADER_INDEX;
    dwSize = 128*2;
    bRes = WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_STATUS_CODE, WINHTTP_HEADER_NAME_BY_INDEX, PWSTR(buffer), &dwSize, &dwIndex);
    if (! bRes)
    {
        status = STATUS_INTERNAL_ERROR;
        error = GetLastError();
        _WinHttpError = error;

        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    buffer.SetLength(dwSize / 2);  // Bytes to chars
    if (! buffer.ToULONG(_HttpStatusCode))
    {
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    // Get the content length.  This is an optional header, so if we don't have it
    // we just progressively try to read content.
    //
    dwIndex = WINHTTP_NO_HEADER_INDEX;
    dwSize = 128*2;
    bRes = WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, PWSTR(buffer), &dwSize, &dwIndex);
    if (! bRes)
    {
        error = GetLastError();
        _WinHttpError = error;

        if (error == ERROR_WINHTTP_HEADER_NOT_FOUND)
        {
            if (! _IsMultiPart)
            {
                // If here, and this is single part, there is no Content-Length header and we'll just default.
                //
                bRes = ReallocateResponseBuffer(DefaultResponseBufSize);
                if (! bRes)
                {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    KTraceOutOfMemory(0, status, this, DefaultResponseBufSize, 0);
                    return(status);
                }
            }
            return STATUS_SUCCESS;
        }

        if (error == ERROR_NOT_ENOUGH_MEMORY)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            KTraceOutOfMemory(0, status, this, 0, 0);
            return(status);
        }

        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    // If here, we have Content-Length and can just allocate once and be done.
    //
    buffer.SetLength(dwSize / 2);  // Bytes to chars
    if (! buffer.ToULONG(_ContentLengthHeader))
    {
        status = STATUS_INTERNAL_ERROR;
        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    // Now allocate a KBuffer to receive the content.
    //
    if (! _IsMultiPart)
    {
        if (_ContentLengthHeader)
        {
            bRes = ReallocateResponseBuffer(_ContentLengthHeader);
            if (! bRes)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, this, _ContentLengthHeader, 0);
                return(status);
            }
        }
    }

    return(STATUS_SUCCESS);
}


//
//  KHttpCliRequest::Response destructor
//
KHttpCliRequest::Response::~Response()
{
    // Do not close _hRequest, as it is owned by the parent object
}


//
//  KHttpCliRequest::Response::GetAllHeaders
//
//  Primarily for debugging. Returns all the headers.
//
NTSTATUS
KHttpCliRequest::Response::GetAllHeaders(
    __out KString::SPtr& Headers
    )
{
    NTSTATUS status;
    ULONG error;
    DWORD dwSize = 0;
    BOOL bRes;

    bRes = WinHttpQueryHeaders(_hRequest,
                               WINHTTP_QUERY_RAW_HEADERS_CRLF,
                               WINHTTP_HEADER_NAME_BY_INDEX,
                               WINHTTP_NO_OUTPUT_BUFFER, &dwSize,
                               WINHTTP_NO_HEADER_INDEX);

    if (! bRes)
    {
        error = GetLastError();
        if (error == ERROR_INSUFFICIENT_BUFFER)
        {
            // REVIEW: TyAdam, BharatN - safe code
            HRESULT hr;
            ULONG result;
            hr = ULongAdd( (dwSize / 2), 1, &result);
            KInvariant(SUCCEEDED(hr));
            KString::SPtr Tmp = KString::Create(GetThisAllocator(), result);
            if (! Tmp)
            {
                status = STATUS_INSUFFICIENT_RESOURCES;
                KTraceOutOfMemory(0, status, this, result, 0);
                return(status);
            }

            bRes = WinHttpQueryHeaders( _hRequest,
                                           WINHTTP_QUERY_RAW_HEADERS_CRLF,
                                           WINHTTP_HEADER_NAME_BY_INDEX,
                                           PWSTR(*Tmp),
                                           &dwSize,
                                           WINHTTP_NO_HEADER_INDEX);
            if (! bRes)
            {
                _WinHttpError = GetLastError();
                status = STATUS_INTERNAL_ERROR;

                KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
                return(status);
            }

            Headers = Tmp;
        } else {
            _WinHttpError = GetLastError();
            status = STATUS_INTERNAL_ERROR;

            KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
            return(status);
        }
    } else {
        // CONSIDER: No headers ?
        Headers = nullptr;
    }

    return(STATUS_SUCCESS);
}

//
// KHttpCliRequest::Response::ReallocateResponseBuffer
//
// Called for initial allocation and all reallocation/resize ops.
//
BOOLEAN
KHttpCliRequest::Response::ReallocateResponseBuffer(
    __in ULONG NewSize
    )
{
    NTSTATUS status;

    // If no previous allocation has occurred
    //
    if (!_Content)
    {
        status = KBuffer::Create(NewSize, _Content, GetThisAllocator());
        if (! NT_SUCCESS(status))
        {
            KTraceOutOfMemory(0, status, this, NewSize, 0);
            return FALSE;
        }
    }

    // Otherwise, we are resizing.
    //
    else
    {
        if (_Content->QuerySize() >= NewSize)
        {
            return TRUE;
        }
        else
        {
            status = _Content->SetSize(NewSize, TRUE);
            if (! NT_SUCCESS(status))
            {
                KTraceOutOfMemory(0, status, this, NewSize, 0);
                return FALSE;
            }
        }
    }
    return TRUE;
}


NTSTATUS
KHttpCliRequest::HeaderPair::AddHeader(
    __in KStringView& HeaderId,
    __in KStringView& Value,
    __in KArray<HeaderPair::SPtr>& Headers,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in PVOID ContextPointer,
    __in ULONGLONG AdditionalErrorTrace0,
    __in ULONGLONG AdditionalErrorTrace1)
{
    NTSTATUS status;

    if (HeaderId.IsEmpty() || Value.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    HeaderPair::SPtr NewPair = _new(AllocationTag, Allocator) HeaderPair();
    if (! NewPair)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, ContextPointer, sizeof(HeaderPair), 0);
        return(status);
    }

    if (! NewPair->_Id.CopyFrom(HeaderId) ||
        ! NewPair->_Value.CopyFrom(Value))
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, ContextPointer, 0, 0);
        return(status);
    }

    status = Headers.Append(NewPair);
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, NULL, AdditionalErrorTrace0, AdditionalErrorTrace1);
    }

    return status;
}


//
// KHttpCliRequest::AddHeader
//
// Adds a header to the request.  Duplicates are not checked for efficiency, as
// a client is unlikely to add the same header twice anyway.
//
NTSTATUS
KHttpCliRequest::AddHeader(
    __in KStringView& HeaderId,
    __in KStringView& Value
    )
{
    return HeaderPair::AddHeader(HeaderId, Value, _Headers, GetThisAllocator(), GetThisAllocationTag(), this, _WinHttpError, (ULONGLONG)_hRequest);
}


NTSTATUS
KHttpCliRequest::HeaderPair::AddHeaderByCode(
    __in KHttpUtils::HttpHeaderType HeaderCode,
    __in KStringView& Value,
    __in KArray<HeaderPair::SPtr>& Headers,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag,
    __in PVOID ContextPointer,
    __in ULONGLONG AdditionalErrorTrace0,
    __in ULONGLONG AdditionalErrorTrace1
    )
{
    KStringView id;
    if (KHttpUtils::StdHeaderToString(HeaderCode, id))
    {
        return AddHeader(id, Value, Headers, Allocator, AllocationTag, ContextPointer, AdditionalErrorTrace0, AdditionalErrorTrace1);
    }

    return STATUS_NOT_FOUND;
}

//
//  KHttpCliRequest::AddHeaderByCode
//
NTSTATUS
KHttpCliRequest::AddHeaderByCode(
    __in KHttpUtils::HttpHeaderType HeaderCode,
    __in KStringView& Value
   )
{
    return HeaderPair::AddHeaderByCode(HeaderCode, Value, _Headers, GetThisAllocator(), GetThisAllocationTag(), this, _WinHttpError, (ULONGLONG)_hRequest);
}


NTSTATUS
KHttpCliRequest::SetHeaders(
    __in KHttpHeaderList const & HeaderList
    )
{
    _Headers = HeaderList._Headers;
    return _Headers.Status();
}

NTSTATUS
KHttpCliRequest::SetHeaders(
__in KString::SPtr const &HeadersBlob
    )
{
    _HeadersBlob = HeadersBlob;
    return STATUS_SUCCESS;
}

//
//  KHttpCliRequest::Response::GetContent
//
NTSTATUS
KHttpCliRequest::Response::GetContent(
    __out KBuffer::SPtr& Buf
    )
{
    NTSTATUS status;

    if (! _Content)
    {
        return STATUS_NO_DATA_DETECTED;
    }

    //
    // Should not fail as the size only shrinks
    //
    status = _Content->SetSize(_EntityBodyLength);
    KInvariant(NT_SUCCESS(status));

    Buf = _Content;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::Response::GetHttpResponseStatusText(
    __out KString::SPtr& Value
)
{
    return GetHeaderByWinHttpCode(WINHTTP_QUERY_STATUS_TEXT, Value);
}

//
//  KHttpCliRequest::Response::GetHeaderByWinHttpCode
//
NTSTATUS
KHttpCliRequest::Response::GetHeaderByWinHttpCode(
    __in DWORD WinHttpCode,
    __out KString::SPtr& Value
)
{
    NTSTATUS status;
    DWORD dwIndex, dwSize;
    BOOL bRes;

    // Get the content length
    //
    dwIndex = WINHTTP_NO_HEADER_INDEX;
    dwSize = 0;
    bRes = WinHttpQueryHeaders(_hRequest, WinHttpCode, WINHTTP_HEADER_NAME_BY_INDEX, NULL, &dwSize, &dwIndex);
    if ((! bRes) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        _WinHttpError = GetLastError();
        return STATUS_NOT_FOUND;
    }

    // Allocate the string
    //
    HRESULT hr;
    ULONG result;
    hr = ULongAdd( (dwSize / 2), 1, &result);
    KInvariant(SUCCEEDED(hr));
    Value = KString::Create(GetThisAllocator(), result);
    if (! Value)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, result, 0);
        return(status);
    }

    dwIndex = WINHTTP_NO_HEADER_INDEX;
    bRes = WinHttpQueryHeaders(_hRequest, WinHttpCode, WINHTTP_HEADER_NAME_BY_INDEX, PWSTR(*Value), &dwSize, &dwIndex);
    if (! bRes)
    {
        _WinHttpError = GetLastError();
        status = STATUS_INTERNAL_ERROR;

        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    Value->SetLength(dwSize / 2);

    return(STATUS_SUCCESS);
}

//
//  KHttpCliRequest::Response::GetHeaderByCode
//
NTSTATUS
KHttpCliRequest::Response::GetHeaderByCode(
    __in  KHttpUtils::HttpHeaderType HeaderCode,
    __out KString::SPtr& Value
    )
{
    DWORD winHttpCode = 0;

    if (! KHttpUtils::StandardCodeToWinHttpCode(HeaderCode, winHttpCode))
    {
        return STATUS_NOT_FOUND;
    }

    return GetHeaderByWinHttpCode(winHttpCode, Value);
}

//
//  KHttpCliRequest::Response::GetHeader
//
NTSTATUS
KHttpCliRequest::Response::GetHeader(
    __in  LPCWSTR  HeaderName,
    __out KString::SPtr& Value
    )
{
    NTSTATUS status;
    DWORD dwIndex, dwSize;
    BOOL bRes;

    // Get the content length
    //
    dwIndex = WINHTTP_NO_HEADER_INDEX;
    dwSize = 0;
    bRes = WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_CUSTOM, HeaderName, NULL, &dwSize, &dwIndex);

    if ((! bRes) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        _WinHttpError = GetLastError();
        return STATUS_NOT_FOUND;
    }

    // Allocate the string
    //
    // REVIEW: TyAdam, BharatN - safe code
    HRESULT hr;
    ULONG result;
    hr = ULongAdd( (dwSize / 2), 1, &result);
    KInvariant(SUCCEEDED(hr));
    Value = KString::Create(GetThisAllocator(), result);
    if (! Value)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, result, 0);
        return(status);
    }

    dwIndex = WINHTTP_NO_HEADER_INDEX;
    bRes = WinHttpQueryHeaders(_hRequest, WINHTTP_QUERY_CUSTOM, HeaderName, PWSTR(*Value), &dwSize, &dwIndex);
    if (! bRes)
    {
        _WinHttpError = GetLastError();
        status = STATUS_INTERNAL_ERROR;

        KTraceFailedAsyncRequest(status, NULL, _WinHttpError, (ULONGLONG)_hRequest);
        return(status);
    }

    Value->SetLength(dwSize / 2);
    return STATUS_SUCCESS;
}


//
//  KHttpCliRequest::SetTarget
//
NTSTATUS
KHttpCliRequest::SetTarget(
    __in  KNetworkEndpoint::SPtr Ep,
    __in  KHttpUtils::OperationType Op
    )
{
    KStringView OpStrView;
    KHttpUtils::OpToString(Op, OpStrView);

    return SetTarget(Ep, OpStrView);
}

//
//  KHttpCliRequest::SetTarget
//
NTSTATUS
KHttpCliRequest::SetTarget(
    __in  KNetworkEndpoint::SPtr Ep,
    __in  KStringView& OpStr
)
{
    if (OpStr.IsEmpty() || !Ep ||
        !is_type<KHttpNetworkEndpoint>(Ep.RawPtr()))
    {
        return STATUS_INVALID_PARAMETER;
    }

    NTSTATUS status = KString::Create(_Op, GetThisAllocator(), OpStr);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    _Url = Ep->GetUri();

    KStringView scheme = _Url->Get(KUriView::UriFieldType::eScheme);
    if (scheme.CompareNoCase(KStringView(KHttpUtils::HttpSchemes::Secure)) == 0)
    {
        _IsSecure = TRUE;
    }

    if (!_Url || _Url->IsEmpty() || !_Url->IsValid())
    {
        return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

//
// KHttpCliRequest::SetContent
//
//*
NTSTATUS
KHttpCliRequest::SetContent(
    __in KBuffer::SPtr& Buffer
    )
{
    KInvariant(! _IsMultiPart);

    if (! Buffer || Buffer->QuerySize() == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // See if the Contet is already set via some other method.
    //
    if (_ContentCarrier != KHttpUtils::eNone)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    _Content_KBuffer = Buffer;
    _ContentCarrier = KHttpUtils::eKBuffer;

    return STATUS_SUCCESS;
}

//
//  KHttpCliRequest::SetContent
//
NTSTATUS
KHttpCliRequest::SetContent(
        __in KMemRef& Mem
        )
{
    KInvariant(! _IsMultiPart);

    if (Mem._Address == nullptr || Mem._Size == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (_ContentCarrier != KHttpUtils::eNone)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    _Content_KMemRef = Mem;
    _ContentCarrier = KHttpUtils::eKMemRef;

    return STATUS_SUCCESS;
}

//
//  KHttpCliRequest::SetContent
//
NTSTATUS
KHttpCliRequest::SetContent(
    __in KStringView& StringContent
    )
{
    KInvariant(! _IsMultiPart);

    if (StringContent.IsEmpty())
    {
        return STATUS_INVALID_PARAMETER;
    }

    if (_ContentCarrier != KHttpUtils::eNone)
    {
        return STATUS_ILLEGAL_FUNCTION;
    }

    _Content_KStringView = StringContent;
    _ContentCarrier = KHttpUtils::eKStringView;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::SetClientCertificate(
    __in PCCERT_CONTEXT CertContext
)
{
    // If _ClientCert has been set before, free the context to avoid a leak
    if (_ClientCert)
    {
        CertFreeCertificateContext(_ClientCert);
    }

    // Duplicate the certificate context and store it. We are now responsible for releasing the context.
    _ClientCert = CertDuplicateCertificateContext(CertContext);
    if (!_ClientCert)
    {
        return STATUS_UNSUCCESSFUL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::SetServerCertValidationHandler(
    __in ServerCertValidationHandler Handler
    )
{
    _ServerCertValidationHandler = Handler;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::SetAllowRedirects(
    __in BOOLEAN AllowRedirects
)
{
    _AllowRedirects = AllowRedirects;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::SetEnableCookies(
    __in BOOLEAN EnableCookies
)
{
    _EnableCookies = EnableCookies;

    return STATUS_SUCCESS;
}

NTSTATUS
KHttpCliRequest::SetAutoLogonForAllRequests(
    __in BOOLEAN EnableAutoLogonForAllRequests
)
{
    _EnableAutoLogonForAllRequests = EnableAutoLogonForAllRequests;

    return STATUS_SUCCESS;
}

//
//  KHttpCliRequest::Execute
//
NTSTATUS
KHttpCliRequest::Execute(
    __out    Response::SPtr&  Response,
    __in_opt KAsyncContextBase* const ParentContext,
    __in_opt KAsyncContextBase::CompletionCallback Callback,
    __in     ULONG DefaultResponseBufSize
    )
{
    UNREFERENCED_PARAMETER(DefaultResponseBufSize);

    NTSTATUS status;

    // Check internals
    //
    if (!_Url || !_Op || _Op->IsEmpty())
    {
        return STATUS_BAD_NETWORK_NAME;
    }

    // If a Post/Put, verify content-length and that a carrier has been set.
    //
    if ((_Op->CompareNoCase(KHttpUtils::HttpVerb::POST) == 0 || _Op->CompareNoCase(KHttpUtils::HttpVerb::PUT) == 0) &&
        ((! _IsMultiPart) && (_ContentCarrier ==  KHttpUtils::eNone)))
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Create the result object. Response is single/multipart depending upon the request
    //
    Response = _new(KTL_TAG_HTTP, GetThisAllocator()) KHttpCliRequest::Response(_IsMultiPart);
    if (! Response)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, sizeof(KHttpCliRequest::Response), 0);
        return(status);
    }

    _Response = Response;

    Start(ParentContext, Callback);
    return(STATUS_PENDING);
}

//
//  KHttpCliRequest::WriteData
//
NTSTATUS
KHttpCliRequest::WriteData(
    __in KHttpUtils::ContentCarrierType ContentCarrier,
    __in KMemRef& ContentKMemRef,
    __in KStringView& ContentKStringView,
    __in_opt KBuffer* const ContentKBuffer
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PVOID Content = 0;
    ULONG ContentLength = 0;

    //
    // Grab data from the content carrier
    //
    if (ContentCarrier != KHttpUtils::eNone)
    {
        if (ContentCarrier == KHttpUtils::eKMemRef)
        {
            Content = ContentKMemRef._Address;
            ContentLength = ContentKMemRef._Param;
        }
        else if (ContentCarrier == KHttpUtils::eKStringView)
        {
            Content = PVOID(ContentKStringView);
            ContentLength = ContentKStringView.LengthInBytes();
        }
        else if (ContentCarrier == KHttpUtils::eKBuffer
            && ContentKBuffer != nullptr)
        {
            ContentLength = ContentKBuffer->QuerySize();
            Content = ContentKBuffer->GetBuffer();
        }
        else
        {
            //
            // Not completed yet cases for files, KMemChannel, etc.
            //
            status = STATUS_NOT_IMPLEMENTED;
            KTraceFailedAsyncRequest(status, this, ContentCarrier, NULL);
            Complete(status);
            return(status);
        }
    }

    //
    // Now write the data 
    //
    BOOL b = WinHttpWriteData(_hRequest,
                              Content,
                              ContentLength,
                              NULL);                // lpdwNumberBytesWritten
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;
        _WinHttpError = GetLastError();
        KTraceFailedAsyncRequest(status, this, _WinHttpError, (ULONGLONG)_hRequest);
        Complete(status);
        return(status);
    }

    return(status);
}

//
//  KHttpCliRequest::EndRequest
//
NTSTATUS
KHttpCliRequest::EndRequest(
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Now start receiving the response 
    //
    BOOL b =  WinHttpReceiveResponse(_hRequest,
                                     NULL);                // Reserved
    if (! b)
    {
        status = STATUS_INTERNAL_ERROR;
        _WinHttpError = GetLastError();
        KTraceFailedAsyncRequest(status, this, _WinHttpError, (ULONGLONG)_hRequest);
        Complete(status);
        return(status);
    }

    return(status);
}

//
//  KHttpCliRequest::WriteData
//
NTSTATUS
KHttpCliRequest::ReadData(
    )
{
    NTSTATUS status = STATUS_SUCCESS;

    //
    // Now start the receive data state machine
    //
    BOOL b =  WinHttpQueryDataAvailable(_hRequest,
                                        NULL);                // lpdwNumberOfBytesAvailable
    if (! b)
    {
        status = STATUS_INTERNAL_ERROR;
        _WinHttpError = GetLastError();
        KTraceFailedAsyncRequest(status, this, _WinHttpError, (ULONGLONG)_hRequest);
        Complete(status);
        return(status);
    }

    return(status);
}

VOID
KHttpCliRequest::FailRequest(
    NTSTATUS Status
    )
{
    //
    // Closing the winhttp request and the connection terminates the connection. This
    // happens in the destructor of the KHttpCliRequest, so just completing the request
    // here is sufficient for now.
    //

    if (_ASDRContext != nullptr)
    {
        _ASDRContext->Complete(Status);
    }

    Complete(Status);
}

//
// KHttpCliRequest::AsyncRequestDataContext
//
VOID
KHttpCliRequest::AsyncRequestDataContext::OnCompleted(
    )
{
    _RequestParentContext = nullptr;
    _Content_KBuffer = nullptr;
    _Request->_ASDRContext = nullptr;    // Break circular reference
    _ReadBuffer = nullptr;
    _Request = nullptr;                                      
}

VOID
KHttpCliRequest::AsyncRequestDataContext::CompleteRead(
    __in NTSTATUS Status
    )
{
    if (_ContentCarrier == KHttpUtils::ContentCarrierType::eKBuffer)
    {
        *_ReadContent = _ReadBuffer;
    }
    Complete(Status);
}

VOID 
KHttpCliRequest::AsyncRequestDataContext::SendRequestHeaderFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    Response::SPtr response;
    status = _Request->Execute(response,
                               _RequestParentContext.RawPtr(),
                               _RequestCallback);
    if (! NT_SUCCESS(status))
    {
        _Response = nullptr;
        KTraceFailedAsyncRequest(status, this, _Request, _RequestParentContext);
        Complete(status);
    }

    _Response->Attach(response.Detach());
}

VOID 
KHttpCliRequest::AsyncRequestDataContext::SendRequestDataFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    status = _Request->WriteData(_ContentCarrier, 
                                 _Content_KMemRef, 
                                 _Content_KStringView,
                                 _Content_KBuffer.RawPtr());
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _Request, _RequestParentContext);
        Complete(status);
    }
}

VOID 
KHttpCliRequest::AsyncRequestDataContext::EndRequestFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    status = _Request->EndRequest();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _Request, _RequestParentContext);
        Complete(status);
    }
}

VOID 
KHttpCliRequest::AsyncRequestDataContext::ReceiveResponseDataFSM(
    __in NTSTATUS Status
    )
{
    UNREFERENCED_PARAMETER(Status);

    NTSTATUS status;

    _ReadBuffer = nullptr;
    status = _Request->ReadData();

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, _Request, _RequestParentContext);
        Complete(status);
    }
}

VOID
KHttpCliRequest::AsyncRequestDataContext::DisconnectFromServerFSM(
    __in NTSTATUS Status
    )
{
    _Request->FailRequest(Status);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::OnStart(
    )
{
    KInvariant(_Request->_IsMultiPart);

    //
    // This establishes a circular reference between this async and the KHttpCliRequest. However
    // this circular reference is removed when this async is completed and thus should not cause
    // a memory leak.
    //
    _Request->_ASDRContext = this;

    switch(_State)
    {
        case SendRequestHeader:
        {
            SendRequestHeaderFSM(STATUS_SUCCESS);
            break;
        }

        case SendRequestData:
        {
            SendRequestDataFSM(STATUS_SUCCESS);
            break;
        }

        case EndRequest:
        {
            EndRequestFSM(STATUS_SUCCESS);
            break;
        }

        case ReceiveResponseData:
        {
            ReceiveResponseDataFSM(STATUS_SUCCESS);
            break;
        }

        case Disconnect:
        {
            DisconnectFromServerFSM(STATUS_SUCCESS);
            break;
        }

        default:
        {
            KInvariant(FALSE);
        }
    }
}

VOID
KHttpCliRequest::AsyncRequestDataContext::OnCancel(
    )
{
    //
    // There is not a way to cancel the underlying KHTTP operation
    //
    KTraceCancelCalled(this, FALSE, FALSE, 0);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartSendRequestHeader(
    __in KHttpCliRequest& Request,                                      
    __in ULONG ContentLength,
    __out KHttpCliRequest::Response::SPtr&  Response,
    __in_opt KAsyncContextBase* const RequestParentContext,
    __in_opt KAsyncContextBase::CompletionCallback RequestCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr
    )
{
    _State = SendRequestHeader;
    _Request = &Request;
    _ContentLength = ContentLength;
    _RequestParentContext = RequestParentContext;
    _RequestCallback = RequestCallback;
    _Response = &Response;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartSendRequestDataContent(
    __in KHttpCliRequest& Request,                                      
    __in KBuffer::SPtr& Buffer,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendRequestData;
    _Request = &Request;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKBuffer;
    _Content_KBuffer = Buffer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartSendRequestDataContent(
    __in KHttpCliRequest& Request,                                      
    __in KMemRef& Mem,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendRequestData;
    _Request = &Request;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKMemRef;
    _Content_KMemRef = Mem;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartSendRequestDataContent(
    __in KHttpCliRequest& Request,                                      
    __in KStringView& StringContent,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = SendRequestData;
    _Request = &Request;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKStringView;
    _Content_KStringView = StringContent;

    Start(ParentAsyncContext, CallbackPtr);
}
            
VOID
KHttpCliRequest::AsyncRequestDataContext::StartEndRequest(
    __in KHttpCliRequest& Request,                                      
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = EndRequest;
    _Request = &Request;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartReceiveResponseData(
    __in KHttpCliRequest& Request, 
    __in ULONG MaxBytesToRead,
    __out KBuffer::SPtr& ReadContent,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = ReceiveResponseData;
    _Request = &Request;
    _MaxBytesToRead = MaxBytesToRead;
    _ReadContent = &ReadContent;
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKBuffer;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartReceiveResponseData(
    __in KHttpCliRequest& Request, 
    __inout KMemRef* Mem,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    _State = ReceiveResponseData;
    _Request = &Request;
    _MaxBytesToRead = Mem->_Size;
    _ReadContent_KMemRef = Mem;
    _ReadContent_KMemRef->_Param = 0; // _Param is set to the number of bytes read during read completion
    _ContentCarrier = KHttpUtils::ContentCarrierType::eKMemRef;

    Start(ParentAsyncContext, CallbackPtr);
}

VOID
KHttpCliRequest::AsyncRequestDataContext::StartDisconnect(
    __in KHttpCliRequest& Request,
    __in NTSTATUS Status,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncContextBase::CompletionCallback CallbackPtr)
{
    UNREFERENCED_PARAMETER(Status);

    _State = Disconnect;
    _Request = &Request;

    Start(ParentAsyncContext, CallbackPtr);
}

NTSTATUS
KHttpCliRequest::AsyncRequestDataContext::GetOrAllocateReadBuffer(
    __in ULONG Size,
    __out PVOID *Address
    )
{
    if (_ContentCarrier == KHttpUtils::ContentCarrierType::eKBuffer)
    {
        NTSTATUS status = KBuffer::Create(Size, _ReadBuffer, GetThisAllocator());
        if (!NT_SUCCESS(status))
        {
            return status;
        }

        *Address = _ReadBuffer->GetBuffer();
        return status;
    }
    else if (_ContentCarrier == KHttpUtils::ContentCarrierType::eKMemRef)
    {
        KAssert(Size <= _ReadContent_KMemRef->_Size);
        _ReadContent_KMemRef->_Param = Size;
        *Address = _ReadContent_KMemRef->_Address;
        return STATUS_SUCCESS;
    }

    KAssert(false);

    // should not get here
    return STATUS_UNSUCCESSFUL;
}

VOID
KHttpCliRequest::AsyncRequestDataContext::Initialize(
    )
{
    _State = Unassigned;

    _Request = nullptr;
    _ContentLength = 0;
    _Response = NULL;
    _RequestParentContext = nullptr;
    _RequestCallback = NULL;

    _ContentCarrier = KHttpUtils::ContentCarrierType::eNone;
    _Content_KBuffer = nullptr;
    _Content_KMemRef._Address = NULL;
    _Content_KBuffer = nullptr;

    _MaxBytesToRead = 0;
    _ReadContent = NULL;
    _ReadBuffer = nullptr;
    _ReadContent_KMemRef = nullptr;
}

VOID
KHttpCliRequest::AsyncRequestDataContext::OnReuse(
    )
{
    Initialize();
}

KHttpCliRequest::AsyncRequestDataContext::AsyncRequestDataContext()
{
    Initialize();
}

KHttpCliRequest::AsyncRequestDataContext::~AsyncRequestDataContext()
{
}

NTSTATUS
KHttpCliRequest::AsyncRequestDataContext::Create(
    __out AsyncRequestDataContext::SPtr& Context,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    AsyncRequestDataContext::SPtr context;

    context = _new(AllocationTag, Allocator) AsyncRequestDataContext();
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


//
//  KHttpCliRequest::OnStart
//
VOID
KHttpCliRequest::OnStart()
{
    // Execute this on a work item thread
    //
    _Continue.Bind(*this, &KHttpCliRequest::Deferred_Continue);
    _Continue();
}

//
//  KHttpCliRequest::Deferred_Continue
//
VOID
KHttpCliRequest::Deferred_Continue()
{
    BOOL bRes;

    // Make sure the KHttpClient is initialized
    //
    NTSTATUS Res = _Parent->WaitStartup();
    if (! NT_SUCCESS(Res))
    {
        if (_IsMultiPart)
        {
            KInvariant(_ASDRContext);
            _ASDRContext->Complete(Res);
        }

        KTraceFailedAsyncRequest(Res, this, 0, _IsMultiPart);
        Complete(Res);
        return;
    }

    // Temporary work string for misc. parsing/stringbuilding later.
    // This can hold a maximum header id/value pair, a max host name, or a max URL.
    // This dynamic string resizes if the field being copied is > DefaultFieldLength of 16KB
    //
    KDynString Scratch(GetThisAllocator(), KHttpUtils::DefaultFieldLength);
    if (! Scratch)
    {
        Res = STATUS_INSUFFICIENT_RESOURCES;
        if (_IsMultiPart)
        {
            KInvariant(_ASDRContext);
            _ASDRContext->Complete(Res);
        }

        KTraceOutOfMemory(0, Res, NULL, KHttpUtils::DefaultFieldLength, 0);
        Complete(Res);
        return;
    }

    // Get the server name & port.
    //
    INTERNET_PORT Port = (INTERNET_PORT) _Url->GetPort();
    if (Port == 0)
    {
        Port = INTERNET_DEFAULT_PORT;
    }

    // These were all checked earlier.
    //
    KStringView HostName = _Url->Get(KUri::eHost);  // Not null-terminated, so we have to copy
    KAssert(HostName.Length() > 0);
    KAssert(HostName.Length() <= KHttpUtils::MaxHostName);

    Scratch.CopyFrom(HostName);
    Scratch.SetNullTerminator();

    _hConnection = WinHttpConnect(_Parent->GetHandle(), LPCWSTR(Scratch), Port, 0);
    if (!_hConnection)
    {
        // CONSIDER: Map WinHttp errors to reasonable NTSTATUS
        _WinHttpError = GetLastError();

        Res = STATUS_UNSUCCESSFUL;
        if (_IsMultiPart)
        {
            KInvariant(_ASDRContext);
            _ASDRContext->Complete(Res);
        }

        KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
        Complete(Res);
        return;
    }

    // Now, create a request.
    //
    // Note that the path segment must be prefixed with a slash
    // according to HTTP rules.
    //
    KStringView PathView = _Url->Get(KUri::ePath);
    Scratch.Clear();
    Scratch.AppendChar(L'/');
    Scratch.Concat(PathView);
    if (_Url->Has(KUri::eQueryString))
    {
        Scratch.AppendChar(L'?');
        Scratch.Concat(_Url->Get(KUri::eQueryString));
    }

    if (_Url->Has(KUri::eFragment))
    {
        Scratch.AppendChar(L'#');
        Scratch.Concat(_Url->Get(KUri::eFragment));
    }

    Scratch.SetNullTerminator();

    _hRequest = WinHttpOpenRequest(_hConnection, LPCWSTR(*_Op), LPCWSTR(Scratch), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, _IsSecure ? WINHTTP_FLAG_SECURE : 0);
    if (!_hRequest)
    {
        // CONSIDER: Convert WinHttp error into something useful
        _WinHttpError = GetLastError();
        Res = STATUS_UNSUCCESSFUL;
        if (_IsMultiPart)
        {
            KInvariant(_ASDRContext);
            _ASDRContext->Complete(Res);
        }

        KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
        Complete(Res);
        return;
    }

    // Propagate this handle to the response object
    //
    _Response->_hRequest = _hRequest;

    if (_IsSecure)
    {
        // If user has specified a custom server certificate validation handler, 
        // have WinHttp ignore some errors so that the validation callback can run.
        if (_ServerCertValidationHandler != NULL)
        {
            DWORD sslFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA
                | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

            bRes = WinHttpSetOption(
                _hRequest,
                WINHTTP_OPTION_SECURITY_FLAGS,
                &sslFlags,
                sizeof(sslFlags));
            if (!bRes)
            {
                _WinHttpError = GetLastError();
                Res = STATUS_UNSUCCESSFUL;
                if (_IsMultiPart)
                {
                    KInvariant(_ASDRContext);
                    _ASDRContext->Complete(Res);
                }

                KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
                Complete(Res);
                return;
            }
        }

        // If client certificate is specified, set it on the request
        if (_ClientCert != NULL)
        {
            // This function operates synchronously even in async mode, it is not a blocking call.
            BOOL bRes1 = WinHttpSetOption(
                _hRequest,
                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                (LPVOID)_ClientCert,
                sizeof(CERT_CONTEXT));

            if (!bRes1)
            {
                _WinHttpError = GetLastError();
                Res = STATUS_UNSUCCESSFUL;
                if (_IsMultiPart)
                {
                    KInvariant(_ASDRContext);
                    _ASDRContext->Complete(Res);
                }

                KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
                Complete(Res);
                return;
            }
        }
    }

    // Set the disable redirect policy if specified.
    if (!_AllowRedirects)
    {
        DWORD dwOptionValue = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

        bRes = WinHttpSetOption(
            _hRequest,
            WINHTTP_OPTION_REDIRECT_POLICY,
            &dwOptionValue,
            sizeof(dwOptionValue));
        if (!bRes)
        {
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }

    // Set the disable cookies option if specified.
    if (!_EnableCookies)
    {
        DWORD dwOptionValue = WINHTTP_DISABLE_COOKIES;

        bRes = WinHttpSetOption(
            _hRequest,
            WINHTTP_OPTION_DISABLE_FEATURE,
            &dwOptionValue,
            sizeof(dwOptionValue));
        if (!bRes)
        {
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }

    if (_EnableAutoLogonForAllRequests)
    {
        DWORD dwOptionValue = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

        bRes = WinHttpSetOption(
            _hRequest,
            WINHTTP_OPTION_AUTOLOGON_POLICY,
            &dwOptionValue,
            sizeof(dwOptionValue));
        if (!bRes)
        {
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }

        bRes = WinHttpSetCredentials(
            _hRequest,
            WINHTTP_AUTH_TARGET_SERVER,
            WINHTTP_AUTH_SCHEME_NEGOTIATE,
            NULL,
            NULL,
            NULL);
        if (!bRes)
        {
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }

    // Set the request timeout values
    //
    bRes = WinHttpSetTimeouts(_hRequest, _resolveTimeoutInMilliSeconds, _connectTimeoutInMilliSeconds, _sendTimeoutInMilliSeconds, _receiveTimeoutInMilliSeconds);
    if (!bRes)
    {
        // CONSIDER: Convert WinHttp error into something useful
        _WinHttpError = GetLastError();
        Res = STATUS_UNSUCCESSFUL;
        if (_IsMultiPart)
        {
            KInvariant(_ASDRContext);
            _ASDRContext->Complete(Res);
        }

        KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
        Complete(Res);
        return;
    }

    // Now add in the headers.
    //
    for (ULONG i = 0; i < _Headers.Count(); i++)
    {
        Scratch.Clear();
        Scratch.Concat(_Headers[i]->_Id);
        Scratch.Concat(KStringView(L": "));
        Scratch.Concat(_Headers[i]->_Value);
        Scratch.Concat(KStringView(L"\r\n"));
        Scratch.SetNullTerminator();

        // This function copies the headers, so there is no need to retain the memory.
        //
        bRes = WinHttpAddRequestHeaders(_hRequest, LPWSTR(Scratch), Scratch.Length(), WINHTTP_ADDREQ_FLAG_ADD);
        if (! bRes)
        {
            // CONSIDER: Convert WinHttp error into something useful
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }

    if (_HeadersBlob)
    {
        bRes = WinHttpAddRequestHeaders(
            _hRequest,
            *_HeadersBlob,
            _HeadersBlob->Length(),
            WINHTTP_ADDREQ_FLAG_ADD);
        if (!bRes)
        {
            // CONSIDER: Convert WinHttp error into something useful
            _WinHttpError = GetLastError();
            Res = STATUS_UNSUCCESSFUL;
            if (_IsMultiPart)
            {
                KInvariant(_ASDRContext);
                _ASDRContext->Complete(Res);
            }

            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }

    if (_IsMultiPart)
    {
        //
        // Fire off the headers
        //
        KInvariant(_ASDRContext);

        bRes = WinHttpSendRequest(_hRequest,
            WINHTTP_NO_ADDITIONAL_HEADERS,
            0,                                    // dwHeadersLength
            WINHTTP_NO_REQUEST_DATA,
            0,                                    // dwOptionalLength
            _ASDRContext->GetContentLength(),
            DWORD_PTR(this));
        if (! bRes)
        {
            Res = STATUS_UNSUCCESSFUL;

            _WinHttpError = GetLastError();
            _ASDRContext->Complete(Res);
            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }
    }
    else
    {
        //
        // For single part messages, continue and set the content.
        //
        KInvariant(! _ASDRContext);

        PVOID Content = 0;
        ULONG ContentLength = 0;

        if (_ContentCarrier != KHttpUtils::eNone)
        {
            if (_ContentCarrier == KHttpUtils::eKMemRef)
            {
                Content = _Content_KMemRef._Address;
                ContentLength = _Content_KMemRef._Param;
            }
            else if (_ContentCarrier == KHttpUtils::eKStringView)
            {
                Content = PVOID(_Content_KStringView);
                ContentLength = _Content_KStringView.LengthInBytes();
            }
            else if (_ContentCarrier == KHttpUtils::eKBuffer)
            {
                ContentLength = _Content_KBuffer->QuerySize();
                Content = _Content_KBuffer->GetBuffer();
            }
            else
            {
                // Not completed yet cases for files, KMemChannel, etc.
                //
                KInvariant(FALSE);
                Complete(STATUS_NOT_IMPLEMENTED);
                return;
            }
        }

        // Fire off the request.
        //
        bRes = WinHttpSendRequest(_hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, Content, ContentLength, ContentLength, DWORD_PTR(this));
        if (! bRes)
        {
            Res = STATUS_UNSUCCESSFUL;
            _WinHttpError = GetLastError();
            KTraceFailedAsyncRequest(Res, this, _WinHttpError, _IsMultiPart);
            Complete(Res);
            return;
        }

        // If here, we let the WinHTTP callback bring us back into this object asynchronously.
    }
}
#pragma endregion KHttpCliRequest


#if NTDDI_VERSION >= NTDDI_WIN8

KHttpClientWebSocket::KHttpClientWebSocket(
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
    )
    :
        KHttpWebSocket(ReceiveOperationQueue, SendOperationQueue, AbortAsync, ActiveSubprotocol),

        _ReceiveOperationProcessor(*this, ReceiveRequestDequeueOperation, WebSocketReceiveOperation),
        _SendRequestDequeueOperation(&SendRequestDequeueOperation),
        _DeferredServiceOpenAsync(&ServiceOpenAsync),
        _WebSocketSendOperation(&WebSocketSendOperation),
        _SendRequestOperation(&SendRequestOperation),
        _ReceiveResponseOperation(&ReceiveResponseOperation),
        _SendCloseOperation(&SendCloseOperation),
        _RemoteCloseReasonBuffer(&RemoteCloseReasonBuffer),

        _TargetURI(nullptr),
        _TransportReceiveBufferSize(0),
        _RequestedSubprotocols(nullptr),
        _Proxy(nullptr),
        _AdditionalHeaders(nullptr),
        _AdditionalHeadersBlob(nullptr),
        _Secure(FALSE),
        _ClientCert(nullptr),
        _AllowRedirects(TRUE),
        _EnableCookies(TRUE),
        _EnableAutoLogonForAllRequests(FALSE),

        _WinhttpWebSocketHandle(nullptr),
        _WinhttpRequestHandle(nullptr),
        _WinhttpConnectionHandle(nullptr),
        _WinhttpSessionHandle(nullptr),

        _IsReceiveRequestQueueDeactivated(FALSE),
        _IsSendRequestQueueDeactivated(FALSE),

        _CurrentSendOperation(nullptr),

        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::~KHttpClientWebSocket()
{
}

NTSTATUS
KHttpClientWebSocket::Create(
    __out KHttpClientWebSocket::SPtr& WebSocket,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    KHttpClientWebSocket::SPtr webSocket;

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

    WinhttpWebSocketReceiveOperation::SPtr webSocketReceiveOperation;
    status = WinhttpWebSocketReceiveOperation::Create(webSocketReceiveOperation, Allocator, AllocationTag);
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

    DeferredServiceOpenAsync::SPtr deferredServiceOpenAsync;
    status = DeferredServiceOpenAsync::Create(deferredServiceOpenAsync, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    WinhttpWebSocketSendOperation::SPtr webSocketSendOperation;
    status = WinhttpWebSocketSendOperation::Create(webSocketSendOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    WinhttpSendRequestOperation::SPtr sendRequestOperation;
    status = WinhttpSendRequestOperation::Create(sendRequestOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    WinhttpReceiveResponseOperation::SPtr receiveResponseOperation;
    status = WinhttpReceiveResponseOperation::Create(receiveResponseOperation, Allocator, AllocationTag);
    if (! NT_SUCCESS(status))
    {
        KTraceOutOfMemory(0, status, nullptr, NULL, NULL);
        return status;
    }

    KHttpClientSendCloseOperation::SPtr sendCloseOperation;
    status = KHttpClientSendCloseOperation::Create(sendCloseOperation, Allocator, AllocationTag);
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


    KBuffer::SPtr remoteCloseReasonBuffer;
    status = KBuffer::Create(MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES, remoteCloseReasonBuffer, Allocator, AllocationTag);
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

    webSocket = _new(AllocationTag, Allocator) KHttpClientWebSocket(
        *receiveOperationQueue,
        *receiveDequeueOperation,
        *sendOperationQueue,
        *sendDequeueOperation,
        *webSocketReceiveOperation,
        *deferredServiceOpenAsync,
        *webSocketSendOperation,
        *sendRequestOperation,
        *receiveResponseOperation,
        *sendCloseOperation,
        *abortAsync,
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

NTSTATUS
KHttpClientWebSocket::SetTimingConstant(
    __in TimingConstant Constant,
    __in TimingConstantValue Value
    )
{
    //
    //  These constants are unsupported, as WinHttp does not expose the sending of PING frames.
    //
    if (Constant == TimingConstant::PingQuietChannelPeriodMs
        || Constant == TimingConstant::PongTimeoutMs
        )
    {
        return K_STATUS_OPTION_UNSUPPORTED;
    }

    return KWebSocket::SetTimingConstant(Constant, Value);
}

VOID CALLBACK
KHttpClientWebSocket::WinHttpCallback(
    __in  HINTERNET hInternet,
    __in  ULONG_PTR dwContext,
    __in  ULONG dwInternetStatus,
    __in  LPVOID lpvStatusInformation,
    __in  ULONG dwStatusInformationLength
    )
{
    UNREFERENCED_PARAMETER(dwStatusInformationLength);

    if (! dwContext)
    {
        return;
    }

    NTSTATUS status;

    KHttpClientWebSocket& webSocket = *reinterpret_cast<KHttpClientWebSocket*>(dwContext);

    switch (dwInternetStatus)
    {
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:

        if (webSocket._SendRequestOperation->IsActive())
        {
            WINHTTP_ASYNC_RESULT* winhttpResult = static_cast<WINHTTP_ASYNC_RESULT*>(lpvStatusInformation);

            KAssert(!webSocket._ReceiveResponseOperation->IsActive());
            KAssert(hInternet == webSocket._WinhttpRequestHandle);

            status = winhttpResult->dwError == ERROR_WINHTTP_NAME_NOT_RESOLVED ? STATUS_INVALID_ADDRESS : K_STATUS_WINHTTP_ERROR;

            KDbgCheckpointWData(0, "WinHTTP http request send failed.", status, dwContext, winhttpResult->dwError, winhttpResult->dwResult, 0);

            webSocket._SendRequestOperation->_WinhttpError = winhttpResult->dwError;
            webSocket._SendRequestOperation->Complete(status);

            return;
        }
        else if (webSocket._ReceiveResponseOperation->IsActive())
        {
            WINHTTP_ASYNC_RESULT* winhttpResult = static_cast<WINHTTP_ASYNC_RESULT*>(lpvStatusInformation);

            KAssert(!webSocket._SendRequestOperation->IsActive());
            KAssert(hInternet == webSocket._WinhttpRequestHandle);

            status = K_STATUS_WINHTTP_ERROR;

            KDbgCheckpointWData(0, "WinHTTP receive response failed.", status, dwContext, winhttpResult->dwError, winhttpResult->dwResult, 0);

            webSocket._ReceiveResponseOperation->_WinhttpError = winhttpResult->dwError;
            webSocket._ReceiveResponseOperation->Complete(status);

            return;
        }
        else
        {
            //
            //  Some Winhttp websocket asynchronous operation failed.  We know which operation failed by mapping the dwResult value
            //  to the type of outstanding operation (as there may only be one send/receive/etc. outstanding at a time).
            //

            WINHTTP_WEB_SOCKET_ASYNC_RESULT& result = *reinterpret_cast<WINHTTP_WEB_SOCKET_ASYNC_RESULT*>(lpvStatusInformation);

            KAssert(hInternet == webSocket._WinhttpWebSocketHandle);

            switch (result.Operation)
            {
            case WINHTTP_WEB_SOCKET_SEND_OPERATION:

                KDbgCheckpointWData(
                    0,
                    "WinHttpWebSocketSend failed.",
                    K_STATUS_WINHTTP_ERROR,
                    result.AsyncResult.dwError,
                    result.AsyncResult.dwResult,
                    0,
                    0
                    );

                webSocket._WebSocketSendOperation->_BytesSent = 0;
                webSocket._WebSocketSendOperation->_WinhttpError = result.AsyncResult.dwError;
                webSocket._WebSocketSendOperation->Complete(K_STATUS_WINHTTP_ERROR);

                return;

            case WINHTTP_WEB_SOCKET_RECEIVE_OPERATION:

                KDbgCheckpointWData(
                    0,
                    "WinHttpWebSocketReceive failed.",
                    K_STATUS_WINHTTP_ERROR,
                    result.AsyncResult.dwError,
                    result.AsyncResult.dwResult,
                    0,
                    0
                    );

                webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->_BytesReceived = 0;
                webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->_WinhttpError = result.AsyncResult.dwError;
                webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->Complete(K_STATUS_WINHTTP_ERROR);

                return;

            case WINHTTP_WEB_SOCKET_SHUTDOWN_OPERATION:

                KDbgCheckpointWData(
                    0,
                    "WinHttpWebSocketShutdown failed.",
                    K_STATUS_WINHTTP_ERROR,
                    result.AsyncResult.dwError,
                    result.AsyncResult.dwResult,
                    0,
                    0
                    );

                webSocket._SendCloseOperation->_WinhttpError = result.AsyncResult.dwError;
                webSocket._SendCloseOperation->Complete(K_STATUS_WINHTTP_ERROR);

                return;

            case WINHTTP_WEB_SOCKET_CLOSE_OPERATION:

                //
                //  WinHttpWebSocketClose is unused
                //

            default:

                KInvariant(FALSE);  //  impossible
            }
        }

    case WINHTTP_CALLBACK_STATUS_SENDING_REQUEST:
        {
            // If request is over HTTPS and server certificate validation handler exists, call it now and decide whether to fail/succeed the request
            if (webSocket._Secure
                && webSocket._ServerCertValidationHandler != NULL)
            {
                BOOLEAN bRet = webSocket.ValidateServerCertificate();

                // If certificate handler asks to fail the request, do so.
                if (bRet == FALSE)
                {
                    webSocket._SendRequestOperation->_WinhttpError = ERROR_WINHTTP_SECURE_INVALID_CERT;
                    // Cancel/Terminate the in-progress request.
                    webSocket._SendRequestOperation->Complete(K_STATUS_WINHTTP_ERROR);
                    return;
                }
            }
        }

        break;

    case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
        //
        //  The request completed successfully. The lpvStatusInformation parameter is NULL.
        //
        {
            KAssert(webSocket._SendRequestOperation->IsActive());
            KAssert(!webSocket._ReceiveResponseOperation->IsActive());
            KAssert(hInternet == webSocket._WinhttpRequestHandle);

            webSocket._SendRequestOperation->_WinhttpError = NO_ERROR;
            webSocket._SendRequestOperation->Complete(STATUS_SUCCESS);

            return;
        }

        break;

    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
        //
        //  The response headers have been received and are available with WinHttpQueryHeaders 
        //
        {
            KAssert(!webSocket._SendRequestOperation->IsActive());
            KAssert(webSocket._ReceiveResponseOperation->IsActive());
            KAssert(hInternet == webSocket._WinhttpRequestHandle);

            webSocket._ReceiveResponseOperation->_WinhttpError = NO_ERROR;
            webSocket._ReceiveResponseOperation->Complete(STATUS_SUCCESS);
        }

        return;

    case WINHTTP_CALLBACK_STATUS_WRITE_COMPLETE:
        //
        //  WinHttpWebSocketSend successfully sent data
        //
        {
            if (webSocket._SendRequestOperation->IsActive() || webSocket._ReceiveResponseOperation->IsActive())
            {
                return;
            }

            WINHTTP_WEB_SOCKET_STATUS& result = *reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(lpvStatusInformation);

            KAssert(hInternet == webSocket._WinhttpWebSocketHandle);
            
            webSocket._WebSocketSendOperation->_BytesSent = result.dwBytesTransferred;
            webSocket._WebSocketSendOperation->Complete(STATUS_SUCCESS);
        }

        return;

    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
        //
        //  WinHttpWebSocketReceive successfully received data
        //
        {
            if (webSocket._SendRequestOperation->IsActive() || webSocket._ReceiveResponseOperation->IsActive())
            {
                return;
            }

            WINHTTP_WEB_SOCKET_STATUS& result = *reinterpret_cast<WINHTTP_WEB_SOCKET_STATUS*>(lpvStatusInformation);

            KAssert(hInternet == webSocket._WinhttpWebSocketHandle);

            FrameType frameType = WebSocketBufferTypeToFrameType(result.eBufferType);
            KInvariant(frameType != FrameType::Invalid);

            webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->_BytesReceived = result.dwBytesTransferred;
            webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->_FrameType = frameType;
            webSocket._ReceiveOperationProcessor._WebSocketReceiveOperation->Complete(STATUS_SUCCESS);
        }

        return;

    case WINHTTP_CALLBACK_STATUS_SHUTDOWN_COMPLETE:
        //
        //  WinHttpWebSocketShutdown has successfully sent the close frame and closed the send channel
        //
        {
            KAssert(hInternet == webSocket._WinhttpWebSocketHandle);
            KAssert(!webSocket._SendRequestOperation->IsActive() && !webSocket._ReceiveResponseOperation->IsActive());
            KAssert(webSocket._SendCloseOperation->IsActive());

            webSocket._SendCloseOperation->Complete(STATUS_SUCCESS);
        }
        
        return;

    case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
        //
        //  The WebSocket is cleaning up, and the handles are reporting their closure.
        //
        {
            if (hInternet == webSocket._WinhttpRequestHandle)
            {
                webSocket._WinhttpRequestHandle = nullptr;
                webSocket.ReleaseActivities();

                KDbgCheckpointWData(0, "WinHTTP request handle closing.", STATUS_SUCCESS, dwContext, ULONG_PTR(hInternet), 0, 0);
            }
            else if (hInternet == webSocket._WinhttpWebSocketHandle)
            {
                webSocket._WinhttpWebSocketHandle = nullptr;
                webSocket.ReleaseActivities();

                KDbgCheckpointWData(0, "WinHTTP websocket handle closing.", STATUS_SUCCESS, dwContext, ULONG_PTR(hInternet), 0, 0);
            }
            else if (hInternet == webSocket._WinhttpConnectionHandle)
            {
                webSocket._WinhttpConnectionHandle = nullptr;
                webSocket.ReleaseActivities();

                KDbgCheckpointWData(0, "WinHTTP connection handle closing.", STATUS_SUCCESS, dwContext, ULONG_PTR(hInternet), 0, 0);
            }
            else if (hInternet == webSocket._WinhttpSessionHandle)
            {
                webSocket._WinhttpSessionHandle = nullptr;
                webSocket.ReleaseActivities();

                KDbgCheckpointWData(0, "WinHTTP session handle closing.", STATUS_SUCCESS, dwContext, ULONG_PTR(hInternet), 0, 0);
            }
        }

        return;

    case WINHTTP_CALLBACK_STATUS_CLOSE_COMPLETE:
        //
        //  WinHttpWebSocketClose is unused
        //
        KInvariant(FALSE);

    default:

        // unhandled cases
        return;
    }
}

BOOLEAN
KHttpClientWebSocket::ValidateServerCertificate()
{
    PCERT_CONTEXT pCertContext = NULL;
    DWORD certContextSize = sizeof(pCertContext);
    BOOLEAN isValid = FALSE;

    // Obtain the server certificate details:
    if (!WinHttpQueryOption(_WinhttpRequestHandle, WINHTTP_OPTION_SERVER_CERT_CONTEXT, &pCertContext, &certContextSize))
    {
        // Failed to get server certificate.
        return FALSE;
    }

    // Call the user provided handler to validate cert.
    isValid = _ServerCertValidationHandler(pCertContext);

    // Free the PCCERT_CONTEXT pointer that is filled into the buffer.
    CertFreeCertificateContext(pCertContext);

    return isValid;
}

WINHTTP_WEB_SOCKET_BUFFER_TYPE
KHttpClientWebSocket::GetWebSocketBufferType(
    __in MessageContentType const & ContentType,
    __in BOOLEAN const & IsFinalFragment
    )
{
    switch (ContentType)
    {
    case MessageContentType::Binary:
        return IsFinalFragment ? WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE : WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE;
    case MessageContentType::Text:
        return IsFinalFragment ? WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE : WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE;
    default:
        return static_cast<WINHTTP_WEB_SOCKET_BUFFER_TYPE>(-1);
    }
}

KWebSocket::FrameType
KHttpClientWebSocket::WebSocketBufferTypeToFrameType(
    __in WINHTTP_WEB_SOCKET_BUFFER_TYPE BufferType
    )
{
    switch (BufferType)
    {
    case WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE:
        return FrameType::BinaryFinalFragment;
    case WINHTTP_WEB_SOCKET_BINARY_FRAGMENT_BUFFER_TYPE:
        return FrameType::BinaryContinuationFragment;
    case WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE:
        return FrameType::TextFinalFragment;
    case WINHTTP_WEB_SOCKET_UTF8_FRAGMENT_BUFFER_TYPE:
        return FrameType::TextContinuationFragment;
    case WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE:
        return FrameType::Close;
       
    default:
        return FrameType::Invalid;
    }
}

ULONG
KHttpClientWebSocket::GetWinhttpError()
{
    return _WinhttpError;
}

//
//  KHttpCliRequest::Response::GetHeader
//
NTSTATUS
KHttpClientWebSocket::GetHttpResponseHeader(
    __in  LPCWSTR  HeaderName,
    __out KString::SPtr& Value
    )
{
    NTSTATUS status;
    DWORD dwIndex, dwSize;
    BOOL bRes;

    // Get the content length
    //
    dwIndex = WINHTTP_NO_HEADER_INDEX;
    dwSize = 0;
    bRes = WinHttpQueryHeaders(_WinhttpRequestHandle, WINHTTP_QUERY_CUSTOM, HeaderName, NULL, &dwSize, &dwIndex);

    if ((!bRes) && (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
        _WinhttpError = GetLastError();
        return STATUS_NOT_FOUND;
    }

    // Allocate the string
    //
    HRESULT hr;
    ULONG result;
    hr = ULongAdd((dwSize / 2), 1, &result);
    KInvariant(SUCCEEDED(hr));
    Value = KString::Create(GetThisAllocator(), result);
    if (!Value)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        KTraceOutOfMemory(0, status, this, result, 0);
        return(status);
    }

    dwIndex = WINHTTP_NO_HEADER_INDEX;
    bRes = WinHttpQueryHeaders(_WinhttpRequestHandle, WINHTTP_QUERY_CUSTOM, HeaderName, PWSTR(*Value), &dwSize, &dwIndex);
    if (!bRes)
    {
        _WinhttpError = GetLastError();
        status = STATUS_INTERNAL_ERROR;

        KTraceFailedAsyncRequest(status, NULL, _WinhttpError, (ULONGLONG)_WinhttpRequestHandle);
        return(status);
    }

    Value->SetLength(dwSize / 2);
    return STATUS_SUCCESS;
}

class KHttpClientWebSocket::KHttpClientSendFragmentOperation : public SendFragmentOperation, public WebSocketSendFragmentOperation
{
    K_FORCE_SHARED(KHttpClientSendFragmentOperation);
    friend class KHttpClientWebSocket;

public:

    KDeclareSingleArgumentCreate(
        SendFragmentOperation,      //  Output SPtr type
        KHttpClientSendFragmentOperation,  //  Implementation type
        SendOperation,              //  Output arg name
        KTL_TAG_WEB_SOCKET,         //  default allocation tag
        KHttpClientWebSocket&,      //  Constructor arg type
        WebSocketContext            //  Constructor arg name
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

    KHttpClientSendFragmentOperation(
        __in KHttpClientWebSocket& Context
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
};

class KHttpClientWebSocket::KHttpClientSendMessageOperation : public SendMessageOperation, public WebSocketSendFragmentOperation
{
    K_FORCE_SHARED(KHttpClientSendMessageOperation);
    friend class KHttpClientWebSocket;

public:

    KDeclareSingleArgumentCreate(
        SendMessageOperation,      //  Output SPtr type
        KHttpClientSendMessageOperation,  //  Implementation type
        SendOperation,              //  Output arg name
        KTL_TAG_WEB_SOCKET,         //  default allocation tag
        KHttpClientWebSocket&,      //  Constructor arg type
        WebSocketContext            //  Constructor arg name
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

    KHttpClientSendMessageOperation(
        __in KHttpClientWebSocket& Context
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
};


#pragma region KHttpClientWebSocket_Receive
KHttpClientWebSocket::ReceiveOperationProcessor::ReceiveOperationProcessor(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncQueue<WebSocketReceiveOperation>::DequeueOperation& RequestDequeueOperation,
    __in WinhttpWebSocketReceiveOperation& WebSocketReceiveOperation
    )
    :
        _ParentWebSocket(ParentWebSocket),
        _RequestDequeueOperation(&RequestDequeueOperation),
        _WebSocketReceiveOperation(&WebSocketReceiveOperation),

        _TransportReceiveBuffer(nullptr),
        _DataOffset(0),
        _CurrentOperation(nullptr),
        _ReceivedContentType(MessageContentType::Invalid),
        _ReceivedFragmentIsFinal(),
        _BytesReceived(0)
{
}

VOID
KHttpClientWebSocket::ReceiveOperationProcessor::FSM()
{
    KAssert(_ParentWebSocket.IsInApartment());
    KAssert(! _ParentWebSocket._IsReceiveChannelClosed);
    KAssert(_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Open
        || _ParentWebSocket.GetConnectionStatus() == ConnectionStatus::CloseInitiated
        );

    //
    //  Process any available user data
    //
    while (_DataOffset < _BytesReceived)  //  Not all the current data has been consumed
    {
        if (! _CurrentOperation)  //  Get the next user receive operation from the queue
        {
            _RequestDequeueOperation->StartDequeue(
                &_ParentWebSocket,
                KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::ReceiveOperationProcessor::RequestDequeueCallback)
                );
            return;
        }

        ULONG dataConsumed;
        BOOLEAN receiveMoreData;
        NTSTATUS status;

        status = _CurrentOperation->HandleReceive(
                static_cast<PBYTE>(_TransportReceiveBuffer->GetBuffer()) + _DataOffset,
                _WebSocketReceiveOperation->GetBytesReceived() - _DataOffset,
                _ReceivedContentType,
                _ReceivedFragmentIsFinal,
                dataConsumed,
                receiveMoreData
                );

        if (! NT_SUCCESS(status))
        {
            _ParentWebSocket.FailWebSocket(status);
            return;
        }

        _DataOffset += dataConsumed;
        KInvariant(_DataOffset <= _TransportReceiveBuffer->QuerySize());
        KInvariant(_DataOffset <= _BytesReceived);

        if (! receiveMoreData)
        {
            _CurrentOperation.Detach();
        }
    }

    //
    //  All the received data has been consumed, so start the next receive
    //
    _WebSocketReceiveOperation->StartReceive(
        _ParentWebSocket,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::ReceiveOperationProcessor::WebSocketReceiveCallback),
        _ParentWebSocket._WinhttpWebSocketHandle,
        _TransportReceiveBuffer->GetBuffer(),
        _TransportReceiveBuffer->QuerySize()
        );

    //  Execution will continue in WebSocketReceiveCallback or in failure path
    return;
}


VOID
KHttpClientWebSocket::ReceiveOperationProcessor::RequestDequeueCallback(
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
        return;
    }
    else if (! NT_SUCCESS(status))
    {
        _ParentWebSocket.FailWebSocket(status);
        return;
    }

    _CurrentOperation = &_RequestDequeueOperation->GetDequeuedItem();

    if (_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Error)
    {
        _CurrentOperation->CompleteOperation(STATUS_REQUEST_ABORTED);
        _CurrentOperation.Detach();
        return;
    }

    _RequestDequeueOperation->Reuse();
    
    FSM();
}

KDefineDefaultCreate(KHttpClientWebSocket::WinhttpWebSocketReceiveOperation, WebSocketReceiveOperation);

KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::WinhttpWebSocketReceiveOperation()
    :
        _FrameType(FrameType::Invalid),
        _BytesReceived(0),

        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::~WinhttpWebSocketReceiveOperation()
{
}

ULONG
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::GetBytesReceived()
{
    return _BytesReceived;
}

KWebSocket::FrameType
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::GetFrameType()
{
    return _FrameType;
}

ULONG
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::GetWinhttpError()
{
    return _WinhttpError;
}

VOID
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::OnStart()
{
    //  No-op
}

VOID
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::OnReuse()
{
    _WinhttpError = NO_ERROR;
}

VOID
KHttpClientWebSocket::WinhttpWebSocketReceiveOperation::StartReceive(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HINTERNET WinhttpWebSocketHandle,
    __out_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    ULONG error;

    error = WinHttpWebSocketReceive(
        WinhttpWebSocketHandle,
        Buffer,
        BufferLength,
        nullptr,
        nullptr
        );

    if (error != NO_ERROR)
    {
        _WinhttpError = error;
        Complete(K_STATUS_WINHTTP_API_CALL_FAILURE);
        return;
    }

    //  Execution continues in WebSocketWinhttpCallback
}

VOID
KHttpClientWebSocket::ReceiveOperationProcessor::WebSocketReceiveCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(_ParentWebSocket.IsInApartment());
    KAssert(Parent == &_ParentWebSocket);
    KAssert(&CompletingOperation == _WebSocketReceiveOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    if (_ParentWebSocket.GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }

    NTSTATUS status;

    status = _WebSocketReceiveOperation->Status();
    if (status == STATUS_CANCELLED)  // The receive operation was cancelled due to failure or cleanup
    {
        return;
    }

    if (! NT_SUCCESS(status))
    {
        _ParentWebSocket._WinhttpError = _WebSocketReceiveOperation->GetWinhttpError();
        _ParentWebSocket.FailWebSocket(status);
        return;
    }

    FrameType frameType = _WebSocketReceiveOperation->GetFrameType();
    KInvariant(frameType == FrameType::Close || !IsControlFrame(frameType));

    //
    //  A close frame has been received, and is ready to be queried
    //
    if (frameType == FrameType::Close)  
    {
        ULONG error;
        USHORT closeStatus;
        ULONG reasonLengthInBytes;
        
        KAssert(_ParentWebSocket._RemoteCloseReasonBuffer->QuerySize() >= MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES);
        error = WinHttpWebSocketQueryCloseStatus(
            _ParentWebSocket._WinhttpWebSocketHandle,
            &closeStatus,
            _ParentWebSocket._RemoteCloseReasonBuffer->GetBuffer(),
            _ParentWebSocket._RemoteCloseReasonBuffer->QuerySize(),
            &reasonLengthInBytes
            );
        KInvariant(error != ERROR_INSUFFICIENT_BUFFER);  //  Size cannot exceed size of buffer

        if (error != NO_ERROR)
        {
            _ParentWebSocket._WinhttpError = error;
            _ParentWebSocket.FailWebSocket(K_STATUS_WINHTTP_API_CALL_FAILURE, error);
            return;
        }

        status = _ParentWebSocket._RemoteCloseReason->MapBackingBuffer(
            *_ParentWebSocket._RemoteCloseReasonBuffer,
            reasonLengthInBytes,
            0,
            reasonLengthInBytes
            );
        KAssert(NT_SUCCESS(status));

        _ParentWebSocket._RemoteCloseStatusCode = static_cast<CloseStatusCode>(closeStatus);


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
    }

    //
    //  Process the received data
    //
    _WebSocketReceiveOperation->Reuse();

    _ReceivedContentType = FrameTypeToMessageContentType(frameType);
    KInvariant(_ReceivedContentType != MessageContentType::Invalid);
    _ReceivedFragmentIsFinal = IsFinalFragment(frameType);

    _BytesReceived = _WebSocketReceiveOperation->GetBytesReceived();
    _DataOffset = 0;

    FSM();
}
#pragma endregion KHttpClientWebSocket_Receive


#pragma region KHttpClientWebSocket_ServiceOpen
NTSTATUS
KHttpClientWebSocket::StartOpenWebSocket(
    __in KUri& TargetURI,
    __in WebSocketCloseReceivedCallback CloseReceivedCallback,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt KAsyncServiceBase::OpenCompletionCallback CallbackPtr,
    __in_opt ULONG TransportReceiveBufferSize,
    __in_opt KStringA::SPtr RequestedSubProtocols,
    __in_opt KString::SPtr Proxy,
    __in_opt KHttpHeaderList* const AdditionalHeaders,
    __in_opt KString::SPtr const &AdditionalHeadersBlob,
    __in_opt PCCERT_CONTEXT CertContext,
    __in_opt ServerCertValidationHandler Handler,
    __in_opt BOOLEAN AllowRedirects,
    __in_opt BOOLEAN EnableCookies,
    __in_opt BOOLEAN EnableAutoLogonForAllRequests
    )
{
    K_LOCK_BLOCK(_StartOpenLock)
    {
        if (! TargetURI.IsValid())
        {
            return STATUS_INVALID_PARAMETER_1;
        }

        KStringView host = TargetURI.Get(KUriView::UriFieldType::eHost);
        if (host.IsEmpty() || host.Length() >= KHttpUtils::MaxHostName)
        {
            return STATUS_INVALID_PARAMETER_1;
        }

        KStringView scheme = TargetURI.Get(KUriView::UriFieldType::eScheme);
        if (scheme.Compare(KStringView(WebSocketSchemes::Insecure)) == 0)
        {
            _Secure = FALSE;
        }
        else if (scheme.Compare(KStringView(WebSocketSchemes::Secure)) == 0)
        {
            _Secure = TRUE;
        }
        else
        {
            return STATUS_INVALID_PARAMETER_1;
        }

        if (! CloseReceivedCallback)  //  Callback is required
        {
            return STATUS_INVALID_PARAMETER_2;
        }

        if (TransportReceiveBufferSize < MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES)
        {
            return STATUS_INVALID_PARAMETER_5;
        }

        if (RequestedSubProtocols && RequestedSubProtocols->Length() > KHttpUtils::DefaultFieldLength)
        {
            return STATUS_INVALID_PARAMETER_6;
        }

        if (Proxy && !Proxy->IsNullTerminated() && !Proxy->SetNullTerminator())
        {
            return STATUS_INVALID_PARAMETER_7;
        }

        if (GetConnectionStatus() == ConnectionStatus::Initialized)
        {
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
                    _ReceiveRequestQueue->Deactivate();
                    _IsReceiveRequestQueueDeactivated = TRUE;

                    _ConnectionStatus = ConnectionStatus::Error;
                    return status;
                }
            }

            _TargetURI = &TargetURI;
            _CloseReceivedCallback = CloseReceivedCallback;
            _RequestedSubprotocols = RequestedSubProtocols;
            _TransportReceiveBufferSize = TransportReceiveBufferSize;
            _Proxy = Proxy;
            _AdditionalHeaders = AdditionalHeaders;
            _AdditionalHeadersBlob = AdditionalHeadersBlob;

            // Duplicate the certificate context and store it. We are now responsible for releasing the context.
            _ClientCert = CertDuplicateCertificateContext(CertContext);
            _ServerCertValidationHandler = Handler;

            _AllowRedirects = AllowRedirects;
            _EnableCookies = EnableCookies;
            _EnableAutoLogonForAllRequests = EnableAutoLogonForAllRequests;

            _ConnectionStatus = ConnectionStatus::OpenStarted;

            return StartOpen(ParentAsyncContext, CallbackPtr);
        }
    }

    return STATUS_SHARING_VIOLATION;  //  StartOpenWebSocket has already been called
}



VOID
KHttpClientWebSocket::OnServiceOpen()
{
    KAssert(IsInApartment());

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        CompleteOpen(_FailureStatus);
        return;
    } 

    NTSTATUS status;

    //
    //  Allocate the transport buffer
    //
    status = KBuffer::Create(_TransportReceiveBufferSize, _ReceiveOperationProcessor._TransportReceiveBuffer, GetThisAllocator());
    if (! NT_SUCCESS(status))
    {
        FailWebSocket(status);
        return;
    }
    KAssert(_ReceiveOperationProcessor._TransportReceiveBuffer->QuerySize() >= MAX_CONTROL_FRAME_PAYLOAD_SIZE_BYTES);

    //
    //  Initialize the timers
    //
    status = InitializeHandshakeTimers();
    if (! NT_SUCCESS(status))
    {
        FailWebSocket(status);
        return;
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

    _DeferredServiceOpenAsync->StartDeferredServiceOpen(
        *this,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::DeferredServiceOpenCallback)
        );
}

KDefineDefaultCreate(KHttpClientWebSocket::DeferredServiceOpenAsync, Async);

KHttpClientWebSocket::DeferredServiceOpenAsync::DeferredServiceOpenAsync()
    :
        _IsActive(FALSE),
        _DeferredServiceOpen(GetThisAllocator().GetKtlSystem()),
        _WinhttpError(NO_ERROR)
{
    _DeferredServiceOpen.Bind(*this, &KHttpClientWebSocket::DeferredServiceOpenAsync::DeferredServiceOpen);
}

KHttpClientWebSocket::DeferredServiceOpenAsync::~DeferredServiceOpenAsync()
{
}

ULONG
KHttpClientWebSocket::DeferredServiceOpenAsync::GetWinhttpError()
{
    return _WinhttpError;
}

VOID
KHttpClientWebSocket::DeferredServiceOpenAsync::OnCompleted()
{
    _ParentWebSocket = nullptr;
}

BOOLEAN
KHttpClientWebSocket::DeferredServiceOpenAsync::IsActive()
{
    return _IsActive;
}

VOID
KHttpClientWebSocket::DeferredServiceOpenAsync::SetActive(
    __in BOOLEAN IsActive
    )
{
    _IsActive = IsActive;
}

VOID
KHttpClientWebSocket::DeferredServiceOpenAsync::StartDeferredServiceOpen(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback OpenCallbackPtr
    )
{
    KAssert(OpenCallbackPtr);
    
    _ParentWebSocket = &ParentWebSocket;
    _OpenCallbackPtr = OpenCallbackPtr;

    KAssert(! IsActive());
    SetActive(TRUE);

    Start(_ParentWebSocket.RawPtr(), _OpenCallbackPtr);

    return;
}

VOID
KHttpClientWebSocket::DeferredServiceOpenAsync::OnStart()
{
    _DeferredServiceOpen();
}

VOID
KHttpClientWebSocket::DeferredServiceOpenAsync::DeferredServiceOpen()
{
    NTSTATUS status;

    KDbgCheckpointWData(0, "Starting deferred service open.", STATUS_SUCCESS, ULONG_PTR(_ParentWebSocket.RawPtr()), ULONG_PTR(this), 0, 0);

    //
    //  Create the Winhttp session
    //
    {
        //  Synchronous
        _ParentWebSocket->_WinhttpSessionHandle = WinHttpOpen(
            nullptr,
            _ParentWebSocket->_Proxy ? WINHTTP_ACCESS_TYPE_NAMED_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
            _ParentWebSocket->_Proxy ? (LPCWSTR) *_ParentWebSocket->_Proxy : WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            WINHTTP_FLAG_ASYNC
            );

        if (! _ParentWebSocket->_WinhttpSessionHandle)
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

            KDbgCheckpointWData(
                0,
                "Failed to open Winhttp session handle.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                ULONG_PTR(this),
                _WinhttpError,
                0
                );

            Complete(status);
            return;
        }

        //
        //  Set the context for the session handle
        //
        {
            KHttpClientWebSocket* parentPtr = _ParentWebSocket.RawPtr();
            if (! WinHttpSetOption(_ParentWebSocket->_WinhttpSessionHandle, WINHTTP_OPTION_CONTEXT_VALUE, &parentPtr, sizeof parentPtr))
            {
                _WinhttpError = GetLastError();
                status = K_STATUS_WINHTTP_API_CALL_FAILURE;

                KDbgCheckpointWData(
                    0,
                    "Call to WinHttpSetOption failed when setting the session handle's context.",
                    status,
                    ULONG_PTR(_ParentWebSocket.RawPtr()),
                    ULONG_PTR(this),
                    _WinhttpError,
                    0
                    );

                Complete(status);
                return;
            }
        }

        //
        //  Set the Winhttp callback for the session handle
        //
        WINHTTP_STATUS_CALLBACK result = WinHttpSetStatusCallback(
            _ParentWebSocket->_WinhttpSessionHandle,
            KHttpClientWebSocket::WinHttpCallback,
            WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
            NULL
            );
        if (result == WINHTTP_INVALID_STATUS_CALLBACK)
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

            KDbgCheckpointWData(
                0,
                "Call to WinHttpSetStatusCallback failed when setting the session handle's callback.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                ULONG_PTR(this),
                _WinhttpError,
                0
                );

            Complete(status);
            return;
        }

        _ParentWebSocket->AcquireActivities();  //  Hold an activity while the session handle is active, and release it when it closes.
    }

    //
    //  Open the Winhttp connection
    //
    {
        INTERNET_PORT port;
        port = static_cast<INTERNET_PORT>(_ParentWebSocket->_TargetURI->GetPort());
        if (port == 0)
        {
            port = INTERNET_DEFAULT_PORT;
        }

        KDynString host(GetThisAllocator());
        if (! host.CopyFrom(_ParentWebSocket->_TargetURI->Get(KUriView::UriFieldType::eHost), TRUE))
        {
            Complete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }

        //  Synchronous
        _ParentWebSocket->_WinhttpConnectionHandle = WinHttpConnect(
            _ParentWebSocket->_WinhttpSessionHandle,
            host,
            port,
            NULL
            );

        if (! _ParentWebSocket->_WinhttpConnectionHandle)
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

            KDbgCheckpointWData(
                0,
                "Failed to open Winhttp connection handle via WinHttpConnect.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                _WinhttpError,
                host,
                port
                );

            Complete(status);
            return;
        }

        //
        //  Set the context for the connection handle
        //
        {
            KHttpClientWebSocket* parentPtr = _ParentWebSocket.RawPtr();
            if (! WinHttpSetOption(_ParentWebSocket->_WinhttpConnectionHandle, WINHTTP_OPTION_CONTEXT_VALUE, &parentPtr, sizeof parentPtr))
            {
                _WinhttpError = GetLastError();
                status = K_STATUS_WINHTTP_API_CALL_FAILURE;

                KDbgCheckpointWData(
                    0,
                    "Call to WinHttpSetOption failed when setting the connection handle's context.",
                    status,
                    ULONG_PTR(_ParentWebSocket.RawPtr()),
                    ULONG_PTR(this),
                    _WinhttpError,
                    0
                    );

                Complete(status);
                return;
            }
        }

        //
        //  Set the Winhttp callback for the connection handle
        //
        WINHTTP_STATUS_CALLBACK result = WinHttpSetStatusCallback(
            _ParentWebSocket->_WinhttpConnectionHandle,
            KHttpClientWebSocket::WinHttpCallback,
            WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
            NULL
            );
        if (result == WINHTTP_INVALID_STATUS_CALLBACK)
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

            KDbgCheckpointWData(
                0,
                "Call to WinHttpSetStatusCallback failed when setting the connection handle's callback.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                ULONG_PTR(this),
                _WinhttpError,
                0
                );

            Complete(status);
            return;
        }

        _ParentWebSocket->AcquireActivities();  //  Hold an activity while the connection handle is active, and release it when it closes.
    }

    //
    //  Create the Winhttp request and set the WebSocket-related options
    //
    {
        ULONG winhttpError;

        status = CreateRequest(
            _ParentWebSocket->_WinhttpRequestHandle,
            winhttpError,
            _ParentWebSocket->_WinhttpConnectionHandle,
            KHttpUtils::OperationType::eHttpGet,
            *_ParentWebSocket->_TargetURI,
            _ParentWebSocket->_RequestedSubprotocols,
            _ParentWebSocket->_AdditionalHeaders ? &(_ParentWebSocket->_AdditionalHeaders->_Headers) : nullptr,
            _ParentWebSocket->_AdditionalHeadersBlob,
            _ParentWebSocket->_ClientCert,
            _ParentWebSocket->_ServerCertValidationHandler,
            _ParentWebSocket->_AllowRedirects,
            _ParentWebSocket->_EnableCookies,
            _ParentWebSocket->_EnableAutoLogonForAllRequests
            );

        if (! NT_SUCCESS(status))
        {
            _WinhttpError = winhttpError;

            KDbgCheckpointWData(
                0,
                "Failed to create Winhttp request.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                ULONG_PTR(this),
                _WinhttpError,
                0
                );

            Complete(status);
            return;
        }

        //
        //  Set the context for the request handle
        //
        {
            KHttpClientWebSocket* parentPtr = _ParentWebSocket.RawPtr();
            if (! WinHttpSetOption(_ParentWebSocket->_WinhttpRequestHandle, WINHTTP_OPTION_CONTEXT_VALUE, &parentPtr, sizeof parentPtr))
            {
                _WinhttpError = GetLastError();
                status = K_STATUS_WINHTTP_API_CALL_FAILURE;

                KDbgCheckpointWData(
                    0,
                    "Call to WinHttpSetOption failed when setting the request handle's context.",
                    status,
                    ULONG_PTR(_ParentWebSocket.RawPtr()),
                    ULONG_PTR(this),
                    _WinhttpError,
                    0
                    );

                Complete(status);
                return;
            }
        }

        //
        //  Set the Winhttp callback for the request handle
        //
        WINHTTP_STATUS_CALLBACK result = WinHttpSetStatusCallback(
            _ParentWebSocket->_WinhttpRequestHandle,
            KHttpClientWebSocket::WinHttpCallback,
            WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
            NULL
            );
        if (result == WINHTTP_INVALID_STATUS_CALLBACK)
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

             KDbgCheckpointWData(
                0,
                "Call to WinHttpSetStatusCallback failed when setting the request handle's callback.",
                status,
                ULONG_PTR(_ParentWebSocket.RawPtr()),
                ULONG_PTR(this),
                _WinhttpError,
                0
                );

            Complete(status);
            return;
        }

        _ParentWebSocket->AcquireActivities();  //  Hold an activity while the request handle is active, and release it when it closes.

        if (! WinHttpSetOption(_ParentWebSocket->_WinhttpRequestHandle, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
        {
            _WinhttpError = GetLastError();
            status = K_STATUS_WINHTTP_API_CALL_FAILURE;

           KDbgCheckpointWData(
                    0,
                    "Call to WinHttpSetOption failed when setting the request handle's upgrade-to-websocket option.",
                    status,
                    ULONG_PTR(_ParentWebSocket.RawPtr()),
                    ULONG_PTR(this),
                    _WinhttpError,
                    0
                    );

            Complete(status);
            return;
        }

        {
            TimingConstantValue keepalivePeriod = _ParentWebSocket->GetTimingConstant(TimingConstant::PongKeepalivePeriodMs);
            KAssert(keepalivePeriod != TimingConstantValue::Invalid);

            ULONG val = static_cast<ULONG>(keepalivePeriod);
            if (val >= WINHTTP_WEB_SOCKET_MIN_KEEPALIVE_VALUE)
            {
                BOOL ret = WinHttpSetOption(
                    _ParentWebSocket->_WinhttpRequestHandle,
                    WINHTTP_OPTION_WEB_SOCKET_KEEPALIVE_INTERVAL,
                    &val,
                    sizeof val
                    );

                if (! ret)
                {
                    _WinhttpError = GetLastError();
                    status = K_STATUS_WINHTTP_API_CALL_FAILURE;

                    KDbgCheckpointWData(
                        0,
                        "Call to WinHttpSetOption failed when setting the request handle's websocket keepalive interval.",
                        status,
                        ULONG_PTR(_ParentWebSocket.RawPtr()),
                        ULONG_PTR(this),
                        _WinhttpError,
                        val
                        );

                    Complete(status);
                    return;
                }
            }
        }
    }

    Complete(STATUS_SUCCESS);
    return;
}

NTSTATUS
KHttpClientWebSocket::CreateRequest(
    __out HINTERNET& WinhttpRequestHandle,
    __out ULONG& WinhttpError,
    __in HINTERNET WinhttpConnectionHandle,
    __in KHttpUtils::OperationType OperationType,
    __in KUriView Uri,
    __in_opt KStringA::SPtr RequestedSubprotocols,
    __in_opt KArray<KHttpCliRequest::HeaderPair::SPtr>* Headers,
    __in_opt KString::SPtr& HeadersBlob,
    __in_opt PCCERT_CONTEXT CertContext,
    __in_opt ServerCertValidationHandler Handler,
    __in_opt BOOLEAN AllowRedirects,
    __in_opt BOOLEAN EnableCookies,
    __in_opt BOOLEAN EnableAutoLogonForAllRequests
    )
{
    WinhttpRequestHandle = nullptr;
    WinhttpError = NO_ERROR;

    KAssert(WinhttpConnectionHandle);
    KAssert(Uri.IsValid());  //  Checked in StartOpenWebSocket

    //
    //  Temporary work string for misc. parsing/stringbuilding later.
    //  This can hold a maximum header id/value pair, a max host name, or a max URL plus lots of padding.
    //
    ULONG const scratchSize = KHttpUtils::DefaultFieldLength;
    KLocalString<scratchSize> scratch;

    //
    //  Get the server name & port.
    //
    INTERNET_PORT port = static_cast<INTERNET_PORT>(Uri.GetPort());
    if (port == 0)
    {
        port = INTERNET_DEFAULT_PORT;
    }

    KStringView hostName = Uri.Get(KUri::eHost);  // Not null-terminated, so we have to copy
    KAssert(! hostName.IsEmpty());  //  Checked in StartOpenWebSocket
    KAssert(hostName.Length() <= KHttpUtils::MaxHostName);  //  Checked in StartOpenWebSocket

    KInvariant(scratch.CopyFrom(hostName));
    KInvariant(scratch.SetNullTerminator());

    KStringView operationName;
    KInvariant(KHttpUtils::OpToString(OperationType, operationName));

    //
    //  Note that the path segment must be prefixed with a slash
    //  according to HTTP rules.
    //
    KStringView pathView = Uri.Get(KUri::ePath);
    scratch.Clear();
    KInvariant(scratch.AppendChar(L'/'));
    KInvariant(scratch.Concat(pathView));

    if (Uri.Has(KUri::eQueryString))
    {
        KInvariant(scratch.AppendChar(L'?'));
        KInvariant(scratch.Concat(Uri.Get(KUri::eQueryString)));
    }

    if (Uri.Has(KUri::eFragment))
    {
        KInvariant(scratch.AppendChar(L'#'));
        KInvariant(scratch.Concat(Uri.Get(KUri::eFragment)));
    }

    KInvariant(scratch.SetNullTerminator());

    BOOLEAN isSecure = FALSE;
    KStringView scheme = Uri.Get(KUriView::UriFieldType::eScheme);
    if (scheme.Compare(KStringView(WebSocketSchemes::Secure)) == 0)
    {
        isSecure = TRUE;
    }

    HINTERNET winhttpRequestHandle;
    winhttpRequestHandle = WinHttpOpenRequest(
        WinhttpConnectionHandle,
        LPCWSTR(operationName),
        LPCWSTR(scratch),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        isSecure ? WINHTTP_FLAG_SECURE : 0
        );
    if (! winhttpRequestHandle)
    {
        WinhttpError = GetLastError();
        return K_STATUS_WINHTTP_API_CALL_FAILURE;
    }

    if (isSecure)
    {
        // If user has specified a custom server certificate validation handler, 
        // have WinHttp ignore some errors so that the validation callback can run.
        if (Handler)
        {
            DWORD sslFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA
                | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID
                | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;

            BOOL bRes = WinHttpSetOption(
                winhttpRequestHandle,
                WINHTTP_OPTION_SECURITY_FLAGS,
                &sslFlags,
                sizeof(sslFlags));

            if (!bRes)
            {
                WinhttpError = GetLastError();
                return K_STATUS_WINHTTP_API_CALL_FAILURE;
            }
        }

        // If client certificate is specified, set it on the request
        if (CertContext)
        {
            // This function operates synchronously even in async mode
            BOOL bRes = WinHttpSetOption(
                winhttpRequestHandle,
                WINHTTP_OPTION_CLIENT_CERT_CONTEXT,
                (LPVOID)CertContext,
                sizeof(CERT_CONTEXT));

            if (!bRes)
            {
                WinhttpError = GetLastError();
                return K_STATUS_WINHTTP_API_CALL_FAILURE;
            }
        }
    }

    // Set the disable redirect policy if specified.
    if (!AllowRedirects)
    {
        DWORD dwOptionValue = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;

        BOOL bRes = WinHttpSetOption(
            winhttpRequestHandle,
            WINHTTP_OPTION_REDIRECT_POLICY,
            &dwOptionValue,
            sizeof(dwOptionValue));

        if (!bRes)
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }
    }

    // Set the disable cookies option if specified.
    if (!EnableCookies)
    {
        DWORD dwOptionValue = WINHTTP_DISABLE_COOKIES;

        BOOL bRes = WinHttpSetOption(
            winhttpRequestHandle,
            WINHTTP_OPTION_DISABLE_FEATURE,
            &dwOptionValue,
            sizeof(dwOptionValue));

        if (!bRes)
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }
    }

    if (EnableAutoLogonForAllRequests)
    {
        DWORD dwOptionValue = WINHTTP_AUTOLOGON_SECURITY_LEVEL_LOW;

        BOOL bRes = WinHttpSetOption(
            winhttpRequestHandle,
            WINHTTP_OPTION_AUTOLOGON_POLICY,
            &dwOptionValue,
            sizeof(dwOptionValue));

        if (!bRes)
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }

        // Preset the server credentials to NULL, NULL. WinHttp will use Windows logon credentials to
        // respond to Negotiate. 
        // With this, we are not required to handle WINHTTP_ERROR_RESEND_REQUEST. Note: This only works for Kerberos.
        // If Negotiate falls back to NTLM, we might see WINHTTP_ERROR_RESEND_REQUEST.
        bRes = WinHttpSetCredentials(
            winhttpRequestHandle,
            WINHTTP_AUTH_TARGET_SERVER,
            WINHTTP_AUTH_SCHEME_NEGOTIATE,
            NULL,
            NULL,
            NULL);

        if (!bRes)
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }
    }

    if (Headers)
    {
        for (ULONG i = 0; i < Headers->Count(); i++)
        {
            scratch.Clear();
            KInvariant(scratch.Concat((*Headers)[i]->_Id));
            KInvariant(scratch.Concat(L": "));
            KInvariant(scratch.Concat((*Headers)[i]->_Value));
            KInvariant(scratch.Concat(L"\r\n"));
            KInvariant(scratch.SetNullTerminator());

            //
            //  This function copies the headers, so there is no need to retain the memory.
            //
            if (! WinHttpAddRequestHeaders(winhttpRequestHandle, LPWSTR(scratch), scratch.Length(), WINHTTP_ADDREQ_FLAG_ADD))
            {
                WinhttpError = GetLastError();
                return K_STATUS_WINHTTP_API_CALL_FAILURE;
            }
        }
    }

    if (HeadersBlob)
    {
        BOOL bRes = WinHttpAddRequestHeaders(
            winhttpRequestHandle,
            *HeadersBlob,
            HeadersBlob->Length(),
            WINHTTP_ADDREQ_FLAG_ADD);
        if (!bRes)
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }
    }

    if (RequestedSubprotocols && !RequestedSubprotocols->IsEmpty())
    {
        KAssert(RequestedSubprotocols->Length() <= KHttpUtils::DefaultFieldLength); // checked in StartOpenWebSocket
        KLocalString<KHttpUtils::DefaultFieldLength> requestedSubprotocolsValue;
        KInvariant(requestedSubprotocolsValue.CopyFromAnsi(*RequestedSubprotocols, RequestedSubprotocols->Length()));

        scratch.Clear();
        KInvariant(scratch.Concat(WebSocketHeaders::Protocol));
        KInvariant(scratch.Concat(L": "));
        KInvariant(scratch.Concat(requestedSubprotocolsValue));
        KInvariant(scratch.Concat(L"\r\n"));
        KInvariant(scratch.SetNullTerminator());

        //
        //  This function copies the headers, so there is no need to retain the memory.
        //
        if (! WinHttpAddRequestHeaders(winhttpRequestHandle, LPWSTR(scratch), scratch.Length(), WINHTTP_ADDREQ_FLAG_ADD))
        {
            WinhttpError = GetLastError();
            return K_STATUS_WINHTTP_API_CALL_FAILURE;
        }
    }

    WinhttpRequestHandle = winhttpRequestHandle;
    return STATUS_SUCCESS;
}

VOID
KHttpClientWebSocket::DeferredServiceOpenCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _DeferredServiceOpenAsync.RawPtr());
    KAssert(IsInApartment());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_DeferredServiceOpenAsync->IsActive());
    _DeferredServiceOpenAsync->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        KDbgCheckpointWData(
            0,
            "Failure (e.g. due to Abort()) raced with deferred service open completion.",
            _FailureStatus,
            ULONG_PTR(this),
            0,
            0,
            0
            );

        CloseHandles();
        return;
    }
    else
    {
        KAssert(GetConnectionStatus() == ConnectionStatus::OpenStarted);
    }

    if (! NT_SUCCESS(_DeferredServiceOpenAsync->Status()))
    {
        _WinhttpError = _DeferredServiceOpenAsync->GetWinhttpError();

        KDbgErrorWData(0, "Deferred service open failed.", _DeferredServiceOpenAsync->Status(), ULONG_PTR(this), _WinhttpError, 0, 0);

        FailWebSocket(_DeferredServiceOpenAsync->Status(), _WinhttpError);
        return;
    }

    //
    //  Send the http request
    //
    _SendRequestOperation->StartSendRequest(
        *this,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::SendHttpRequestCallback),
        _WinhttpRequestHandle
        );
}

KDefineDefaultCreate(KHttpClientWebSocket::WinhttpSendRequestOperation, SendRequestOperation);

KHttpClientWebSocket::WinhttpSendRequestOperation::WinhttpSendRequestOperation()
    :
        _IsActive(FALSE),
        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::WinhttpSendRequestOperation::~WinhttpSendRequestOperation()
{
}



VOID
KHttpClientWebSocket::WinhttpSendRequestOperation::StartSendRequest(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HINTERNET RequestHandle
    )
{
    KAssert(CallbackPtr);
    KAssert(RequestHandle);
    KAssert(ParentWebSocket.IsInApartment());

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    //
    //  Send the request
    //
    BOOL res = WinHttpSendRequest(
        RequestHandle,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        nullptr,
        0,
        0,
        reinterpret_cast<ULONG_PTR>(&ParentWebSocket)
        );
    if (! res)
    {
        _WinhttpError = GetLastError();
        NTSTATUS status = K_STATUS_WINHTTP_API_CALL_FAILURE;

        KDbgErrorWData(0, "Call to WinHttpSendRequest failed.", status, ULONG_PTR(&ParentWebSocket), ULONG_PTR(this), _WinhttpError, 0);

        Complete(status);
        return;
    }

    //  Execution will continue in WinhttpCallback or in async callback
}

ULONG
KHttpClientWebSocket::WinhttpSendRequestOperation::GetWinhttpError()
{
    return _WinhttpError;
}

BOOLEAN
KHttpClientWebSocket::WinhttpSendRequestOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpClientWebSocket::WinhttpSendRequestOperation::SetActive(
    __in BOOLEAN IsActive
    )
{
    _IsActive = IsActive;
}

VOID
KHttpClientWebSocket::WinhttpSendRequestOperation::OnStart()
{
    //  No-op   
}

VOID
KHttpClientWebSocket::WinhttpSendRequestOperation::OnReuse()
{
    _WinhttpError = NO_ERROR; 
}

VOID
KHttpClientWebSocket::SendHttpRequestCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendRequestOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_SendRequestOperation->IsActive());
    _SendRequestOperation->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }
    KInvariant(GetConnectionStatus() == ConnectionStatus::OpenStarted);

    NTSTATUS status;

    status = _SendRequestOperation->Status();
    if (! NT_SUCCESS(status))
    {
        _WinhttpError = _SendRequestOperation->GetWinhttpError();

        KDbgErrorWData(0, "Sending client http request failed.", status, ULONG_PTR(this), _WinhttpError, 0, 0);

        FailWebSocket(status, _WinhttpError);
        return;
    }

    KDbgCheckpointWData(0, "Client http request send successful.  Starting http response receive.", status, ULONG_PTR(this), 0, 0, 0);

    _ReceiveResponseOperation->StartReceiveResponse(
        *this,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::ReceiveHttpResponseCallback),
        _WinhttpRequestHandle
        );
}

KDefineDefaultCreate(KHttpClientWebSocket::WinhttpReceiveResponseOperation, ReceiveResponseOperation);

KHttpClientWebSocket::WinhttpReceiveResponseOperation::WinhttpReceiveResponseOperation()
    :
        _IsActive(FALSE),
        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::WinhttpReceiveResponseOperation::~WinhttpReceiveResponseOperation()
{
}

ULONG
KHttpClientWebSocket::WinhttpReceiveResponseOperation::GetWinhttpError()
{
    return _WinhttpError;
}

BOOLEAN
KHttpClientWebSocket::WinhttpReceiveResponseOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpClientWebSocket::WinhttpReceiveResponseOperation::SetActive(
    __in BOOLEAN IsActive
    )
{
    _IsActive = IsActive;
}

VOID
KHttpClientWebSocket::WinhttpReceiveResponseOperation::StartReceiveResponse(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HINTERNET RequestHandle
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    //
    //  Receive the response
    //
    if (! WinHttpReceiveResponse(RequestHandle, nullptr))
    {
        _WinhttpError = GetLastError();
        NTSTATUS status = K_STATUS_WINHTTP_API_CALL_FAILURE;

        KDbgErrorWData(0, "Call to WinHttpReceiveResponse failed.", status, ULONG_PTR(&ParentWebSocket), ULONG_PTR(this), _WinhttpError, 0);

        Complete(status);
        return;
    }

    //  Execution will continue in WinhttpCallback
}

VOID
KHttpClientWebSocket::WinhttpReceiveResponseOperation::OnStart()
{
    //  No-op
}

VOID
KHttpClientWebSocket::WinhttpReceiveResponseOperation::OnReuse()
{
    _WinhttpError = NO_ERROR;
}

VOID
KHttpClientWebSocket::ReceiveHttpResponseCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _ReceiveResponseOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_ReceiveResponseOperation->IsActive());
    _ReceiveResponseOperation->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }
    KInvariant(GetConnectionStatus() == ConnectionStatus::OpenStarted);

    NTSTATUS status;
    
    //
    //  Fail if receiving the response failed
    //
    status = _ReceiveResponseOperation->Status();
    if (! NT_SUCCESS(status))
    {
        _WinhttpError = _ReceiveResponseOperation->GetWinhttpError();
        KTraceFailedAsyncRequest(status, _ReceiveResponseOperation.RawPtr(), _WinhttpError, 0);
        FailWebSocket(status, _WinhttpError);
        return;
    }

    //
    //  Parse the headers 
    //
    {
        BOOL res;
        KLocalString<KHttpUtils::DefaultFieldLength> buffer;
        ULONG bufferSizeInBytes = buffer.BufferSizeInChars() * 2;
        ULONG headerIndex = WINHTTP_NO_HEADER_INDEX;

        //
        //  Verify the status code is 101 (Upgrade)
        //
        {
            res = WinHttpQueryHeaders(
                _WinhttpRequestHandle,
                WINHTTP_QUERY_STATUS_CODE,
                WINHTTP_HEADER_NAME_BY_INDEX,
                buffer,
                &bufferSizeInBytes,
                &headerIndex
                );
            if (! res)
            {
                _WinhttpError = GetLastError();
                FailWebSocket(K_STATUS_WINHTTP_API_CALL_FAILURE, _WinhttpError);
                return;
            }

            buffer.SetLength(bufferSizeInBytes / 2);
            if (! buffer.ToULONG(_HttpStatusCode))
            {
                //
                //  Status code returned failed to convert to a ULONG; bug in KStringView or in WinHttp
                //
                FailWebSocket(K_STATUS_WINHTTP_ERROR, static_cast<ULONGLONG>(STATUS_INTERNAL_ERROR));
                return;
            }

            if (_HttpStatusCode != HTTP_STATUS_SWITCH_PROTOCOLS)
            {
                FailWebSocket(K_STATUS_WEBSOCKET_STANDARD_VIOLATION, _HttpStatusCode, HTTP_STATUS_SWITCH_PROTOCOLS);
                return;
            }
        }

        buffer.Clear();

        //
        //  Set the selected subprotocol, if present
        //
        {
            KLocalStringA<KHttpUtils::DefaultFieldLength / 2> ansiBuffer;

            bufferSizeInBytes = buffer.BufferSizeInChars() * 2;
            headerIndex = WINHTTP_NO_HEADER_INDEX;

            res = WinHttpQueryHeaders(
                _WinhttpRequestHandle,
                WINHTTP_QUERY_CUSTOM,
                WebSocketHeaders::Protocol,
                buffer,
                &bufferSizeInBytes,
                &headerIndex
                );

            if (res)
            {
                if (bufferSizeInBytes > 0)
                {
                    buffer.SetLength(bufferSizeInBytes / 2);
                    if (!buffer.CopyToAnsi(ansiBuffer, buffer.Length()))
                    {
                        //
                        //  Subprotocol header value failed to convert to ANSI string; according to standard, must consist of only ANSI characters
                        //
                        FailWebSocket(K_STATUS_WEBSOCKET_STANDARD_VIOLATION);
                        return;
                    }
                    ansiBuffer.SetLength(buffer.Length());

                    if (!_ActiveSubprotocol->CopyFrom(ansiBuffer))
                    {
                        //
                        //  Failed to copy to active subprotocol string, possibly due to allocation failure
                        //
                        FailWebSocket(STATUS_UNSUCCESSFUL);
                        return;
                    }
                }
            }
            else
            {
                ULONG winhttpError = GetLastError();
                if (winhttpError != ERROR_WINHTTP_HEADER_NOT_FOUND)
                {
                    _WinhttpError = winhttpError;
                    FailWebSocket(K_STATUS_WINHTTP_ERROR, _WinhttpError);
                    return;
                }
            }
        }
    }

    //
    //  Complete the WebSocket upgrade and set the context
    //
    _WinhttpWebSocketHandle = WinHttpWebSocketCompleteUpgrade(_WinhttpRequestHandle, reinterpret_cast<ULONG_PTR>(this)); 
    if (! _WinhttpWebSocketHandle)
    {
        _WinhttpError = GetLastError();
        FailWebSocket(K_STATUS_WINHTTP_API_CALL_FAILURE, _WinhttpError);
        return;
    }

    //
    //  Set Winhttp callback for the websocket handle
    //
    WINHTTP_STATUS_CALLBACK result = WinHttpSetStatusCallback(
        _WinhttpWebSocketHandle,
        KHttpClientWebSocket::WinHttpCallback,
        WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,
        NULL
        );
    if (result == WINHTTP_INVALID_STATUS_CALLBACK)
    {
        _WinhttpError = GetLastError();
        FailWebSocket(status, _WinhttpError);
        return;
    }

    AcquireActivities();  //  Hold an activity while the websocket handle is active, and release it when it closes.

    //
    //  Close the request handle, as it is no longer needed
    //
    if (! WinHttpCloseHandle(_WinhttpRequestHandle))
    {
        _WinhttpError = GetLastError();
        FailWebSocket(K_STATUS_WINHTTP_API_CALL_FAILURE, _WinhttpError);
        return;
    }

    if (_OpenTimeoutTimer)
    {
        _OpenTimeoutTimer->Cancel();
    }
    _ConnectionStatus = ConnectionStatus::Open;
    CompleteOpen(STATUS_SUCCESS);

    //
    //  Start the send and receive operations
    //
    _ReceiveOperationProcessor.FSM();
    _SendRequestDequeueOperation->StartDequeue(
        this,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::SendRequestDequeueCallback)
        );
}
#pragma endregion KHttpClientWebSocket_ServiceOpen


#pragma region KHttpClientWebSocket_ServiceClose
VOID
KHttpClientWebSocket::OnServiceClose()
{
    KAssert(IsInApartment());

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
            "Completing the service close as the WebSocket has already failed and cleaned up.",
            _FailureStatus,
            ULONG_PTR(this),
            static_cast<SHORT>(connectionStatus),
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
}

VOID
KHttpClientWebSocket::CloseHandles()
{
    KDbgCheckpointWData(
        0,
        "Closing the Winhttp handles.",
        _FailureStatus,
        ULONG_PTR(this),
        ULONG_PTR(_WinhttpWebSocketHandle),
        ULONG_PTR(_WinhttpConnectionHandle),
        ULONG_PTR(_WinhttpSessionHandle)
        );

    if (_ClientCert)
    {
        CertFreeCertificateContext(_ClientCert);
        _ClientCert = NULL;
    }

    //
    //  Close the WinHttp handles (and close the transport)
    //
    //  We cannot null any handles, as in-flight callbacks cancelled by any of these will 
    //  refer to them.
    //
    if (_WinhttpRequestHandle)
    {
        WinHttpCloseHandle(_WinhttpRequestHandle);
    }

    if (_WinhttpWebSocketHandle)
    {
        WinHttpCloseHandle(_WinhttpWebSocketHandle);
    }

    if (_WinhttpConnectionHandle)
    {
        WinHttpCloseHandle(_WinhttpConnectionHandle);
    }

    if (_WinhttpSessionHandle)
    {
        WinHttpCloseHandle(_WinhttpSessionHandle);
    }
}

VOID
KHttpClientWebSocket::CleanupWebSocket(
    __in NTSTATUS Status
    )
{
    KAssert(IsInApartment());
    KDbgCheckpointWData(
        0,
        "Cleaning up WebSocket.",
        Status,
        ULONG_PTR(this),
        static_cast<SHORT>(_ConnectionStatus),
        ULONG_PTR(_CurrentSendOperation),
        _DeferredServiceOpenAsync->IsActive()
        );

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

    if (_CurrentSendOperation)
    {
        _CurrentSendOperation->CompleteOperation(Status);
        _CurrentSendOperation.Detach();
    }

    if (_ReceiveOperationProcessor._CurrentOperation)
    {
        _ReceiveOperationProcessor._CurrentOperation->CompleteOperation(Status);
        _ReceiveOperationProcessor._CurrentOperation.Detach();
    }

    //
    //  Cancel all the timers
    //
    if (_OpenTimeoutTimer)
    {
        _OpenTimeoutTimer->Cancel();
    }

    if (_CloseTimeoutTimer)
    {
        _CloseTimeoutTimer->Cancel();
    }

    //
    //  Abort and cleanup may race with deferred service open, due to it necessarily being executed outside the service's apartment.
    //  Therefore, if the deferred service open is active, allow it to close the handles upon its completion.
    //
    if (! _DeferredServiceOpenAsync->IsActive())
    {
        CloseHandles();
    }
}
#pragma endregion KHttpClientWebSocket_ServiceClose


#pragma region KHttpClientWebSocket_Send
KDefineDefaultCreate(KHttpClientWebSocket::WinhttpWebSocketSendOperation, WebSocketSendOperation);

KHttpClientWebSocket::WinhttpWebSocketSendOperation::WinhttpWebSocketSendOperation()
    :
        _BytesSent(0),
        _IsActive(FALSE),
        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::WinhttpWebSocketSendOperation::~WinhttpWebSocketSendOperation()
{
}

BOOLEAN
KHttpClientWebSocket::WinhttpWebSocketSendOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpClientWebSocket::WinhttpWebSocketSendOperation::SetActive(
    __in BOOLEAN IsActive
    )
{
    _IsActive = IsActive;
}

VOID
KHttpClientWebSocket::SendRequestDequeueCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendRequestDequeueOperation.RawPtr());
    KAssert(! _CurrentSendOperation);

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
        KHttpClientSendFragmentOperation& sendFragmentOperation = static_cast<KHttpClientSendFragmentOperation&>(op);

        _CurrentSendOperation = &sendFragmentOperation;
        KAssert(_CurrentSendOperation);

        _SendRequestDequeueOperation->Reuse();  //  Next dequeue will be started by _WebSocketSendOperation's callback

        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = GetWebSocketBufferType(
            sendFragmentOperation.GetContentType(),
            sendFragmentOperation.IsFinalFragment()
            );
        if (bufferType == -1)
        {
            // either content type was invalid, or bug in conversion
            sendFragmentOperation.CompleteOperation(STATUS_REQUEST_ABORTED);
            FailWebSocket(STATUS_INTERNAL_ERROR, static_cast<ULONGLONG>(sendFragmentOperation.GetContentType()));  
            return;
        }

        _WebSocketSendOperation->StartSend(
            *this,
            KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::WebSocketSendCallback),
            _WinhttpWebSocketHandle,
            bufferType,
            sendFragmentOperation.GetData(),
            sendFragmentOperation.GetDataLength()
            );

        return;
    }
    else if (op.GetOperationType() == UserOperationType::MessageSend)
    {
        KHttpClientSendMessageOperation& sendMessageOperation = static_cast<KHttpClientSendMessageOperation&>(op);

        _CurrentSendOperation = &sendMessageOperation;
        KAssert(_CurrentSendOperation);

        _SendRequestDequeueOperation->Reuse();  //  Next dequeue will be started by _WebSocketSendOperation's callback

        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType = GetWebSocketBufferType(
            sendMessageOperation.GetContentType(),
            TRUE
            );
        if (bufferType == -1)
        {
            // either content type was invalid, or bug in conversion
            sendMessageOperation.CompleteOperation(STATUS_REQUEST_ABORTED);
            FailWebSocket(STATUS_INTERNAL_ERROR, static_cast<ULONGLONG>(sendMessageOperation.GetContentType()));  
            return;
        }

        _WebSocketSendOperation->StartSend(
            *this,
            KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::WebSocketSendCallback),
            _WinhttpWebSocketHandle,
            bufferType,
            sendMessageOperation.GetData(),
            sendMessageOperation.GetDataLength()
            );

        return;
    }
    else
    {
        KInvariant(op.GetOperationType() == UserOperationType::CloseSend);
        KAssert(&static_cast<KHttpClientSendCloseOperation&>(op) == _SendCloseOperation.RawPtr());

        _SendRequestQueue->Deactivate();
        _IsSendRequestQueueDeactivated = TRUE;
        _SendRequestQueue->CancelAllEnqueued(KAsyncQueue<WebSocketOperation>::DroppedItemCallback(this, &KHttpWebSocket::HandleDroppedSendRequest));


        CloseStatusCode closeStatus;
        if (GetRemoteWebSocketCloseStatusCode() == KWebSocket::CloseStatusCode::InvalidStatusCode)
        {
            closeStatus = GetLocalWebSocketCloseStatusCode();
        }
        else
        {
            closeStatus = GetRemoteWebSocketCloseStatusCode();
        }
        
        KSharedBufferStringA::SPtr closeReason;
        if (! GetLocalCloseReason() || GetLocalCloseReason()->IsEmpty())
        {
            closeReason = nullptr;
        }
        else
        {
            closeReason = GetLocalCloseReason();
            closeReason->SetLength(__min(GetLocalCloseReason()->LengthInBytes(), WINHTTP_WEB_SOCKET_MAX_CLOSE_REASON_LENGTH));
        }

        _SendCloseOperation->StartSendClose(
            *this,
            KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::SendCloseCallback),
            _WinhttpWebSocketHandle,
            closeStatus,
            closeReason
            );

        return;
    }
}

ULONG
KHttpClientWebSocket::WinhttpWebSocketSendOperation::GetWinhttpError()
{
    return _WinhttpError;
}

ULONG
KHttpClientWebSocket::WinhttpWebSocketSendOperation::GetBytesSent()
{
    return _BytesSent;
}

VOID
KHttpClientWebSocket::WinhttpWebSocketSendOperation::OnReuse()
{
    _WinhttpError = NO_ERROR;
}

VOID
KHttpClientWebSocket::WinhttpWebSocketSendOperation::StartSend(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HINTERNET WinhttpWebSocketHandle,
    __in WINHTTP_WEB_SOCKET_BUFFER_TYPE BufferType,
    __in_bcount(BufferLength) PVOID Buffer,
    __in ULONG BufferLength
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    ULONG error;

    error = WinHttpWebSocketSend(
        WinhttpWebSocketHandle,
        BufferType,
        Buffer,
        BufferLength
        );

    if (error != NO_ERROR)
    {
        _WinhttpError = error;
        Complete(K_STATUS_WINHTTP_API_CALL_FAILURE);
        return;
    }

    //  Execution continues in WebSocketWinhttpCallback, and then to WebSocketSendCallback
}

VOID
KHttpClientWebSocket::WinhttpWebSocketSendOperation::OnStart()
{
    //  No-op
}

VOID
KHttpClientWebSocket::WebSocketSendCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _WebSocketSendOperation.RawPtr());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_WebSocketSendOperation->IsActive());
    _WebSocketSendOperation->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }

    NTSTATUS status;

    status = _WebSocketSendOperation->Status();
    if (status == STATUS_CANCELLED)  // The send operation was cancelled due to failure or cleanup
    {
        if (_CurrentSendOperation)
        {
            _CurrentSendOperation->CompleteOperation(STATUS_CANCELLED);
            _CurrentSendOperation.Detach();
        }
        return;
    }

    if (! NT_SUCCESS(status))
    {
        _WinhttpError = _WebSocketSendOperation->GetWinhttpError();
        FailWebSocket(status);
        return;
    }

    _CurrentSendOperation->CompleteOperation(STATUS_SUCCESS);
    _CurrentSendOperation.Detach();

    _WebSocketSendOperation->Reuse();

    //
    //  Dequeue the next send operation
    //
    _SendRequestDequeueOperation->StartDequeue(
        this,
        KAsyncContextBase::CompletionCallback(this, &KHttpClientWebSocket::SendRequestDequeueCallback)
        );
}

KHttpClientWebSocket::KHttpClientSendCloseOperation::KHttpClientSendCloseOperation()
    :
        _IsActive(FALSE),
        _WinhttpError(NO_ERROR)
{
}

KHttpClientWebSocket::KHttpClientSendCloseOperation::~KHttpClientSendCloseOperation()
{
}

KDefineDefaultCreate(KHttpClientWebSocket::KHttpClientSendCloseOperation, SendCloseOperation);

BOOLEAN
KHttpClientWebSocket::KHttpClientSendCloseOperation::IsActive()
{
    return _IsActive;
}

VOID
KHttpClientWebSocket::KHttpClientSendCloseOperation::SetActive(
    __in BOOLEAN IsActive
    )
{
    _IsActive = IsActive;
}

ULONG
KHttpClientWebSocket::KHttpClientSendCloseOperation::GetWinhttpError()
{
    return _WinhttpError;
}

VOID
KHttpClientWebSocket::KHttpClientSendCloseOperation::StartSendClose(
    __in KHttpClientWebSocket& ParentWebSocket,
    __in KAsyncContextBase::CompletionCallback CallbackPtr,
    __in HINTERNET WinhttpWebSocketHandle,
    __in CloseStatusCode const CloseStatusCode,
    __in_opt KSharedBufferStringA::SPtr CloseReason
    )
{
    KAssert(ParentWebSocket.IsInApartment());
    KAssert(CallbackPtr);

    Start(&ParentWebSocket, CallbackPtr);

    KAssert(! IsActive());
    SetActive(TRUE);

    if (CloseReason && CloseReason->LengthInBytes() > WINHTTP_WEB_SOCKET_MAX_CLOSE_REASON_LENGTH)
    {
        Complete(STATUS_INVALID_PARAMETER);
        return;
    }

    ULONG error;

    USHORT usStatus = static_cast<USHORT>(CloseStatusCode);
    PVOID pvReason = CloseReason ? (PVOID) *CloseReason : nullptr;
    ULONG reasonLength = CloseReason ? CloseReason->LengthInBytes() : 0;
    error = WinHttpWebSocketShutdown(
        WinhttpWebSocketHandle,
        usStatus,
        pvReason,
        reasonLength
        );

    if (error != NO_ERROR)
    {
        _WinhttpError = error;
        Complete(K_STATUS_WINHTTP_API_CALL_FAILURE);
        return;
    }

    //  Execution continues in WebSocketWinhttpCallback
}

VOID
KHttpClientWebSocket::KHttpClientSendCloseOperation::OnStart()
{
    //  No-op
}

VOID
KHttpClientWebSocket::KHttpClientSendCloseOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID
KHttpClientWebSocket::KHttpClientSendCloseOperation::OnReuse()
{
    _WinhttpError = NO_ERROR;
}

VOID
KHttpClientWebSocket::SendCloseCallback(
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase& CompletingOperation
    )
{
    KAssert(IsInApartment());
    KAssert(Parent == this);
    KAssert(&CompletingOperation == _SendCloseOperation.RawPtr());
    KAssert(IsCloseStarted());

    #if !DBG
        UNREFERENCED_PARAMETER(Parent);
        UNREFERENCED_PARAMETER(CompletingOperation);
    #endif

    KAssert(_SendCloseOperation->IsActive());
    _SendCloseOperation->SetActive(FALSE);

    if (GetConnectionStatus() == ConnectionStatus::Error)
    {
        return;
    }

    if (! NT_SUCCESS(_SendCloseOperation->Status()))
    {
        _WinhttpError= _SendCloseOperation->GetWinhttpError();
        FailWebSocket(_SendCloseOperation->Status());
        return;
    }

    SendChannelCompleted();
}
#pragma endregion KHttpClientWebSocket_Send


#pragma region KHttpClientWebSocketSendFragmentOperation
KDefineSingleArgumentCreate(KWebSocket::SendFragmentOperation, KHttpClientWebSocket::KHttpClientSendFragmentOperation, SendOperation, KHttpClientWebSocket&, WebSocketContext);

NTSTATUS
KHttpClientWebSocket::CreateSendFragmentOperation(
    __out SendFragmentOperation::SPtr& SendOperation
    )
{
    return KHttpClientSendFragmentOperation::Create(SendOperation, *this, GetThisAllocator(), GetThisAllocationTag());
}

KHttpClientWebSocket::KHttpClientSendFragmentOperation::KHttpClientSendFragmentOperation(
    __in KHttpClientWebSocket& WebSocketContext
    )
    :
        WebSocketSendFragmentOperation(WebSocketContext, UserOperationType::FragmentSend)
{
}

KHttpClientWebSocket::KHttpClientSendFragmentOperation::~KHttpClientSendFragmentOperation()
{
}

VOID
KHttpClientWebSocket::KHttpClientSendFragmentOperation::StartSendFragment(
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
KHttpClientWebSocket::KHttpClientSendFragmentOperation::OnStart()
{
    NTSTATUS status;

    status = static_cast<KHttpClientWebSocket&>(*_Context)._SendRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

VOID
KHttpClientWebSocket::KHttpClientSendFragmentOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID KHttpClientWebSocket::KHttpClientSendFragmentOperation::OnCompleted()
{
    _Buffer = nullptr;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}
#pragma endregion KHttpClientWebSocketSendFragmentOperation


#pragma region KHttpClientWebSocketSendMessageOperation
KDefineSingleArgumentCreate(KWebSocket::SendMessageOperation,KHttpClientWebSocket::KHttpClientSendMessageOperation, SendOperation, KHttpClientWebSocket&, WebSocketContext);

NTSTATUS
KHttpClientWebSocket::CreateSendMessageOperation(
    __out SendMessageOperation::SPtr& SendOperation
    )
{
    return KHttpClientSendMessageOperation::Create(SendOperation, *this, GetThisAllocator(), GetThisAllocationTag());
}

KHttpClientWebSocket::KHttpClientSendMessageOperation::KHttpClientSendMessageOperation(
    __in KHttpClientWebSocket& WebSocketContext
    )
    :
        WebSocketSendFragmentOperation(WebSocketContext, UserOperationType::MessageSend)
{
}

KHttpClientWebSocket::KHttpClientSendMessageOperation::~KHttpClientSendMessageOperation()
{
}

VOID
KHttpClientWebSocket::KHttpClientSendMessageOperation::StartSendMessage(
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
KHttpClientWebSocket::KHttpClientSendMessageOperation::OnStart()
{
    NTSTATUS status;

    status = static_cast<KHttpClientWebSocket&>(*_Context)._SendRequestQueue->Enqueue(*this);
    if (! NT_SUCCESS(status))
    {
        Complete(K_STATUS_API_CLOSED);
    }
}

VOID
KHttpClientWebSocket::KHttpClientSendMessageOperation::CompleteOperation(
    __in NTSTATUS Status
    )
{
    Complete(Status);
}

VOID KHttpClientWebSocket::KHttpClientSendMessageOperation::OnCompleted()
{
    _Buffer = nullptr;
    _Offset = 0;
    _BufferSizeInBytes = 0;
}
#pragma endregion KHttpClientWebSocketSendMessageOperation

#endif  //  #if NTDDI_VERSION >= NTDDI_WIN8

#endif  //  #if KTL_USER_MODE

// kernel proxy code would go here, when/if we need it
