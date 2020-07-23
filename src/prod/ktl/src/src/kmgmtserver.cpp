
/*++

    (c) 2012 by Microsoft Corp. All Rights Reserved.

    kmgmtserver.cpp

    Description:
      Kernel Tempate Library (KTL): KStringView

      Management server dispatching engine

    History:
      raymcc          27-Feb-2012         Initial version.

--*/

#include <ktl.h>


// Constructor
//

#if KTL_USER_MODE
KMgmtServer::KMgmtServer()
   :
   _Providers(GetThisAllocator())
{
    _Stopped = FALSE;
}
#else
KMgmtServer::KMgmtServer()
   :
   _Providers(GetThisAllocator())
{
}
#endif

KMgmtServer::~KMgmtServer()
{
}


// Create
//
// Creates an instance of KMgmtServer.  Multiple simultaneous instances
// can coexist.
//
NTSTATUS
KMgmtServer::Create(
    __in  KAllocator& Allocator,
    __out KMgmtServer::SPtr& Server
    )
{
    Server = _new(KTL_TAG_MGMT_SERVER, Allocator) KMgmtServer();
    if (!Server)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    return STATUS_SUCCESS;
}


//
//  GetProvider
//
//  Retrieves the provider based on the exact URI used to register it.
//
NTSTATUS
KMgmtServer::GetProvider(
    __in  KUriView& ProviderUri,
    __out KIManagementOps::SPtr& Provider,
    __in  BOOLEAN AcquireLock
    )
{
    if (AcquireLock)
    {
        _TableLock.Acquire();
    }

    KFinally([&](){  if (AcquireLock) _TableLock.Release(); });

    for (ULONG i = 0; i < _Providers.Count(); i++)
    {
        ProviderRegistration& Reg = _Providers[i];
        if (Reg._Uri->Compare(ProviderUri))
        {
            Provider = Reg._Provider;
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}

//
//  FindProvider
//
//  Retrieves the provider which owns the specified the URI, if any.  This
//  takes into account fuzzy post-fix matching of the URI and the owning provider.
//
NTSTATUS
KMgmtServer::FindProvider(
    __in  KDynUri& Candidate,
    __out KIManagementOps::SPtr& Provider,
    __in  BOOLEAN AcquireLock
    )
{
    NTSTATUS Res;
    if (AcquireLock)
    {
        _TableLock.Acquire();
    }

    KFinally([&](){  if (AcquireLock) _TableLock.Release(); });

    for (ULONG i = 0; i < _Providers.Count(); i++)
    {
        ProviderRegistration& Reg = _Providers[i];
        KDynUri Suffix(GetThisAllocator());

        Res = Reg._Uri->GetSuffix(Candidate, Suffix);

        if (NT_SUCCESS(Res))
        {
            // If a prefix match is allowed, we're done.
            //
            if (Reg._Flags & eMatchPrefix)
            {
                Provider = Reg._Provider;
                return STATUS_SUCCESS;
            }
            // If here, an exact match is required.
            //
            else if (Suffix.GetSegmentCount() == 0)
            {
                Provider = Reg._Provider;
                return STATUS_SUCCESS;
            }
            else
            {
                break;
            }
        }
    }

    return STATUS_NOT_FOUND;
}

//
//  UnregisterProvider
//
//  Unregisters a provider.  Does not interfere with any requests currently
//  in progress for that provider, but prevents future requests from being
//  dispatched.
//
NTSTATUS
KMgmtServer::UnregisterProvider(
    __in KUriView& ProviderUri
    )
{
    K_LOCK_BLOCK(_TableLock)
    {
        for (ULONG i = 0; i < _Providers.Count(); i++)
        {
            ProviderRegistration& Reg = _Providers[i];
            if (Reg._Uri->Compare(ProviderUri))
            {
                _Providers.Remove(i);
                return STATUS_SUCCESS;
            }
        }
    }
    return STATUS_NOT_FOUND;
}


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
//      STATUS_ SUCCESS
//      STATUS_INVALID_PARAMETER
//      STATUS_INSUFFICIENT_RESOURCES
//

NTSTATUS
KMgmtServer::RegisterProvider(
    __in KUriView& ProviderUri,
    __in KStringView& Description,
    __in KIManagementOps::SPtr Provider,
    __in ULONG Flags
    )
{    
    NTSTATUS Res = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(Flags);            

    if (!Provider || !ProviderUri.IsValid())
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    K_LOCK_BLOCK(_TableLock)
    {
        // Test to see if an existing provider owns this URI:
        //
        KIManagementOps::SPtr PrevProvider;
        Res = GetProvider(ProviderUri, PrevProvider, FALSE);
        if (NT_SUCCESS(Res))
        {
            Res = STATUS_CONFLICTING_ADDRESSES;
            break;
        }

        // Clone the Description string.
        //
        KString::SPtr Descrip = KString::Create(Description, GetThisAllocator());
        if (!Descrip)
        {
            Res = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        if (!Descrip->CopyFrom(Description, TRUE))
        {
            Res = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        // Clonse the URI

        KUri::SPtr UriPtr;
        Res = KUri::Create(ProviderUri, GetThisAllocator(), UriPtr);
        if (!NT_SUCCESS(Res))
        {
            break;
        }

        ProviderRegistration Reg;
        Reg._Uri = UriPtr;
        Reg._Description = Descrip;
        Reg._Provider = Provider;

        Res = _Providers.Append(Reg);
    }

    return Res;
}




// Activate
//
//

#if KTL_USER_MODE

NTSTATUS
KMgmtServer::Activate(
    __in KHttpServer::SPtr HttpServer,
    __in KUriView& HttpUrlSuffix,
    __in KStringView& Description,
    __in StatusCallback Cb,
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase::CompletionCallback DeactivateCompletion,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )

{
    NTSTATUS Res;

    UNREFERENCED_PARAMETER(Description);                

    if (!HttpServer || !HttpUrlSuffix.IsValid())
    {
        return STATUS_INVALID_PARAMETER;
    }

    Res = KUri::Create(HttpUrlSuffix, GetThisAllocator(), _HttpUrlSuffix);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    Res = HttpServer->RegisterHandler(*_HttpUrlSuffix, FALSE, KHttpServer::RequestHandler(this, &KMgmtServer::HttpHandler));
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    _HttpServer = HttpServer;
    Start(ParentAsync, DeactivateCompletion, GlobalContextOverride);

    _StatusCb = Cb;

    return STATUS_SUCCESS;
}

VOID
KMgmtServer::Deactivate()
{
    NTSTATUS Res;

    Res = _HttpServer->UnregisterHandler(*_HttpUrlSuffix);
    if (!NT_SUCCESS(Res))
    {
        KFatal(FALSE);
    }
    _Stopped = TRUE;
    Complete(STATUS_SUCCESS);
    ReleaseActivities();
}

VOID
KMgmtServer::OnStart()
{
    AcquireActivities();
   _StatusCb(STATUS_SUCCESS);
}


//
//  KMgmtServer::HttpHandler
//
//  Vectors the Get vs. Post
//
VOID
KMgmtServer::HttpHandler(
    __in KHttpServerRequest::SPtr Request
    )
{
    BOOLEAN bRes = TryAcquireActivities();
    if (!bRes)
    {
        Request->SendResponse(KHttpUtils::ServiceUnavailable);
        return;
    }
    KFinally([&](){ ReleaseActivities(); });

    if (_Stopped)
    {
        Request->SendResponse(KHttpUtils::ServiceUnavailable);
        return;
    }

    // Examine the request type
    //

    NTSTATUS Res;

    KHttpUtils::OperationType Op = Request->GetOpType();
    if (Op == KHttpUtils::eHttpGet)
    {
        Res = ProcessGet(Request);
    }
    else
    {
        Res = ProcessPost(Request);
    }
    // BUGBUG: Check for other HTTP verbs explicitly

    // Now check Res.  If it is STATUS_SUCCESS or STATUS_PENDING,
    // we just return.  The response will be sent (or already has been)
    // from somewhere else.

    if (Res == STATUS_SUCCESS || Res == STATUS_PENDING)
    {
        return;
    }

    // All other codes mean that there was a failsure such that
    // we need to send the response from here.
    //
    else if (Res == STATUS_INSUFFICIENT_RESOURCES)
    {
        Request->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }
    else if (Res == STATUS_UNSUCCESSFUL)
    {
        Request->SendResponse(KHttpUtils::BadRequest);
        return;
    }
    else if (Res == STATUS_NOT_FOUND)
    {
        Request->SendResponse(KHttpUtils::NotFound);
        return;
    }

    // If here, we aren't handling the situation properly.
    //
    Request->SendResponse(KHttpUtils::InternalServerError);
}

//
//  KMgmtServer::ProcessGet
//
//  Processes a raw HTTP get where all the request info is encoded in the URL.
//  This simply fakes up a regular POST-based Get and then dispatches to the provider
//  as if an XML-based body had been posted.
//
NTSTATUS
KMgmtServer::ProcessGet(
    __in KHttpServerRequest::SPtr Request
    )
{
    return STATUS_NOT_IMPLEMENTED;
}

//
//  KMgmtServer::ProcessPost
//
//  All the XML-HTTP style of REST messages come through this entry point.
//
//
NTSTATUS
KMgmtServer::ProcessPost(
    __in KHttpServerRequest::SPtr Request
    )
{
    NTSTATUS Res;

    // Try to get the DOM SPtr
    //
    KIMutableDomNode::SPtr RequestDom;
    Res = TryParseContent(Request, RequestDom);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KVariant vTargetUri;
    Res = RequestDom->GetValue(KDomPath(L"KtlHttpMessage/MessageHeader/RequestMessageHeader/ObjectUri"), vTargetUri);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Find the provider.
    //
    KUri::SPtr ObjUri = (KUri::SPtr) vTargetUri;
    KIManagementOps::SPtr Provider;
    Res = FindProvider(*ObjUri, Provider);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    KVariant vVerb;
    Res = RequestDom->GetValue(KDomPath(L"KtlHttpMessage/MessageHeader/RequestMessageHeader/OperationVerb"), vVerb);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }



    // Build an op request
    //
    OpRequest::SPtr OpReq = OpRequest::Create(GetThisAllocator());
    if (!OpReq)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    // Get the message ID for logging & correlation
    //
    KVariant vMsgId;
    Res = RequestDom->GetValue(KDomPath(L"KtlHttpMessage/MessageHeader/RequestMessageHeader/MessageId"), vMsgId);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    OpReq->_RequestMessageId = GUID(vMsgId);

    // Figure out the verb
    //
    KUri::SPtr VerbUri= (KUri::SPtr) vVerb;

    if (VerbUri->Compare(KUriView(L"ktl.verb:/Get")))
    {
        OpReq->_Verb = eGet;
    }
    else if (VerbUri->Compare(KUriView(L"ktl.verb:/Create")))
    {
        OpReq->_Verb = eCreate;
    }
    else if (VerbUri->Compare(KUriView(L"ktl.verb:/Delete")))
    {
        OpReq->_Verb = eDelete;
    }
    else if (VerbUri->Compare(KUriView(L"ktl.verb:/Update")))
    {
        OpReq->_Verb = eUpdate;
    }
    else if (VerbUri->Compare(KUriView(L"ktl.verb:/Query")))
    {
        OpReq->_Verb = eQuery;
    }
    else   // Bad verb
    {
        OpReq->_Verb = eInvoke;
        OpReq->_InvokeVerb = VerbUri;
        return STATUS_UNSUCCESSFUL;
    }

    // Other fields
    //
    OpReq->_ObjUri = ObjUri;
    OpReq->_Provider = Provider;
    OpReq->_HttpRequest = Request;
    OpReq->_RequestDom = RequestDom;
    OpReq->_ResponseBody = 0;
    OpReq->_ResponseHeaders = 0;
    OpReq->_Parent = this;

    Res = RequestDom->GetNode(KDomPath(L"KtlHttpMessage/MessageHeader"),OpReq->_RequestHeaders);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = RequestDom->GetNode(KDomPath(L"KtlHttpMessage/MessageBody"), OpReq->_RequestBody);
    if (!NT_SUCCESS(Res))
    {
        // Get/Delete don't require bodies.  The other verbs require the body.
        //
        if (!(OpReq->_Verb == eGet || OpReq->_Verb == eDelete))
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    // If we get here, activity count may  be bumped up in the deeper code.
    //
    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KMgmtServer::OpCompletionCallback);

    // This transfers ownership of the message to the OpReq from this point on.
    //
    Res = OpReq->StartOp(nullptr, Cb);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    return STATUS_PENDING;
}

VOID
KMgmtServer::OpCompletionCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    // TBD
    UNREFERENCED_PARAMETER(Parent);                
    UNREFERENCED_PARAMETER(Op);
}


NTSTATUS
KMgmtServer::OpRequest::StartOp(
    __in_opt KAsyncContextBase* Parent,
    __in  KAsyncContextBase::CompletionCallback Callback
    )
{
    Start(Parent, Callback);
    return STATUS_PENDING;
}


KMgmtServer::OpRequest::SPtr
KMgmtServer::OpRequest::Create(
    __in KAllocator& Alloc
    )
{
    return _new(KTL_TAG_MGMT_SERVER, Alloc) KMgmtServer::OpRequest;
}


// KMgmtServer::OpRequest::Completion
//
// Composes the response and delivers it to the sender.
//
VOID
KMgmtServer::OpRequest::Completion(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    UNREFERENCED_PARAMETER(Op);
    
    KLocalString<KStringView::MaxStandardType> WorkBuffer;

    // Cobble together the HTTP response
    //
    KStringView BaseMessage(
        L"<KtlHttpMessage\n"
        L"  xmlns=\"http://schemas.microsoft.com/2012/03/rvd/adminmessage\" \n"
        L"  xmlns:ktl=\"http://schemas.microsoft.com/2012/03/ktl\" \n"
        L"  xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" \n"
        L"  xmlns:xs=\"http://www.w3.org/2001/XMLSchema\" \n"
        L">\n"
        L"  <MessageHeader>\n"
        L"  </MessageHeader>\n"
        L"  <MessageBody>\n"
        L"  </MessageBody>\n"
        L"</KtlHttpMessage>\n"
        );

    KIMutableDomNode::SPtr ResponseMessage;
    NTSTATUS Res = KDom::FromString(BaseMessage, GetThisAllocator(), ResponseMessage);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }

    // Merge in the stuff from the provider.
    //
    //
    KIDomNode::SPtr MsgBody;
    Res = ResponseMessage->GetNode(KDomPath(L"KtlHttpMessage/MessageBody"), MsgBody);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InternalServerError);
        return;
    }

    if (_ResponseBody)
    {
        Res = MsgBody->ToMutableForm()->AddChildDom(*_ResponseBody);
        if (!NT_SUCCESS(Res))
        {
            _HttpRequest->SendResponse(KHttpUtils::InternalServerError);
            return;
        }
    }


    // Manufacture the required response headers. These are defaults, but the provider
    // is allowed to override (in the next step).
    //
    // [1] Current time
    // [2] InResponseToMessage (GUID)
    // [3]

    KStringView ResponseHeaderTemplate(
        L"<ResponseMessageHeader>\n"
        L"    <To xsi:type=\"ktl:URI\">$1</To>\n"
        L"    <From xsi:type=\"ktl:URI\">$2</From>\n"
        L"    <MessageId xsi:type=\"ktl:GUID\">$3</MessageId>\n"
        L"    <InResponseToMessageId xsi:type=\"ktl:GUID\">$4</InResponseToMessageId>\n"
        L"    <SentTimestamp xsi:type=\"ktl:DATETIME\">$5</SentTimestamp>\n"
        L"    <ActivityId xsi:type=\"ktl:ULONGLONG\">$6</ActivityId>\n"
        L"    <ObjectUri xsi:type=\"ktl:URI\">$7</ObjectUri>\n"
        L"    <Status xsi:type=\"ktl:ULONG\">$8</Status>\n"
        L"    <StatusText xsi:type=\"ktl:STRING\">$9</StatusText>\n"
        L"</ResponseMessageHeader>\n"
        );

    KString::SPtr ResponseHeadersStr = KString::Create(ResponseHeaderTemplate, GetThisAllocator());
    if (!ResponseHeadersStr)
    {
        _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }

    // Fill in the template values.
    //
    // "To"
    {
        KStringView target(L"$1");
        KStringView replacement(L"ktl.keyword:/sender");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, replacement);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }


    // "From"
    //
    KUri::SPtr FullUrl;
    Res = _HttpRequest->GetUrl(FullUrl);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InternalServerError);
        return;
    }

    {
        KStringView target(L"$2");
        KStringView replacement(*FullUrl);
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, replacement);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // "MessageId"
    //
    {
        KGuid MessageId;
        MessageId.CreateNew();
        WorkBuffer.FromGUID(MessageId);
        KStringView target(L"$3");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, WorkBuffer);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // "InResponseToMessage"
    //

    {
        WorkBuffer.FromGUID(_RequestMessageId);

        KStringView target(L"$4");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, WorkBuffer);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }


    // "SentTimestamp"   (Defaults to 'now')
    //
    {
        KDateTime Now = KDateTime::Now();
        WorkBuffer.FromSystemTimeToIso8601(Now);
        KStringView target(L"$5");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, WorkBuffer);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // "ActivityId"
    //
    {
        WorkBuffer.FromULONGLONG(_ActivityId);
        KStringView target(L"$6");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, WorkBuffer);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // "ObjectUri"
    //
    {
        KStringView target(L"$7");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, *_ObjUri);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }


    // "Status"
    //
    {
        KStringView target(L"$8");
        KStringView replacement(L"0");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, replacement);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // "StatusText"
    //
    {
        KStringView target(L"$9");
        KStringView replacement(L"(unassigned)");
        BOOLEAN bRes = ResponseHeadersStr->Replace(target, replacement);
        if (!bRes)
        {
            if (!NT_SUCCESS(Res))
            {
                _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                return;
            }
        }
    }

    // Convert this text to a Dom object.
    //
    KIMutableDomNode::SPtr HeaderDom;
    Res = KDom::FromString(*ResponseHeadersStr, GetThisAllocator(), HeaderDom);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }


    // Now examine headers returned to us by the provider and merge
    // in any overrides to the defaults we have just computed.
    //
    if (_ResponseHeaders)
    {
        // Loop through the values.
        //
        KArray<KIDomNode::SPtr> Children(GetThisAllocator());

        Res = _ResponseHeaders->GetAllChildren(Children);
        if (NT_SUCCESS(Res))
        {
            for (ULONG i = 0; i < Children.Count(); i++)
            {
                KIDomNode::SPtr Node = Children[i];

                // Add the override.
                //
                Res = HeaderDom->SetChildValue(Node->GetName(), Node->GetValue());
                if (!NT_SUCCESS(Res))
                {
                    _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
                    return;
                }
            }
        }
    }

    // Attach all these headers to the message.
    //
    KIDomNode::SPtr Hdr;
    Res = ResponseMessage->GetNode(KDomPath(L"KtlHttpMessage/MessageHeader"), Hdr);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InternalServerError);
        return;
    }

    Res = Hdr->ToMutableForm()->AddChildDom(*HeaderDom);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InternalServerError);
        return;
    }

    // Set the content, content-type and dispatch the response.
    //
    Res = KDom::ToString(ResponseMessage, GetThisAllocator(), _ResponseMessageText);
    if (!NT_SUCCESS(Res))
    {
        _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }

    if (!_ResponseMessageText->EnsureBOM())
    {
        _HttpRequest->SendResponse(KHttpUtils::InsufficientResources);
        return;
    }

    _HttpRequest->SetResponseContent(*_ResponseMessageText);

    // For now, we are hard-writing the response content-type.
    //
    KStringView value(L"text/xml; encoding=\"utf-16\"");
    _HttpRequest->SetResponseHeaderByCode(KHttpUtils::HttpHeaderContentType, value);
    _HttpRequest->SendResponse(KHttpUtils::Ok, "Success", KHttpServerRequest::ResponseCompletion(this, &KMgmtServer::OpRequest::HttpCompletion));
}

VOID
KMgmtServer::OpRequest::HttpCompletion(
    __in KSharedPtr<KHttpServerRequest> Request,
    __in NTSTATUS FinalStatus
    )
{
    UNREFERENCED_PARAMETER(Request);            
    UNREFERENCED_PARAMETER(FinalStatus);
    
    _Parent->ReleaseActivities();
    Complete(STATUS_SUCCESS);
}


VOID
KMgmtServer::OpRequest::OnStart()
{
    NTSTATUS Res;

    // If we get this far, we are going to invoke a provider asynchronously.
    // Since we may be in shutdown, we have to be careful to handle the activity
    // count.
    //
    BOOLEAN bRes = _Parent->TryAcquireActivities();
    if (!bRes)
    {
        Complete(STATUS_SHUTDOWN_IN_PROGRESS);
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KMgmtServer::OpRequest::Completion);

    switch (_Verb)
    {
        case KMgmtServer::eGet:
            Res = _Provider->StartGetAsync(
                 *_ObjUri,
                 _RequestHeaders,
                _ResponseHeaders,
                _ResponseBody,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        case KMgmtServer::eCreate:
            Res = _Provider->StartCreateAsync(
                *_ObjUri,
                _RequestBody,
                _RequestHeaders,
                _ResponseHeaders,
                _ResponseBody,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        case KMgmtServer::eDelete:
            Res = _Provider->StartDeleteAsync(
                *_ObjUri,
                _RequestHeaders,
                _ResponseHeaders,
                _ResponseBody,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        case KMgmtServer::eUpdate:
            Res = _Provider->StartUpdateAsync(
                *_ObjUri,
                _RequestBody,
                _RequestHeaders,
                _ResponseHeaders,
                _ResponseBody,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        case KMgmtServer::eQuery:
            Res = _Provider->StartQueryAsync(
                *_ObjUri,
                _Query,
                _RequestHeaders,
                _ResponseHeaders,
                _ResponseBodies,
                _MoreData,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        case KMgmtServer::eInvoke:
            Res = _Provider->StartInvokeAsync(
                *_ObjUri,
                *_InvokeVerb,
                _RequestBody,
                _RequestHeaders,
                _ResponseHeaders,
                _ResponseBody,
                Cb,
                &_ProviderOp,
                this
                );
            break;

        default:
            _Parent->ReleaseActivities();
            Complete(STATUS_INTERNAL_ERROR);
    }
}




//
//  KMgmtServer::TryParseContet
//
//  Attempts to parse the content in the hopes it is XML.
//
//  Return types of STATUS_UNSUCCESSFUL should result in a "Bad Request" (HTTP 400) to the HTTP Client.
//
NTSTATUS
KMgmtServer::TryParseContent(
    __in  KHttpServerRequest::SPtr Request,
    __out KIMutableDomNode::SPtr&  RequestBody
    )
{
    // Check the charset
    //
    KString::SPtr ContType;
    NTSTATUS Res = Request->GetRequestHeaderByCode(KHttpUtils::HttpHeaderContentType, ContType);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    // Search the content type.
    // We insist on text/xml or application/xml
    //
    //
    BOOLEAN bRes;
    ULONG Pos;

    bRes = ContType->Search(KStringView(L"text/xml"), Pos);
    if (!bRes)
    {
        bRes = ContType->Search(KStringView(L"application/xml"), Pos);
        return STATUS_UNSUCCESSFUL;
    }

    // If here, we don't know if it is UNICODE or ASCII yet.
    // So we get the buffer and look for the BOM.
    //
    // If we don't see UTF-16 in the ContType, we assume ANSI/ASCII.
    //
    BOOLEAN Unicode = FALSE;

    bRes = ContType->Search(KStringView(L"utf-16"), Pos);
    if (bRes)
    {
        Unicode = TRUE;
    }

    KBuffer::SPtr EntityBody;
    Res = Request->GetRequestContent(EntityBody);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (EntityBody->QuerySize() < 4)
    {
        // Not large enough to have a BOM or any XML
        //
        return STATUS_UNSUCCESSFUL;
    }

    if (Unicode)
    {
        KStringView UnicodeView(LPWSTR(EntityBody->GetBuffer()), 2, 2);
        if (UnicodeView.HasBOM())
        {
            Res = KDom::CreateFromBuffer(EntityBody, GetThisAllocator(), KTL_TAG_MGMT_SERVER, RequestBody);
            return Res;
        }
        else
        {
            return STATUS_UNSUCCESSFUL;
        }
    }
    else
    {
        // Make an UNICODE buffer for the parser, since the incoming request was UTF-8

    }

    return STATUS_SUCCESS;
}


//
// ************** Kernel Mode **********************
//

#else


#endif



