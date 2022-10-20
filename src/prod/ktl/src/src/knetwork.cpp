/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    knetwork.cpp

    Description:
      Kernel Tempate Library (KTL): Networking & Transport Support

      Implementation of classes for networking and datagram transmission

    History:
      raymcc          28-Feb-2011         Initial draft
      raymcc          02-Apr-2011         Updates for in-memory transport
      raymcc          26-May-2011         Incorporate TCP network
      raymcc          23-Jun-2012         Anonymous client; KNetwork endpoint transition


--*/
#include <ktl.h>
#include <ktrace.h>

#include <ntstrsafe.h>


NTSTATUS
KNetUriInfo::Create(
    __out KNetUriInfo::SPtr& NetUriInfo,
    __in KAllocator& Allocator,
    __in ULONG AllocationTag
    )
{
    NTSTATUS status;

    NetUriInfo = _new(AllocationTag, Allocator) KNetUriInfo;

    if (!NetUriInfo) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = NetUriInfo->Status();

    if (!NT_SUCCESS(status)) {
        NetUriInfo = NULL;
        return status;
    }

    return STATUS_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////
//
//  KNetwork
//

LONG __KNetwork_PendingOperations = 0;
LONG __KNetwork_DroppedInboundMessages   = 0;
LONG __KNetwork_DroppedOutboundMessages   = 0;
LONG __KNetwork_PendingAccepts = 0;

//
//  KNetwork::Create
//
NTSTATUS
KNetwork::Create(
    __in  ULONG Flags,
    __in  KAllocator& Alloc,
    __out KNetwork::SPtr& Network,
    __in_opt KAsyncContextBase* const ParentAsyncContext,
    __in_opt CompletionCallback CallbackPtr
    )
{
    NTSTATUS Result = STATUS_INTERNAL_ERROR;

    UNREFERENCED_PARAMETER(Flags);                

    KNetwork* New = _new(KTL_TAG_NET, Alloc) KNetwork;
    if (!New)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Add it to VNetwork's table of all KNetworks
    //
    VNetwork* VNet = VNetwork::Acquire();

    if (VNet)
    {
        Result = VNet->Add(New);
    }
    else
    {
        Result = STATUS_SHUTDOWN_IN_PROGRESS;
    }

    VNetwork::Release();

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    Network = New;
    Network->_SvcLayer = KNetServiceLayer::Get();

    Network->Start(ParentAsyncContext, CallbackPtr );
    return Result;
}

// KNetwork override the default on OnStart which does a complete.
VOID
KNetwork::OnStart()
{

}

//
//  KNetwork Constructor
//
KNetwork::KNetwork(
    ) :
    _EpTable(GetThisAllocator()),
    _ReceiverTable(GetThisAllocator())
{
    // Nothing.
}

//
//  KNetwork::~KNetwork
//
KNetwork::~KNetwork()
{
    // Nothing.
}


// GetFaultInjector
//
// If fault injection is enabled, this will return a shared
// pointer to the injector interface
//
NTSTATUS
KNetwork::GetFaultInjector(
    __out KNetFaultInjector::SPtr& Injector
    )
{
    NTSTATUS Result;

    VNetwork* VNet = VNetwork::Acquire();

    if (VNet)
    {
        Result = VNet->GetFaultInjector(Injector);
    }
    else
    {
        Result = STATUS_SHUTDOWN_IN_PROGRESS;
    }

    VNetwork::Release();
    return Result;
}


//
//  KNetwork::EnableFaultInjection
//
VOID
KNetwork::EnableFaultInjection()
{
    VNetwork* VNet = VNetwork::Acquire();

    if (VNet)
    {
        VNet->EnableFaultInjection();
    }
    VNetwork::Release();
}

//
//  KNetwork::DisableFaultInjection
//
VOID
KNetwork::DisableFaultInjection()
{
    VNetwork* VNet = VNetwork::Acquire();

    if (VNet)
    {
        VNet->DisableFaultInjection();
    }
    VNetwork::Release();
}


//
// KNetwork::Shutdown
//
VOID
KNetwork::Shutdown()
{
    VNetwork* VNet = VNetwork::Acquire();
    if (VNet)
    {
        VNet->Remove(this);
        VNetwork::Release();
    }

    K_LOCK_BLOCK(_TableLock)
    {
        if (_SvcLayer)
        {
            for (ULONG i = 0; i < _EpTable.Count(); i++)
            {
                EpEntry::SPtr Ep = _EpTable[i];

                if (Ep->_Flags & eLocalEp)
                {
                    _SvcLayer->UnpublishEp(Ep->_UriInfo->_RelUri);
                }
            }
        }

        _ReceiverTable.Clear();
    }

    Complete(STATUS_SUCCESS );
}

KNetwork::EpEntry::EpEntry()
{
    _Flags = 0;
}

KNetwork::EpEntry::~EpEntry()
{
    // Nothing.
}

KNetwork::ReceiverInfo::~ReceiverInfo()
{
    _Network->ReleaseActivities(1);
}

//
//  KNetwork::RegisterTypedReceiver
//
NTSTATUS
KNetwork::RegisterTypedReceiver(
        __in const PfnKConstruct KConstruct,
        __in const PfnKDeserialize KDeserialize,
        __in const GUID& LocalEp,
        __in const GUID& MessageId,
        __in ULONG Flags,
        __in PVOID UserContext,
        __in ReceiverType Receiver
        )
{
    NTSTATUS Result;
    ReceiverInfo::SPtr Tmp;

    //  Verify that we don't already have this message type handled.
    //
    Result = FindReceiverInfo(MessageId, Tmp);

    if (Result != STATUS_NOT_FOUND)
    {
        return STATUS_OBJECTID_EXISTS;
    }

    // Allocate & fill up the new info.
    //
    ReceiverInfo::SPtr NewInf = _new(KTL_TAG_NET, GetThisAllocator()) ReceiverInfo(this);
    if (!NewInf)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewInf->_LocalEp = LocalEp;
    NewInf->_MessageId = MessageId;
    NewInf->_KConstruct = KConstruct;
    NewInf->_KDeserialize = KDeserialize;
    NewInf->_Flags = Flags,
    NewInf->_UserContext = UserContext;
    NewInf->_OldReceiver = Receiver;

    //  Add it to table.
    //
    _TableLock.Acquire();
    Result = _ReceiverTable.Append(NewInf);
    _TableLock.Release();

    return Result;
}

// KNetwork::RegisterTypedReceiver (Overload)
//
// For new-style bindings.
//
// This type of binding has no real GUID.  When messages
// arrives for the receiver, we instead look for a matching
// receiver on one of the networks.  If there is no match,
// we discard the message.
//
//
//
NTSTATUS
KNetwork::RegisterTypedReceiver(
    __in  KUriView& RelativeOrFullUri,
    __in  PfnKConstruct KConstruct,
    __in  PfnKDeserialize KDeserializer,
    __in  const GUID& MessageId,
    __in  PVOID UserContext,
    __in  KNetReceiverType Receiver,
    __out KNetworkEndpoint::SPtr& Ep
    )
{
    NTSTATUS Result;
    ReceiverInfo::SPtr Tmp;

    //  Verify that we don't already have this message type handled.
    //
    Result = FindReceiverInfo(MessageId, Tmp);
    if (Result != STATUS_NOT_FOUND)
    {
        KDbgPrintf("KNetwork error: Already have a handler for the specified type\n");
        return STATUS_OBJECTID_EXISTS;
    }

    // Check out the URI
    //
    if (!RelativeOrFullUri.IsValid())
    {
        KDbgPrintf("KNetwork error: Uri is invalid\n");
        return STATUS_INVALID_PARAMETER;
    }

    // Allocate & fill up the new info.
    //
    ReceiverInfo::SPtr NewInf = _new(KTL_TAG_NET, GetThisAllocator()) ReceiverInfo(this);
    if (!NewInf)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Now we have to create an endpoint for KNetServiceLayer to map
    // back to this KNetwork.

    NewInf->_MessageId = MessageId;
    NewInf->_KConstruct = KConstruct;
    NewInf->_KDeserialize = KDeserializer;
    NewInf->_UserContext = UserContext;
    NewInf->_KNetReceiver = Receiver;

    // Manufacture the local network endpoint.
    //
    Result = MakeAndRegisterLocalEndpoint(RelativeOrFullUri, NewInf->_LocalNetEp, NewInf->_LocalEp);
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("MakeAndRegisterLocalEndpoint() failed with 0x%X\n", Result);
        return Result;
    }

    //  Add it to table.
    //
    _TableLock.Acquire();
    Result = _ReceiverTable.Append(NewInf);
    _TableLock.Release();

    // Inform the caller of the new EP.
    //
    Ep = NewInf->_LocalNetEp;

//    KDbgPrintf("RegisterTypedReceiver() has succeeded\n");

    return STATUS_SUCCESS;
}


NTSTATUS
KNetwork::MakeLocalEndpoint(
    __in  KUriView& PathOnlyUri,
    __out KNetworkEndpoint::SPtr& FullEp
    )
{
    NTSTATUS Result;

    // Build up the full new URI.
    //
    KStringView RootView = *_SvcLayer->GetRootEp()->GetUri();
    KDynString NewFullUri(GetThisAllocator());
    if (!NewFullUri.CopyFrom(RootView))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NewFullUri.Concat(KStringView(L"/")))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NewFullUri.Concat(KStringView(PathOnlyUri), TRUE))
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KUriView Validator(NewFullUri);
    if (!Validator.IsValid())
    {
        return STATUS_INTERNAL_ERROR;
    }
    // Convert it to an endpoint.
    //
    KNetworkEndpoint::SPtr Tmp;
    Result = KRvdTcpNetworkEndpoint::Create(Validator, GetThisAllocator(), Tmp);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    FullEp = Tmp;
    return STATUS_SUCCESS;
}


//  MakeAndRegisterLocalEndpoint
//
//  Creates a new local endpoint and returns the KNetworkEndpoint representation.
//  This also adds the endpoint to the Ep table and returns the generated GUID.
//
NTSTATUS
KNetwork::MakeAndRegisterLocalEndpoint(
    __in  KUriView& RelativeOrFullUri,
    __out KNetworkEndpoint::SPtr& FullEp,
    __out GUID& NewGuid
    )
{
    NTSTATUS Result;
    KUri::SPtr RootUri = _SvcLayer->GetRootEp()->GetUri();
    KUri::SPtr NewEpUri;

    //
    // If relative, build up the full new URI.
    //
    if (RelativeOrFullUri.IsRelative())
    {
#if defined(PLATFORM_UNIX)
        KDbgPrintf("MakeAndRegisterLocalEndpoint() URI is relative [%s]  Creating a new composed URI\n", Utf16To8(LPWSTR(RelativeOrFullUri.Get(KUri::eRaw))).c_str());
#else
        KDbgPrintf("MakeAndRegisterLocalEndpoint() URI is relative [%S]  Creating a new composed URI\n", LPWSTR(RelativeOrFullUri.Get(KUri::eRaw)));
#endif

        Result = KUri::Create(*RootUri, RelativeOrFullUri, GetThisAllocator(), NewEpUri);
        if (!NT_SUCCESS(Result))
        {
            KDbgPrintf("MakeAndRegisterLocalEndpoint() created composed URI has failed\n");
            return Result;
        }
    }
    // Else verify that the specified full URI is a prefix match for the current root URI.
    //
    else
    {
//        KDbgPrintf("MakeAndRegisterLocalEndpoint() URI is full.  Testing for viable prefix.\n");

        Result = KUri::Create(RelativeOrFullUri, GetThisAllocator(), NewEpUri);
        if (!NT_SUCCESS(Result))
        {
            KDbgPrintf("MakeAndRegisterLocalEndpoint() failed to create new full URI\n");
            return Result;
        }
#if defined(PLATFORM_UNIX)
        KDbgPrintf("NewEpUri is %s\n", Utf16To8(LPWSTR(NewEpUri->Get(KUri::eRaw))).c_str());
        KDbgPrintf("RootUri is %s\n", Utf16To8(LPWSTR(RootUri->Get(KUri::eRaw))).c_str());
#else
        KDbgPrintf("NewEpUri is %S\n", LPWSTR(NewEpUri->Get(KUri::eRaw)));
        KDbgPrintf("RootUri is %S\n", LPWSTR(RootUri->Get(KUri::eRaw)));
#endif

        if (!RootUri->IsPrefixFor(*NewEpUri))
        {
            KDbgPrintf("MakeAndRegisterLocalEndpoint() IsPrefixFor() has failed\n");
            return STATUS_INVALID_PARAMETER;
        }
    }

    // Convert it to an endpoint.
    //
    KNetworkEndpoint::SPtr Tmp;
    Result = KRvdTcpNetworkEndpoint::Create(*NewEpUri, GetThisAllocator(), Tmp);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // See if we already have an endpoint for this URI before we create a new one.
    //

    // First, make a string out of it for compatibility with the older code.
    //
    KWString StrFormOfUri(GetThisAllocator());
    StrFormOfUri = (KStringView) *NewEpUri;
    if (!NT_SUCCESS(StrFormOfUri.Status()))
    {
        return StrFormOfUri.Status();
    }

#if defined(PLATFORM_UNIX)
    KDbgPrintf("MakeAndRegisterLocalEp is registering %s\n", Utf16To8(LPCWSTR(StrFormOfUri)).c_str());
#else
    KDbgPrintf("MakeAndRegisterLocalEp is registering %S\n", LPCWSTR(StrFormOfUri));
#endif

    // See if it is there.
    //
    ULONG EpType = 0;
    Result = GetEp(StrFormOfUri, EpType, NewGuid);
    if (Result == STATUS_NOT_FOUND)
    {
        // If here, this endpoint has not previously existed.
        // Now create a GUID for it and register it the 'old' way as well.
        //
        _SvcLayer->NextSequentialGuid(NewGuid);
        Result = RegisterEp(StrFormOfUri, eRemoteEp|eLocalEp, NewGuid);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }
    else if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    FullEp = Tmp;
//    KDbgPrintf("MakeAndRegister is successful\n");
    return STATUS_SUCCESS;
}


//
// KNetwork::GetEp
//
NTSTATUS
KNetwork::GetEp(
    __in   KWString& Uri,
    __out  ULONG& EpType,
    __out  GUID& Endpoint
    )
{
    EpEntry::SPtr Entry = LookupByUri(Uri, TRUE);

    if (!Entry)
    {
        return STATUS_NOT_FOUND;
    }

    EpType = Entry->_Flags;
    Endpoint = Entry->_Ep;
    return STATUS_SUCCESS;
}

//
// KNetwork::GetEp
//
NTSTATUS
KNetwork::GetEp(
    __in  GUID& Endpoint,
    __out ULONG& EpType,
    __out KWString& Uri
    )
{
    EpEntry::SPtr Entry = LookupByGUID(Endpoint, TRUE);

    if (!Entry)
    {
        return STATUS_NOT_FOUND;
    }

    EpType = Entry->_Flags;
    Uri = Entry->_UriInfo->_FullUri;
    return Uri.Status();
}



//
//  KNetwork::FindReceiver
//
NTSTATUS
KNetwork::FindReceiverInfo(
    __in const GUID& MessageId,
    __out ReceiverInfo::SPtr& ReceiverInf
    )
{
    _TableLock.Acquire();
    KFinally([&](){ _TableLock.Release(); });


    for (ULONG i = 0; i < _ReceiverTable.Count(); i++)
    {
        if (MessageId == _ReceiverTable[i]->_MessageId)
        {
            ReceiverInf = _ReceiverTable[i];
            return STATUS_SUCCESS;
        }
    }
    return STATUS_NOT_FOUND;
}


//  (deprecated)
//  KNetwork::GetBinding
//
//  This is the older GUID-based version.  All local and remote
//  endpoints have to be registered.
//
NTSTATUS
KNetwork::GetBinding(
    __in  GUID& RemoteEpGuid,
    __in  GUID& OriginatingLocalEpGuid,
    __in  ULONG  Flags,
    __out KNetBinding::SPtr& Binding
    )
{
    NTSTATUS Result;

    UNREFERENCED_PARAMETER(Flags);

    if (PKGuid(&RemoteEpGuid)->IsNull())
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (PKGuid(&OriginatingLocalEpGuid)->IsNull())
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    // Verify that the local endpoint is registered.
    //
    EpEntry::SPtr Local = LookupByGUID(OriginatingLocalEpGuid, TRUE);
    if (!Local)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

        // Check the cache and see there is already a binding.
    //
    K_LOCK_BLOCK(_TableLock)
    {
        // Attempt to look up the remote endpoint in the local table.
        //
        KWString RemoteUri(GetThisAllocator());
        BOOLEAN  NetworkDestination = FALSE;
        USHORT RemotePort = 0;

        EpEntry::SPtr RemoteEp = LookupByGUID(RemoteEpGuid, FALSE);
        if (RemoteEp)
        {
            RemoteUri = RemoteEp->_UriInfo->_FullUri;
            if (RemoteEp->_UriInfo->_Flags != KNetUriInfo::eVNet)
            {
                NetworkDestination = TRUE;
                RemotePort = RemoteEp->_UriInfo->_Port;
            }
        }
        else if (_SvcLayer)
        {
            Result = _SvcLayer->GuidToUri(RemoteEpGuid, RemoteUri, RemotePort);
            if (!NT_SUCCESS(Result))
            {
                return STATUS_INVALID_PARAMETER_1;
            }
            NetworkDestination = TRUE;
        }
        else
        {
            return STATUS_INVALID_PARAMETER_1;
        }

        // Create a remote ep from the string URI
        //
        KNetworkEndpoint::SPtr RemoteNetEp;
        KUriView proposed = KUriView(LPCWSTR(RemoteUri));
        Result = KRvdTcpNetworkEndpoint::Create(proposed, GetThisAllocator(), RemoteNetEp);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }

        // Create a new binding.  If there is a subsequent error, we'll let it
        // self destruct, but the normal case is that there is no error.
        //
        KNetBinding* pBinding = _new(KTL_TAG_NET, GetThisAllocator()) KNetBinding();
        if (!pBinding)
        {
            KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        NTSTATUS Result1 = KNetUriInfo::Create(pBinding->_DestInf, GetThisAllocator());
        if (!NT_SUCCESS(Result1))
        {
            KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
            return Result1;
        }

        Result1 = pBinding->_DestInf->Parse(RemoteUri);
        if (!NT_SUCCESS(Result1))
        {
            return STATUS_INTERNAL_ERROR;
        }

        pBinding->_LocalEp  =  OriginatingLocalEpGuid;
        pBinding->_RemoteEp =  RemoteEpGuid;

        pBinding->_LocalUri  = Local->_UriInfo->_FullUri; // Include the port
        pBinding->_RemoteUri = RemoteUri;
        pBinding->_Network =   this;
        pBinding->_NetworkDestination = NetworkDestination;

        pBinding->_LocalNetEp = up_cast<KNetworkEndpoint, KRvdTcpNetworkEndpoint>(Local->_UriInfo->_NetEp);
        pBinding->_RemoteNetEp = RemoteNetEp;

        Binding = pBinding;
    }
    return STATUS_SUCCESS;
}

// KNetwork::GetBinding
//
// This should replace the legacy version of this function.  This version is just a helper: the binding
// is simply composed based on the endpoints and the endpoints do not have to be preregistered.
//
//
NTSTATUS
KNetwork::GetBinding(
    __in  KNetworkEndpoint::SPtr& LocalSenderEp,
    __in  KNetworkEndpoint::SPtr& RemoteReceiverEp,
    __out KNetBinding::SPtr& Binding
    )
{
    KNetBinding* pBinding = _new(KTL_TAG_NET, GetThisAllocator()) KNetBinding();
    if (!pBinding)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NTSTATUS Result = KNetUriInfo::Create(pBinding->_DestInf, GetThisAllocator());
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // Null GUIDs mark the new-style binding with
    // just KNetworkEndpoints.
    //
    KGuid NullGuid;

    pBinding->_LocalEp  =  NullGuid; // Legacy
    pBinding->_RemoteEp =  NullGuid;

    // Extract the relevant URI strings.
    //
    pBinding->_LocalUri  = KStringView(*LocalSenderEp->GetUri());
    pBinding->_RemoteUri = KStringView(*RemoteReceiverEp->GetUri());
    pBinding->_Network =   this;

    Result = pBinding->_DestInf->Parse(pBinding->_RemoteUri);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    pBinding->_LocalNetEp  = LocalSenderEp;
    pBinding->_RemoteNetEp = RemoteReceiverEp;

    if (pBinding->_DestInf->_Flags == KNetUriInfo::eVNet)
    {
        pBinding->_NetworkDestination = FALSE;
    }
    else
    {
        pBinding->_NetworkDestination = TRUE;
        if (pBinding->_DestInf->_Port == 0)
        {
            // Remote destination missing the port number!
            //
            return STATUS_INVALID_PARAMETER_2;
        }
    }

    Binding = pBinding;

    return STATUS_SUCCESS;
}



//
//  KNetBinding::KNetBinding
//
KNetBinding::KNetBinding()
    :   _LocalUri(GetThisAllocator()),
        _RemoteUri(GetThisAllocator())
{
    _SocketAcquireActive = FALSE;
    _NetworkDestination = FALSE;
}


//
//  KNetBinding::~KNetBinding
//
KNetBinding::~KNetBinding()
{
}

//
//  KNetBinding::CreateDatagramWriteOp
//
NTSTATUS
KNetBinding::CreateDatagramWriteOp(
    __out KDatagramWriteOp::SPtr& NewObject,
    __in  ULONG AllocationTag
    )
{
    KAssert(this != nullptr);

    UNREFERENCED_PARAMETER(AllocationTag);                

    KDatagramWriteOp* NewOp = _new(KTL_TAG_NET, GetThisAllocator()) KDatagramWriteOp;
    if (!NewOp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    NewObject = NewOp;
    NewObject->_Binding = this;
    NewObject->_ActId = 0;

    return STATUS_SUCCESS;
}

//
//  KDatagramWriteOp::KDatagramWriteOp
//
KDatagramWriteOp::KDatagramWriteOp(
    )
{
    InterlockedIncrement(&__KNetwork_PendingOperations);
}

//
//  KDatagramWriteOp::~KDatagramWriteOp
//
KDatagramWriteOp::~KDatagramWriteOp()
{
    InterlockedDecrement(&__KNetwork_PendingOperations);
}

//
//  KDatagramWriteOp::AcquireSocket
//
NTSTATUS
KNetBinding::AcquireSocket(
    __out KSharedPtr<KAsyncEvent::WaitContext>&  WaitContext,
    __in_opt KAsyncContextBase* const Parent,
    __in KAsyncContextBase::CompletionCallback Callback
    )
{
    NTSTATUS Result;
    BOOLEAN StartAcquire = FALSE;
    Result = _SocketAcquired.CreateWaitContext( KTL_TAG_NET, GetThisAllocator(), WaitContext );
    if (!NT_SUCCESS(Result))
    {
        return(Result);
    }

    KNetAcquireSocketOp::SPtr AcquireOp;
    Result = GetNetwork()->_SvcLayer->CreateAcquireSocketOp(AcquireOp);
    if (!NT_SUCCESS(Result))
    {
        WaitContext = nullptr;
        return(Result);
    }

    K_LOCK_BLOCK(_Lock )
    {
        if (!_SocketAcquireActive && !_WriteSocket)
        {
            StartAcquire = TRUE;
            _SocketAcquireActive = TRUE;
            _SocketAcquired.ResetEvent();
        }
    }

    if (StartAcquire)
    {
        KGuid RemoteGUID;
        GetRemoteEp(RemoteGUID);

        // Add a reference ensure the binding stays around until the callback has
        // run.

        AddRef();

        Result = AcquireOp->StartAcquire(_DestInf, (USHORT) _RemoteNetEp->GetUri()->GetPort(), KAsyncContextBase::CompletionCallback(this, &KNetBinding::SocketAcquiredSignal));

        if (!NT_SUCCESS(Result ))
        {
            Release();

            WaitContext = nullptr;
            _SocketAcquired.SetEvent();
            _SocketAcquireActive = FALSE;
            return(Result);
        }
    }

    WaitContext->StartWaitUntilSet( Parent, Callback);
    return(STATUS_PENDING);
}

//
//  KNetBinding::SocketAcquiredSignal
//
VOID
KNetBinding::SocketAcquiredSignal(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    NTSTATUS Result;
    KNetAcquireSocketOp& AcquireOp = (KNetAcquireSocketOp &) Op;

    UNREFERENCED_PARAMETER(Parent);    

    Result = AcquireOp.Status();

    K_LOCK_BLOCK(_Lock )
    {
        _SocketAcquireActive = FALSE;
        if (NT_SUCCESS(Result))
        {
            _WriteSocket = AcquireOp.GetSocket();
            _NetworkDestination = TRUE;
        }
    }

    _SocketAcquired.SetEvent();

    // Release the reference that was added when the AcquireOp was started.
    Release();

}


///////////////////////////////////////////////////////////////////////////////
//



//
// KNetwork::RegisterEp
//
NTSTATUS
KNetwork::RegisterEp(
     __in  KWString& RawUri,
     __in  ULONG Flags,
     __in  GUID& Ep
     )
{
    NTSTATUS Result;

    KNetUriInfo::SPtr Tmp;

    Result = KNetUriInfo::Create(Tmp, GetThisAllocator());

    if (!NT_SUCCESS(Result)) {
        KTraceOutOfMemory(0, Result, this, 0, 0);
        return Result;
    }

    Result = Tmp->Parse(RawUri);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // LEGACY Code
    //
    // If port is zero (was no present in the original URI), replace it with default port number in use
    // by the current KNetServiceLayer.
    //
    if (Tmp->_Port == 0)
    {
        KDynString NewUri(GetThisAllocator());
        NewUri.Concat(KStringView(L"rvd://"));
        NewUri.Concat(KStringView(LPCWSTR(Tmp->_Authority)));
        KLocalString<32> PortStr;
        PortStr.FromULONG(_SvcLayer->GetRootEp()->GetUri()->GetPort());
        PortStr.PrependChar(L':');
        PortStr.AppendChar(L'/');
        NewUri.Concat(PortStr);
        NewUri.Concat(KStringView(LPCWSTR(Tmp->_RelUri)));
        Tmp = 0;
        Result = KNetUriInfo::Create(Tmp, GetThisAllocator());
        if (!NT_SUCCESS(Result)) {
            KTraceOutOfMemory(0, Result, this, 0, 0);
            return Result;
        }

        RawUri = NewUri;
        Result = Tmp->Parse(RawUri);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }


    //  Parameter checks
    //
    if (Tmp->_FullUri.Length() == 0)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (Flags == 0)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    if (KGuid(Ep).IsNull())
    {
        return STATUS_INVALID_PARAMETER_3;
    }

    EpEntry::SPtr Existing;

    //  Allow the registration to succeed as a logical
    //  overwrite.
    //
    K_LOCK_BLOCK(_TableLock)
    {
        Existing = LookupByUri(Tmp->_FullUri, FALSE);  // FALSE==Already holding a lock in this case
        if (Existing)
        {
            if (Existing->_Ep == Ep)
            {
                return STATUS_SUCCESS;
            }
            else
            {
                return STATUS_CONFLICTING_ADDRESSES;
            }
        }

        // If here, there is no URI registered.

        // Double check to ensure that the GUID isn't being used
        // for something else.
        //
        Existing = LookupByGUID(Ep, FALSE);
        if (Existing)
        {
            return STATUS_OBJECTID_EXISTS;
        }
    }

    // If here, we're in the clear.
    //
    EpEntry::SPtr NewEntry = _new(KTL_TAG_NET, GetThisAllocator()) EpEntry;
    if (NewEntry == nullptr)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewEntry->_Ep = Ep;
    NewEntry->_Flags = Flags;
    NewEntry->_UriInfo = Tmp;

    // If the URI is remote but the network is not started, return an error.
    //
    if (!_SvcLayer && NewEntry->_UriInfo->_Flags != KNetUriInfo::eVNet)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    NewEntry->_UriInfo->_GUID = Ep;

    K_LOCK_BLOCK(_TableLock)
    {
        Result = _EpTable.Append(NewEntry);
    }

    if (!NT_SUCCESS(Result))
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!Tmp->_NetEp->GetUri()->Get(KUriView::eHost).Compare(KStringView(L"(local)")) && (Flags & eLocalEp))
    {
        VNetwork *VNet = VNetwork::Acquire();
        KFinally([&VNet](){ VNet->Release(); });

        KArray<KSharedPtr<KNetwork>>  Networks(GetThisAllocator());
        KAssert(_EpTable.Count() == 1);

        K_LOCK_BLOCK(VNet->_TableLock )
        {
            Networks = VNet->_Networks;
        }

        if (!NT_SUCCESS(Networks.Status()))
        {
            return(Networks.Status());
        }

        KWString RemoteUri(GetThisAllocator());
        GUID RemoteEp;
        BOOLEAN Found;

        for(ULONG i = 0; i < Networks.Count(); i++)
        {
            Found = FALSE;
            K_LOCK_BLOCK(Networks[i]->_TableLock )
            {
                Found = Networks[i]->_EpTable.Count() >= 1 && (Networks[i]->_EpTable[0]->_Flags & eLocalEp);
                if (!Found)
                {
                    KDbgCheckpoint( 0, "Empty Network found.");
                    break;
                }

                RemoteUri = Networks[i]->_EpTable[0]->_UriInfo->_FullUri;
                RemoteEp = Networks[i]->_EpTable[0]->_Ep;
            }

            if (!Found)
            {
                continue;
            }

            Result = Networks[i]->RegisterEp( Tmp->_FullUri, eRemoteEp, Ep);
            if (Result == STATUS_CONFLICTING_ADDRESSES) {
                Result = STATUS_SUCCESS;
            }
            KAssert(NT_SUCCESS(Result ));
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }

            Result = RegisterEp(RemoteUri, eRemoteEp, RemoteEp );
            KAssert(NT_SUCCESS(Result ));
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }
        }
    }

    // If the URI is remote, add it to the KNetServiceLayer table.
    //
    if (_SvcLayer && NewEntry->_UriInfo->_Flags != KNetUriInfo::eVNet)
    {
        if (Flags & eLocalEp)
        {
            Result = _SvcLayer->PublishEp(Tmp, Ep, this);
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }
        }
        Result = _SvcLayer->AddAddressEntry(Tmp);
    }

    return Result;
}



NTSTATUS
KNetwork::UnregisterReceiver(
    __in const GUID& MessageId,
    __in KNetworkEndpoint::SPtr Ep
    )
{
    _TableLock.Acquire();
    KFinally([&](){_TableLock.Release();});

    for (ULONG i = 0; i < _ReceiverTable.Count(); i++)
    {
        ReceiverInfo::SPtr Test = _ReceiverTable[i];
        if (Test->_MessageId == MessageId && Test->_LocalNetEp->IsEqual(Ep))
        {
            _ReceiverTable.Remove(i);
            return STATUS_SUCCESS;
        }
    }

    return STATUS_NOT_FOUND;
}

//
//  KNetwork::UnregisterReceiver
//
NTSTATUS
KNetwork::UnregisterReceiver(
    __in const GUID& LocalEp,
    __in const GUID* MessageId,
    __in ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);            
    
    // Remove the receiver if it is there.
    //
    _TableLock.Acquire();
    KFinally([&](){ _TableLock.Release(); });

    KGuid MsgId;

    if (MessageId)
    {
        MsgId = *MessageId;
    }

    KGuid LocEp(LocalEp);

    if (!MsgId.IsNull())
    {
        // Cases where we are just removing a receiver for a specific
        // message type.
        //
        for (ULONG i = 0; i < _ReceiverTable.Count(); i++)
        {
            if ((MsgId == _ReceiverTable[i]->_MessageId) &&
                (LocEp == _ReceiverTable[i]->_LocalEp))
            {
                _ReceiverTable.Remove(i);
                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        // Cases where we are removing all receivers for an endpoint.
        //
        ULONG Removed = 0;

        for (ULONG i = 0; i < _ReceiverTable.Count(); i++)
        {
            if (LocEp == _ReceiverTable[i]->_LocalEp)
            {
                _ReceiverTable.Remove(i);
                Removed++;
            }
        }

        if (Removed)
        {
            return STATUS_SUCCESS;
        }
    }

     return STATUS_NOT_FOUND;
}

//
// KNetNameService::LookupByUri
//

KNetwork::EpEntry::SPtr
KNetwork::LookupByUri(
    __in KWString& Uri,
    __in BOOLEAN   AcquireLock
    )
{
    if (AcquireLock)
    {
        _TableLock.Acquire();
    }
    KFinally([&](){ if (AcquireLock) _TableLock.Release(); });

    EpEntry::SPtr Tmp;

    for (ULONG i = 0; i < _EpTable.Count(); i++)
    {
        Tmp = _EpTable[i];
        if (Tmp->_UriInfo->_FullUri.Compare(Uri) == 0)
        {
            return Tmp;
        }
    }

    return nullptr;
}

//
// TBD: Replace with a hash table
//
KNetwork::EpEntry::SPtr
KNetwork::LookupByGUID(
    __in GUID& Guid,
    __in BOOLEAN AcquireLock
    )
{
    if (AcquireLock)
    {
        _TableLock.Acquire();
    }
    KFinally([&]() { if (AcquireLock) _TableLock.Release(); });

    EpEntry::SPtr Tmp;

    for (ULONG i = 0; i < _EpTable.Count(); i++)
    {
        Tmp = _EpTable[i];
        if (Tmp->_Ep == Guid)
        {
            return Tmp;
        }
    }

    Tmp = nullptr;
    return Tmp;
}



//
//  KDatagramWriteOp::OnStart
//
//  Carries out the transfer of the serialized data to a
//  virtual socket.
//
VOID
KDatagramWriteOp::OnStart()
{
    NTSTATUS Result;

    if (_Binding->NetworkDestination())
    {
        // The destination is only reachable via the
        // network.
        NetworkSend();
        return;
    }

    // If here, the destination URI is local and we use the
    // in memory transport.
    //
    // Use _KTransferAgent to move data to a separate In-channel via
    // memory copies.
    //
    VNetwork::VNetMessage::SPtr Msg = _new(KTL_TAG_NET, GetThisAllocator()) VNetwork::VNetMessage;

    if (!Msg)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    Result = Msg->_Xfer.Read(*_Serializer);

    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    _Binding->GetRemoteEpUri(Msg->_DestinationUri);
    _Binding->GetLocalEpUri(Msg->_OriginatingUri);
    Msg->_ActId =           _ActId;
    Msg->_Priority =        _Priority;

    // Deliver the request to the foreign thread
    // owned by VNetwork to simulate a datagram
    // through the ether.
    //
    VNetwork* VNet = VNetwork::Acquire();
    if (VNet)
    {
        Result = VNet->Post(Msg);
        VNetwork::Release();
    }
    else
    {
        Result = STATUS_SHUTDOWN_IN_PROGRESS;
    }

    Complete(Result);
}

//
// KDatagramWriteOp::NetworkSend
//
// This code path assumes the target URI is known to be
// a remote URI to which a socket is bound.
//
VOID
KDatagramWriteOp::NetworkSend()
{
    NTSTATUS Result;

    // If the binding already has a socket, then we just jump
    // head.
    //
    _WriteSocket = _Binding->GetWriteSocket();
    if (_WriteSocket)
    {
        SerializeToSocket();
        return;
    }

    // If here, we have no socket.  This means that we
    // need to replace the underlying socket in the binding.
    //
    // If the binding does not have a socket, we have to
    // put things on hold and get one.  Once we have it,
    // we'll attach it to the binding for future use.
    //
    // Start a timer to limit how long we wait for the connection to complete.

    _WaitContext = nullptr;
    Result = KTimer::Create(_Timer, GetThisAllocator(), KTL_TAG_NET);
    if (!NT_SUCCESS(Result ))
    {
        KTraceOutOfMemory(_ActId, Result, this, 0, 0 );
        Complete(Result);
        return;
    }

    _Timer->StartTimer( _ConnectTimeout, this, KAsyncContextBase::CompletionCallback(this, &KDatagramWriteOp::SocketAcquiredTimeout));

    Result = _Binding->AcquireSocket(_WaitContext, this, KAsyncContextBase::CompletionCallback(this, &KDatagramWriteOp::SocketAcquiredSignal));
    if (!NT_SUCCESS(Result))
    {
        _Timer->Cancel();
        Complete(Result);
        return;
    }
}

//
// KDatgramWriteOp::SocketAcquiredTimeout
//
VOID KDatagramWriteOp::SocketAcquiredTimeout(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    
    NTSTATUS Result = Op.Status();

    if (Result == STATUS_CANCELLED)
    {
        return;
    }

    // Cancel the wait.
    if (_WaitContext)
    {
        _WaitContext->Cancel();
    }

    Complete(STATUS_IO_TIMEOUT);
}
//
// KDatgramWriteOp::SocketAcquired
//
VOID KDatagramWriteOp::SocketAcquiredSignal(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    NTSTATUS Result = Op.Status();

    UNREFERENCED_PARAMETER(Parent);

    // Cancel the timer no matter what happened with the socket
    _Timer->Cancel();

    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    _WriteSocket = _Binding->GetWriteSocket();

    // The binding may have lost the socket since we got the
    // signal, so double-check.

    if (!_WriteSocket)
    {
        Complete(STATUS_UNEXPECTED_NETWORK_ERROR);
        return;
    }

    // If here, we have a socket to write to.
    //
    SerializeToSocket();
}

//
//  KDatagramWriteOp::SerializeToSocket
//
//  Once we have a socket, we start here.
//
VOID
KDatagramWriteOp::SerializeToSocket()
{
    NTSTATUS Result;

    ISocketWriteOp::SPtr WriteOp;
    Result = _WriteSocket->CreateWriteOp(WriteOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Now read the serializer into the socket buffers.
    //
    // First, set up an array of KMemRef objects to match
    // all the content in the serializer plus headers/trailers.
    //
    KArray<KMemRef> MemRefs(GetThisAllocator());

    // Reset the header state. This write op may have been reused.
    //
    _NetHeader.Zero();

    // First, map the network header.
    //
    KMemRef HeaderMap;
    HeaderMap._Address = &_NetHeader;
    HeaderMap._Param   = sizeof(KNetworkHeader);
    MemRefs.Append(HeaderMap);

    // We can assign after mapping, because the mapping step
    // only takes a reference, and doesn't make copies.
    //
    _NetHeader._ActivityId = _ActId.Get();
    _NetHeader._Control |= KNet_ContentType_KtlSerialMessage;

    // Map the destination URI.
    //
    // Note we are not transmitting the null terminator
    //
    KMemRef DestUriMap;
    DestUriMap._Address = _Binding->GetRemoteUriAddress();
    DestUriMap._Param =   _Binding->GetRemoteUriLength();
    _NetHeader._DestUriLength = USHORT(DestUriMap._Param);
    MemRefs.Append(DestUriMap);

    // Map the source URI
    //
    KMemRef SrcUriMap;
    SrcUriMap._Address = _Binding->GetLocalUriAddress();
    SrcUriMap._Param = _Binding->   GetLocalUriLength();
    _NetHeader._SrcUriLength = USHORT(SrcUriMap._Param);
    MemRefs.Append(SrcUriMap);

    // Now loop through all the various blocks.
    // and map them to KMemRefs and get them
    // into the array.
    //
    for (_Serializer->Reset(eBlockTypeMeta);;)
    {
        KMemRef Src;
        Result = _Serializer->NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        MemRefs.Append(Src);
        _NetHeader._MetaLength += Src._Param;
    }

    for (_Serializer->Reset(eBlockTypeData);;)
    {
        KMemRef Src;
        Result = _Serializer->NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        MemRefs.Append(Src);
        _NetHeader._DataLength += Src._Param;
    }

    for (_Serializer->Reset(eBlockTypeDeferred);;)
    {
        KMemRef Src;
        Result = _Serializer->NextBlock(Src);
        if (Result == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }
        MemRefs.Append(Src);
        _NetHeader._DeferredLength += Src._Param;
    }

    // Now execute a 'gather' write and send it out into the ether...
    //
    KAsyncContextBase::CompletionCallback CBack(this, &KDatagramWriteOp::NetWriteResult);
    Result = WriteOp->StartWrite(MemRefs.Count(), MemRefs.Data(), 1, CBack, this);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
    }
}

//
//  KDatagramWriteOp::NetWriteResult
//
VOID
KDatagramWriteOp::NetWriteResult(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    NTSTATUS Result;

    UNREFERENCED_PARAMETER(Parent);

    ISocketWriteOp& WriteOp = (ISocketWriteOp &) Op;
    Result = Op.Status();

    ULONG BytesTransferred = WriteOp.BytesTransferred();
    if (!NT_SUCCESS(Result) || BytesTransferred != WriteOp.BytesToTransfer())
    {
        // Close the socket and log an error
        //
        KDbgPrintf("INTERNAL ERROR: Failed to write datagram. Status=0x%X [Bytes transferred = %u. CLOSING socket]\n", Result, BytesTransferred);

        if (_WriteSocket && _WriteSocket->Ok())
        {
            _WriteSocket->Close();
            _WriteSocket = 0;
        }

        InterlockedIncrement(&__KNetwork_DroppedOutboundMessages);
        Complete(Result);
        return;
    }

    Complete(WriteOp.Status());
}


///////////////////////////////////////////////////////////////////////////////
//
//  VNetwork
//
//  Virtual network simulator
//

VNetwork* VNetwork::_g_pVNetPtr = nullptr;
LONG      VNetwork::_g_VNetPtrUseCount = 0;

//
// VNetwork::VNetwork
//
VNetwork::VNetwork(
    ) :
    _Queue(GetThisAllocator()),
    _Networks(GetThisAllocator())
{
    _ThreadHandle = 0;
    _Shutdown = FALSE;
    _InjectorEnabled = FALSE;
    //_Injector = _new(KTL_TAG_NET, GetThisAllocator()) KNetFaultInjector;
    _DelayEngine = nullptr;
}

VNetwork::VNetMessage::VNetMessage(
    ) :
    _Xfer(GetThisAllocator()),
    _DestinationUri(GetThisAllocator()),
    _OriginatingUri(GetThisAllocator())
{
    // Nothing
}

VNetwork::VNetMessage::~VNetMessage()
{
    // Nothing
}

//
//  VNetwork::Acquire
//
VNetwork*
VNetwork::Acquire()
{
    LONG Res = InterlockedIncrement(&_g_VNetPtrUseCount);
    if (Res & VNET_SHUTDOWN_FLAG)
    {
         InterlockedDecrement(&_g_VNetPtrUseCount);
         return nullptr;
    }

    return _g_pVNetPtr;
}

//
//  VNetwork::Redelivery
//
VOID
VNetwork::Redelivery(
    __in PVOID Msg
    )
{
    UNREFERENCED_PARAMETER(Msg);            
}

//
//  VNetwork::DelayMessage
//
VOID
VNetwork::DelayMessage(
    __in VNetMessage::SPtr& Msg,
    __in ULONG Delay
    )
{
    UNREFERENCED_PARAMETER(Msg);            
    UNREFERENCED_PARAMETER(Delay);
}

//
//  VNetwork::Release
//
VOID
VNetwork::Release()
{
    InterlockedDecrement(&_g_VNetPtrUseCount);
}

//
// VNetwork::~VNetwork
//
VNetwork::~VNetwork()
{
}


//
//  VNetwork::EnableFaultInjection
//
VOID
VNetwork::EnableFaultInjection()
{
    _InjectorEnabled = TRUE;
}

//
//  VNetwork::DisableFaultInjection
//
VOID
VNetwork::DisableFaultInjection()
{
    _InjectorEnabled = FALSE;
}

//
//  VNetwork::GetFaultInjector
//
NTSTATUS
VNetwork::GetFaultInjector(
    __out KNetFaultInjector::SPtr& Injector
    )
{
    if (!_Injector)
    {
        return STATUS_INTERNAL_ERROR;
    }
    Injector = _Injector;
    return STATUS_SUCCESS;
}

//
//  VNetwork::Add
//
NTSTATUS
VNetwork::Add(
    __in KSharedPtr<KNetwork> Net
    )
{
    // TBD: Check for pre-existence
    _TableLock.Acquire();
    NTSTATUS Result = _Networks.Append(Net);
    _TableLock.Release();
    return Result;
}

//
//  VNetwork::Remove
//
VOID
VNetwork::Remove(
    __in KSharedPtr<KNetwork> Net
    )
{
    _TableLock.Acquire();
    for (ULONG i = 0; i < _Networks.Count(); i++)
    {
        KNetwork::SPtr Tmp = _Networks[i];
        if (Tmp == Net)
        {
            _Networks.Remove(i);
            break;
        }
    }
    _TableLock.Release();
}

//
//  VNetwork::Shutdown
//
VOID
VNetwork::Shutdown()
{
    // Prevent further acquisition.
    //
    _InterlockedOr(&_g_VNetPtrUseCount, VNET_SHUTDOWN_FLAG);

    VNetwork* Tmp = _g_pVNetPtr;
    if (!Tmp)
    {
        return;
    }

    // Prevent further acquisition.
    //
    _g_pVNetPtr = nullptr;

    Tmp->_Shutdown = TRUE;
    Tmp->_NewMsgEvent.SetEvent();

    // The dedicated thread will complete the first
    // round of shutdown operations, and will set
    // the 'shutdown' event when it is done.
    //
    Tmp->_ShutdownEvent.WaitUntilSet();
    _delete(Tmp);
}


//
//  VNetwork::ShutdownSequence
//
//  Implements a clean shutdown.  This is executing
//  from the VNetwork private thread, so it can sleep/poll.
//  We are guaranteed not to be setting up a receiver
//  at this point.
//
void VNetwork::ShutdownSequence()
{
    // First, we want to make sure that each
    // KNetwork is decoupled from the VNetwork.
    // They can do their shutdown thing safely. We
    // just need them to quit posting new
    // datagrams.

    // First, notify each KNetwork to quit talking to us.
    //
    _TableLock.Acquire();
    _Networks.Clear();
    _TableLock.Release();

    // Since other users may have acquired a VNet pointer
    // before we began shutdown (reflected in the use
    // count in _g_VNetPtrUseCount, we have to wait
    // for that to drain to zero before exiting.
    //

    while (~VNET_SHUTDOWN_FLAG & _g_VNetPtrUseCount)
    {
#if KTL_USER_MODE
        SleepEx(50, TRUE);
#else
        ULONGLONG WaitIn100nsUnit = 10000 * 50;
        KeDelayExecutionThread(KernelMode, FALSE, PLARGE_INTEGER(&WaitIn100nsUnit));
#endif
    }

    _ShutdownEvent.SetEvent();
}


//
//  VNetwork::Startup
//
NTSTATUS
VNetwork::Startup(
    __in KAllocator& Alloc
    )
{
    _g_VNetPtrUseCount = 0;

    VNetwork* Tmp = _new(KTL_TAG_NET, Alloc) VNetwork;
    if (!Tmp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,nullptr, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    _g_pVNetPtr = Tmp;

    return _g_pVNetPtr->StartThreads();
}


//
//  VNetwork::StartThreads
//
NTSTATUS
VNetwork::StartThreads()
{
    // Create the servicing thread
#if KTL_USER_MODE
    DWORD dwId;
    _ThreadHandle = CreateThread(0, 0, &VNetwork::_UmThreadEntry, this, 0, &dwId);
    if (_ThreadHandle == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
#else

    NTSTATUS Status;
    OBJECT_ATTRIBUTES oa;

    InitializeObjectAttributes(&oa, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = PsCreateSystemThread(&_ThreadHandle, 0, &oa, 0, NULL, &VNetwork::_KmThreadEntry, this);

    if (!NT_SUCCESS(Status)) {
        _ThreadHandle = NULL;
        return Status;
    }
#endif

    return STATUS_SUCCESS;
}


//
//  Controller::Enqueue
//
//  Enqueues an operation for use within this specific instance.
//
NTSTATUS
VNetwork::Post(
    __in VNetMessage::SPtr& Msg
   )
{
    if (_Shutdown == TRUE)
    {
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }
    _QueueLock.Acquire();
    BOOLEAN Res = _Queue.Enq(Msg);
    _QueueLock.Release();

    if (!Res)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    _NewMsgEvent.SetEvent();
    return STATUS_SUCCESS;
}


//
// Kernel mode thread entry point.  Transfers control to
// ThreadProc, which is non-static.
//
VOID
VNetwork::_KmThreadEntry(PVOID _This)
{
    ((VNetwork *) _This)->ThreadProc();
}



//
//  User mode thread entry point. Transfers control to
//  ThreadProc, which is non-static.
//
ULONG
VNetwork::_UmThreadEntry(PVOID _This)
{
    ((VNetwork *) _This)->ThreadProc();
    return 0;
}


//
// ThreadProc
//
// This runs in a separate system thread and simulates the 'ether'. All
// cross-ep requests come into this loop via derived types from Operation and
// are dispatched whenever the event is signalled.
//
void VNetwork::ThreadProc()
{
    VNetMessage::SPtr Msg;

    for (;;)
    {
        _NewMsgEvent.WaitUntilSet();

        if (_Shutdown)
        {
            break;
        }
        for (;;)
        {
            // Dequeue an operation
            _QueueLock.Acquire();
            if (!_Queue.IsEmpty())
            {
                Msg = _Queue.Peek();
                _Queue.Deq();
            }
            _QueueLock.Release();

            if (!Msg)
            {
                break; // To outer loop
            }

            Dispatch(Msg);
            Msg = nullptr;  // Necessary for correc processing
        }
    }
    ShutdownSequence();
}


//
// Dispatch
//
// Forward the operation to the correct strongly typed handler
//
void VNetwork::Dispatch(
    __in VNetMessage::SPtr Msg
    )
{
    NTSTATUS Result;

    // Test the message for dropping or delay.
    //
    if (_InjectorEnabled == TRUE && _Injector)
    {
        ULONG Delay;
        Result = _Injector->TestMessage(Msg->_DestinationUri, Msg->_Id, Delay);
        if (Result == STATUS_CANCELLED)
        {
            // This just means to drop the message, so we let the ref go out of scope and it's dropped.
            return;
        }
        else if (Result == STATUS_WAIT_1)
        {
            // Here we have to delay the message.
            DelayMessage(Msg, Delay);
            return;
        }
    }

    KNetwork::SPtr TargetNet = nullptr;

    _TableLock.Acquire();

    // Loop through and find the owning namespace/binding
    //
    // Replace this later with a faster lookup.
    //
    for (ULONG i = 0; i < _Networks.Count(); i++)
    {
        KNetwork::SPtr Tmp = _Networks[i];
        KNetwork::EpEntry::SPtr  EpEntry = nullptr;

        EpEntry = Tmp->LookupByUri(Msg->_DestinationUri, TRUE);

        // Make sure that the matching endpoint is published by
        // the KNetwork in question, not just registered for outbound use.
        //
        if (EpEntry && (EpEntry->_Flags & KNetwork::eLocalEp))
        {
            TargetNet = Tmp;
            break;
        }
    }

    _TableLock.Release();

    // Did we find the receiving KNetwork?
    //
    if (TargetNet == nullptr)
    {
        // If were, we have a situation where the destination endpoint
        // disappeared while we were trying to send it, and we have to abandon the message.
        //
        //KDbgPrintf("Internal Errror:  Endpoint not available for message");
        return;
    }

    // If here, we have a KNetwork to which we can deliver the message.
    //
    KDatagramReadOp::SPtr ReadOp = nullptr;
    Result = TargetNet->CreateDatagramReadOp(ReadOp);

    if (!NT_SUCCESS(Result))
    {
        // Major issue.  We can't receive a datagram that was sent.
        // We will drop it
        //KDbgPrintf("Internal error:  Endpoint unable to receive datagram");
        return;
    }

    // Okay, dispatch this for execution in the KTL space
    //
    ReadOp->StartRead(Msg, TargetNet, 0);
}


////////////////////////////////////
//
// KDatagramReadOp
//



//
// KDatagramReadOp::OnStart
//
// This is where we decode incoming bits.  First, the original output channel is examined
// to determine the Message Id GUID.  We then look up the associated receiver.
// We also have to translate the Uri back to the GUID representation that is used
// in that specific KNetwork object.
//
// Then, we invoke the KConstruct/KDeserialize sequence for the object. In this situation, we
// don't have the object type, just pointers to the implementations for construction and deserialization.
// Finally, we construct a KInDatagram object and send it to the Receiver.
//
// If anything goes wrong in this sequence, there is nothing we can do except drop the message
// and log it, since we are operating from the outside (network) trying to reach into the
// space where the user of this stuff lives.
//
//
void
KDatagramReadOp::OnStart()
{
    if (_InMemoryMode)
    {
        OnMemoryTransportStart();
    }
    else
    {
        OnNetworkTransportStart();
    }
}

//
// KDatagramReadOp::OnMemoryTransportStart
//
void
KDatagramReadOp::OnMemoryTransportStart()
{
    NTSTATUS Result;

    // First, determine the GUID of the message.
    //
    KGuid MessageId;
    Result = _Msg->_Xfer.GetMessageId(MessageId);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Coult not get the message ID from the transfer agent");
        Complete(Result);
        return;
    }

    // Next, find the receiver for the Uri.
    //
    KNetwork::ReceiverInfo::SPtr ReceiverInf;
    Result = _ReceivingNetwork->FindReceiverInfo(MessageId, ReceiverInf);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Could not find receiver for the specified message type");
        Complete(Result);
        return;
    }

    // Read from the transfer agent into the Deserializer.
    //
    Result = _Msg->_Xfer.WritePass1(*_Deserializer);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Could not move data from transfer agent to deserializer");
        Complete(Result);
        return;
    }


    // Look up the local GUIDs that go with the Uris in the message.
    //
    KGuid GuidFrom;
    KGuid GuidTo;
    ULONG EpType;

    Result = _ReceivingNetwork->GetEp(_Msg->_OriginatingUri, EpType, GuidFrom);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Second pass on deserializer failed\n");
        Complete(Result);
        return;
    }

    Result = _ReceivingNetwork->GetEp(_Msg->_DestinationUri, EpType, GuidTo);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Second pass on deserializer failed\n");
        Complete(Result);
        return;
    }

    // Retrieve a binding for this KNetwork object.
    //
    KNetBinding::SPtr Binding;
    Result = _ReceivingNetwork->GetBinding(GuidFrom, GuidTo, 0, Binding);

    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Invoke the K_CONSTRUCT/K_DESERIALIZE implementations
    //

    VOID* TheObject = 0;

    Result = _Deserializer->UntypedDeserialize(ReceiverInf->_UserContext, ReceiverInf->_KConstruct, ReceiverInf->_KDeserialize, TheObject);
    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Could not construct/deserialize the object");
        Complete(Result);
        return;
    }

    // Now fill in any deferred blocks.
    //
    Result = _Msg->_Xfer.WritePass2(*_Deserializer);

    if (!NT_SUCCESS(Result))
    {
//        KDbgPrintf("KDatagramReadOp Failure: Second pass on deserializer failed\n");
        Complete(Result);
        return;
    }


    KInDatagram::SPtr Datagram = _new(KTL_TAG_NET, GetThisAllocator()) KInDatagram(TheObject, _Msg->_ActId);

    // Deliver the package to the receiver.
    //
    if (ReceiverInf->_KNetReceiver)
    {
        ReceiverInf->_KNetReceiver(
            Binding,
            Datagram
            );
    }
    else
    {
        ReceiverInf->_OldReceiver(
            GuidTo,
            GuidFrom,
            0,
            ReceiverInf->_UserContext,
            Binding,
            Datagram
            );
    }

    // All done.
    // Let everything self-destruct via our highly intelligent smart pointers.
    Complete(STATUS_SUCCESS);
}

//
//  KDatatgramReadOp::NetworkReadStart
//
VOID
KDatagramReadOp::OnNetworkTransportStart()
{
    NTSTATUS Result;

    // If here, we have an incoming datagram off the network
    // via KNetDispatch.
    //
    // We have read just the header & URI so far, so we'll need to
    // Allocate memory for the Meta & Data blocks and read them.
    //
    ISocketReadOp::SPtr ReadOp;
    Result = _Binding->GetReadSocket()->CreateReadOp(ReadOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Meta & Data streams into some new
    // buffers.  Then we do a first pass deserialize to get
    // buffers for the rest of it, followed by a second pass
    // socket scatter read into the deferred buffers.

    KArray<KMemRef> MemRefs(GetThisAllocator());

    if (_NetHeader._MetaLength)
    {
        _MetaMemRef._Address = _newArray<UCHAR>(KTL_TAG_NET, GetThisAllocator(), _NetHeader._MetaLength);
        if (!_MetaMemRef._Address)
        {

            KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, _NetHeader._MetaLength, 0 );
            Complete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        _MetaMemRef._Size = _NetHeader._MetaLength;
        _MetaMemRef._Param = _NetHeader._MetaLength;
        MemRefs.Append(_MetaMemRef);
    }

    if (_NetHeader._DataLength)
    {
        _DataMemRef._Address = _newArray<UCHAR>(KTL_TAG_NET, GetThisAllocator(), _NetHeader._DataLength);
        if (!_DataMemRef._Address)
        {
            KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, _NetHeader._DataLength, 0 );
            Complete(STATUS_INSUFFICIENT_RESOURCES);
            return;
        }
        _DataMemRef._Size = _NetHeader._DataLength;
        _DataMemRef._Param = _NetHeader._DataLength;
        MemRefs.Append(_DataMemRef);
    }

    if (MemRefs.Count() == 0)
    {
        Complete(STATUS_INTERNAL_ERROR);
        return;
    }

    Result = ReadOp->StartRead(MemRefs.Count(), MemRefs.Data(), ISocketReadOp::ReadAll, KAsyncContextBase::CompletionCallback(this, &KDatagramReadOp::Phase1ReadComplete));
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
}


//
//  KDatagramReadOp::Phase1ReadComplete
//
VOID
KDatagramReadOp::Phase1ReadComplete(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    
    ISocketReadOp& ReadOp = (ISocketReadOp &) Op;
    NTSTATUS Result = ReadOp.Status();
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Load the first two blocks into the deserializer.
    //
    Result = _Deserializer->LoadBlock( eBlockTypeMeta, _MetaMemRef);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    Result = _Deserializer->LoadBlock( eBlockTypeData, _DataMemRef);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Ask what message type is contained in the transmission.
    //
    GUID MessageId = *(GUID *) _DataMemRef._Address;

    // If here, we have the meta/data blocks for the object and can begin pass 1 of deserialization.
    // The next step is to find the correct receiver for the message.
    //
    KNetwork::ReceiverInfo::SPtr ReceiverInf;
    Result = _ReceivingNetwork->FindReceiverInfo(MessageId, ReceiverInf);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Now, deserialize, pass 1.
    //
    _TheObject = 0;

    Result = _Deserializer->UntypedDeserialize(ReceiverInf->_UserContext, ReceiverInf->_KConstruct, ReceiverInf->_KDeserialize, _TheObject);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Now, we have to figure out if we have any deferred blocks.  If so, schedule another socket read.
    // Otherwise, skip to finalize.

    if (_Deserializer->HasDeferredBlocks() == FALSE)
    {
        KInDatagram::SPtr Datagram = _new(KTL_TAG_NET, GetThisAllocator()) KInDatagram(_TheObject, _NetHeader._ActivityId);

        // Deliver the package to the receiver.
        //
        KGuid GuidTo;
        KGuid GuidFrom;

        _Binding->GetLocalEp(GuidTo);
        _Binding->GetRemoteEp(GuidFrom);

        if (ReceiverInf->_KNetReceiver)
        {
            ReceiverInf->_KNetReceiver(_Binding, Datagram);
        }
        else
        {
            ReceiverInf->_OldReceiver(
                GuidTo,
                GuidFrom,
                0,
                ReceiverInf->_UserContext,
                _Binding,
                Datagram
                );
        }

        // All done.
        // Let everything self-destruct via our highly intelligent smart pointers.
        //
        Complete(STATUS_SUCCESS);
        return;
    }

    // If here, we have to schedule another read for deferred blocks.
    //
    ISocketReadOp::SPtr ReadOpPhase2;
    Result = _Binding->GetReadSocket()->CreateReadOp(ReadOpPhase2);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Tranfer stuff from the _BlockData channel into the
    // object buffers
    //
    KArray<KMemRef> MemRefs(GetThisAllocator());
    KMemRef Mem;
    for (;;)
    {
       Result = _Deserializer->NextDeferredBlock(Mem);
       if (Result == STATUS_NO_MORE_ENTRIES)
       {
           break;
       }
       if (!NT_SUCCESS(Result))
       {
            Complete(Result);
            return;
       }
       MemRefs.Append(Mem);
    }

    Result = ReadOpPhase2->StartRead(MemRefs.Count(), MemRefs.Data(), ISocketReadOp::ReadAll, KAsyncContextBase::CompletionCallback(this, &KDatagramReadOp::Phase2ReadComplete));
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // If here, detach the ReceiverInf for later reconstitution.
    //
    _GenReceiverInf = ReceiverInf.Detach();
}


//
//  KDatagramReadOp::Phase2ReadComplete
//
VOID
KDatagramReadOp::Phase2ReadComplete(
    __in KAsyncContextBase* const Parent,
    __in KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    
    // First reattach the receiver info for correct later cleanup
    //
    KNetwork::ReceiverInfo::SPtr ReceiverInf;
    ReceiverInf.Attach((KNetwork::ReceiverInfo*)_GenReceiverInf);
    _GenReceiverInf = nullptr;

    // Now check the status of the operation
    //
    ISocketReadOp& ReadOp = (ISocketReadOp &) Op;
    NTSTATUS Result = ReadOp.Status();
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    // Load the first two blocks into the deserializer.
    //
    Result = _Deserializer->LoadBlock( eBlockTypeMeta, _MetaMemRef);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
    KInDatagram::SPtr Datagram = _new(KTL_TAG_NET, GetThisAllocator()) KInDatagram(_TheObject, _NetHeader._ActivityId);

    // Deliver the package to the receiver.
    //
    KGuid GuidTo;
    KGuid GuidFrom;

    _Binding->GetLocalEp(GuidTo);
    _Binding->GetRemoteEp(GuidFrom);

    if (ReceiverInf->_KNetReceiver)
    {
        ReceiverInf->_KNetReceiver(_Binding, Datagram);
    }
    else
    {
        ReceiverInf->_OldReceiver(
            GuidTo,
            GuidFrom,
            0,
            ReceiverInf->_UserContext,
            _Binding,
            Datagram
            );
    }

    // All done.
    // Let everything self-destruct via our highly intelligent smart pointers.
    //
    Complete(STATUS_SUCCESS);
    return;
}


//
//  KDatagramReadOp::FinalizeAndDeliver
//
VOID
KDatagramReadOp::FinalizeAndDeliver()
{
}


//
//  KDatagramReadOp::KDatagramReadOp
//
KDatagramReadOp::KDatagramReadOp(
    )
{
    InterlockedIncrement(&__KNetwork_PendingOperations);

    _InMemoryMode = FALSE;
    _TheObject = nullptr;
    _BytesToTransfer = 0;
    _GenReceiverInf = nullptr;

    _Deserializer = _new(KTL_TAG_NET, GetThisAllocator()) KDeserializer();
    if (_Deserializer == nullptr)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
}

//
//  KDatagramReadOp::~KDatagramReadOp
//
KDatagramReadOp::~KDatagramReadOp()
{
    // Cleanup
    //
    _delete(_MetaMemRef._Address);
    _delete(_DataMemRef._Address);

    // Clean up the receiver smart pointer, if still outstanding.  This
    // is done by reattach and then letting the smart pointer go out of
    // scope to correctly release it.
    //
    if (_GenReceiverInf)
    {
       KNetwork::ReceiverInfo::SPtr ReceiverInf;
       ReceiverInf.Attach((KNetwork::ReceiverInfo*)_GenReceiverInf);
       _GenReceiverInf = nullptr;
    }

    InterlockedDecrement(&__KNetwork_PendingOperations);
}



//
//  KNetHostFile
//
//  Parser for external host file.
//


//
// KNetHostFile::IsHexDigit
//
BOOLEAN
KNetHostFile::IsHexDigit(WCHAR c)
{
    if ( (c >= L'a' && c <= L'f') ||
         (c >= L'A' && c <= L'F') ||
         (c >= L'0' && c <= L'9'))
    {
        return TRUE;
    }
    return FALSE;
}


//
//  KNetHostFile::IsDigit
//
BOOLEAN
KNetHostFile::IsDigit(WCHAR c)
{
    if (c >= L'0' && c <= L'9')
    {
        return TRUE;
    }
    return FALSE;
}


//
// KNetHostFile::IsWhitespace
//
BOOLEAN
KNetHostFile::IsWhitespace(WCHAR c)
{
    if (c <= 32)
    {
        return TRUE;
    }
    return FALSE;
}

//
//  KNetHostFile::StripComment
//
NTSTATUS
KNetHostFile::StripComment(
    _Outref_result_buffer_(Count) WCHAR*& Current,
    _Inout_ ULONG& Count
    )
{
    while (Count)
    {
        // Strip whitespace
        //
        if (IsWhitespace(*Current))
        {
            Current++;
            Count--;
        }
        else if (*Current == L'/')
        {
            Count--;
            Current++;
            if (Count && *Current == L'/')
            {
                // This line is a comment.
                return STATUS_SOURCE_ELEMENT_EMPTY;
            }
            else // Bad comment
            {
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }
        }
        else // No comment or whitespace
        {
            break;
        }
    }

    if (Count == 0)
    {
        return STATUS_SOURCE_ELEMENT_EMPTY;
    }
    return STATUS_SUCCESS;
}



//
//  KNetHostFile::TryGuid
//
NTSTATUS
KNetHostFile::TryGuid(
    _Outref_result_buffer_(Count) WCHAR*& Current,
    _Inout_ ULONG& Count,
    __out   KGuid& Guid
    )
{
    KWString GuidStr(*_Allocator);
    NTSTATUS Res;
    Res = GuidStr.Reserve(0x2000);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    if (!Count || *Current != L'{')
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }
    GuidStr += *Current++;
    Count--;

    for (ULONG Prefix = 0; Prefix < 8; Prefix++)
    {
        if (!Count || !IsHexDigit(*Current))
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        GuidStr += *Current++;
        Count--;
    }

    if (!Count || *Current != L'-')
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }
    GuidStr += *Current++;
    Count--;

    for (ULONG Inner = 0; Inner < 3; Inner++)
    {
        for (ULONG InnerSequence = 0; InnerSequence < 4; InnerSequence++)
        {
            if (!Count || !IsHexDigit(*Current))
            {
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }
            GuidStr += *Current++;
            Count--;
        }

        if (!Count || *Current != L'-')
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        GuidStr += *Current++;
        Count--;
    }

    for (ULONG Suffix = 0; Suffix < 12; Suffix++)
    {
        if (!Count || !IsHexDigit(*Current))
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        GuidStr += *Current++;
        Count--;
    }

    if (!Count || *Current != L'}')
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }
    GuidStr += *Current++;
    Count--;

    return RtlGUIDFromString(PUNICODE_STRING(GuidStr), &Guid);
}


//
// KNetHostFile::TryPort
//
NTSTATUS
KNetHostFile::TryPort(
    _Outref_result_buffer_(Count) WCHAR*& Current,
    _Inout_ ULONG& Count,
    __out   USHORT& Port
    )
{
    KWString PortStr(*_Allocator);
    PortStr.Reserve(128);

    // Strip leading spaces
    //
    for (;;)
    {
        if (!Count)
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        if (IsWhitespace(*Current))
        {
            Count--;
            Current++;
        }
        else break;
    }

    for (;;)
    {
        if (!Count)
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        if (IsDigit(*Current))
        {
            PortStr += *Current++;
            Count--;
        }
        else break;
    }

    if (PortStr.Length() == 0)
    {
        return STATUS_SUCCESS;  // Port number is optional in this position
    }

    ULONG Tmp = 0;
    NTSTATUS Result = RtlUnicodeStringToInteger(PUNICODE_STRING(PortStr), 10, &Tmp);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }
    Port = USHORT(Tmp);
    return STATUS_SUCCESS;
}


//
// KNetHostFile::TryUri
//
NTSTATUS
KNetHostFile::TryUri(
    _Outref_result_buffer_(Count) WCHAR*& Current,
    _Inout_ ULONG& Count,
    __in    USHORT OverridePort,
    __out   KWString& FullUri,
    __out   KWString& Server,
    __out   KWString& RelativeUri,
    __out   USHORT& Port,
    __out   ULONG& Flags
    )
{
    NTSTATUS Result;
    KWString PortString(*_Allocator);

    // BUGBUG: Fix constant or change loop to test max
    Result = Server.Reserve(0x200);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // BUGBUG: Fix constant or change loop to test max
    RelativeUri.Reserve(0x400);
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    // Strip leading spaces
    //
    for (;;)
    {
        if (!Count)
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        if (IsWhitespace(*Current))
        {
            Count--;
            Current++;
        }
        else break;
    }

    // Read the RVD scheme and leading slashes
    //
    PCWSTR Prefix = L"rvd://";
    for (ULONG i = 0; i < 6; i++)
    {
        if (!Count || *Current != Prefix[i])
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        Current++;
        Count--;
    }

    FullUri += L"rvd://";


    // Recognize computer name until next slash. Don't allow whitespace
    //
    BOOLEAN PortDesignator = FALSE;
    for (;;)
    {
        if (!Count || *Current <= 32)
        {
            return STATUS_OBJECT_PATH_SYNTAX_BAD;
        }
        if (*Current == '/')
        {
            break;
        }
        if (*Current == ':')
        {
            PortDesignator = TRUE;
            break;
        }
        Server += *Current++;
        Count--;
    }

    if (Server.Length() == 0)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    // THe use of . is same as ..localmachine
    //
    if (Server.CompareTo(L".") == 0)
    {
        Flags =  KNetUriInfo::eLocalMachine;
        Server = L"..localmachine";
    }
    else if (Server.CompareTo(L"(local)") == 0)
    {
        Flags =  KNetUriInfo::eVNet;
    }
    else if (Server.CompareTo(L"..localmachine") == 0)
    {
        Flags =  KNetUriInfo::eLocalMachine;
    }
    else
    {
        Flags =  KNetUriInfo::eServerName;
    }

    FullUri += Server;

    if (PortDesignator)
    {
        // Skip the colon.
        //
        Current++;
        Count--;

        // If here, we have an integer port number, so we acquire characters until the next slash or end of string.
        // Also, we want to normalize the URI to a form without the port present.
        //
        UNICODE_STRING TmpStr;
        TmpStr.Buffer = Current;
        TmpStr.Length = 0;

        for (;;)
        {
            if (!Count)
            {
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }

            if (*Current >= '0' && *Current <='9')
            {
                Count--;
                Current++;
                TmpStr.Length += 2;
                continue;
            }

            ULONG TmpVal = 0;
            NTSTATUS Result1 = RtlUnicodeStringToInteger(&TmpStr, 10, &TmpVal);
            if (!NT_SUCCESS(Result1))
            {
                return STATUS_OBJECT_PATH_SYNTAX_BAD;
            }

            PortString = TmpStr;
            Port = USHORT(TmpVal);
            break;
        }
    }

    // Rest of text is relative URI, until Count==0 or whitespace
    // Trailing comments are supported, but are essentially based on the whitespace
    // at end of the URI before the comment starts, so we don't have an explicit
    // comment recognizer
    //
    for (;;)
    {
        if (!Count || *Current <= 32)
        {
            break;
        }

        RelativeUri += *Current++;
        Count--;
    }

    if (RelativeUri.Length() == 0)
    {
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    // At this point, we must either have seen an explicit override port or one
    // must have appeared in the URI itself.
    //

    if (PortDesignator)
    {
        FullUri += L":";
        FullUri += PortString;
        FullUri += RelativeUri;
    }
    else if (OverridePort != 0)
    {
        KLocalString<256> Tmp;
        Tmp.FromULONG(OverridePort);
        PortString = Tmp;
        FullUri += L":";
        FullUri += PortString;
        FullUri += RelativeUri;
    }
    else
    {
        // Missing port in both positions.
        //
        return STATUS_OBJECT_PATH_SYNTAX_BAD;
    }

    return STATUS_SUCCESS;
}


//
//  KNetHostFile::ParseLine
//
//
//  Insists on the following:
//  {GUID} Port //server/uriPart1/uriPart2.../uriPartN
//
//  Returns:
//      STATUS_SUCCESS if a proper line was recognized.
//      STATUS_SOURCE_ELEMENT_EMPTY if the line was empty or all comments.
//      STATUS_UNSUCCESSFUL if the line contains a syntax error or is incomplete.
//
NTSTATUS
KNetHostFile::ParseLine(
    __in  KWString& Line,
    __out KGuid&    Guid,
    __out USHORT&   Port,
    __out KWString& FullUri,
    __out KWString& ServerStr,
    __out KWString& RelativeUri,
    __out ULONG&    Flags
    )
{
    wchar_t* Current = PWSTR(Line);
    ULONG    Count = Line.Length()/2; // Count of chars
    NTSTATUS Res;

    // Recognize comment or line with all whitespace
    //
    Res = StripComment(Current, Count);
    if (Res == STATUS_SOURCE_ELEMENT_EMPTY)
    {
        return Res;
    }

    // Recognize a GUID
    //
    Res = TryGuid(Current, Count, Guid);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Recognize decimal port.
    // BUGBUG: This code is obsolete since we included the port in the URI
    // Remove this at some point once we have eliminated all the old host files.
    //
    // For now, this executes, but the resulting port number may be overwritten
    // if the URI itself has a port number.
    //
    Res = TryPort(Current, Count, Port);
    if (!NT_SUCCESS(Res))
    {
        return Res;
    }

    // Recognize leading double slash
    //
    Res = TryUri(Current, Count, Port, FullUri, ServerStr, RelativeUri, Port, Flags);

    return Res;
}

//
//  KNetHostFile::Parse
//
NTSTATUS
KNetHostFile::Parse(
    __in  KWString& PathToFile,
    __out ULONG& ErrorLine
    )
{
    UNREFERENCED_PARAMETER(PathToFile);            
    
    NTSTATUS Result;
    KTextFile::SPtr TxtFile;
    const ULONG MaxLine = 2048;
    KWString Path(*_Allocator, L"\\SystemRoot\\rvd\\hosts.txt");

    Result = KTextFile::OpenSynchronous(
        Path,
        MaxLine,
        TxtFile,
        KTL_TAG_NET,
        _Allocator
        );

    if (!NT_SUCCESS(Result))
    {
        return Result;
    }

    for (;;)
    {
        KWString Line(*_Allocator);
        Result = TxtFile->ReadLine(Line);
        if (Result == STATUS_END_OF_FILE)
        {
            break;
        }

        if (!NT_SUCCESS(Result))
        {
           // _printf("FAILED to read\n");
            return Result;
        }

        KGuid Guid;
        USHORT Port = 0;
        KWString Host(*_Allocator);
        KWString RelUri(*_Allocator);
        KWString FullUri(*_Allocator);
        ULONG  Flags = 0;

        Result = ParseLine(Line, Guid, Port, FullUri, Host, RelUri, Flags);

        if (Result == STATUS_SOURCE_ELEMENT_EMPTY)
        {
            continue;
        }
        if (!NT_SUCCESS(Result))
        {
            ErrorLine = TxtFile->GetCurrentLineNumber();
            return Result;
        }

        // Add new entry
        //
        KNetUriInfo::SPtr Inf;
        Result = KNetUriInfo::Create(Inf, *_Allocator);

        if (!NT_SUCCESS(Result)) {
            return Result;
        }

        Inf->_FullUri = FullUri;
        Inf->_Authority  = Host;
        Inf->_RelUri  = RelUri;
        Inf->_GUID    = Guid;
        Inf->_Port    = Port;
        Inf->_Flags   = Flags;

        _Output.Append(Inf);
    }

    return STATUS_SUCCESS;
}

KNetUriInfo::KNetUriInfo()
  : _Authority(GetThisAllocator()),
    _FullUri(GetThisAllocator()),
    _RelUri(GetThisAllocator())
{
    _Flags = 0;
    _Port = 0;
}

KNetUriInfo::~KNetUriInfo()
{
    // Nothing.
}



//
// KNetUriInfo::Parse
//
NTSTATUS
KNetUriInfo::Parse(
    __in KWString& RawUri
    )
{
    KNetworkEndpoint::SPtr Tmp;
    KUriView proposed = KUriView(LPCWSTR(RawUri));
    NTSTATUS Result = KRvdTcpNetworkEndpoint::Create(proposed, GetThisAllocator(), Tmp);
    if (!NT_SUCCESS(Result))
    {
       return STATUS_INVALID_PARAMETER;
    }

    _NetEp = down_cast<KRvdTcpNetworkEndpoint, KNetworkEndpoint>(Tmp);

    if (!_NetEp->GetUri()->IsValid())
    {
       return STATUS_INVALID_PARAMETER;
    }

    if (_NetEp->GetUri()->Get(KUriView::eScheme).Compare(KStringView(L"rvd")) != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    _FullUri = RawUri;
    _Authority = _NetEp->GetUri()->Get(KUriView::eAuthority);
    _RelUri = _NetEp->GetUri()->Get(KUriView::ePath);
    _Port = (USHORT) _NetEp->GetUri()->GetPort();

    KStringView Host = _NetEp->GetUri()->Get(KUriView::eHost);

    // Process the _Flags
    //
    // The use of . is same as ..localmachine
    //
    if (Host.Compare(KStringView(L".")) == 0)
    {
        _Flags =  KNetUriInfo::eLocalMachine;
    }
    else if (Host.Compare(KStringView(L"(local)")) == 0)
    {
        _Flags =  KNetUriInfo::eVNet;
    }
    else if (Host.Compare(KStringView(L"..localmachine")) == 0)
    {
        _Flags =  KNetUriInfo::eLocalMachine;
    }
    else
    {
        _Flags =  KNetUriInfo::eServerName;
    }
    return STATUS_SUCCESS;
}

//------------------------------------------------------------------------------------------------------------------------

//
// KNetServiceLayerStartup::KNetServiceLayerStartup
//
KNetServiceLayerStartup::KNetServiceLayerStartup()
{
    _SvcLayer = nullptr;
}

//
// KNetServiceLayerStartup::~KNetServiceLayerStartup
//
KNetServiceLayerStartup::~KNetServiceLayerStartup()
{
}

//
//  static
//
KNetServiceLayer::SPtr* KNetServiceLayer::_gSvcLayer;

//
//  KNetServiceLayer::KNetServiceLayer
//
KNetServiceLayer::KNetServiceLayer(
    __in KRvdTcpNetworkEndpoint::SPtr RootEp
    )
    :   _PublishedEpTable(GetThisAllocator()),
        _AddressTableByFullUri(GetThisAllocator()),
        _AddressTableByGuid(GetThisAllocator()),
        _SocketTable(GetThisAllocator()),
        _InboundSocketTable(GetThisAllocator()),
        _LocalServerAddresses(GetThisAllocator())

{
    _RootEp = RootEp;
    _ShutdownFlag = FALSE;
}

//
//  KNetServiceLayer::~KNetServiceLayer
//
KNetServiceLayer::~KNetServiceLayer()
{
}

//
// KNetServiceLayer::Startup
//
VOID
KNetServiceLayer::Startup(
   __in StatusCallback StartupCallback,
   __in PVOID StartupCtx
   )
{
    _StartupCallback = StartupCallback;
    _StartupCtx = StartupCtx;

    // Initialize the lookup tables
    //
    NTSTATUS Result;
    Result = _PublishedEpTable.Initialize(128, K_DefaultHashFunction);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    Result = _AddressTableByFullUri.Initialize(128, K_DefaultHashFunction);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    Result = _AddressTableByGuid.Initialize(128, K_DefaultHashFunction);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    Result = _SocketTable.Initialize(128, K_DefaultHashFunction);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    KDbgCheckpoint(2, "Loading host file");

    Result = LoadExternalAddressFile();
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    // Initialize the transport.
    //

    KDbgCheckpoint(3, "Initializing transport");

#if KTL_USER_MODE

    Result = KUmTcpTransport::Initialize(GetThisAllocator(), _Transport);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

#else

    Result = KKmTcpTransport::Initialize(GetThisAllocator(), _Transport);
    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

#endif

    //
    // Use the default wildcard local address.
    // Leave the routing decision to the OS.
    //
    _LocalClientAddress.Reset();


//   CONFLICT (raymcc) with KTL merge; is Fabric using hard-wired port?
//    Result = _Transport->SpawnDefaultListenerAddresses(_RootEp->GetPort(), _LocalServerAddresses);
    Result = _Transport->SpawnDefaultListenerAddresses(_LocalServerAddresses);

    if (!NT_SUCCESS(Result))
    {
        _StartupCallback(Result, _StartupCtx);
        return;
    }

    // Now start up the listeners. We don't know which NIC/IP
    // the caller will reach us at, so we try all of them.
    //

    // BUGBUG: HACK until we get rid of array
    for (ULONG i = 0; i < __min(_LocalServerAddresses.Count(), 1); i++)
    {
        ISocketListener::SPtr Listener;

        Result = _Transport->CreateListener(_LocalServerAddresses[i], Listener);
        if (!NT_SUCCESS(Result))
        {
            _StartupCallback(Result, _StartupCtx);
            return;
        }
        _Listener = Listener;
    }

    KDbgCheckpoint(4, "Posting initial accept");

    Result = PostAccept();

   _StartupCallback(Result, _StartupCtx);
}


VOID
KNetServiceLayer::NextSequentialGuid(
    __out GUID& NewGuid
    )
{
    static ULONG Seed = 0;
    RtlZeroMemory(&NewGuid, sizeof(GUID));
    ULONG Res = InterlockedIncrement((PLONG) &Seed);
    PULONG Tmp = (PULONG) & NewGuid;
    *Tmp = Res;
}

//
//  KNetServiceLayer::OnAcceptComplete
//
//  Called whenever a new socket connects and is accepted.
//
VOID
KNetServiceLayer::OnAcceptComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    
    NTSTATUS Result;
    InterlockedDecrement(&__KNetwork_PendingAccepts);

    ISocketAcceptOp& AcceptOp = (ISocketAcceptOp&) Op;
    Result = AcceptOp.Status();

    // If here, we have succeeded.
    //
    if (NT_SUCCESS(Result))
    {
        ISocket::SPtr _InSocket = AcceptOp.GetSocket();
        if (_InSocket && _InSocket->Ok())
        {
            Result = AddSocketToInboundTable(_InSocket);
            if (NT_SUCCESS(Result))
            {
                PrepostRead(_InSocket);
            }
            else
            {
                // If we couldn't add it, we are low on memory.
                // Just close the socket and let it self-destruct.
                // This code path is very unlikely.
                //
                _InSocket->Close();
            }
        }
        else // This is pathological.  We 'succeeded', yet don't have a socket.
        {
            KDbgPrintf("KFatal(): WSK Accept succeeded but no socket is available; this should not occur\n");
            KFatal(FALSE);
        }
    }
    else
    {
        KDbgPrintf("Accept Failed: Status code =0x%x\n", Result);
    }

    // We always do this.  Shutdown tests are there.
    //

    PostAccept();
}

//
// KNetServiceLayer::OnPrepostReadComplete
//
VOID
KNetServiceLayer::OnPrepostReadComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);            
    
    KNetHeaderReadOp& ReadOp = (KNetHeaderReadOp&) Op;
    
    if (!NT_SUCCESS(ReadOp.Status()))
    {
        ISocket::SPtr Socket = ReadOp.GetSocket();
        Socket->Close();
        RemoveSocketFromInboundTable(Socket);
        KDbgPrintf("Prepost of a read: Socket read failure, Status = 0x%X. Closing socket\n", ReadOp.Status());
        return;
    }

    // Prepost another read
    //
    ISocket::SPtr Socket = ReadOp.GetSocket();
    if (Socket && Socket->Ok())
    {
        PrepostRead(Socket);
    }
}

//
// KNetServiceLayer::PrepostRead
//
VOID
KNetServiceLayer::PrepostRead(
    __in ISocket::SPtr& Socket
    )
{
    // Called whenever we need to prepost a read for a listener.
    KNetHeaderReadOp::SPtr HeaderReadOp;

    NTSTATUS Res = KNetHeaderReadOp::Create(GetThisAllocator(), HeaderReadOp);
    if (!NT_SUCCESS(Res))
    {
        // BUGBUG: WHat do we do here
        return;
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetServiceLayer::OnPrepostReadComplete);

    HeaderReadOp->StartRead(Socket, this, Cb);
}

//  KNetServiceLayer::PostAccept
//
//  Called whenever we need to reuse an accept for
//  acquiring new sockets.
//
//  BUGBUG: Shutdown is not atomic relative to Listener shutdown;
//  Listener can become null asynchronously.
//
NTSTATUS
KNetServiceLayer::PostAccept()
{
    NTSTATUS Result;
    if (__KSocket_Shutdown)
    {
        KDbgPrintf("PostAccept is not posting a new accept because system is shutting down\n");
        _Listener->SetAcceptTerminated();
        return STATUS_SHUTDOWN_IN_PROGRESS;
    }
    ISocketAcceptOp::SPtr Accept;
    Result = _Listener->CreateAcceptOp(Accept);
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("KFatal(): Unable to cre ate an accept op; out of memory\n");
        KFatal(FALSE);
        return STATUS_INTERNAL_ERROR;
    }
    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetServiceLayer::OnAcceptComplete);
    Result = Accept->StartAccept(Cb);
    if (NT_SUCCESS(Result))
    {
        InterlockedIncrement(&__KNetwork_PendingAccepts);
    }
    else
    {
        // This may occur if we are in shutdown.
        //
        _Listener->SetAcceptTerminated();
    }
    return Result;
}

//
// KNetServiceLayer::AddSocketToInboundTable
//
NTSTATUS
KNetServiceLayer::AddSocketToInboundTable(
    __in ISocket::SPtr& Socket
    )
{
    NTSTATUS Res = STATUS_SUCCESS;
    K_LOCK_BLOCK(_TableLock)
    {
        if (!__KSocket_Shutdown)
        {
            Res = _InboundSocketTable.Append(Socket);
        }
        else
        {
            Res = STATUS_SHUTDOWN_IN_PROGRESS;
        }
    }
    return Res;
}

//
//  KNetServiceLayer::RemoveSocketFromInboundTable
//
VOID
KNetServiceLayer::RemoveSocketFromInboundTable(
    __in ISocket::SPtr& Socket
    )
{
    K_LOCK_BLOCK(_TableLock)
    {
        for (ULONG i = 0; i < _InboundSocketTable.Count(); i++)
        {
            if (Socket->GetSocketId() == _InboundSocketTable[i]->GetSocketId())
            {
                _InboundSocketTable.Remove(i);
                break;
            }
        }
    }
}

//
//  KNetServiceLayer::Shutdown
//
VOID
KNetServiceLayer::Shutdown()
{
    if (!_gSvcLayer)
    {
        return;
    }

    // Prevent new operations
    // BUGBUG Consider using __KSocket_Shutdown to prevent new operations.
    //
    KNetServiceLayer::SPtr Tmp = Ktl::Move(*_gSvcLayer);

    Tmp->_Shutdown();

    Tmp.Reset();
    _delete(_gSvcLayer);
    _gSvcLayer = nullptr;
}

VOID
KNetServiceLayer::_Shutdown()
{
    __KSocket_Shutdown = 1;
    __KSocket_ShutdownPhase = 1;

    _ShutdownFlag = TRUE;

    ULONG MaxWait = 5000;
    if (_Listener)
    {
        _Listener->Shutdown();
        while (!_Listener->AcceptTerminated() && MaxWait)
        {
            KNt::Sleep(25);
            MaxWait -= 25;
        }
        _Listener = 0;
    }

    __KSocket_ShutdownPhase = 2;

    _LocalServerAddresses.Clear();
    _LocalClientAddress = 0;

    K_LOCK_BLOCK(_TableLock)
    {
        _SocketTable.Clear();
    }

    __KSocket_ShutdownPhase = 3;

    while (_InboundSocketTable.Count())
    {
        ISocket::SPtr Tmp;
        K_LOCK_BLOCK(_TableLock)
        {
            // Retest the condition inside the lock.
            //
            if (_InboundSocketTable.Count())
            {
                ULONG Last = _InboundSocketTable.Count()-1;
                Tmp = _InboundSocketTable[Last];
                _InboundSocketTable.Remove(Last);
            }
        }
        if (Tmp)
        {
            Tmp->Close();
        }
    }

    __KSocket_ShutdownPhase = 4;

    _Transport->Shutdown();

    __KSocket_ShutdownPhase = 5;
}


VOID
KNetServiceLayerStartup::ZeroCounters()
{
    __KSocket_BytesWritten = 0;
    __KSocket_BytesRead = 0;
    __KSocket_Writes = 0;
    __KSocket_Reads = 0;
    __KSocket_SocketObjects = 0;
    __KSocket_SocketCount = 0;
    __KSocket_AddressCount = 0;
    __KSocket_AddressObjects = 0;
    __KSocket_WriteFailures = 0;
    __KSocket_ReadFailures = 0;
    __KSocket_ConnectFailures = 0;
    __KSocket_PendingOperations = 0;
    __KSocket_PendingReads = 0;
    __KSocket_PendingWrites = 0;
    __KSocket_PendingAccepts = 0;
    __KSocket_Shutdown = 0;
    __KSocket_ShutdownPhase = 0;
    __KSocket_Accepts = 0;
    __KSocket_AcceptFailures = 0;
    __KSocket_Connects = 0;
    __KSocket_Closes = 0;
    __KSocket_OpNumber = 0;
    __KSocket_LastWriteError = 0;
}


//
//  KNetServiceLayerStartup::StartNetwork
//
NTSTATUS
KNetServiceLayerStartup::StartNetwork(
    __in       KNetworkEndpoint::SPtr RootEndpoint,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    ZeroCounters();

    KDbgCheckpoint(1, "Checking parameters");

    if (!is_type<KRvdTcpNetworkEndpoint>(RootEndpoint))
    {
        return STATUS_INVALID_PARAMETER;
    }

    KRvdTcpNetworkEndpoint::SPtr Ep = down_cast<KRvdTcpNetworkEndpoint, KNetworkEndpoint>(RootEndpoint);
    if (!Ep)
    {
        return STATUS_INVALID_PARAMETER;
    }
    KUri::SPtr TestUri = RootEndpoint->GetUri();

    if (!TestUri->IsValid() || Ep->GetPort() == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

#if defined(PLATFORM_UNIX)
    KDbgPrintf("Startup root endpoint URI = %s\n", Utf16To8(LPCWSTR(TestUri->Get(KUri::eRaw))).c_str());
#else
    KDbgPrintf("Startup root endpoint URI = %S\n", LPCWSTR(TestUri->Get(KUri::eRaw)));
#endif

    _SvcLayer = _new(KTL_TAG_NET, GetThisAllocator()) KNetServiceLayer(Ep);
    if (!_SvcLayer)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KDbgCheckpoint(2, "Creating service layer object");

    KNetServiceLayer::_gSvcLayer = _new(KTL_TAG_NET, GetThisAllocator()) KSharedPtr<KNetServiceLayer>();
    if (!KNetServiceLayer::_gSvcLayer)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        _delete(_SvcLayer);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    *KNetServiceLayer::_gSvcLayer = _SvcLayer;

    KDbgCheckpoint(3, "Calling Start()");

    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
// KNetServiceLayerStartup::Completed
//
VOID
KNetServiceLayerStartup::Completed(
    __in NTSTATUS Status,
    __in PVOID Ctx
    )
{
    UNREFERENCED_PARAMETER(Ctx);                
    
    KDbgCheckpointWStatus(1, "Service layer startup completed status=", Status);

    if (NT_SUCCESS(Status))
    {
        // Coalesce pending starts to a plain vanilla success
        Complete(STATUS_SUCCESS);
    }
    else
    {
        Complete(Status);
    }
}


//
//  KNetServiceLayerStartup::OnStart
//
VOID
KNetServiceLayerStartup::OnStart()
{
    KNetServiceLayer::StatusCallback Cb;
    Cb.Bind(this, &KNetServiceLayerStartup::Completed);
    _SvcLayer->Startup(Cb, nullptr);
}



//
//  KNetServiceLayer::PublishEp
//
//  Publishes a URI so that messages can be received
//
//

NTSTATUS
KNetServiceLayer::PublishEp(
    __in KNetUriInfo::SPtr NewUriInf,
    __in GUID&           EpGuid,
    __in KNetwork::SPtr  OwningNetwork
    )
{
    NTSTATUS Result = STATUS_SUCCESS;

    PublishedEpEntry::SPtr NewEp = _new(KTL_TAG_NET, GetThisAllocator()) PublishedEpEntry;
    if (!NewEp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    NewEp->_UriInfo = NewUriInf;
    NewEp->_UriInfo->_GUID = EpGuid;
    NewEp->_OwningNetwork = OwningNetwork;

    K_LOCK_BLOCK(_TableLock)
    {
        Result = _PublishedEpTable.Put(NewEp->_UriInfo->_RelUri, NewEp);
        if (!NT_SUCCESS( Result ) || Result == STATUS_OBJECT_NAME_EXISTS)
        {
            KDbgCheckpointWStatus( 0, "Publish endpoint failed", Result );
        }
    }

    return Result;
}

//
//  KNetServiceLayer::UnpublishEp
//
//  Removes the URI from the lookup table
//
NTSTATUS
KNetServiceLayer::UnpublishEp(
    __in KWString& RelUri
    )
{
    K_LOCK_BLOCK(_TableLock)
    {
        _PublishedEpTable.Remove(RelUri);
    }
    return STATUS_SUCCESS;
}



//
//  KNetServiceLayer::RegisterSystemMessageHandler
//
NTSTATUS
KNetServiceLayer::RegisterSystemMessageHandler(
    __in ULONG Id,
    __in SysMessageHandler CBack
    )
{
    UNREFERENCED_PARAMETER(Id);                
    UNREFERENCED_PARAMETER(CBack);
    
    return STATUS_SUCCESS;
}

//
// KNetServiceLayer::UriToGuid
//
NTSTATUS
KNetServiceLayer::UriToGuid(
    __in KWString& Uri,
    __out GUID& Guid
    )
{
    NTSTATUS Result = STATUS_SUCCESS;
    EpEntry::SPtr Ep;

    K_LOCK_BLOCK(_TableLock)
    {
        Result = _AddressTableByFullUri.Get(Uri, Ep);
        if (NT_SUCCESS(Result))
        {
            Guid = Ep->_UriInfo->_GUID;
            return STATUS_SUCCESS;
        }
    }
    return Result;
}


//
// KNetServiceLayer::UriToGuid
//
NTSTATUS
KNetServiceLayer::GuidToUri(
    __in  GUID& Guid,
    __out KWString& Uri,
    __out USHORT& RemotePort
    )
{
    NTSTATUS Result = STATUS_SUCCESS;
    EpEntry::SPtr Ep;

    K_LOCK_BLOCK(_TableLock)
    {
        Result = _AddressTableByGuid.Get(Guid, Ep);
        if (NT_SUCCESS(Result))
        {
            Uri = Ep->_UriInfo->_FullUri;
            RemotePort = Ep->_UriInfo->_Port;
            return STATUS_SUCCESS;
        }
    }
    return Result;
}

//
// KNetServiceLayer::AddAddressEntry
//
NTSTATUS
KNetServiceLayer::AddAddressEntry(
    __in KNetUriInfo::SPtr& Inf
    )
{
    NTSTATUS Result = STATUS_SUCCESS;
    EpEntry::SPtr Tmp = _new(KTL_TAG_NET, GetThisAllocator()) EpEntry;
    if (!Tmp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Tmp->_UriInfo = Inf;

    K_LOCK_BLOCK(_TableLock)
    {
       Result = _AddressTableByFullUri.Put(Inf->_FullUri, Tmp);
    }
    if (!NT_SUCCESS(Result))
    {
        return Result;
    }
    K_LOCK_BLOCK(_TableLock)
    {
        Result = _AddressTableByGuid.Put(Inf->_GUID, Tmp);
    }
    return Result;
}



//
//  KNetServiceLayer::CreateAcquireSocketOp
//
NTSTATUS
KNetServiceLayer::CreateAcquireSocketOp(
    __out KNetAcquireSocketOp::SPtr& NewOp
    )
{
    KNetAcquireSocketOp* Tmp = _new(KTL_TAG_NET, GetThisAllocator()) KNetAcquireSocketOp(this);
    if (!Tmp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = Tmp;
    return STATUS_SUCCESS;
}

//
//  KNetAcquireSocketOp::StartAcquire
//
NTSTATUS
KNetAcquireSocketOp::StartAcquire(
    __in KNetUriInfo::SPtr Dest,
    __in USHORT TargetPort,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    _Dest = Dest;
    _TargetPort = TargetPort;
    _StartTime = KNt::GetSystemTime();
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

//
// KNetAcquireSocketOp::~KNetAcquireSocketOp
//
KNetAcquireSocketOp::~KNetAcquireSocketOp()
{
    // TBD
}

//
//  KNetAcquireSocketOp::LockCallback
//
VOID
KNetAcquireSocketOp::LockCallback(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    NTSTATUS Result;

    UNREFERENCED_PARAMETER(Parent);    
    UNREFERENCED_PARAMETER(Op);                    

    // Once here, we own the SocketEntry.
    // Now we have to decide whether to initiate a connection
    // or if there is a connection that we can use already cached.
    //

    _Socket =_SockEnt->_Socket;
    if (_Socket && _Socket->Ok())
    {
        _LockHandle->ReleaseLock();
        Complete(STATUS_SUCCESS);
        return;
    }

    // If here, we have to connect/reconnect the socket.
    //
    K_LOCK_BLOCK(_SvcLayer->_TableLock)
    {
        _Socket = nullptr;
        _SockEnt->_Address = nullptr;           // Wipe the old address
        _SockEnt->_Socket = nullptr;            // Wipe the old socket  BUGBUG: Check that destructor invokes Close if not closed
        _SockEnt->_Ready = FALSE;
    }

    // Now create & resolve the address.
    //
    IBindAddressOp::SPtr BindOp;

    Result = _SvcLayer->_Transport->CreateBindAddressOp(BindOp);
    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);

        return;
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetAcquireSocketOp::OnAddressComplete);

    Result = BindOp->StartBind(_SockEnt->_Server, _TargetPort, Cb);
    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);
        return;
    }
}


//
//  KNetAcquireSocketOp::OnAddressComplete
//
VOID
KNetAcquireSocketOp::OnAddressComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    NTSTATUS Result;
    IBindAddressOp& Bind = (IBindAddressOp &) Op;
    Result = Bind.Status();
    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);
        return;
    }

    _SockEnt->_Address = Bind.GetAddress();

    // Now try to connect a socket to this address
    //
    ISocketConnectOp::SPtr ConnectOp;

    Result = _SvcLayer->_Transport->CreateConnectOp(ConnectOp);
    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetAcquireSocketOp::OnSocketComplete);

    Result = ConnectOp->StartConnect(_SvcLayer->_LocalClientAddress, _SockEnt->_Address, Cb);
    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);
        return;
    }
}


//
//  KNetAcquireSocketOp::OnSocketComplete
//
VOID
KNetAcquireSocketOp::OnSocketComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    ISocketConnectOp& ConnectOp = (ISocketConnectOp&) Op;
    NTSTATUS Result = ConnectOp.Status();

    // BUGBUG:  Make this a performance trace operation.
    KDbgCheckpointWData(
        0,
        "Connection completed.",
        Result,
        KNt::GetSystemTime() - _StartTime,
        (ULONGLONG ) _SockEnt.RawPtr(),
        0,
        0
        );

    if (!NT_SUCCESS(Result))
    {
        _LockHandle->ReleaseLock();
        _SockEnt->_Ready = TRUE;
        Complete(Result);
        return;
    }

    // If here, we have the socket.
    //
    _SockEnt->_Socket = ConnectOp.GetSocket();
    _Socket = ConnectOp.GetSocket();
    _SockEnt->_Ready = TRUE;
    _LockHandle->ReleaseLock();

    Complete(Result);
}



//
//  KNetAcquireSocketOp::OnStart
//
VOID
KNetAcquireSocketOp::OnStart()
{
    NTSTATUS Result;

    // Find the socket entry, if there is one.
    // If not, we are the first one trying to connect/reconnect
    // so we have to lock things up and establish ourselves as 'owner'.

    K_LOCK_BLOCK(_SvcLayer->_TableLock)
    {
        Result = _SvcLayer->_SocketTable.Get(_Dest->_Authority, _SockEnt);

        if (NT_SUCCESS(Result))
        {
            // If the cached entry is null or the socket is closed,
            // remove it from the table and acquire a new one.
            //
            if (_SockEnt->_Ready && (!_SockEnt->_Socket || !_SockEnt->_Socket->Ok()))
            {
                _SvcLayer->_SocketTable.Remove(_Dest->_Authority);
                Result = STATUS_NOT_FOUND;
            }
        }

        if (Result == STATUS_NOT_FOUND)
        {
            _SockEnt = _new(KTL_TAG_NET, GetThisAllocator()) KNetServiceLayer::SocketEntry;
            if (!_SockEnt)
            {
                KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
                Complete(STATUS_INSUFFICIENT_RESOURCES);
                return;
            }

            Result = KSharedAsyncLock::Create(KTL_TAG_NET, GetThisAllocator(), _SockEnt->_Lock);

            if (!NT_SUCCESS(Result))
            {
                Complete(Result);
                return;
            }

            // While the destination is indentied and indexed under full authority,
            // the connection per se just needs the host name.
            //
            _SockEnt->_Server = _Dest->_NetEp->GetUri()->Get(KUriView::eHost);

            // Add it to the table
            //
            Result = _SvcLayer->_SocketTable.Put(_Dest->_Authority, _SockEnt);
            if (!NT_SUCCESS(Result))
            {
                Complete(Result);
                return;
            }
        }

        if (!NT_SUCCESS(Result))
        {
            Complete(Result);
            return;
        }
    }

    //  Now get a socket handle for this entry
    //
    //  This may occur simultaneous with some other request to open a socket,
    //  but it doesn't really matter who wins the right to create the socket.
    //
    Result = _SockEnt->_Lock->CreateHandle(KTL_TAG_NET, _LockHandle);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetAcquireSocketOp::LockCallback);

    _LockHandle->StartAcquireLock(nullptr, Cb);
}

//
//  KNetServiceLayer::BuildLookupTables
//
NTSTATUS
KNetServiceLayer::BuildLookupTables(
    __in KArray<KNetUriInfo::SPtr>& Src

    )
{
    NTSTATUS Result;

    for (ULONG i = 0; i < Src.Count(); i++)
    {
        Result = AddAddressEntry(Src[i]);
        if (!NT_SUCCESS(Result))
        {
            return Result;
        }
    }
    return STATUS_SUCCESS;
}

//
// KNetServiceLayer::LoadExternalAddressFile
//
NTSTATUS
KNetServiceLayer::LoadExternalAddressFile()
{
    KNetHostFile HostFile(GetThisAllocator());
    KWString Path(GetThisAllocator(), L"SystemRoot\\Rvd\\hosts.txt");
    ULONG ErrorLine = 0;

    NTSTATUS Result = HostFile.Parse(Path, ErrorLine);
    if (!NT_SUCCESS(Result) && Result != STATUS_OBJECT_NAME_NOT_FOUND)
    {
        KDbgPrintf("Could not parse host file.  Error on line %u\n", ErrorLine);
        return Result;
    }

    Result = BuildLookupTables(HostFile._Output);

    return Result;
}

KNetServiceLayer::PublishedEpEntry::PublishedEpEntry()
{
    // Nothing.
}

KNetServiceLayer::PublishedEpEntry::~PublishedEpEntry()
{
    // Nothing.
}

KNetServiceLayer::SocketEntry::SocketEntry()
    :   _Server(GetThisAllocator())
{
    _Port = 0;
    _Inbound = FALSE;
    _Outbound = FALSE;
    _Ready = FALSE;
}

KNetServiceLayer::SocketEntry::~SocketEntry()
{
    // Nothing.
}

KNetServiceLayer::EpEntry::EpEntry()
{
    // Nothing.
}

KNetServiceLayer::EpEntry::~EpEntry()
{
    // Nothing.
}

/////////////////////////////////////////////////////////////////////////////////////////////

//
// KNetHeadReadOp::Create
//
NTSTATUS
KNetHeaderReadOp::Create(
    __in  KAllocator& Allocator,
    __out KNetHeaderReadOp::SPtr& NewOp
    )
{
    KNetHeaderReadOp* ReadOp = _new(KTL_TAG_NET, Allocator) KNetHeaderReadOp;
    if (!ReadOp)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES, nullptr, 0, 0 );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    NewOp = ReadOp;
    return STATUS_SUCCESS;
}

KNetHeaderReadOp::KNetHeaderReadOp(
    )
{
    SetConstructorStatus(Initialize());
}

NTSTATUS
KNetHeaderReadOp::Initialize(
    )
{
    NTSTATUS status;

    _ActId = 0;
    _UriBuffer = 0;
    _SystemMessage.Buffer = 0;
    _StripSizeRequired = 0;

    status = KNetUriInfo::Create(_DestUriInf, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KNetUriInfo::Create(_SrcUriInf, GetThisAllocator());

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

//
//  KNetHeaderReadOp::~KNetHeaderReadOp
//
KNetHeaderReadOp::~KNetHeaderReadOp()
{
    if (_UriBuffer)
    {
        _delete(_UriBuffer);
    }

    if (_SystemMessage.Buffer)
    {
        _delete(_SystemMessage.Buffer);
    }
}

//
//  KNetHeadReadOp::StartRead
//
NTSTATUS
KNetHeaderReadOp::StartRead(
    __in ISocket::SPtr& Socket,
    __in KNetServiceLayer::SPtr SvcLayer,
    __in_opt   KAsyncContextBase::CompletionCallback Callback,
    __in_opt   KAsyncContextBase* const ParentContext
    )
{
    _SvcLayer = SvcLayer;
    _Socket = Socket;
    Start(ParentContext, Callback);
    return STATUS_SUCCESS;
}




VOID
KNetHeaderReadOp::OnStart()
{
    NTSTATUS Result;
    ISocketReadOp::SPtr ReadOp;

    Result = _Socket->CreateReadOp(ReadOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
    KMemRef MemRef;
    MemRef._Address = &_Header;
    MemRef._Size = sizeof(KNetworkHeader);

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetHeaderReadOp::Phase1ReadComplete);

    Result = ReadOp->StartRead(1, &MemRef, ISocketReadOp::ReadAll, Cb);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
}

VOID
KNetHeaderReadOp::Phase1ReadComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    ISocketReadOp& ReadOp = (ISocketReadOp&) Op;
    NTSTATUS Result = ReadOp.Status();
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("Failure to read the message header. Status=0x%x\n", ReadOp.Status());
        Complete(Result);
        return;
    }

    ULONG BytesTransferred = ReadOp.BytesTransferred();
    if (BytesTransferred != sizeof(KNetworkHeader))
    {
        KDbgPrintf("Header Read Failure. Insufficient bytes read. Bytes read=%u CLOSING socket 0x%X\n", ReadOp.BytesTransferred(), _Socket->GetSocketId());
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        Complete(STATUS_FILE_CLOSED);
        return;
    }

    // Verify the signature.
    //
    if ((_Header._Control >> 48 << 48) != KNet_Message_Signature)
    {
        KDbgPrintf("Header Signature check failure.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        Complete(STATUS_INVALID_IMAGE_FORMAT);
        _Socket->Close();
        return;
    }

    // Now we have the header read. Next
    // Now read the URI values
    //
    ISocketReadOp::SPtr NextReadOp;
    Result = _Socket->CreateReadOp(NextReadOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }
	
    ULONG Sz;
	HRESULT hr;
	hr = ULongAdd(_Header._DestUriLength, _Header._SrcUriLength, &Sz);
	KInvariant(SUCCEEDED(hr));

    _UriBuffer = _newArray<UCHAR>(KTL_TAG_NET, GetThisAllocator(), Sz);
    if (!_UriBuffer)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, Sz, 0 );
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }

    KMemRef MemRef;
    MemRef._Address = _UriBuffer;
    MemRef._Size = Sz;

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetHeaderReadOp::Phase2ReadComplete);

    Result = NextReadOp->StartRead(1, &MemRef, ISocketReadOp::ReadAll, Cb);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
       _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }
}

//
//  KNetHeaderReadOp::Phase2ReadComplete
//
//  This is called once we have completed reading the
//  URI values.  We now know enough to look up the
//  owning network and to start a KDatagramReadOp.
//
//
VOID
KNetHeaderReadOp::Phase2ReadComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    ISocketReadOp& ReadOp = (ISocketReadOp&) Op;
    NTSTATUS Result = ReadOp.Status();
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("Header URI block read failure.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        Complete(Result);
        return;
    }

    // If here, we now have a enough to dispatch to KNetwork for
    // a read of the object.
    //
    // First split the URIs out of the _UriBuffer into
    // separate 'real' KWString variables.

    UNICODE_STRING SrcUri;
    UNICODE_STRING DestUri;

    DestUri.Buffer = PWCH(_UriBuffer);
    DestUri.Length = _Header._DestUriLength;
    DestUri.MaximumLength = _Header._DestUriLength;

    KWString DestTmp(GetThisAllocator(), DestUri);

    SrcUri.Buffer = PWCH(_UriBuffer + _Header._DestUriLength);
    SrcUri.Length = _Header._SrcUriLength;
    SrcUri.MaximumLength = _Header._SrcUriLength;

    KWString SrcTmp(GetThisAllocator(), SrcUri);


    Result = _DestUriInf->Parse(DestTmp );
    if (!NT_SUCCESS(Result))
    {
        KDbgCheckpoint(3, "Destination URI is bad");
        KDbgPrintf("URI parse failure.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        Complete(Result);
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }

    Result = _SrcUriInf->Parse(SrcTmp);
    if (!NT_SUCCESS(Result))
    {
        KDbgCheckpoint(4, "Source URI is bad");
        KDbgPrintf("URI parse failure.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        Complete(Result);
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }

    if (!(_DestUriInf->_NetEp->GetUri()->Has(KUriView::eHost | KUriView::ePort)))
    {
        KDbgPrintf("Destination URI in message is missing port!\n");
        KFatal(FALSE);
    }
    if (!(_SrcUriInf->_NetEp->GetUri()->Has(KUriView::eHost | KUriView::ePort)))
    {
        KDbgPrintf("Source URI in message is missing port!\n");
        KFatal(FALSE);
    }

    // If this is a system message, branch and deal with it elsewhere.
    //
    if (_Header._Control & KNet_ContentType_SystemMesasge)
    {
        KDbgCheckpoint(4, "A system message has been received.");
        HandleSystemMessage();
        return;
    }

    //
    // Now look up the source URI. If it is not in the table, add it.
    //
    //
    // We reorder to test the source URI first, since we are going to accept the message
    // in any case.
    //
    //
    // We reorder to test the source URI first, since we are going to accept the message
    // in any case.
    //

    KNetServiceLayer::EpEntry::SPtr SrcEpEntry;

    K_LOCK_BLOCK(_SvcLayer->_TableLock)
    {
        Result = _SvcLayer->_AddressTableByFullUri.Get(_SrcUriInf->_FullUri, SrcEpEntry);
    }

    if (!NT_SUCCESS(Result))
    {
        // Make up a GUID for it
        KDbgCheckpoint(7, "Sending Uri was not known. Auto-generating a GUID for the sending endpoint.");

        _SvcLayer->NextSequentialGuid(_SrcUriInf->_GUID);
        Result = _SvcLayer->AddAddressEntry(_SrcUriInf);
        if (!NT_SUCCESS(Result))
        {
            _Socket->Close();
            Complete(Result);
            return;
        }
        // Get it back to get the SrcEpEntry
        //
        Result = _SvcLayer->_AddressTableByFullUri.Get(_SrcUriInf->_FullUri, SrcEpEntry);
        if (!NT_SUCCESS(Result))
        {
            _Socket->Close();
            Complete(Result);
            return;
        }
    }

    // Find the owning KNetwork based on the relative URI of the incoming message.
    //
    KNetServiceLayer::PublishedEpEntry::SPtr DestEp;
    K_LOCK_BLOCK(_SvcLayer->_TableLock)
    {
        Result = _SvcLayer->_PublishedEpTable.Get(_DestUriInf->_RelUri, DestEp);
    }

    if (!NT_SUCCESS(Result))
    {
#if defined(PLATFORM_UNIX)
        KDbgPrintf("Unable to find a KNetwork to receive destination URI=%s [SourceUri=%s]\n", Utf16To8(PWSTR(_DestUriInf->_FullUri)).c_str(), Utf16To8(PWSTR(_SrcUriInf->_FullUri)).c_str());
#else
        KDbgPrintf("Unable to find a KNetwork to receive destination URI=%S [SourceUri=%S]\n", PWSTR(_DestUriInf->_FullUri), PWSTR(_SrcUriInf->_FullUri));
#endif
        StripMessage();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }


    // Now let's get a binding based on the GUIDs and attach a socket to it.
    //
    GUID& SrcGuid = SrcEpEntry->_UriInfo->_GUID;
    GUID& DestGuid = DestEp->_UriInfo->_GUID;

    KNetBinding::SPtr Binding;
    Result = DestEp->_OwningNetwork->GetBinding(SrcGuid, DestGuid, 0, Binding);
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("INTERNAL ERROR: Unable to get a binding to receive message.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        Complete(Result);
        return;
    }

    // Attach the socket & remote port to the binding
    //
    Binding->SetReadSocket(_Socket);

    // Create an KDatagramReadOp
    //
    KDatagramReadOp::SPtr DGramReadOp;
    DestEp->_OwningNetwork->CreateDatagramReadOp(DGramReadOp);
    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("Unable to create a KDatagramReadOp.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        Complete(Result);
        return;
    }

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetHeaderReadOp::DatagramReadComplete);

    DGramReadOp->StartRead(
        _Header,
        Binding,
        Cb
        );

    if (!NT_SUCCESS(Result))
    {
        KDbgPrintf("  **** INTERNAL ERROR: Unable to start reading datagram.  CLOSING socket 0x%X\n", _Socket->GetSocketId());
        _Socket->Close();
        Complete(Result);
        InterlockedIncrement(&__KNetwork_DroppedInboundMessages);
        return;
    }
}


VOID
KNetHeaderReadOp::DatagramReadComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    KDatagramReadOp& ReadOp = (KDatagramReadOp&) Op;
    Complete(ReadOp.Status());
}



// KNetHeaderReadOp::StripMessage
//
// This is invoked when we have to strip out a message because there is no receiver
// endpoint for it.  We want to keep the socket open, so we just do a dummy read
// and resynchronize the socket for another read.
//
VOID
KNetHeaderReadOp::StripMessage()
{
    // Read the rest of the message into a dummy buffer.

    InterlockedIncrement(&__KNetwork_DroppedInboundMessages);

    // Figure out how much space we need and allocate a buffer to
    // receive it.
	HRESULT hr;
	hr = ULongAdd(_Header._MetaLength, _Header._DataLength, &_StripSizeRequired);
	KInvariant(SUCCEEDED(hr));
	
	hr = ULongAdd(_StripSizeRequired, _Header._DeferredLength, &_StripSizeRequired);
	KInvariant(SUCCEEDED(hr));	

    KAssert(_StripSizeRequired > 0);

    NTSTATUS Result = KBuffer::Create(_StripSizeRequired, _StripBuffer, GetThisAllocator());
    if (!NT_SUCCESS(Result))
    {
        _Socket->Close();
        Complete(Result);
        return;
    }

    // Post a raw socket read op for the total
    //
    ISocketReadOp::SPtr ReadOp;
    Result = _Socket->CreateReadOp(ReadOp);

    if (!NT_SUCCESS(Result))
    {
        _Socket->Close();
        Complete(Result);
        return;
    }

    KMemRef MemRef;
    MemRef._Address = _StripBuffer->GetBuffer();
    MemRef._Size    = _StripSizeRequired;
    MemRef._Param   = _StripSizeRequired;

    Result = ReadOp->StartRead(1, &MemRef, ISocketReadOp::ReadAll, KAsyncContextBase::CompletionCallback(this, &KNetHeaderReadOp::StripReadComplete));
    if (!NT_SUCCESS(Result))
    {
        _Socket->Close();
        Complete(Result);
        return;
    }
}

// KNetHeaderReadop::StripReadComplete
//
// Called when we have completed stripping an undeliverable message.
//
VOID
KNetHeaderReadOp::StripReadComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    ISocketReadOp& Read = (ISocketReadOp&) Op;

    // Double check that it succeeded. It may have failed
    // because the sender closed the socket, so we don't
    // worry too much about it not succeeding.
    //w
    if (!NT_SUCCESS(Op.Status()))
    {
        _Socket->Close();
        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    ULONG Res = Read.BytesTransferred();
    if (Res != _StripSizeRequired)
    {
        _Socket->Close();
        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    // Finish up this and start over
    // listening to the incoming socket again.
    //
    Complete(STATUS_SUCCESS);
}




VOID
KNetHeaderReadOp::SystemMessageWriteComplete(
    KAsyncContextBase* const Parent,
    KAsyncContextBase& Op
    )
{
    UNREFERENCED_PARAMETER(Parent);    
    
    ISocketWriteOp& Write = (ISocketWriteOp &) Op;
    NTSTATUS Result = Write.Status();
    Complete(Result);
}

VOID
KNetHeaderReadOp::HandleSystemMessage()
{
    NTSTATUS Result;
    ISocketWriteOp::SPtr WriteOp;
    Result = _Socket->CreateWriteOp(WriteOp);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }

	static const ULONG systemMessageLength = 0x4000;
    _SystemMessage.MaximumLength = systemMessageLength;
    _SystemMessage.Length = systemMessageLength;
    _SystemMessage.Buffer = PWCH(_newArray<UCHAR>(KTL_TAG_NET, GetThisAllocator(), systemMessageLength));

    if (!_SystemMessage.Buffer)
    {
        KTraceOutOfMemory(0, STATUS_INSUFFICIENT_RESOURCES,this, 0, 0 );
        Complete(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }
    RtlUnicodeStringPrintf(&_SystemMessage,
      L"RVD System Message Response: Version=%d\r\n OpNumber=%u Writes=%I64u Reads=%I64u Connects=%u Accepts=%u \r\n ReadFailures=%u WriteFailures=%u ConnectFailures=%u\r\n"
      L"BytesRead=%I64u (0x%I64x) BytesWritten=%I64u (0x%I64x)\r\nPendingReads=%d PendingWrites=%u PendingAccepts=%u\r\n Shutdown=%d ShutdownPhase=%d\r\n LastWriteError=0x%x\r\n",
       __KSocket_Version,
       __KSocket_OpNumber,
       __KSocket_Writes,
       __KSocket_Reads,
       __KSocket_Connects,
       __KSocket_Accepts,
       __KSocket_ReadFailures,
       __KSocket_WriteFailures,
       __KSocket_ConnectFailures,
       __KSocket_BytesRead,
       __KSocket_BytesRead,
       __KSocket_BytesWritten,
       __KSocket_BytesWritten,

       __KSocket_PendingReads,
       __KSocket_PendingWrites,
       __KSocket_PendingAccepts,

       __KSocket_Shutdown,
       __KSocket_ShutdownPhase,
       __KSocket_LastWriteError
       );

    KMemRef MemRef;
    MemRef._Address = _SystemMessage.Buffer;
    MemRef._Param = _SystemMessage.Length;

    KAsyncContextBase::CompletionCallback Cb;
    Cb.Bind(this, &KNetHeaderReadOp::SystemMessageWriteComplete);

    Result = WriteOp->StartWrite(1, &MemRef, 0, Cb);
    if (!NT_SUCCESS(Result))
    {
        Complete(Result);
        return;
    }
}


VOID
KNetServiceLayer::DumpAddressTableToDebugger()
{
    NTSTATUS Res;
    KDbgPrintf("**** DUMPING ADDRESS TABLE ***\n");

    _AddressTableByFullUri.Reset();

    for (;;)
    {
        EpEntry::SPtr Entry;
        KWString UriStr(GetThisAllocator());

        Res = _AddressTableByFullUri.Next(UriStr, Entry);
        if (Res == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

#if defined(PLATFORM_UNIX)
        KDbgPrintf("Entry Key=<%s>\n", Utf16To8(PWSTR(UriStr)).c_str());
        KDbgPrintf("    _FullUri      = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_FullUri)).c_str());
        KDbgPrintf("    _Authority    = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_Authority)).c_str());
        KDbgPrintf("    _RelUri       = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_RelUri)).c_str());
#else
        KDbgPrintf("Entry Key=<%S>\n", PWSTR(UriStr));
        KDbgPrintf("    _FullUri      = <%S>\n", PWSTR(Entry->_UriInfo->_FullUri));
        KDbgPrintf("    _Authority    = <%S>\n", PWSTR(Entry->_UriInfo->_Authority));
        KDbgPrintf("    _RelUri       = <%S>\n", PWSTR(Entry->_UriInfo->_RelUri));
#endif
        KDbgPrintf("    _Port         = %u\n", Entry->_UriInfo->_Port);
        KDbgPrintf("\n");
    }

    KDbgPrintf("**** END DUMP ****\n");
}

VOID
KNetServiceLayer::DumpEpTableToDebugger()
{
    KDbgPrintf("**** DUMPING EP TABLE ***\n");

    _PublishedEpTable.Reset();

    for (;;)
    {
        KWString RelUri(GetThisAllocator());
        PublishedEpEntry::SPtr Entry;

        NTSTATUS Res = _PublishedEpTable.Next(RelUri, Entry);
        if (Res == STATUS_NO_MORE_ENTRIES)
        {
            break;
        }

#if defined(PLATFORM_UNIX)
        KDbgPrintf("Entry Key=<%s>\n", Utf16To8(PWSTR(RelUri)).c_str());
        KDbgPrintf("    _FullUri      = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_FullUri)).c_str());
        KDbgPrintf("    _Authority    = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_Authority)).c_str());
        KDbgPrintf("    _RelUri       = <%s>\n", Utf16To8(PWSTR(Entry->_UriInfo->_RelUri)).c_str());
#else
        KDbgPrintf("Entry Key=<%S>\n", PWSTR(RelUri));
        KDbgPrintf("    _FullUri      = <%S>\n", PWSTR(Entry->_UriInfo->_FullUri));
        KDbgPrintf("    _Authority    = <%S>\n", PWSTR(Entry->_UriInfo->_Authority));
        KDbgPrintf("    _RelUri       = <%S>\n", PWSTR(Entry->_UriInfo->_RelUri));
#endif
        KDbgPrintf("    _Port         = %u\n", Entry->_UriInfo->_Port);
        KDbgPrintf("\n");
    }

    KDbgPrintf("**** END DUMP ****\n");
}
