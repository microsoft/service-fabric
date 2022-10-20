/*++

    (c) 2011 by Microsoft Corp. All Rights Reserved.

    ksspi.cpp

    Description:
      Kernel Tempate Library (KTL): KSSPI

      Implements a general purpose SSPI wrapper for
      network security purposes.

    History:
      raymcc          2-May-2011         Initial draft
      leikong           Oct-2011.........Adding implementation

--*/

#include <ktl.h>
#include <ktrace.h>

#if KTL_USER_MODE
 #include <ws2tcpip.h>
 #include <ws2ipdef.h>
 #include <iphlpapi.h>
#else
 #include <netioapi.h>
 #include <windef.h>
 #include <wbasek.h>
 #include <winerror.h>
 #include <spseal.h>
#endif

#include <schannel.h>

#pragma region KSspiTransport

KSspiTransport::KSspiTransport(
    ITransport::SPtr const & innerTransport,
    KSspiCredential::SPtr const & clientCredential,
    KSspiCredential::SPtr const & serverCredential,
    AuthorizationContext::SPtr const & clientAuthContext,
    AuthorizationContext::SPtr const & serverAuthContext)
    : _innerTransport(innerTransport)
    , _clientCredential(clientCredential)
    , _serverCredential(serverCredential)
    , _clientContextRequirement(0)
    , _serverContextRequirement(0)
    , _sspiMaxTokenSize(0)
    , _clientAuthContext(clientAuthContext)
    , _serverAuthContext(serverAuthContext)
    , _negotiationTimeoutInMs(30000)
    , _maxPendingInboundNegotiation(256)
{
}

_Use_decl_annotations_
NTSTATUS KSspiTransport::CreateBindAddressOp(IBindAddressOp::SPtr& bindOp)
{
    return _innerTransport->CreateBindAddressOp(bindOp);
}

_Use_decl_annotations_
NTSTATUS KSspiTransport::CreateNetAddress(SOCKADDR_STORAGE const * address, INetAddress::SPtr & netAddress)
{
    return _innerTransport->CreateNetAddress(address, netAddress);
}

void KSspiTransport::Shutdown()
{
    _innerTransport->Shutdown();
}

_Use_decl_annotations_
NTSTATUS KSspiTransport::SpawnDefaultClientAddress(INetAddress::SPtr& address)
{
    return _innerTransport->SpawnDefaultClientAddress(address);
}

_Use_decl_annotations_
NTSTATUS KSspiTransport::SpawnDefaultListenerAddresses(KArray<INetAddress::SPtr>& addresses)
{
    return _innerTransport->SpawnDefaultListenerAddresses(addresses);
}

ITransport::SPtr const & KSspiTransport::InnerTransport() const
{
    return _innerTransport;
}

KSspiTransport::AuthorizationContext* KSspiTransport::ClientAuthContext()
{
    return _clientAuthContext.RawPtr();
}

KSspiTransport::AuthorizationContext* KSspiTransport::ServerAuthContext()
{
    return _serverAuthContext.RawPtr();
}

KSspiCredential::SPtr KSspiTransport::ClientCredential() const
{
    _thisLock.Acquire();
    auto result = _clientCredential;
    _thisLock.Release();

    return result;
}

KSspiCredential::SPtr KSspiTransport::ServerCredential() const
{
    _thisLock.Acquire();
    auto result = _serverCredential;
    _thisLock.Release();

    return result;
}

void KSspiTransport::SetCredentials(
    KSspiCredential::SPtr && clientCredential,
    KSspiCredential::SPtr && serverCredential)
{
    KSspiCredential::SPtr clientCred;
    KSspiCredential::SPtr serverCred;

    _thisLock.Acquire();
    // Move the old credential to local variable
    // to ensure it is not destructed under the spinlock
    clientCred = Ktl::Move(_clientCredential);
    serverCred = Ktl::Move(_serverCredential);

    _clientCredential = Ktl::Move(clientCredential);
    _serverCredential = Ktl::Move(serverCredential);
    _thisLock.Release();
}

ULONG KSspiTransport::ClientContextRequirement() const
{
    return _clientContextRequirement;
}

ULONG KSspiTransport::ServerContextRequirement() const
{
    return _serverContextRequirement;
}

void KSspiTransport::SetClientContextRequirement(ULONG contextRequirement)
{
    _clientContextRequirement = contextRequirement;
}

void KSspiTransport::SetServerContextRequirement(ULONG contextRequirement)
{
    _serverContextRequirement = contextRequirement;
}

NTSTATUS KSspiTransport::CreateListener(
    __in  INetAddress::SPtr& address,
    __out ISocketListener::SPtr& listener)
{
    NTSTATUS status = _innerTransport->CreateListener(address, listener);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    listener = _new (KTL_TAG_SSPI, GetThisAllocator()) KSspiSocketListener(
        this,
        Ktl::Move(listener));
    if (!listener)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return listener->Status();
}

NTSTATUS KSspiTransport::CreateConnectOp(__out ISocketConnectOp::SPtr& connectOp)
{
    connectOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KSspiSocketConnectOp(this);
    if (!connectOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return connectOp->Status();
}

ULONG KSspiTransport::SspiMaxTokenSize()
{
    if (_sspiMaxTokenSize == 0)
    {
        PSecPkgInfo pSecPkgInfo;
        SECURITY_STATUS status = QuerySecurityPackageInfo(SspiPackageName(), &pSecPkgInfo);
        KFatal(status == SEC_E_OK);
        _sspiMaxTokenSize = pSecPkgInfo->cbMaxToken;
        FreeContextBuffer(pSecPkgInfo);
        KDbgPrintf("SSPI max token size: %u", _sspiMaxTokenSize);
    }

    return _sspiMaxTokenSize;
}

LONG KSspiTransport::MaxPendingInboundNegotiation() const
{
    return _maxPendingInboundNegotiation;
}

ULONG KSspiTransport::NegotiationTimeout() const
{
    return _negotiationTimeoutInMs;
}

void KSspiTransport::SetNegotiationTimeout(ULONG value)
{
    _negotiationTimeoutInMs = value;
}

KSspiTransport::AuthorizationContext::AuthorizationContext()
{
}

KSspiTransport::AuthorizationContext::~AuthorizationContext()
{
}

#pragma endregion

#pragma region KSspiSocket

KSspiSocket::KSspiSocket(
    KSspiTransport::SPtr && sspiTransport,
    ISocket::SPtr && innerSocket,
    KSspiContext::UPtr && sspiContext)
    : _sspiTransport(Ktl::Move(sspiTransport))
    , _innerSocket(Ktl::Move(innerSocket))
    , _sspiContext(Ktl::Move(sspiContext))
    , _userData(nullptr)
    , _shouldNegotiate(true)
    , _inboundNegotiationSlotHeld(false)
    , _sspiNegotiateOp(nullptr)
    , _securityStatus(_sspiContext->SecurityStatus())
    , _savedCleartextBuffer(nullptr)
{
    _traceId = _innerSocket->GetSocketId();
}

KSspiSocket::~KSspiSocket()
{
    ReleaseInboundNegotiationSlotIfNeeded();

    _delete(_readMemRef._Address);
    _readMemRef = KMemRef();

    _delete(_writeMemRef._Address);
    _writeMemRef = KMemRef();

    _delete(_savedCleartextBuffer);
    _savedCleartextMemRef = KMemRef();
}

NTSTATUS KSspiSocket::AcquireInboundNegotiationSlot(KSharedPtr<KSspiSocketListener> const & listener)
{
    KAssert(_sspiContext->InBound());
    _sspiListener = listener;
    NTSTATUS status = _sspiListener->AcquireNegotiationSlot();
    _inboundNegotiationSlotHeld = NT_SUCCESS(status);
    return status;
}

void KSspiSocket::ReleaseInboundNegotiationSlotIfNeeded()
{
    if (_inboundNegotiationSlotHeld)
    {
        _sspiListener->ReleaseNegotiationSlot();
        _inboundNegotiationSlotHeld = false;
    }
}

void KSspiSocket::SaveNegotiateOp(KSspiNegotiateOp const * negotiateOp)
{
    KAssert(_sspiNegotiateOp == nullptr);
    _sspiNegotiateOp = negotiateOp;
}

NTSTATUS KSspiSocket::OnNegotiationSucceeded()
{
    return STATUS_SUCCESS;
}

void KSspiSocket::Close()
{
    _innerSocket->Close();
}

NTSTATUS KSspiSocket::GetLocalAddress(__out INetAddress::SPtr& addr)
{
    return _innerSocket->GetLocalAddress(addr);
}

NTSTATUS KSspiSocket::GetRemoteAddress(__out INetAddress::SPtr& addr)
{
    return _innerSocket->GetRemoteAddress(addr);
}

ULONG KSspiSocket::PendingOps()
{
    return _innerSocket->PendingOps();
}

void KSspiSocket::SetUserData(__in PVOID Ptr)
{
    _userData = Ptr;
}

PVOID KSspiSocket::GetUserData()
{
    return _userData;
}

BOOLEAN KSspiSocket::Ok()
{
    return _innerSocket->Ok();
}

PVOID KSspiSocket::GetSocketId(void)
{
    return _innerSocket->GetSocketId();
}

BOOLEAN KSspiSocket::Lock()
{
    return _innerSocket->Lock();
}

void KSspiSocket::Unlock()
{
    return _innerSocket->Unlock();
}

ISocket & KSspiSocket::InnerSocket()
{
    return *_innerSocket;
}

KSspiContext & KSspiSocket::SspiContext()
{
    return *_sspiContext;
}

KSspiTransport & KSspiSocket::SspiTransport()
{
    return *_sspiTransport;
}

NTSTATUS KSspiSocket::SecurityStatus() const
{
    return _securityStatus;
}

bool KSspiSocket::ShouldNegotiate() const
{
    return _shouldNegotiate;
}

_Use_decl_annotations_
NTSTATUS KSspiSocket::StartNegotiate(
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    KSspiNegotiateOp::SPtr negotiateOp;
    negotiateOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KSspiNegotiateOp(this);
    if (!negotiateOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    if (!NT_SUCCESS(negotiateOp->Status()))
    {
        return negotiateOp->Status();
    }

    negotiateOp->Start(parentContext, callback);
    return STATUS_PENDING;
}

bool KSspiSocket::NegotiationSucceeded() const
{
    return _securityStatus == STATUS_SUCCESS;
}

NTSTATUS KSspiSocket::CompleteNegotiate(NTSTATUS status)
{
    _securityStatus = status;
    _shouldNegotiate = false;

    if (_securityStatus == STATUS_SUCCESS)
    {
        _securityStatus = SetupBuffers();
    }

    if (_securityStatus == STATUS_SUCCESS)
    {
        _securityStatus = OnNegotiationSucceeded();
    }

    _sspiContext->FreeBuffers();

    return _securityStatus;
}

_Use_decl_annotations_
NTSTATUS KSspiSocket::InitializeBuffer(KMemRef& memRef)
{
    memRef._Address = _new (KTL_TAG_SSPI, GetThisAllocator()) UCHAR[_messageBufferSize];
    if (!memRef._Address)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    memRef._Size = _messageBufferSize;
    memRef._Param = 0;

    return STATUS_SUCCESS;
}

ULONG KSspiSocket::GetMaxMessageSize()
{
    return _maxMessageSize;
}

KMemRef KSspiSocket::SavedCleartextMemRef()
{
    return _savedCleartextMemRef;
}

void KSspiSocket::ConsumeSavedCleartext(ULONG length)
{
    KFatal(_savedCleartextMemRef._Param >= length);

    _savedCleartextMemRef._Param -= length;
    if (_savedCleartextMemRef._Param > 0)
    {
        _savedCleartextMemRef._Address = (PUCHAR)_savedCleartextMemRef._Address + length;
    }
    else
    {
        _savedCleartextMemRef._Address = _savedCleartextBuffer;
    }
}

void KSspiSocket::SaveExtraCleartext(PVOID src, ULONG length)
{
    KFatal(_savedCleartextMemRef._Param == 0);
    KMemCpySafe(_savedCleartextMemRef._Address, _savedCleartextMemRef._Size, src, length);
    _savedCleartextMemRef._Param = length;
}

#pragma endregion

#pragma region KSspiSocketListener

KSspiSocketListener::KSspiSocketListener(
    KSspiTransport::SPtr && sspiTransport,
    ISocketListener::SPtr && socketListener)
    : _sspiTransport(Ktl::Move(sspiTransport))
    , _innerSocketListener(Ktl::Move(socketListener))
    , _userData(nullptr)
    , _maxPendingInboundNegotiation(_sspiTransport->MaxPendingInboundNegotiation())
    , _pendingInboundNegotiation(0)
{
}

void KSspiSocketListener::SetMaxPendingInboundNegotiation(LONG max)
{
    KAssert(_pendingInboundNegotiation == 0); // Should be set before there is any activity
    _maxPendingInboundNegotiation = max; // Non-postive value disables throttle
}

bool KSspiSocketListener::IsNegotiationThrottleEnabled() const
{
    return _maxPendingInboundNegotiation > 0;
}

NTSTATUS KSspiSocketListener::AcquireNegotiationSlot()
{
    if (!IsNegotiationThrottleEnabled())
    {
        return STATUS_SUCCESS;
    }

    KInStackSpinLock lockInThisScope(_lock);
    if (_pendingInboundNegotiation == _maxPendingInboundNegotiation)
    {
        return STATUS_INSUFF_SERVER_RESOURCES;
    }

    ++ _pendingInboundNegotiation;
    return STATUS_SUCCESS;
}

void KSspiSocketListener::ReleaseNegotiationSlot()
{
    if (IsNegotiationThrottleEnabled())
    {
        KInStackSpinLock lockInThisScope(_lock);
        -- _pendingInboundNegotiation;
    }
}

void KSspiSocketListener::Shutdown()
{
    _innerSocketListener->Shutdown();
}

NTSTATUS KSspiSocketListener::CreateListenOp(__out ISocketListenOp::SPtr& sspiListenOp)
{
    return _innerSocketListener->CreateListenOp(sspiListenOp);
}

NTSTATUS KSspiSocketListener::CreateAcceptOp(__out ISocketAcceptOp::SPtr& sspiAcceptOp)
{
    sspiAcceptOp = _new (KTL_TAG_SSPI, GetThisAllocator()) KSspiSocketAcceptOp(this);
    if (!sspiAcceptOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return sspiAcceptOp->Status();
}

#pragma endregion

#pragma region KSspiAuthorizationOp

_Use_decl_annotations_
KSspiAuthorizationOp::KSspiAuthorizationOp(
    KSspiTransport::AuthorizationContext * authorizationContext,
    PCtxtHandle sspiContext)
    : _authorizationContext(authorizationContext), _sspiContext(sspiContext)
{
}

_Use_decl_annotations_
NTSTATUS KSspiAuthorizationOp::StartAuthorize(
    KAllocator & alloc,
    KSspiTransport::AuthorizationContext * authorizationContext,
    PCtxtHandle sspiContext,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    KSspiAuthorizationOp::SPtr newOp = _new (KTL_TAG_SSPI, alloc) KSspiAuthorizationOp(
        authorizationContext,
        sspiContext);

    if (!newOp)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    return newOp->StartAuthorize(
        callback,
        parentContext);
}

NTSTATUS KSspiAuthorizationOp::StartAuthorize(
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    KDbgPrintf("start authorization op %llx ", this);
    Start(parentContext, callback);
    return STATUS_PENDING;
}

void KSspiAuthorizationOp::OnStart()
{
    if (_authorizationContext == nullptr)
    {
        Complete(STATUS_SUCCESS);
        return;
    }

    NTSTATUS status = _authorizationContext->Authorize(_sspiContext, *this);
    if (status != STATUS_PENDING)
    {
        Complete(status);
    }
}

void KSspiAuthorizationOp::CompleteAuthorize(NTSTATUS status)
{
    Complete(status);
}

void KSspiAuthorizationOp::OnCompleted()
{
    KDbgPrintf("authorization op %llx completed: %u", this, Status());
}

#pragma endregion

#pragma region KSspiSocketConnectOp

KSspiSocketConnectOp::KSspiSocketConnectOp(KSspiTransport::SPtr && sspiTransport)
    : _sspiTransport(Ktl::Move(sspiTransport))
    , _userData(nullptr)
{
    _traceId = this;
    SetConstructorStatus(_sspiTransport->InnerTransport()->CreateConnectOp(_innerConnectOp));
}

void KSspiSocketConnectOp::OnReuse()
{
    _sspiSocket.Reset();
    _innerConnectOp->Reuse();
}

NTSTATUS KSspiSocketConnectOp::StartConnect(
    __in      INetAddress::SPtr& connectFrom,
    __in      INetAddress::SPtr& connectTo,
    __in_opt  KAsyncContextBase::CompletionCallback callback,
    __in_opt  KAsyncContextBase* const parentContext)
{
    _connectFrom = connectFrom;
    _connectTo = connectTo;
    Start(parentContext, callback);
    return STATUS_PENDING;
}

void KSspiSocketConnectOp::OnStart()
{
    NTSTATUS status = _innerConnectOp->StartConnect(
        _connectFrom,
        _connectTo,
        CompletionCallback(this, &KSspiSocketConnectOp::InnerConnectCallback),
        this);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
    }
}

void KSspiSocketConnectOp::InnerConnectCallback(
    __in_opt KAsyncContextBase* const parent,
    __in KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);

    ISocketConnectOp& innerConnectOp = static_cast<ISocketConnectOp&>(completingOperation);
    NTSTATUS status = innerConnectOp.Status();
    if (status != STATUS_SUCCESS)
    {
        Complete(status);
        return;
    }

    KSspiContext::UPtr sspiContext;
    status = _sspiTransport->CreateSspiClientContext(
        static_cast<KSspiAddress*>(_connectTo.RawPtr())->SspiTarget(),
        sspiContext);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    ISocket::SPtr framingSocket;
    status = _sspiTransport->CreateFramingSocket(innerConnectOp.GetSocket(), framingSocket);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    status = _sspiTransport->CreateSspiSocket(
        Ktl::Move(framingSocket),
        Ktl::Move(sspiContext),
        _sspiSocket);
    Complete(status);
}

#pragma endregion

#pragma region KSspiNegotiateOp

KSspiNegotiateOp::KSspiNegotiateOp(KSspiSocket::SPtr && sspiSocket)
    : _sspiSocket(Ktl::Move(sspiSocket))
    , _shouldUnlock(false)
    , _startedNegotiate(false)
{
    _traceId = _sspiSocket->InnerSocket().GetSocketId();
    NTSTATUS status = _sspiSocket->InnerSocket().CreateWriteOp(_sendOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    status = _sspiSocket->InnerSocket().CreateReadOp(_receiveOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    SetConstructorStatus(KTimer::Create(_timer, GetThisAllocator(), KTL_TAG_SSPI));
}

KSspiNegotiateOp::~KSspiNegotiateOp()
{
}

void KSspiNegotiateOp::OnStart()
{
    AcquireLock(
        _sspiSocket->_negotiateLock,
        LockAcquiredCallback(this, &KSspiNegotiateOp::OnLockAcquired));
}

_Use_decl_annotations_
void KSspiNegotiateOp::OnLockAcquired(BOOLEAN isAcquired, KAsyncLock&)
{
    if (!isAcquired)
    {
        CompleteNegotiate(STATUS_UNSUCCESSFUL);
        return;
    }

    _shouldUnlock = true;

    if (!_sspiSocket->ShouldNegotiate())
    {
        CompleteNegotiate(_sspiSocket->SecurityStatus());
        return;
    }

    _startedNegotiate = true;
    _sspiSocket->SaveNegotiateOp(this);

    _timer->StartTimer(
        _sspiSocket->SspiTransport().NegotiationTimeout(),
        this,
        CompletionCallback(this, &KSspiNegotiateOp::TimerCallback));

    if (_sspiSocket->SspiContext().InBound())
    {
        KDbgPrintf("socket %llx: start ASC", _traceId);
        StartReceive();
        return;
    }

    KDbgPrintf("socket %llx: start ISC", _traceId);
    NTSTATUS status = _sspiSocket->SspiContext().StartContextCall(
        0,
        CompletionCallback(this, &KSspiNegotiateOp::OnContextCallCompleted),
        this);

    if (!NT_SUCCESS(status)) // Multiple non-failure status values are allowed to continue
    {
        CompleteNegotiate(status);
    }
}

void KSspiNegotiateOp::TimerCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KAssert(static_cast<KAsyncContextBase*>(this) == parent); parent;

    NTSTATUS status = completingOperation.Status();
    if (!NT_SUCCESS(status))
    {
        return;
    }

    Complete(STATUS_TIMEOUT);
}

void KSspiNegotiateOp::OnCompleted()
{
    KFatal(_sspiSocket->ShouldNegotiate() == false);
    _sspiSocket->ReleaseInboundNegotiationSlotIfNeeded();
}

void KSspiNegotiateOp::OnReuse()
{
    // This is not supposed to be reused because negotiation should happen once per socket
    KFatal(false);
}

void KSspiNegotiateOp::TraceNegoInfo(NTSTATUS status)
{
    if (!NT_SUCCESS(status))
    {
#if KTL_USER_MODE
        TRACE_SecurityNegotiationFailed((unsigned __int64)_traceId, _sspiSocket->_sspiContext->InBound(), status);
#else
        TRACE_SecurityNegotiationFailed(NULL, (unsigned __int64)_traceId, _sspiSocket->_sspiContext->InBound(), status);
#endif
        return;
    }

    SecPkgContext_NegotiationInfo negotiationInfo = {};
    _sspiSocket->_sspiContext->QueryContextAttributes(SECPKG_ATTR_NEGOTIATION_INFO, &negotiationInfo);
    KFinally([&negotiationInfo] { FreeContextBuffer(negotiationInfo.PackageInfo); });

#if KTL_USER_MODE
    TRACE_SecurityNegotiationSucceeded(
        (unsigned __int64)_traceId,
        _sspiSocket->_sspiContext->InBound(),
        negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->Name : L"",
        negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->fCapabilities : 0);
#else
    TRACE_SecurityNegotiationSucceeded(
        NULL,
        (unsigned __int64)_traceId,
        _sspiSocket->_sspiContext->InBound(),
        negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->Name : L"",
        negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->fCapabilities : 0);
#endif
}

void KSspiNegotiateOp::CompleteNegotiate(NTSTATUS status)
{
    if (_startedNegotiate)
    {
        _timer->Cancel();
        TraceNegoInfo(status);
        status = _sspiSocket->CompleteNegotiate(status);
    }

    if (_shouldUnlock)
    {
        _shouldUnlock = false;
        ReleaseLock(_sspiSocket->_negotiateLock);
    }

    Complete(status);
}

_Use_decl_annotations_
void KSspiNegotiateOp::OnContextCallCompleted(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    parent;
    KAssert(parent == static_cast<KAsyncContextBase * const >(this));

    NTSTATUS status = completingOperation.Status();
    KDbgPrintf("socket %llx: sspi context call returned: %x", _traceId, status);

    if(_sspiSocket->SspiContext().SendMemRef()._Size > 0)
    {
        StartSend();
        return;
    }

    CheckContextStatus(status);
}

void KSspiNegotiateOp::StartSend()
{
    _sendMemRef = _sspiSocket->SspiContext().SendMemRef();
    KAssert(_sendMemRef._Size > 0);

    _sendOp->Reuse();
    NTSTATUS status = _sendOp->StartWrite(
        1,
        &_sendMemRef,
        ISocketWriteOp::ePriorityHigh,
        CompletionCallback(this, &KSspiNegotiateOp::SendCallback),
        this);

    if (!NT_SUCCESS(status))
    {
        CompleteNegotiate(status);
    }
}

_Use_decl_annotations_
void KSspiNegotiateOp::SendCallback(KAsyncContextBase* const parent, KAsyncContextBase& completingOperation)
{
    KFatal(parent == static_cast<KAsyncContextBase * const >(this));

    ISocketWriteOp& sendOp = static_cast<ISocketWriteOp&>(completingOperation);
    NTSTATUS status = sendOp.Status();
    if (status != STATUS_SUCCESS)
    {
        CompleteNegotiate(status);
        return;
    }

    CheckContextStatus(_sspiSocket->SspiContext().SecurityStatus());
}

void KSspiNegotiateOp::CheckContextStatus(SECURITY_STATUS status)
{
    if ((status == SEC_I_CONTINUE_NEEDED) || (status == SEC_E_INCOMPLETE_MESSAGE))
    {
        StartReceive();
        return;
    }

    if (status == SEC_E_OK)
    {
        KDbgPrintf("socket %llx: sspi negotiation completed", _traceId);

        KSspiTransport::AuthorizationContext* authContext = _sspiSocket->SspiContext().InBound() ? 
            _sspiSocket->SspiTransport().ClientAuthContext() : _sspiSocket->SspiTransport().ServerAuthContext();

        status = KSspiAuthorizationOp::StartAuthorize(
            GetThisAllocator(),
            authContext,
            _sspiSocket->SspiContext().Handle(),
            CompletionCallback(this, &KSspiNegotiateOp::AuthorizationCompleted),
            this);

        if (!NT_SUCCESS(status))
        {
            CompleteNegotiate(status);
        }

        return;
    }

    KDbgPrintf("socket %llx: sspi negotiation failed: %x", _traceId, status);
    CompleteNegotiate(status);
}

_Use_decl_annotations_
void KSspiNegotiateOp::AuthorizationCompleted(KAsyncContextBase* const parent, KAsyncContextBase& completingOperation)
{
    KFatal(parent == static_cast<KAsyncContextBase * const >(this));

    KSspiAuthorizationOp& authOp = static_cast<KSspiAuthorizationOp&>(completingOperation);
    NTSTATUS status = authOp.Status();
    if (status != STATUS_SUCCESS)
    {
        KDbgPrintf(" socket %llx: authorization failed: %x", _traceId, status);
    }

    CompleteNegotiate(status);
}

void KSspiNegotiateOp::StartReceive()
{
    _receiveMemRef = _sspiSocket->SspiContext().ReceiveMemRef();
    _receiveOp->Reuse();
    NTSTATUS status = _receiveOp->StartRead(
        1,
        &_receiveMemRef,
        ISocketReadOp::ReadAny,
        CompletionCallback(this, &KSspiNegotiateOp::ReceiveCallback),
        this);

    if (!NT_SUCCESS(status))
    {
        CompleteNegotiate(status);
    }
}

_Use_decl_annotations_
void KSspiNegotiateOp::ReceiveCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
     KFatal(parent == static_cast<KAsyncContextBase * const>(this));

     ISocketReadOp& receiveOp = static_cast<ISocketReadOp&>(completingOperation);
     NTSTATUS status = receiveOp.Status();
     if (status != STATUS_SUCCESS)
     {
         CompleteNegotiate(status);
         return;
     }

     if (receiveOp.BytesTransferred() == 0)
     {
         KDbgPrintf("socket %llx: received 0 data unexpectedly", _traceId);
         CompleteNegotiate(STATUS_UNSUCCESSFUL);
         return;
     }

     status = _sspiSocket->SspiContext().StartContextCall(
         receiveOp.BytesTransferred(),
         CompletionCallback(this, &KSspiNegotiateOp::OnContextCallCompleted),
         this);

    if (!NT_SUCCESS(status))
    {
        CompleteNegotiate(status);
        return;
    }
}

#pragma endregion

#pragma region KSspiSocketAcceptOp

KSspiSocketAcceptOp::KSspiSocketAcceptOp(KSspiSocketListener::SPtr && sspiSocketListener)
    : _sspiSocketListener(Ktl::Move(sspiSocketListener)), _userData(nullptr)
{
    _traceId = this;
    NTSTATUS status = _sspiSocketListener->InnerSocketListener()->CreateAcceptOp(_innerAcceptOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    SetConstructorStatus(KTimer::Create(_delayTimer, GetThisAllocator(), KTL_TAG_SSPI));
}

void KSspiSocketAcceptOp::OnReuse()
{
    _sspiSocket.Reset();
    _innerAcceptOp->Reuse();
    _delayTimer->Reuse();
}

void KSspiSocketAcceptOp::OnCancel()
{
    _innerAcceptOp->Cancel();
    _delayTimer->Cancel();
}

NTSTATUS KSspiSocketAcceptOp::StartAccept(
    __in_opt  KAsyncContextBase::CompletionCallback Callback,
    __in_opt  KAsyncContextBase* const ParentContext)
{
    Start(ParentContext, Callback);
    return STATUS_PENDING;
}

void KSspiSocketAcceptOp::OnStart()
{
    NTSTATUS status = _innerAcceptOp->StartAccept(
        CompletionCallback(this, &KSspiSocketAcceptOp::InnerAcceptCallback),
        this);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
    }
}

_Use_decl_annotations_
void KSspiSocketAcceptOp::InnerAcceptCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KAssert(static_cast<KAsyncContextBase*>(this) == parent); parent;

    ISocketAcceptOp& innerAcceptOp = static_cast<ISocketAcceptOp&>(completingOperation);
    NTSTATUS status = innerAcceptOp.Status();
    KDbgPrintf("KSspiSocketAcceptOp %llx: socket accept completed: %x", _traceId, status);
    if (status != STATUS_SUCCESS)
    {
        Complete(status);
        return;
    }

    KSspiContext::UPtr sspiContext;
    status = _sspiSocketListener->SspiTransport()->CreateSspiServerContext(sspiContext);
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    ISocket::SPtr framingSocket;
    status = _sspiSocketListener->SspiTransport()->CreateFramingSocket(
        innerAcceptOp.GetSocket(),
        framingSocket);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    status = _sspiSocketListener->SspiTransport()->CreateSspiSocket(
        Ktl::Move(framingSocket),
        Ktl::Move(sspiContext),
        _sspiSocket);

    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    status = static_cast<KSspiSocket*>(_sspiSocket.RawPtr())->AcquireInboundNegotiationSlot(_sspiSocketListener);
    if (status == STATUS_INSUFF_SERVER_RESOURCES)
    {
        KDbgPrintf("KSspiSocketAcceptOp %llx: security negotiation throttled, will retry later", _traceId);
        _delayTimer->StartTimer(1000, this, CompletionCallback(this, &KSspiSocketAcceptOp::DelayTimerCallback));
        return;
    }

    Complete(status);
}

_Use_decl_annotations_
void KSspiSocketAcceptOp::DelayTimerCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KAssert(static_cast<KAsyncContextBase*>(this) == parent); parent;

    NTSTATUS status = completingOperation.Status();
    if (!NT_SUCCESS(status))
    {
        Complete(status);
        return;
    }

    Complete(static_cast<KSspiSocket*>(_sspiSocket.RawPtr())->AcquireInboundNegotiationSlot(_sspiSocketListener));
}

_Use_decl_annotations_
void KSspiSocketAcceptOp::SetUserData(PVOID ptr)
{
    _userData = ptr;
}

PVOID KSspiSocketAcceptOp::GetUserData()
{
    return _userData;
}

ISocket::SPtr KSspiSocketAcceptOp::GetSocket()
{
    return _sspiSocket;
}

#pragma endregion

#pragma region KSspiCredential

#if KTL_USER_MODE == 0

_Use_decl_annotations_
SECURITY_STATUS KSspiCredential::AcquireSsl(
    SPtr & credential,
    KAllocator & alloc,
    UCHAR const* certHashValue,
    WCHAR const* certStoreName,
    size_t cbCertStoreNameLength,
    ULONG credentialUse)
{
    if (cbCertStoreNameLength > (SCH_CRED_MAX_STORE_NAME_SIZE - 1) * sizeof(WCHAR))
    {
        credential = nullptr;
        return STATUS_INVALID_PARAMETER;
    }

    SCHANNEL_CERT_HASH_STORE certHash = { 0 };
    certHash.dwLength = sizeof(certHash);
    certHash.dwFlags = SCH_MACHINE_CERT_HASH;
    certHash.hProv = 0;

    KMemCpySafe(certHash.ShaHash, sizeof(certHash.ShaHash), certHashValue, sizeof(certHash.ShaHash));
    KMemCpySafe(certHash.pwszStoreName, sizeof(certHash.pwszStoreName), certStoreName, cbCertStoreNameLength);

    SCHANNEL_CRED schannelCred = {};
    schannelCred.dwVersion = SCHANNEL_CRED_VERSION;
    schannelCred.dwCredFormat = SCH_CRED_FORMAT_CERT_HASH_STORE;
    schannelCred.cCreds = 1;
    schannelCred.paCred = reinterpret_cast<PCCERT_CONTEXT*>(&certHash);
    schannelCred.dwFlags = SCH_CRED_MANUAL_CRED_VALIDATION;

    credential = _new (KTL_TAG_SSPI, alloc) KSspiCredential();

    if (credential == nullptr)
    {
        KDbgPrintf(__FUNCTION__ ": Allocate credential failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    UNICODE_STRING schannelName = { 0 };
    RtlInitUnicodeString(&schannelName, SCHANNEL_NAME_W);

    return AcquireCredentialsHandleW(
        nullptr,
        &schannelName,
        credentialUse,
        nullptr,
        &schannelCred,
        nullptr,
        nullptr,
        &credential->_value,
        &credential->_expiry);
}

SECURITY_STATUS KSspiCredential::QueryRemoteCertificate(
        PCtxtHandle securityContext,
        __inout ULONG & cbCertificateChain,
        __out_bcount(cbCertificateChain) UCHAR * pbCertificateChain)
{
    SECURITY_STATUS SecStatus;
    SecPkgContext_Certificates ClientCert;

    SecStatus = QueryContextAttributesW(securityContext,
                                        SECPKG_ATTR_REMOTE_CERTIFICATES,
                                        &ClientCert);

    if (SecStatus != SEC_E_OK || ClientCert.cbCertificateChain == 0)
    {
        RtlZeroMemory(pbCertificateChain, cbCertificateChain);
        cbCertificateChain = 0;
        return STATUS_INVALID_PARAMETER;
    }

    if (cbCertificateChain < ClientCert.cbCertificateChain)
    {
        RtlZeroMemory(pbCertificateChain, cbCertificateChain);
        cbCertificateChain = ClientCert.cbCertificateChain;
        SecStatus = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        cbCertificateChain = ClientCert.cbCertificateChain;
        KMemCpySafe(
            pbCertificateChain,
            cbCertificateChain,
            ClientCert.pbCertificateChain,
            cbCertificateChain);
    }

    FreeContextBuffer(ClientCert.pbCertificateChain);

    return SecStatus;
}

#endif

_Use_decl_annotations_
SECURITY_STATUS KSspiCredential::AcquireKerberos(
    SPtr & credential,
    KAllocator & alloc,
    LPWSTR principal,
    unsigned long credentialUse,
    void * logonId,
    void * authData,
    SEC_GET_KEY_FN getKeyFn,
    void * getKeyArgument)
{
    credential = _new (KTL_TAG_SSPI, alloc) KSspiCredential();
    if (credential == nullptr)
    {
        KDbgPrintf(__FUNCTION__ ": Allocate credential failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if KTL_USER_MODE
    return AcquireCredentialsHandle(
        principal,
        const_cast<WCHAR*>(MICROSOFT_KERBEROS_NAME),
        credentialUse,
        logonId,
        authData,
        getKeyFn,
        getKeyArgument,
        &credential->_value,
        &credential->_expiry);
#else
    SECURITY_STRING kerberosPackageName;
    RtlInitUnicodeString(&kerberosPackageName, MICROSOFT_KERBEROS_NAME);

    SECURITY_STRING principalName;
    RtlInitUnicodeString(&principalName, principal);

    return AcquireCredentialsHandle(
        &principalName,
        &kerberosPackageName,
        credentialUse,
        logonId,
        authData,
        getKeyFn,
        getKeyArgument,
        &credential->_value,
        &credential->_expiry);
#endif
}

_Use_decl_annotations_
SECURITY_STATUS KSspiCredential::AcquireNegotiate(
    SPtr & credential,
    KAllocator & alloc,
    LPWSTR principal,
    unsigned long credentialUse,
    void * logonId,
    void * authData,
    SEC_GET_KEY_FN getKeyFn,
    void * getKeyArgument)
{
    credential = _new(KTL_TAG_SSPI, alloc) KSspiCredential();
    if (credential == nullptr)
    {
        KDbgPrintf(__FUNCTION__ ": Allocate credential failed");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

#if KTL_USER_MODE
    return AcquireCredentialsHandle(
        principal,
        const_cast<WCHAR*>(NEGOSSP_NAME),
        credentialUse,
        logonId,
        authData,
        getKeyFn,
        getKeyArgument,
        &credential->_value,
        &credential->_expiry);
#else
    SECURITY_STRING securityPackageName;
    RtlInitUnicodeString(&securityPackageName, NEGOSSP_NAME);

    SECURITY_STRING principalName;
    RtlInitUnicodeString(&principalName, principal);

    return AcquireCredentialsHandle(
        &principalName,
        &securityPackageName,
        credentialUse,
        logonId,
        authData,
        getKeyFn,
        getKeyArgument,
        &credential->_value,
        &credential->_expiry);
#endif
}

#pragma endregion

#pragma region KSspiContext

_Use_decl_annotations_
KSspiContext::KSspiContext(
    KAllocator& alloc,
    PCredHandle credHandle,
    WCHAR* targetName,
    bool inbound,
    ULONG requirement,
    ULONG bufferSize)
    : _allocator(alloc)
    , _credHandle(credHandle)
    , _targetName(alloc, targetName)
    , _inbound(inbound)
    , _requirement(requirement)
    , _firstPass(true)
    , _contextAttr(0)
    , _securityStatus(SEC_I_CONTINUE_NEEDED)
    , _outputBuffer(0, nullptr)
    , _sendMemRef(0, nullptr)
    , _inputBuffer(0, nullptr)
    , _receiveMemRef(0, nullptr)
    , _inputLength(0)
{
    _contextCallOp = _new (KTL_TAG_SSPI, _allocator) ContextCallOp(*this);
    if (!_contextCallOp)
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    _outputBuffer._Address = _new (KTL_TAG_SSPI, alloc) UCHAR[bufferSize];
    if (_outputBuffer._Address)
    {
        _outputBuffer._Size  = bufferSize;
    }
    else
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }


    _inputBuffer._Address = _new (KTL_TAG_SSPI, alloc) UCHAR[bufferSize];
    if (_inputBuffer._Address)
    {
        _inputBuffer._Size = bufferSize;
        _receiveMemRef = _inputBuffer;
    }
    else
    {
        SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
        return;
    }

    SecInvalidateHandle(&_handle);
    RtlZeroMemory(&_outputSecBuffer, sizeof(_outputSecBuffer));
}

KSspiContext::~KSspiContext()
{
    DeleteSecurityContext(&_handle);
    FreeBuffers();
}

void KSspiContext::FreeBuffers()
{
    _delete(_outputBuffer._Address);
    _outputBuffer = KMemRef();

    _delete(_inputBuffer._Address);
    _inputBuffer = KMemRef();

    _receiveMemRef = KMemRef();
    _sendMemRef = KMemRef();
}

_Use_decl_annotations_
NTSTATUS KSspiContext::StartContextCall(
    ULONG newlyReceived,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    _contextCallOp->Reuse();
    return _contextCallOp->StartCall(newlyReceived, callback, parentContext);
}

SECURITY_STATUS KSspiContext::InitializeContext(ULONG newlyReceived)
{
    KFatal(!_inbound);

    OnNewInput(newlyReceived);

    for (;;)
    {
        _securityStatus = InitializeSecurityContext(
            _credHandle,
            InputContext(),
            _targetName,
            _requirement | _contextAttr,
            0,
            TargetDataRep(),
            InputSecBufferDesc(),
            0,
            &_handle,
            OutputSecBufferDesc(),
            &_contextAttr,
            &_expiry);

        if ((_securityStatus == SEC_I_CONTINUE_NEEDED) && (OutputSize() == 0))
        {
            DeleteProcessedInput();
            continue;
        }

        break;
    }

    if (_securityStatus != SEC_E_INCOMPLETE_MESSAGE)
    {
        _firstPass = false;
    }

    if (_securityStatus == SEC_I_CONTINUE_NEEDED)
    {
        _inputLength = 0;
    }
    else if (_securityStatus == SEC_E_OK)
    {
        SaveExtraInputIfAny();
    }

    SetSendMemRef();
    SetReceiveMemRef();

    return _securityStatus;
}

SECURITY_STATUS KSspiContext::AcceptContext(ULONG newlyReceived)
{
    KFatal(_inbound);

    OnNewInput(newlyReceived);

    for (;;)
    {
        //KDbgPrintf("calling AcceptSecurityContext");
        _securityStatus = AcceptSecurityContext(
            _credHandle,
            InputContext(),
            InputSecBufferDesc(),
            _requirement | _contextAttr,
            TargetDataRep(),
            &_handle,
            OutputSecBufferDesc(),
            &_contextAttr,
            &_expiry);

        //KDbgPrintf("AcceptSecurityContext returned: %x", _securityStatus);
        if ((_securityStatus == SEC_I_CONTINUE_NEEDED) && (OutputSize() == 0))
        {
            DeleteProcessedInput();
            continue;
        }

        break;
    }

    if (_securityStatus != SEC_E_INCOMPLETE_MESSAGE)
    {
        _firstPass = false;
    }

    if (_securityStatus == SEC_I_CONTINUE_NEEDED)
    {
        _inputLength = 0;
    }
    else if (_securityStatus == SEC_E_OK)
    {
        SaveExtraInputIfAny();
    }

    SetSendMemRef();
    SetReceiveMemRef();

    return _securityStatus;
}

bool KSspiContext::InBound() const
{
    return _inbound;
}

void KSspiContext::OnNewInput(ULONG newlyReceived)
{
    //KDbgPrintf("current inputLength=%u, newlyReceived=%u, ", _inputLength, newlyReceived);
    _inputLength += newlyReceived;
    KFatal(_inputLength <= _inputBuffer._Size);
    SetInputSecBufferDesc(_inputBuffer._Address, _inputLength);
}

void KSspiContext::DeleteProcessedInput()
{
    ULONG dataLeft = RemainingInputLength();
    //KDbgPrintf("DeleteProcessedInput: dataLeft = %d", dataLeft);

    RtlMoveMemory(
        _inputBuffer._Address,
        (PUCHAR)_inputBuffer._Address + (_inputLength - dataLeft),
        dataLeft);

    _inputLength = dataLeft;

    SetInputSecBufferDesc(_inputBuffer._Address, _inputLength);
}

ULONG KSspiContext::RemainingInputLength()
{
    // SSL context will override this for SEC_E_INCOMPLETE_MESSAGE
    return 0;
}

void KSspiContext::SetReceiveMemRef()
{
    _receiveMemRef._Address = (PUCHAR)_inputBuffer._Address + _inputLength;
    _receiveMemRef._Param = 0;
    _receiveMemRef._Size = _inputBuffer._Size - _inputLength;

    //KDbgPrintf(
    //    __FUNCTION__ ": receive buf = %llx, len = %u, input buf starts at %llx, total = %u,  received = %u",
    //    _receiveMemRef._Address, _receiveMemRef._Size, _inputBuffer._Address, _inputBuffer._Size, _inputLength);
}

PCtxtHandle KSspiContext::Handle()
{
    return &_handle;
}

TimeStamp const & KSspiContext::Expiry() const
{
    return _expiry;
}

KMemRef KSspiContext::SendMemRef()
{
    return _sendMemRef;
}

KMemRef KSspiContext::ReceiveMemRef()
{
    return _receiveMemRef;
}

PCredHandle KSspiContext::InputContext()
{
    return _firstPass ? nullptr : &_handle;
}

PSecBufferDesc KSspiContext::OutputSecBufferDesc()
{
    _outputSecBuffer.BufferType = SECBUFFER_TOKEN;
    _outputSecBuffer.cbBuffer = _outputBuffer._Size;
    _outputSecBuffer.pvBuffer = _outputBuffer._Address;

    _outputSecBufferDesc.ulVersion = SECBUFFER_VERSION;
    _outputSecBufferDesc.cBuffers = 1;
    _outputSecBufferDesc.pBuffers = &_outputSecBuffer;

    return &_outputSecBufferDesc;
}

ULONG KSspiContext::OutputSize()
{
    return _outputSecBuffer.cbBuffer;
}

void KSspiContext::SetSendMemRef()
{
    if (_securityStatus != SEC_E_INCOMPLETE_MESSAGE)
    {
        _sendMemRef._Address = _outputSecBuffer.pvBuffer;
        _sendMemRef._Param = _outputSecBuffer.cbBuffer;
        _sendMemRef._Size = _outputSecBuffer.cbBuffer;
    }
    else
    {
        _sendMemRef._Address = nullptr;
        _sendMemRef._Size = _sendMemRef._Param = 0;
    }
}

SECURITY_STATUS KSspiContext::QueryContextAttributes(ULONG attribute, PVOID buffer)
{
    return ::QueryContextAttributes(&_handle, attribute, buffer);
}

NTSTATUS KSspiContext::SecurityStatus() const
{
    // todo, leikong, do a proper conversion from SECURITY_STATUS to NTSTATUS
    return _securityStatus;
}

KMemRef KSspiContext::ExtraData() const
{
    return KMemRef(0, nullptr);
}

#pragma endregion

#pragma region KSspiContext::ContextCallOp

KSspiContext::ContextCallOp::ContextCallOp(KSspiContext & sspiContext) : _sspiContext(sspiContext)
{
}

_Use_decl_annotations_
NTSTATUS KSspiContext::ContextCallOp::StartCall(
    ULONG newlyReceived,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    _newlyReceived = newlyReceived;
    Start(parentContext, callback);

    return STATUS_PENDING;
}

void KSspiContext::ContextCallOp::OnStart()
{
    // No need to increment activity count for this, since this class does not implement cancel
    GetThisKtlSystem().DefaultSystemThreadPool().QueueWorkItem(*this);
}

void KSspiContext::ContextCallOp::Execute()
{
    if (_sspiContext.InBound())
    {
        Complete(_sspiContext.AcceptContext(_newlyReceived));
        return;
    }

    Complete(_sspiContext.InitializeContext(_newlyReceived));
}

#pragma endregion

#pragma region KSspiAddress

KSspiAddress::KSspiAddress(INetAddress::SPtr const & innerAddress, PCWSTR sspiTarget)
    : _innerAddress(innerAddress)
    , _sspiTarget(GetThisAllocator(), sspiTarget)
{
}

KSspiAddress::~KSspiAddress()
{
}

PSOCKADDR KSspiAddress::Get()
{
    return _innerAddress->Get();
}

ULONG KSspiAddress::GetFormat()
{
    return _innerAddress->GetFormat();
}

PVOID KSspiAddress::GetUserData()
{
    return _innerAddress->GetUserData();
}

VOID KSspiAddress::SetUserData(__in PVOID Data)
{
    _innerAddress->SetUserData(Data);
}

size_t KSspiAddress::Size()
{
    return _innerAddress->Size();
}

NTSTATUS KSspiAddress::ToStringW(__out_ecount(len) PWSTR buf, __inout ULONG & len) const
{
    return _innerAddress->ToStringW(buf, len);
}

NTSTATUS KSspiAddress::ToStringA(__out_ecount(len) PSTR buf, __inout ULONG & len) const
{
    return _innerAddress->ToStringA(buf, len);
}

PWSTR KSspiAddress::SspiTarget()
{
    return PWSTR(_sspiTarget);
}

#pragma endregion

#pragma region KSspiSocketReadOp

KSspiSocketReadOp::KSspiSocketReadOp(KSspiSocket::SPtr && sspiSocket)
    : _sspiSocket(Ktl::Move(sspiSocket))
    , _shouldUnlock(false)
    , _outputBuffers(GetThisAllocator())
    , _outputBufferSizeTotal(0)
    , _cleartextRead(0)
    , _flags(0)
    , _userData(nullptr)
{
    if (_outputBuffers.Status() != STATUS_SUCCESS)
    {
        SetConstructorStatus(_outputBuffers.Status());
        return;
    }

    NTSTATUS status = _sspiSocket->InnerSocket().CreateReadOp(_innerSocketReadOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _lockAcquiredCallback.Bind(this, &KSspiSocketReadOp::OnLockAcquired);
    _innerReadCallback.Bind(this, &KSspiSocketReadOp::InnerReadCallback);

    _traceId = _sspiSocket->InnerSocket().GetSocketId();
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KSspiSocketReadOp::IoTimestamps()
{
    return _innerSocketReadOp->IoTimestamps();
}

KSspiTimestamps const * KSspiSocketReadOp::SspiTimestamps()
{
    return &_sspiTimestamps;
}

#endif

_Use_decl_annotations_
NTSTATUS KSspiSocketReadOp::StartRead(
    ULONG bufferCount,
    KMemRef* buffers,
    ULONG flags,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    if (bufferCount == 0)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (buffers == nullptr)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    NTSTATUS status = _outputBuffers.Reserve(bufferCount);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    BOOLEAN setCountSucceeded = _outputBuffers.SetCount(bufferCount);
    KFatal(setCountSucceeded);

    _outputBufferSizeTotal = 0;
    for (ULONG i = 0; i < bufferCount; ++i)
    {
        _outputBuffers[i] = buffers[i];
        _outputBuffers[i]._Param = 0;
        _outputBufferSizeTotal += _outputBuffers[i]._Size;
    }

    _flags = flags;
    _currentOutputBuffer = 0;
    _cleartextRead = 0;

    Start(parentContext, callback);
    return STATUS_PENDING;
}

ULONG KSspiSocketReadOp::BytesTransferred()
{
    return _cleartextRead;
}

ULONG KSspiSocketReadOp::BytesToTransfer()
{
    return _outputBufferSizeTotal;
}

ISocket::SPtr KSspiSocketReadOp::GetSocket()
{
    return const_cast<KSspiSocket::SPtr&>(_sspiSocket).RawPtr();
}

_Use_decl_annotations_
void KSspiSocketReadOp::SetUserData(PVOID Ptr)
{
    _userData = Ptr;
}

PVOID KSspiSocketReadOp::GetUserData()
{
    return _userData;
}

void KSspiSocketReadOp::OnStart()
{
    if (_sspiSocket->ShouldNegotiate())
    {
        NTSTATUS status = _sspiSocket->StartNegotiate(
            CompletionCallback(this, &KSspiSocketReadOp::OnNegotiationCompleted),
            this);

        if (!NT_SUCCESS(status))
        {
            Complete(status);
        }

        return;
    }

    if (_sspiSocket->NegotiationSucceeded())
    {
        AcquireLock(_sspiSocket->_readLock, _lockAcquiredCallback);
        return;
    }

    KDbgPrintf("socket %llx: sspi negotiation already failed: %x", _traceId, _sspiSocket->SecurityStatus());
    UnlockAndComplete(_sspiSocket->SspiContext().SecurityStatus());
}

_Use_decl_annotations_
void KSspiSocketReadOp::OnNegotiationCompleted(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);

    NTSTATUS status = completingOperation.Status();
    if (status != SEC_E_OK)
    {
        KDbgPrintf("socket %llx: sspi negotiation failed: %x", _traceId, status);
        UnlockAndComplete(status);
        return;
    }

    AcquireLock(_sspiSocket->_readLock, _lockAcquiredCallback);
}

_Use_decl_annotations_
void KSspiSocketReadOp::OnLockAcquired(BOOLEAN isAcquired, KAsyncLock &)
{
    if (!isAcquired)
    {
        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    _shouldUnlock = true;

    StartRead();
}

void KSspiSocketReadOp::StartRead()
{
    KMemRef savedCleartext = _sspiSocket->SavedCleartextMemRef();
    if (savedCleartext._Param > 0)
    {
        ULONG copied = CopyCleartext(savedCleartext._Address, savedCleartext._Param);
        _sspiSocket->ConsumeSavedCleartext(copied);
        //KDbgPrintf("socket %llx: copied saved cleartext: %u", _traceId, copied);

        if (ShouldComplete())
        {
            //KDbgPrintf("socket %llx: complete with saved cleartext", _traceId);
            UnlockAndComplete(SEC_E_OK);
            return;
        }
    }

    if (_sspiSocket->_readMemRef._Param > 0)
    {
        DoDecryption();
    }
    else
    {
        StartInnerReadOp();
    }
}

void KSspiSocketReadOp::DoDecryption()
{
#if KTL_IO_TIMESTAMP
    _sspiTimestamps.RecordEncryptStart();
#endif

    DecryptMessage();

#if KTL_IO_TIMESTAMP
    _sspiTimestamps.RecordEncryptComplete();
#endif
}

void KSspiSocketReadOp::StartInnerReadOp()
{
    if (_sspiSocket->_readMemRef._Param >= _sspiSocket->_readMemRef._Size)
    {
        KDbgPrintf("socket %llx: incoming message exceeds receive buffer limit of %u", _traceId, _sspiSocket->_readMemRef._Size);
        UnlockAndComplete(STATUS_NOT_SUPPORTED);
        return;
    }

    KMemRef readBuffer(
        _sspiSocket->_readMemRef._Size - _sspiSocket->_readMemRef._Param,
        (PUCHAR)_sspiSocket->_readMemRef._Address + _sspiSocket->_readMemRef._Param);

    _innerSocketReadOp->Reuse();
    NTSTATUS status = _innerSocketReadOp->StartRead(
        1,
        &readBuffer,
        ISocketReadOp::ReadAny,
        _innerReadCallback,

        this);
    if (!NT_SUCCESS(status))
    {
        UnlockAndComplete(status);
    }
}

_Use_decl_annotations_
void KSspiSocketReadOp::InnerReadCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);
    KFatal(static_cast<KAsyncContextBase*>(_innerSocketReadOp.RawPtr()) == &completingOperation);

    NTSTATUS status = _innerSocketReadOp->Status();
    if ((status != STATUS_SUCCESS) || (_innerSocketReadOp->BytesTransferred() == 0))
    {
        UnlockAndComplete(status);
        return;
    }

    _sspiSocket->_readMemRef._Param += _innerSocketReadOp->BytesTransferred();
    DoDecryption();
}

void KSspiSocketReadOp::UnlockAndComplete(NTSTATUS status)
{
    if (_shouldUnlock)
    {
        _shouldUnlock = false;
        ReleaseLock(_sspiSocket->_readLock);
    }
    Complete(status);
}

bool KSspiSocketReadOp::OutputBuffersFull()
{
    return _cleartextRead == _outputBufferSizeTotal;
}

bool KSspiSocketReadOp::ShouldComplete()
{
    return ((_cleartextRead > 0) && !(_flags & ISocketReadOp::ReadAll)) || OutputBuffersFull();
}

ULONG KSspiSocketReadOp::CopyCleartext(PVOID from, ULONG length)
{
    KFatal(!OutputBuffersFull());

    ULONG copiedTotal = 0;
    PUCHAR src = (PUCHAR)from;
    ULONG srcRemaining = length;

    for (ULONG i = _currentOutputBuffer; i < _outputBuffers.Count(); ++i)
    {
        ULONG bufferSize = _outputBuffers[i]._Size - _outputBuffers[i]._Param;
        ULONG toCopy = (srcRemaining <= bufferSize) ? srcRemaining : bufferSize;
        KMemCpySafe(
            (PUCHAR)_outputBuffers[i]._Address + _outputBuffers[i]._Param,
            _outputBuffers[i]._Size - _outputBuffers[i]._Param,
            src,
            toCopy);

        src += toCopy;
        srcRemaining -= toCopy;

        _outputBuffers[i]._Param += toCopy;
        copiedTotal += toCopy;
        _cleartextRead += toCopy;

        if (srcRemaining == 0)
        {
            _currentOutputBuffer = i;
            return copiedTotal;
        }
    }

    KFatal(OutputBuffersFull());
    return copiedTotal;
}

#pragma endregion

#pragma region KSspiSocketWriteOp

KSspiSocketWriteOp::KSspiSocketWriteOp(KSspiSocket::SPtr && sspiSocket)
    : _sspiSocket(Ktl::Move(sspiSocket))
    , _shouldUnlock(false)
    , _inputBuffers(GetThisAllocator())
    , _priority(0)
    , _userData(nullptr)
{
    if (_inputBuffers.Status() != STATUS_SUCCESS)
    {
        SetConstructorStatus(_inputBuffers.Status());
        return;
    }

    NTSTATUS status = _sspiSocket->InnerSocket().CreateWriteOp(_innerSocketWriteOp);
    if (!NT_SUCCESS(status))
    {
        SetConstructorStatus(status);
        return;
    }

    _lockAcquiredCallback.Bind(this, &KSspiSocketWriteOp::OnLockAcquired);
    _innerWriteCallback.Bind(this, &KSspiSocketWriteOp::InnerWriteCallback);

    _traceId = _sspiSocket->InnerSocket().GetSocketId();
}

#if KTL_IO_TIMESTAMP

KIoTimestamps const * KSspiSocketWriteOp::IoTimestamps()
{
    return _innerSocketWriteOp->IoTimestamps();
}

KSspiTimestamps const * KSspiSocketWriteOp::SspiTimestamps()
{
    return &_sspiTimestamps;
}

#endif

_Use_decl_annotations_
NTSTATUS KSspiSocketWriteOp::StartWrite(
    ULONG bufferCount,
    KMemRef* buffers,
    ULONG priority,
    KAsyncContextBase::CompletionCallback callback,
    KAsyncContextBase* const parentContext)
{
    if (bufferCount == 0)
    {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (buffers == nullptr)
    {
        return STATUS_INVALID_PARAMETER_2;
    }

    NTSTATUS status = _inputBuffers.Reserve(bufferCount);
    if (status != STATUS_SUCCESS)
    {
        return status;
    }

    BOOLEAN setCountSucceeded = _inputBuffers.SetCount(bufferCount);
    KFatal(setCountSucceeded);

    for (ULONG i = 0; i < bufferCount; ++ i)
    {
        _inputBuffers[i] = buffers[i];
    }

    _priority = priority;

    Start(parentContext, callback);
    return STATUS_PENDING;
}

ULONG KSspiSocketWriteOp::BytesToTransfer()
{
    return _cleartextToTransfer;
}

ULONG KSspiSocketWriteOp::BytesTransferred()
{
    return (Status() == STATUS_SUCCESS)? _cleartextToTransfer : 0;
}

ISocket::SPtr KSspiSocketWriteOp::GetSocket()
{
    return const_cast<KSspiSocket::SPtr&>(_sspiSocket).RawPtr();
}

_Use_decl_annotations_
void KSspiSocketWriteOp::SetUserData(PVOID Ptr)
{
    _userData = Ptr;
}

PVOID KSspiSocketWriteOp::GetUserData()
{
    return _userData;
}

void KSspiSocketWriteOp::OnStart()
{
    if (_sspiSocket->ShouldNegotiate())
    {
#if KTL_IO_TIMESTAMP
        _sspiTimestamps.RecordNegotiateStart();
#endif

        NTSTATUS status = _sspiSocket->StartNegotiate(
            CompletionCallback(this, &KSspiSocketWriteOp::OnNegotiationCompleted),
            this);

        if (!NT_SUCCESS(status))
        {
            Complete(status);
        }

        return;
    }

    if (_sspiSocket->NegotiationSucceeded())
    {
        AcquireLock(_sspiSocket->_writeLock, _lockAcquiredCallback);
        return;
    }

    KDbgPrintf("socket %llx: sspi negotiation already failed: %x", _traceId, _sspiSocket->SecurityStatus());
    UnlockAndComplete(_sspiSocket->SspiContext().SecurityStatus());
}

_Use_decl_annotations_
void KSspiSocketWriteOp::OnNegotiationCompleted(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);

#if KTL_IO_TIMESTAMP
    _sspiTimestamps.RecordNegotiateComplete();
#endif

    NTSTATUS status = completingOperation.Status();
    if (status != STATUS_SUCCESS)
    {
        KDbgPrintf("socket %llx: sspi negotiation failed: %x", _traceId, status);
        UnlockAndComplete(status);
        return;
    }

    AcquireLock(_sspiSocket->_writeLock, _lockAcquiredCallback);
}

_Use_decl_annotations_
void KSspiSocketWriteOp::OnLockAcquired(BOOLEAN isAcquired, KAsyncLock &)
{
    if (!isAcquired)
    {
        Complete(STATUS_UNSUCCESSFUL);
        return;
    }

    _shouldUnlock = true;

#if KTL_IO_TIMESTAMP
    _sspiTimestamps.RecordEncryptStart();
#endif

    //todo, leikong, consider fragmenting data to multiple blocks when the size is too large:
    //  1. ssl streaming has cbMaximumMessage limit
    //  2. Our Kerberos framing socket has MaxFrameSize
    NTSTATUS status = EncryptMessage();

#if KTL_IO_TIMESTAMP
    _sspiTimestamps.RecordEncryptComplete();
#endif

    if (status != SEC_E_OK)
    {
        KDbgPrintf("socket %llx: EncryptMessage failed: %d", _traceId, status);
        UnlockAndComplete(status);
        return;
    }

    StartInnerWrite();
}

void KSspiSocketWriteOp::OnReuse()
{
#if KTL_IO_TIMESTAMP
    _sspiTimestamps.Reset();
#endif

    _innerSocketWriteOp->Reuse();
}

void KSspiSocketWriteOp::StartInnerWrite()
{
    NTSTATUS status = _innerSocketWriteOp->StartWrite(
        1,
        &_sspiSocket->_writeMemRef,
        _priority,
        _innerWriteCallback,
        this);
    if (!NT_SUCCESS(status))
    {
        UnlockAndComplete(status);
    }
}

_Use_decl_annotations_
void KSspiSocketWriteOp::InnerWriteCallback(
    KAsyncContextBase* const parent,
    KAsyncContextBase& completingOperation)
{
    KFatal(static_cast<KAsyncContextBase*>(this) == parent);
    KFatal(static_cast<KAsyncContextBase*>(_innerSocketWriteOp.RawPtr()) == &completingOperation);
    UnlockAndComplete(_innerSocketWriteOp->Status());
}

void KSspiSocketWriteOp::UnlockAndComplete(NTSTATUS status)
{
    if (_shouldUnlock)
    {
        _shouldUnlock = false;
        ReleaseLock(_sspiSocket->_writeLock);
    }

    Complete(status);
}

#pragma endregion

#if KTL_IO_TIMESTAMP

KSspiTimestamps::KSspiTimestamps()
{
    Reset();
}

void KSspiTimestamps::Reset()
{
    _negotiateStartTime = 0;
    _negotiateCompleteTime = 0;
    _encryptStartTime = 0;
    _encryptCompleteTime = 0;
}

ULONGLONG KSspiTimestamps::EncryptStartTime() const
{
    return _encryptStartTime;
}

void KSspiTimestamps::RecordEncryptStart()
{
    KIoTimestamps::RecordCurrentTime(_encryptStartTime);
}

ULONGLONG KSspiTimestamps::EncryptCompleteTime() const
{
    return _encryptCompleteTime;
}

void KSspiTimestamps::RecordEncryptComplete()
{
    KIoTimestamps::RecordCurrentTime(_encryptCompleteTime);
}

ULONGLONG KSspiTimestamps::NegotiateStartTime() const
{
    return _negotiateStartTime;
}

void KSspiTimestamps::RecordNegotiateStart()
{
    KIoTimestamps::RecordCurrentTime(_negotiateStartTime);
}

ULONGLONG KSspiTimestamps::NegotiateCompleteTime() const
{
    return _negotiateCompleteTime;
}

void KSspiTimestamps::RecordNegotiateComplete()
{
    KIoTimestamps::RecordCurrentTime(_negotiateCompleteTime);
}

#endif
