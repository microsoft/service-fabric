// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

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
class IKHttpServer : public KAsyncServiceBase
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

    NTSTATUS
    PostRead();

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
    ReadCompletion(
        __in KAsyncContextBase* const Parent,
        __in KAsyncContextBase& Op
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

