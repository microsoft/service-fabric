/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kmgmtserver.h

    Description:
      Kernel Tempate Library (KTL): KMgmtServer

      Implements an HTTP-based management server by forming a bridge
      between KHttpServer and KIManagementOps.

    History:
      raymcc          16-Aug-2012           First version

--*/

#pragma once

//
//  KMgmtServer
//
//  Implements an HTTP-based management server.  Network requests over HTTP are automatically
//  routed to any matching registered provider.
//
//


class KMgmtServer :  public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(KMgmtServer);

public:
    enum  VerbType { eNone = 0, eGet = 1, eCreate = 2, eDelete = 3, eUpdate = 4, eQuery = 5, eInvoke = 6 };


    typedef KDelegate<VOID(__in ULONG StatusCode)> StatusCallback;

    // Create
    //
    // Creates an instance of KMgmtServer.  Multiple simultaneous instances
    // can coexist.
    //
    static NTSTATUS
    Create(
        __in  KAllocator& Allocator,
        __out KMgmtServer::SPtr& Server
        );


#if KTL_USER_MODE

    // Activate
    //
    // Activates the user mode server.
    //
    // This will not complete until Deactivate is called.  It is possible to immediately begin
    // registering providers, even though the HTTP startup sequence has not completed.
    //
    // Parameters:
    //      HttpServer                      A running KHttpServer instance.
    //      HttpUrlSuffix                   The URL suffix (relative to the full URL of the KHttpServer)
    //                                      to which to attach this KMgmtServer.
    //
    //      Description                     The role of this KMgmtServer instance
    //      Cb                              Status callback used to indicate when startup is complete.
    //
    //
    // Return value:
    //      STATUS_CONFLICING_ADDRESSES     If the URL conflicts with one used by other HTTP servers on the machine.
    //      STATUS_INVALID_PARAMETER
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    NTSTATUS
    Activate(
        __in KHttpServer::SPtr HttpServer,
        __in KUriView& HttpUrlSuffix,
        __in KStringView& Description,
        __in StatusCallback Cb,
        __in_opt KAsyncContextBase* const ParentAsync = nullptr,
        __in KAsyncContextBase::CompletionCallback DeactivateCompletion = 0,
        __in_opt KAsyncGlobalContext* const GlobalContextOverride = nullptr
        );


#else  // Kernel

   NTSTATUS Activate()
   {
       return STATUS_SUCCESS;
   }

#endif

    // Deactivate
    //
    // Requests a shutdown the server.
    // This returns immediately and shutdown is complete only after async Completion.
    //
    VOID
    Deactivate();

    VOID
    OnStart();

    // GetProvider
    //
    // Returns the locally provider associated with the URI.
    // This does not do prefix testing; the requested URI must match a previously registered one precisely.
    //
    //
    // Parameters:
    //      ProviderUri         The URI for which a provider is needed.
    //      Provider            Receives the provider. This is the actual
    //                          SPtr of the provider or a proxy to a kernel
    //                          provider if one exists. This does not do network operations.
    //
    // Return value:
    //      STATUS_SUCCESS
    //      STATUS_NOT_FOUND if no corresponding provider could be located.
    //      STATUS_INVALID_PARAMETER if the URI is invalid.
    //
    //
    NTSTATUS
    GetProvider(
        __in  KUriView& ProviderUri,
        __out KIManagementOps::SPtr& Provider,
        __in  BOOLEAN AcquireLock = TRUE
        );

    // RegisterProvider
    //
    // Registers a provider.
    //
    // Parameters:
    //      ProviderUri         The URI of the provider.
    //      Description         A textual description of the provider and its role.
    //      Provider            Pointer to the provider implementation.
    //      Flags               Use one of ( eUserMode | eKernelMode ) and one of
    //                          (eMatchPrefix | eMatchExact).
    //                          eMatchExact means that the provider is invoked only
    //                          if the URI matches exactly what is registered.  eMatchPrefix
    //                          will additionally forward all longer URIs that match the registration.
    //
    // Return value:
    //      STATUS_CONFLICTING_ADDRESSES    If the URI has already been claimed by some other registration.
    //      STATUS_SUCCESS
    //      STATUS_INVALID_PARAMETER
    //      STATUS_INSUFFICIENT_RESOURCES
    //
    enum { eUserMode = 1, eKernelMode = 2, eMatchPrefix = 4, eMatchExact = 8 };

    NTSTATUS
    RegisterProvider(
        __in KUriView& ProviderUri,
        __in KStringView& Description,
        __in KIManagementOps::SPtr Provider,
        __in ULONG Flags
        );

    // UnregisterProvider
    //
    // Unregisters a previously registered provider.
    //
    //
    NTSTATUS
    UnregisterProvider(
        __in KUriView& ProviderUri
        );


private:

    // FindProvider
    //
    // Does a prefix match to see if there is an owning provider.
    //
    NTSTATUS
    FindProvider(
        __in  KDynUri& Candidate,
        __out KIManagementOps::SPtr& Provider,
        __in  BOOLEAN AcquireLock = TRUE
        );

#if KTL_USER_MODE

    VOID
    HttpHandler(
        __in KHttpServerRequest::SPtr Request
        );

    NTSTATUS
    ProcessGet(
        __in KHttpServerRequest::SPtr Request
        );

    NTSTATUS
    ProcessPost(
        __in KHttpServerRequest::SPtr Request
        );

    NTSTATUS
    TryParseContent(
        __in  KHttpServerRequest::SPtr Request,
        __out KIMutableDomNode::SPtr&  RequestDom
        );

    VOID
    OpCompletionCallback(
        KAsyncContextBase* const Parent,
        KAsyncContextBase& Op
        );


    struct OpRequest : public KAsyncContextBase
    {
        friend class KMgmtServer;

        typedef KSharedPtr<OpRequest> SPtr;

        KMgmtServer::SPtr               _Parent;
        KUri::SPtr                      _ObjUri;
        KIManagementOps::SPtr           _Provider;
        KAsyncContextBase::SPtr         _ProviderOp;
        KMgmtServer::VerbType           _Verb;
        KUri::SPtr                      _InvokeVerb;
        KSharedPtr<KHttpServerRequest>  _HttpRequest;

        KIMutableDomNode::SPtr          _RequestDom;
        KGuid                           _RequestMessageId;
        ULONGLONG                       _ActivityId;
        KIDomNode::SPtr                 _RequestHeaders;
        KIDomNode::SPtr                 _RequestBody;
        KIDomNode::SPtr                 _Query;

        KIMutableDomNode::SPtr          _ResponseHeaders;
        KIMutableDomNode::SPtr          _ResponseBody;
        BOOLEAN                         _MoreData;
        KArray<KIMutableDomNode::SPtr>  _ResponseBodies;
        KString::SPtr                   _ResponseMessageText;

        // Methods
        //
        VOID
        Completion(
            KAsyncContextBase* const Parent,
            KAsyncContextBase& Op
            );

        VOID
        HttpCompletion(
            __in KSharedPtr<KHttpServerRequest> Request,
            __in NTSTATUS FinalStatus
            );

        VOID
        OnStart();

        NTSTATUS
        StartOp(
            __in_opt KAsyncContextBase* Parent,
            __in  KAsyncContextBase::CompletionCallback Callback
            );

        static OpRequest::SPtr
        Create(
            __in KAllocator& Alloc
            );

        OpRequest()
            : _ResponseBodies(GetThisAllocator())
        {
            _MoreData = FALSE;
            _ActivityId = 0;
        }

    };


    KHttpServer::SPtr _HttpServer;
    KUri::SPtr        _HttpUrlSuffix;

#endif

    friend struct OpRequest;

    struct ProviderRegistration
    {
        KUri::SPtr              _Uri;
        KString::SPtr           _Description;
        ULONG                   _Flags;
        KIManagementOps::SPtr   _Provider;

        ProviderRegistration()
        {
            _Flags = 0;
        }
    };

    BOOLEAN                      _Stopped;
    StatusCallback               _StatusCb;
    KSpinLock                    _TableLock;
    KArray<ProviderRegistration> _Providers;
};




