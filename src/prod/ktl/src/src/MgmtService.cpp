/*--

Copyright (c) 2011  Microsoft Corporation

Module Name:

    lsmanagement.cpp

Abstract:

    This file implements a management service

Author:

    Alan Warwick (AlanWar) 2-July-2012

Environment:

Notes:

Revision History:

--*/

#include <ktl.h>
#include <ktrace.h>
#include <kmgmt.h>
#include <kmgmtserver.h>
#include "MgmtService.h"

//const KTagType KManagementService::_ThisTypeTag = "KMS-KMS";
//const KTagType KManagementService::ManagementRequestContext::_ThisTypeTag = "KMS-MRCC";

KManagementService::KManagementService(
) : _Description(GetThisAllocator()),
    _ProviderUri(GetThisAllocator()),
    _UrlPrefix(GetThisAllocator())
{
//    SetTypeTag(_ThisTypeTag); 
    _State = Created;
    
}

KManagementService::~KManagementService()
{
}

NTSTATUS KManagementService::Create(
    __out KManagementService::SPtr& Service,
    __in  KAllocator& Allocator,
    __in  ULONG AllocationTag
)
{
    NTSTATUS status;
    
    Service = _new (AllocationTag, Allocator) KManagementService();
    if (Service == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = Service->Status();
    if (!NT_SUCCESS(status))
    {
        Service = nullptr;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS KManagementService::Activate(
    __in  KMgmtServer::SPtr MgmtServer,
    __in  KUriView& ProviderUri,
    __in  KStringView& Description,
    // TODO: list of objects/URI that are handled
    __in  KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    BOOLEAN b;

    _State = Created;
    
    b = _Description.CopyFrom(Description);
    if (! b)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    b = _ProviderUri.CopyFrom(ProviderUri);
    if (! b)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }
    
    _MgmtServer = MgmtServer;
    
    if (ChildAsync)
    {
        *ChildAsync = this;
    }
    
    Start(Parent, Callback, GlobalContextOverride);

    return(STATUS_PENDING);
}

VOID KManagementService::Deactivate(
    )
{
    KAssert(_State == Running);
    Cancel();
}

VOID KManagementService::OnStart(
    )
{
    FSMContinue(STATUS_SUCCESS);
}

VOID KManagementService::OnReuse(
    )
{
    _State = Created;
    _MgmtServer = nullptr;
    _Description.Zero();
    _UrlPrefix.Zero();
    _ProviderUri.Zero();
}

VOID KManagementService::OnCancel(
    )
{
    KAssert(_State == Running);

    Complete(STATUS_SUCCESS);
}
        
VOID KManagementService::CompletionRoutine(
    __in_opt KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    FSMContinue(Async.Status());
}


VOID KManagementService::FSMContinue(
    __in NTSTATUS Status
    )
{
    NTSTATUS status;
    KAsyncContextBase::CompletionCallback completion(this, &KManagementService::CompletionRoutine);

    switch(_State)
    {
        //
        // On initial entry, go ahead and activate the management server
        //
        case Created:
        {
            KFatal(NT_SUCCESS(Status));

            //
            // Register the provider
            //
            _State = Registering;
// TODO: Reenable when Raymond is ready
#if 0
            Status = _MgmtServer->RegisterProvider(KUriView(_ProviderUri),
                                                   _Description,
                                                   this,
                                                   KMgmtServer::eUserMode | KMgmtServer::eMatchPrefix);
            if (! NT_SUCCESS(Status))
            {
                //
                // Register provider failed. We should complete ourselves with
                // an error.
                //
                KTraceFailedAsyncRequest(Status, this, 0, 0);
                _State = CompletedWithError;
                Complete(Status);
                break;
            }
#endif
            _State = Running;
            FSMContinue(STATUS_SUCCESS);
            
            break;
        }

        //
        // The management server is up and running, lets go and
        // handle requests
        //
        case Running:
        {
            break;
        }

        case Completed:
        case CompletedWithError:
        {
            //
            // Completed is not a state where callbacks occur
            //
            KFatal(FALSE);
            break;
        }
        
        default:
        {
            KFatal(FALSE);
            break;
        }
    }   
}


//
// Callbacks from KMgmtServer when incoming requests are received
//

KManagementService::ManagementRequestContext::ManagementRequestContext(
    __in KManagementService& Kms,
    __in KIDomNode::SPtr RequestHeaders,                                       
    __in KIDomNode::SPtr RequestBody,
    __in KIMutableDomNode::SPtr& ResponseHeaders,
    __in KIMutableDomNode::SPtr& ResponseBody
    ) : _ObjectType(GetThisAllocator()),
        _Target(GetThisAllocator()),
        _ObjectUri(GetThisAllocator()),
        _InvokeVerb(GetThisAllocator()),
        _Kms(Kms),
        _RequestHeaders(RequestHeaders),
        _RequestBody(RequestBody),
        _ResponseHeaders(ResponseHeaders),
        _ResponseBody(ResponseBody),
        _State(Unassigned)
{
}

KManagementService::ManagementRequestContext::~ManagementRequestContext(
    )
{
}

NTSTATUS KManagementService::ManagementRequestContext::CopyNodeValue(
    __in KIDomNode::SPtr Source,
    __in LPCWSTR SourceChild,
    __in KIMutableDomNode::SPtr Destination,
    __in LPCWSTR DestinationChild   
    )
{
    NTSTATUS status;
    KVariant value;
    KIMutableDomNode::SPtr childNode;
    
    status = Source->GetChildValue(KIDomNode::QName(SourceChild), value);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = Destination->AddChild(KIDomNode::QName(DestinationChild), childNode);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    status = childNode->SetValue(value);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}


NTSTATUS KManagementService::ManagementRequestContext::InitializeResponseHeaders(
    )
{
    NTSTATUS status;    
    KIDomNode::SPtr requestMessageHeader;
    KIMutableDomNode::SPtr mutableMessageHeader;
    
    //
    // Move specific values from the request header into the response
    // header:
    //
    //     ReplyTo --> To
    //     MessageId --> InResponseToMessageId
    //     MessageId --> RelatesToMessageId
    //     ActivityId --> ActivityId
    //     ObjectUri --> ObjectUri
    //

    status = _ResponseHeaders->AddChild(KIDomNode::QName(L"ResponseMessageHeader"), _ResponseMessageHeader);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _RequestHeaders->GetChild(KIDomNode::QName(L"RequestMessageHeader"), requestMessageHeader);
    if (! NT_SUCCESS(status))
    {
        _ResponseMessageHeader = nullptr;
        return(status);
    }

    //
    //     ReplyTo --> To
    //
    status = CopyNodeValue(requestMessageHeader,      L"ReplyTo",
                           _ResponseMessageHeader,    L"To");
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    //     MessageId --> InResponseToMessageId
    //
    status = CopyNodeValue(requestMessageHeader,      L"MessageId",
                           _ResponseMessageHeader,    L"InResponseToMessageId");
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    //     MessageId --> RelatesToMessageId
    //
    status = CopyNodeValue(requestMessageHeader,      L"MessageId",
                           _ResponseMessageHeader,    L"RelatesToMessageId");
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    //     ActivityId --> ActivityId
    //
    status = CopyNodeValue(requestMessageHeader,      L"ActivityId",
                           _ResponseMessageHeader,    L"ActivityId");
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    //     ObjectUri --> ObjectUri
    //
    status = CopyNodeValue(requestMessageHeader,      L"ObjectUri",
                           _ResponseMessageHeader,    L"ObjectUri");
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS KManagementService::ManagementRequestContext::InitializeForUse(
    __in Verbs Verb,
    __in KUriView& Target
    )
{
    NTSTATUS status;
    BOOLEAN b;
    KIDomNode::SPtr node;
    KVariant value;

    status = InitializeForUseDerived(Verb, Target);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    //
    // Remember the verb and the target URI
    //
    _Verb = Verb;   
    status = _Target.CopyFrom(Target);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    //
    // Pull out the ObjectUri from the header in case anyone needs it
    //
    status = KManagementService::FindDOMNode(*_RequestHeaders,
                                             KStringView(L"RequestMessageHeader\\ObjectUri"),
                                             GetThisAllocator(),
                                             node);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    value = node->GetValue();
    b = _ObjectUri.CopyFrom(static_cast<KStringView&>(value));
    if (! b)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    //
    // Build initial response headers DOM and copy the pertinent fields
    // from the request header into the response header
    //
    status = KDom::CreateEmpty(_ResponseHeaders,
                               GetThisAllocator(),
                               TAG_KMANAGEMENT_SERVICE);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = _ResponseHeaders->SetName(KIDomNode::QName(L"MessageHeader"));
    if (! NT_SUCCESS(status))
    {
        _ResponseHeaders = nullptr;
        return(status);
    }
    
    status = InitializeResponseHeaders();
    if (! NT_SUCCESS(status))
    {
        _ResponseHeaders = nullptr;
        return(status);
    }
    
    _State = Initial;
    
    return(STATUS_SUCCESS);
}

VOID KManagementService::ManagementRequestContext::OnReuse(
    )
{
    //
    // invoke any derived classes
    //
    OnReuseDerived();
    
    _State = Unassigned;
    
    _RequestHeaders = nullptr;
    _RequestBody = nullptr; 
    _ResponseHeaders = nullptr;
    _ResponseBody = nullptr;
    _Target.Zero();
    _ObjectUri.Zero();
    _Verb = KManagementService::NoVerb;
    _InvokeVerb.Zero();
}

VOID KManagementService::ManagementRequestContext::OnCancel(
    )
{
    // TODO:
}

VOID KManagementService::ManagementRequestContext::OnStart(
    )
{
    KAssert(_State == Initial);

    //
    // dispatch to appropriate handler
    //
    _State = Dispatch;

    switch (_Verb)
    {
        case KManagementService::GetVerb:
        {
            _Kms.GetHandler(this);
            break;
        }

        case KManagementService::CreateVerb:
        {
            _Kms.CreateHandler(this);                                      
            break;
        }

        case KManagementService::UpdateVerb:
        {
            _Kms.UpdateHandler(this);                                      
            break;
        }

        case KManagementService::QueryVerb:
        {
            _Kms.QueryHandler(this);                                       
            break;
        }

        case KManagementService::DeleteVerb:
        {
            _Kms.DeleteHandler(this);                                      
            break;
        }

        case KManagementService::InvokeVerb:
        {
            _Kms.InvokeHandler(this);
            break;
        }

        default:
        {
            KAssert(FALSE);

            KTraceFailedAsyncRequest(STATUS_UNSUCCESSFUL, this, 0, 0);
            _State = CompletedWithError;
            Complete(STATUS_UNSUCCESSFUL);
            break;                  
        }
    }
}


NTSTATUS KManagementService::ManagementRequestContext::SetStandardResponse(
    __in NTSTATUS FinalStatus,
    __in KStringView* FinalStatusText
    )
{
    NTSTATUS status;
    KIMutableDomNode::SPtr childNode;
    
    status = _ResponseMessageHeader->AddChild(KIDomNode::QName(L"Status"), childNode);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }
    
    KVariant value(FinalStatus);
    status = childNode->SetValue(value);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    childNode = nullptr;

    if (FinalStatusText != NULL)
    {
        status = _ResponseMessageHeader->AddChild(KIDomNode::QName(L"StatusText"), childNode);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }

        KVariant value = KVariant::Create(*FinalStatusText, GetThisAllocator());
        if (! NT_SUCCESS(value.Status()))
        {
            return(status);
        }
        
        status = childNode->SetValue(value);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    } else {
        // TODO: use FormatMessage to convert NTSTATUS to string
    }

    return(STATUS_SUCCESS); 
}

VOID KManagementService::ManagementRequestContext::CompleteRequest(
    __in NTSTATUS Status,
    __in NTSTATUS ResponseStatus,
    __in KStringView* ResponseStatusText
    )
{
    SetStandardResponse(ResponseStatus,
                        ResponseStatusText);
    
    Complete(Status);

    _Kms.ReleaseActivities();   
}

NTSTATUS KManagementService::GetValuesAndValidateTypes(
    __in KIDomNode::SPtr Node,
    __in QueryToken* QueryTokens,
    __in ULONG QueryTokenCount,
    __in ULONG& FailedIndex,
    __in KAllocator& Allocator
    )
{
    NTSTATUS status;

    for (ULONG i = 0; i < QueryTokenCount; i++)
    {
        KIDomNode::SPtr childNode;
        
        status = FindDOMNode(*Node,
                             KStringView(QueryTokens[i].Path),
                             Allocator,
                             childNode);
        if (! NT_SUCCESS(status))
        {
            FailedIndex = i;
            return(status);
        }

        *(QueryTokens[i].Value) = childNode->GetValue();
        if (QueryTokens[i].Value->Type() != QueryTokens[i].Type)
        {
            status = STATUS_OBJECT_TYPE_MISMATCH;
            FailedIndex = i;
            return(status);
        }            
    }
    
    return(STATUS_SUCCESS);
}

NTSTATUS KManagementService::GetChildValueAndValidateType(
    __in KIDomNode::SPtr Node,
    __in KIDomNode::QName& ChildNodeName,
    __in KVariant::KVariantType Type,
    __out KVariant& Value,
    __in LONG Index
    )
{
    NTSTATUS status;
    
    status = Node->GetChildValue(ChildNodeName, Value, Index);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    if (Value.Type() != Type)
    {
        status = STATUS_OBJECT_TYPE_MISMATCH;
    }

    return(status);
}

// TODO: array based method to get list of values

    
NTSTATUS KManagementService::FindDOMNode(
    __in KIDomNode& Source,
    __in KStringView& NodePath,
    __in KAllocator& Allocator,
    __out KIDomNode::SPtr& Node
    )
{
    NTSTATUS status;
    BOOLEAN b;
    KStringView nodeElementView;
    KIDomNode::SPtr node = &Source;
    KStringView nodePath = NodePath;
    
    //
    // Node path must be a relative path from the Source node, ie,
    // Fred\Barney\Wilma
    //
    while (nodePath.Length() > 0)
    {
        KString::SPtr nodeElement;

        nodePath.MatchUntil(KStringView(L"\\"), nodeElementView);
        nodeElement = KString::Create(nodeElementView,
                                      Allocator);
        if (! nodeElement)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        nodePath.ConsumeChars(nodeElementView.Length() + 1);
        status = node->GetChild(KIDomNode::QName(*nodeElement), node);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    };

    Node = Ktl::Move(node);
    
    return(STATUS_SUCCESS);
}

NTSTATUS KManagementService::BuildXMLFromTemplate(
    __inout KString::SPtr XmlTemplate,
    __out_opt KIMutableDomNode::SPtr* Dom,
    __in ReplaceToken *Tokens,
    __in ULONG TokenCount,
    __in KAllocator& Allocator
)
{
    NTSTATUS status;
    ULONG count;
    BOOLEAN b;

    for (ULONG i = 0; i < TokenCount; i++)
    {
        KString::SPtr valueString;

        b = Tokens[i].ReplacementValue->ToString(Allocator, valueString);
        if (! b)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }

        b = XmlTemplate->Replace(KStringView(Tokens[i].TextPlaceholder),
                                 *valueString,
                                 count,
                                 FALSE);    // ReplaceAll
        if (! b)
        {
            return(STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    if (Dom)
    {
        status = KDom::FromString(*XmlTemplate,
                                  Allocator,
                                  *Dom);
        if (! NT_SUCCESS(status))
        {
            return(status);
        }
    }

    return(STATUS_SUCCESS);
}


NTSTATUS KManagementService::CreateManagementRequestContext(
    __in Verbs Verb,
    __in KIDomNode::SPtr RequestHeaders,                                       
    __in KIDomNode::SPtr RequestBody,
    __in KIMutableDomNode::SPtr& ResponseHeaders,
    __in KIMutableDomNode::SPtr& ResponseBody,
    __out ManagementRequestContext::SPtr& Mrc,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;
    
    Mrc = _new (AllocationTag, Allocator) KManagementService::ManagementRequestContext(*this,
                                                                                       RequestHeaders,
                                                                                       RequestBody,
                                                                                       ResponseHeaders,
                                                                                       ResponseBody);
    if (Mrc == nullptr)
    {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return(status);
    }

    status = Mrc->Status();
    if (!NT_SUCCESS(status))
    {
        Mrc = nullptr;
        return(status);
    }

    return(STATUS_SUCCESS);
}

NTSTATUS KManagementService::StartVerbAsync(
    __in  Verbs Verb,
    __in  KUriView& Target,
    __in  KIDomNode::SPtr RequestHeaders,
    __in  KIDomNode::SPtr RequestBody,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in  KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;
    BOOLEAN b;
    ManagementRequestContext::SPtr mrc;

    status = CreateManagementRequestContext(Verb,
                                            RequestHeaders,
                                            RequestBody,
                                            ResponseHeaders,
                                            ResponseBody,
                                            mrc,
                                            GetThisAllocator(),
                                            TAG_KMANAGEMENT_SERVICE);
    if (! NT_SUCCESS(status))
    {
        return(status);
    }

    status = mrc->InitializeForUse(Verb,
                                   Target);

    if (! NT_SUCCESS(status))
    {
        ResponseHeaders = nullptr;
        ResponseBody = nullptr;
        mrc = nullptr;
        return(status);
    }

    if (ChildAsync != nullptr)
    {
        *ChildAsync = mrc.RawPtr();
    }

    b = TryAcquireActivities();
    if (! b)
    {
        status = STATUS_UNSUCCESSFUL;

        ResponseHeaders = nullptr;
        ResponseBody = nullptr;
        mrc = nullptr;
        
        return(status);
    }
    
    mrc->Start(Parent, Callback, GlobalContextOverride);

    // CONSIDER: if mrc is not returned, is it going to be out of scope
    // and deleted ?

    return(STATUS_PENDING);
}

NTSTATUS KManagementService::StartGetAsync(
    __in  KUriView& Target,
    __in  KIDomNode::SPtr RequestHeaders,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in  KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;

    status = StartVerbAsync(GetVerb,
                            Target,
                            RequestHeaders,
                            nullptr,
                            ResponseHeaders,
                            ResponseBody,
                            Callback,
                            ChildAsync,
                            Parent,
                            GlobalContextOverride);

    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    return(status);
}

NTSTATUS KManagementService::StartCreateAsync(
    __in KUriView& Target,
    __in KIDomNode::SPtr RequestHeaders,
    __in KIDomNode::SPtr RequestBody,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;

    status = StartVerbAsync(CreateVerb,
                            Target,
                            RequestHeaders,
                            RequestBody,
                            ResponseHeaders,
                            ResponseBody,
                            Callback,
                            ChildAsync,
                            Parent,
                            GlobalContextOverride);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    return(status);
}

NTSTATUS KManagementService::StartUpdateAsync(
    __in KUriView& Target,
    __in KIDomNode::SPtr RequestHeaders,
    __in KIDomNode::SPtr RequestBody,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;

    status = StartVerbAsync(UpdateVerb,
                            Target,
                            RequestHeaders,
                            RequestBody,
                            ResponseHeaders,
                            ResponseBody,
                            Callback,
                            ChildAsync,
                            Parent,
                            GlobalContextOverride);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    return(status);
}

NTSTATUS KManagementService::StartDeleteAsync(
    __in KUriView& Target,
    __in KIDomNode::SPtr RequestHeaders,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    NTSTATUS status;

    status = StartVerbAsync(DeleteVerb,
                            Target,
                            RequestHeaders,
                            nullptr,
                            ResponseHeaders,
                            ResponseBody,
                            Callback,
                            ChildAsync,
                            Parent,
                            GlobalContextOverride);
    
    if (! NT_SUCCESS(status))
    {
        KTraceFailedAsyncRequest(status, this, 0, 0);
    }
    
    return(status);
}

NTSTATUS KManagementService::StartQueryAsync(
    __in  KUriView& Target,
    __in  KIDomNode::SPtr Query,
    __in  KIDomNode::SPtr RequestHeaders,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KArray<KIMutableDomNode::SPtr>& ResponseBodies,
    __out BOOLEAN& MoreData,
    __in KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{
    // TODO: fix callback - many of the fields are broken out of the
    // XML request/response. Do we want this ?
    return(STATUS_NOT_IMPLEMENTED);
}

NTSTATUS KManagementService::StartInvokeAsync(
    __in  KUriView& Target,
    __in  KUriView& Verb,
    __in  KIDomNode::SPtr RequestHeaders,
    __in  KIDomNode::SPtr RequestBody,
    __out KIMutableDomNode::SPtr& ResponseHeaders,
    __out KIMutableDomNode::SPtr& ResponseBody,
    __in KAsyncContextBase::CompletionCallback Callback,
    __out_opt KAsyncContextBase::SPtr *ChildAsync,
    __in_opt KAsyncContextBase* Parent,
    __in_opt KAsyncGlobalContext* const GlobalContextOverride
    )
{

    // TODO: fix callback - Where does the invoke verb live in the XML
    // and who is responsible for pulling it out ?
    return(STATUS_NOT_IMPLEMENTED); 
}
