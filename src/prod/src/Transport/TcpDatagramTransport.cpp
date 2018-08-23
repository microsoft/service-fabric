// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

atomic_uint64  TcpDatagramTransport::objCount_(0);

bool TcpDatagramTransport::Test_ReceiveMissingAssertDisabled = false;
bool TcpDatagramTransport::Test_ReceiveMissingDetected = false;

namespace
{
    wstring CreateTraceId(TcpDatagramTransport const* ptr, wstring const & id)
    {
        return
            id.empty()?
            wformatString("{0}", TextTracePtrAs(ptr, IDatagramTransport)) :
            wformatString("{0}-{1}", TextTracePtrAs(ptr, IDatagramTransport), id);
    }

    auto InitOnceMessageHeaderCompactThreshold()
    {
        static auto threshold = TransportConfig::GetConfig().MessageHeaderCompactThreshold;
        return MessageHeaders::SetCompactThreshold(threshold);
    }
}

TcpDatagramTransport::TcpDatagramTransport(std::wstring const & id, std::wstring const & owner)
    : id_(id)
    , traceId_(CreateTraceId(this, id))
    , owner_(owner)
    , clientOnly_(true)
    , connectionOpenTimeout_(TransportConfig::GetConfig().ConnectionOpenTimeout)
    , connectionIdleTimeout_(TransportConfig::GetConfig().ConnectionIdleTimeout)
    , security_(std::make_shared<TransportSecurity>(clientOnly_))
    , throttle_(Throttle::GetThrottle())
    , perTargetSendQueueSizeLimitInBytes_(TransportConfig::GetConfig().DefaultSendQueueSizeLimit)
    , outgoingMessageExpiration_(
        (TransportConfig::GetConfig().DefaultOutgoingMessageExpiration > TimeSpan::Zero) ?
        TransportConfig::GetConfig().DefaultOutgoingMessageExpiration :
        TimeSpan::MaxValue)
    , recvBufferSize_(TransportConfig::GetConfig().TcpReceiveBufferSize)
{
    Initialize();
}

TcpDatagramTransport::TcpDatagramTransport(std::wstring const & listenAddress, std::wstring const & id, std::wstring const & owner)
    : listenAddress_(listenAddress)
    , id_(id)
    , traceId_(CreateTraceId(this, id))
    , owner_(owner)
    , clientOnly_(false)
    , connectionOpenTimeout_(TransportConfig::GetConfig().ConnectionOpenTimeout)
    , connectionIdleTimeout_(TransportConfig::GetConfig().ConnectionIdleTimeout)
    , security_(std::make_shared<TransportSecurity>(clientOnly_))
    , throttle_(Throttle::GetThrottle())
    , perTargetSendQueueSizeLimitInBytes_(TransportConfig::GetConfig().DefaultSendQueueSizeLimit)
    , outgoingMessageExpiration_(
        (TransportConfig::GetConfig().DefaultOutgoingMessageExpiration > TimeSpan::Zero) ?
        TransportConfig::GetConfig().DefaultOutgoingMessageExpiration :
        TimeSpan::MaxValue)
    , recvBufferSize_(TransportConfig::GetConfig().TcpReceiveBufferSize)
{
    Initialize();
}

TcpDatagramTransport::~TcpDatagramTransport()
{
    Stop();
    AcceptThrottle::GetThrottle()->RemoveListener(this);
    trace.TransportDestroyed(traceId_, --objCount_);
}

void TcpDatagramTransport::Initialize()
{
    trace.TransportCreated(traceId_, listenAddress_, owner_, ++objCount_);
    trace.ConnectionIdleTimeoutSet(traceId_, connectionIdleTimeout_.TotalSeconds());
    WriteInfo(Constants::TcpTrace, traceId_, "setting compact threshold to {0}", InitOnceMessageHeaderCompactThreshold());

    auto pci = wformatString("{0}-{1}-{2}-{3}", ::GetCurrentProcessId(), owner_, traceId_, Guid::NewGuid());
    WriteNoise(Constants::TcpTrace, traceId_, "perf counter: {0}", pci); 
    perfCounters_ = PerfCounters::CreateInstance(pci);

#ifdef PLATFORM_UNIX
    SetEventLoopPool(GetDefaultTransportEventLoopPool());
#else
    Sockets::Startup();
#endif
}

void TcpDatagramTransport::SetBufferFactory(unique_ptr<IBufferFactory> && bufferFactory)
{
    WriteInfo(Constants::TcpTrace, traceId_, "setting buffer factory to {0}", typeid(*bufferFactory).name());
    bufferFactory_ = move(bufferFactory);
}

#ifdef PLATFORM_UNIX

EventLoopPool* TcpDatagramTransport::EventLoops() const
{
    return eventLoopPool_;
}

void TcpDatagramTransport::SetEventLoopPool(EventLoopPool* pool)
{
    eventLoopPool_ = pool;
    WriteInfo(Constants::TcpTrace, traceId_, "eventLoopPool_ set to {0}", TextTracePtr(eventLoopPool_));
}

#endif

PerfCountersSPtr const & TcpDatagramTransport::PerfCounters() const
{
    return perfCounters_;
}

IListenSocket::SPtr TcpDatagramTransport::CreateListenSocket(Endpoint const& endpoint)
{
    return make_shared<ListenSocket>(
        endpoint,
        // LINUXTODO, consider capturing weak ptr
        [this](IListenSocket & ls, ErrorCode const & result) { AcceptComplete(ls, result); },
        traceId_);
}

uint64 TcpDatagramTransport::Instance() const
{
    return instance_;
}

void TcpDatagramTransport::SetInstance(uint64 instance)
{
    ASSERT_IF(listenAddress_.empty(), "cannot set instance unless there is listen address");

    trace.TransportInstanceSet(traceId_, listenAddress_, instance_, instance);
    instance_ = instance;

    // Clean up all existing connections since we probably have sent out old instance for them
    vector<NamedSendTargetPair> namedSendTargets;
    vector<AnonymousSendTargetPair> anonymousSendTargets;
    {
        AcquireWriteLock grab(lock_);

        namedSendTargets = vector<NamedSendTargetPair>(namedSendTargets_.begin(), namedSendTargets_.end());
        anonymousSendTargets = vector<AnonymousSendTargetPair>(anonymousSendTargets_.begin(), anonymousSendTargets_.end());
    }

    for (auto entry = namedSendTargets.begin(); entry < namedSendTargets.end(); ++ entry)
    {
        auto target = entry->second.lock();
        if (target)
        {
            target->CloseConnections(false);
        }
    }

    for (auto entry = anonymousSendTargets.begin(); entry < anonymousSendTargets.end(); ++ entry)
    {
        auto target = entry->second.lock();
        if (target)
        {
            target->CloseConnections(false);
        }
    }
}

bool TcpDatagramTransport::ShouldTraceInboundActivity() const
{
    return shouldTraceInboundActivity_;
}

void TcpDatagramTransport::EnableInboundActivityTracing()
{
    shouldTraceInboundActivity_ = true;
}

bool TcpDatagramTransport::ShouldTracePerMessage() const
{
    return shouldTracePerMessage_;
}

void Transport::TcpDatagramTransport::DisableAllPerMessageTraces()
{
    shouldTracePerMessage_ = false;
}

void TcpDatagramTransport::SetMessageHandler(MessageHandler const & handler)
{
    AcquireWriteLock grab(lock_);
    SetMessageHandler_CallerHoldingLock(handler);
}

void TcpDatagramTransport::SetMessageHandler_CallerHoldingLock(MessageHandler const & handler)
{
    Invariant(handler);
    ASSERT_IF(started_, "message handler must be set before starting");
    ASSERT_IF(stopping_, "message handler cannot be set after stopping");
    ASSERT_IF(handler_, "message handler cannot be set more than once");

    handler_ = make_shared<MessageHandler>(handler);
}

void TcpDatagramTransport::RemoveMessageHandlerCallerHoldingLock()
{
    handler_.reset();
}

TimeSpan TcpDatagramTransport::ConnectionIdleTimeout() const
{
    return this->connectionIdleTimeout_;
}

void TcpDatagramTransport::SetConnectionIdleTimeout(Common::TimeSpan timeout)
{
    ASSERT_IF(started_, "connection idle timeout must be set before starting");

    this->connectionIdleTimeout_ = timeout;
    trace.ConnectionIdleTimeoutSet(traceId_, this->connectionIdleTimeout_.TotalSeconds());
    if ((keepAliveTimeout_ > TimeSpan::Zero) && (connectionIdleTimeout_ > TimeSpan::Zero) && (keepAliveTimeout_ > connectionIdleTimeout_))
    {
        WriteWarning(
            Constants::TcpTrace, traceId_,
            "{0}: keepAliveTimeout_ {1} is greater than connectionIdleTimeout_ {2}, keepalive probably will not work",
            listenAddress_, keepAliveTimeout_, connectionIdleTimeout_);
    }
}

void TcpDatagramTransport::ConnectionValidationTimerCallback()
{
    ValidateConnections();
}

void TcpDatagramTransport::ValidateConnections()
{
    WriteNoise(Constants::TcpTrace, traceId_, "ValidateConnections starts");

    vector<TcpSendTargetSPtr> sendTargets = CreateSendTargetSnapshot();
    auto now = Stopwatch::Now();
    for (auto const & target : sendTargets)
    {
        target->ValidateConnections(now);
    }
}

void TcpDatagramTransport::StartCertMonitorTimerIfNeeded_CallerHoldingWLock()
{
    auto certMonitorInterval = SecurityConfig::GetConfig().CertificateMonitorInterval;

    if (!(starting_ || started_)||
        stopping_ ||
        certMonitorTimer_ ||
        (certMonitorInterval <= TimeSpan::Zero) ||
        (security_->SecurityProvider != SecurityProvider::Ssl))
    {

        WriteInfo(Constants::TcpTrace, traceId_, "skipping certificate monitor start");
        return;
    }

    WriteInfo(
        Constants::TcpTrace, traceId_,
        "{0}: starting certificate monitor, interval = {1}",
        listenAddress_, certMonitorInterval);

    static StringLiteral const timerTag("CertificateMonitor");
    certMonitorTimer_ = Timer::Create(
        timerTag,
        [this](TimerSPtr const &) { CertMonitorTimerCallback(); },
        false);

    certMonitorTimer_->SetCancelWait();

    Random r;
    TimeSpan dueTime = TimeSpan::FromMilliseconds(certMonitorInterval.TotalMilliseconds() * r.NextDouble());
    certMonitorTimer_->Change(dueTime, certMonitorInterval);
}

void TcpDatagramTransport::CertMonitorTimerCallback()
{
    WriteInfo(Constants::TcpTrace, traceId_, "CertMonitorTimerCallback: owner = {0}", owner_);

    {
        AcquireReadLock grab(lock_); // read lock for reading security_
        security_->RefreshX509CredentialsIfNeeded();
    }

    TcpDatagramTransport::RefreshIssuersIfNeeded();
}

void TcpDatagramTransport::StartListenerStateTraceTimer_CallerHoldingWLock()
{
    auto traceInterval = TransportConfig::GetConfig().ListenerStateTraceInterval;
    if (clientOnly_ || traceInterval <= TimeSpan::Zero) return;

    listenerStateTraceTimer_ = Timer::Create(
        "ListenerStateTrace",
        [this](TimerSPtr const&) { ListenerStateTraceCallback(); },
        false,
        Throttle::GetThrottle()->MonitorCallbackEnv());

    listenerStateTraceTimer_->SetCancelWait();
    auto randomStartDelay = TimeSpan::FromMilliseconds(traceInterval.TotalMilliseconds() * (0.5 + 0.5 * Random().NextDouble()));
    listenerStateTraceTimer_->Change(randomStartDelay, traceInterval);
}

void TcpDatagramTransport::ListenerStateTraceCallback()
{
    AcquireReadLock grab(lock_);
    trace.State(traceId_, owner_, listenAddress_, instance_, security_->ToString());
}

void TcpDatagramTransport::StartConnectionValicationTimer_CallerHoldingWLock()
{
    auto checkInterval = std::min(connectionIdleTimeout_, TransportConfig::GetConfig().ReceiveMissingThreshold);
    if (checkInterval <= TimeSpan::Zero)
    {
        checkInterval = std::max(connectionIdleTimeout_, TransportConfig::GetConfig().ReceiveMissingThreshold);
    }

    if (TransportConfig::GetConfig().SendTimeout <= TimeSpan::Zero)
    {
        if (checkInterval <= TimeSpan::Zero)
        {
            WriteInfo(Constants::TcpTrace, traceId_, "connectionValidationTimer_ is disabled");
            return;
        }
    }
    else if (TransportConfig::GetConfig().SendTimeout < checkInterval)
    {
        checkInterval = TransportConfig::GetConfig().SendTimeout;
    }

    WriteInfo(Constants::TcpTrace, traceId_, "connectionValidationTimer_ period: {0}", checkInterval);
    connectionValidationTimer_ = Timer::Create(
        "ConnectionValidation",
        [this] (TimerSPtr const&) { ConnectionValidationTimerCallback(); },
        false);

    connectionValidationTimer_->SetCancelWait();
    auto randomStartDelay = TimeSpan::FromMilliseconds(checkInterval.TotalMilliseconds() * (0.5 + 0.5 * Random().NextDouble()));
    connectionValidationTimer_->Change(randomStartDelay, checkInterval);
}

void TcpDatagramTransport::StartSendQueueCheckTimer_CallerHoldingWLock()
{
    TimeSpan checkInterval = TransportConfig::GetConfig().OutgoingMessageExpirationCheckInterval;
    if (checkInterval == TimeSpan::Zero || checkInterval == TimeSpan::MaxValue)
    {
        WriteInfo(
            Constants::TcpTrace, traceId_, 
            "{0}: periodic check of expired outgoing messages is disabled",
            listenAddress_);

        return;
    }

    sendQueueCheckTimer_ = Timer::Create(
        "SendQueueCheck",
        [this] (TimerSPtr const&) { SendQueueCheckCallback(); },
        false);

    sendQueueCheckTimer_->SetCancelWait();
    auto randomStartDelay = TimeSpan::FromMilliseconds(checkInterval.TotalMilliseconds() * (0.5 + 0.5 * Random().NextDouble()));
    sendQueueCheckTimer_->Change(randomStartDelay, checkInterval);
}

vector<TcpSendTargetSPtr> TcpDatagramTransport::CreateSendTargetSnapshot()
{
    AcquireReadLock lockInScope(lock_);
    return CreateSendTargetSnapshot_CallerHoldingLock();
}


void TcpDatagramTransport::SendQueueCheckCallback()
{
    auto sendTargets = CreateSendTargetSnapshot();
    StopwatchTime now = Stopwatch::Now();
    for (auto const & target : sendTargets)
    {
        target->PurgeExpiredOutgoingMessages(now);
    }
}

Common::ErrorCode TcpDatagramTransport::SetSecurity(SecuritySettings const & securitySettings)
{
    auto newTransportSecurity = std::make_shared<TransportSecurity>(clientOnly_);
    auto errorCode = newTransportSecurity->Set(securitySettings);
    if (!errorCode.IsSuccess())
    {
        trace.SecuritySettingsSetFailure(traceId_, securitySettings.ToString(), errorCode);
        return errorCode;
    }

    vector<TcpSendTargetSPtr> keepAlive; // To avoid TcpSendTarget destruction under lock
    {
        AcquireWriteLock grab(lock_);

        if (stopping_) return errorCode;

        newTransportSecurity->CopyNonSecuritySettings(*security_);

        errorCode = security_->CanUpgradeTo(*newTransportSecurity);
        if (!errorCode.IsSuccess())
        {
            trace.SecuritySettingsSetFailure(traceId_, securitySettings.ToString(), errorCode);
            return errorCode;
        }

        if (!security_->SessionExpirationEnabled())
        {
            newTransportSecurity->DisableSecureSessionExpiration();
        }

        security_ = std::move(newTransportSecurity);

        keepAlive.reserve(namedSendTargets_.size() + anonymousSendTargets_.size());

        // Update all named targets.
        for (auto iter = namedSendTargets_.cbegin(); iter != namedSendTargets_.cend(); ++ iter)
        {
            TcpSendTargetSPtr target = iter->second.lock();
            if (target)
            {
                target->UpdateSecurity(security_);
                keepAlive.emplace_back(move(target));
            }
        }

        // Update all anonymous targets
        for (auto iter = anonymousSendTargets_.cbegin(); iter != anonymousSendTargets_.cend(); ++ iter)
        {
            TcpSendTargetSPtr target = iter->second.lock();
            if (target)
            {
                target->UpdateSecurity(security_);
                keepAlive.emplace_back(move(target));
            }
        }

        StartCertMonitorTimerIfNeeded_CallerHoldingWLock();

        trace.SecuritySettingsSet(traceId_, security_->ToString());
    }

    return errorCode;
}

void TcpDatagramTransport::SetFrameHeaderErrorChecking(bool enabled)
{
    frameHeaderErrorCheckingEnabled_ = enabled;
    WriteInfo(
        Constants::TcpTrace, traceId_,
        "{0}: frameHeaderErrorCheckingEnabled_ set to {1}",
        listenAddress_, enabled);
}

void TcpDatagramTransport::SetMessageErrorChecking(bool enabled)
{
    messageErrorCheckingEnabled_ = enabled;
    WriteInfo(
        Constants::TcpTrace, traceId_,
        "{0}: messageErrorCheckingEnabled_ set to {1}",
        listenAddress_, enabled);
}

void TcpDatagramTransport::OnListenInstanceMessage(
    Message & message,
    IConnection & connection,
    ISendTarget::SPtr const & target)
{
    ListenInstance remoteListenInstance;
    message.GetBody<ListenInstance>(remoteListenInstance);
    if (!message.IsValid)
    {
        WriteWarning(
            Constants::TcpTrace, traceId_, 
            "{0}: failed to deserialize ListenInstance: {1:x}, dropping message {2}",
            listenAddress_, (uint32)(message.Status), message.TraceId());

        Common::Assert::TestAssert();
        target->TargetDown(); // This anonymous target should have only one connection
        return;
    }

    connection.OnConnectionReady();

#ifdef DBG
    TcpSendTarget* tcpSendTarget = dynamic_cast<TcpSendTarget*>(target.get());
#else
    TcpSendTarget* tcpSendTarget = static_cast<TcpSendTarget*>(target.get());
#endif

    if (target->IsAnonymous() && !remoteListenInstance.Address().empty())
    {
        // move connection to a named ISendTarget since we've got its FromAddress now
        TcpSendTargetSPtr namedSendTarget = InnerResolveTarget(
            remoteListenInstance.Address(),
            L"",
            L"",
            false,
            remoteListenInstance.Instance());

        if (namedSendTarget)
        {
            namedSendTarget->AcquireConnection(
                *tcpSendTarget,
                connection,
                remoteListenInstance);
        }
    }
    else
    {
        tcpSendTarget->UpdateConnectionInstance(connection, remoteListenInstance);
    }
}

std::shared_ptr<TcpSendTarget> TcpDatagramTransport::InnerResolveTarget(
    wstring const & address,
    wstring const & targetId,
    wstring const & sspiTarget,
    bool ensureSspiTarget,
    uint64 instance)
{
    std::wstring key = TargetAddressToTransportAddress(address);
    TcpSendTargetSPtr target;
    bool foundExistingTarget = false;
    {
        AcquireWriteLock grab(lock_);

        if (stopping_)
        {
            return nullptr;
        }

        wstring const * sspiTargetPtr = &sspiTarget;
        wstring derivedSspiTarget;
        if (ensureSspiTarget && sspiTarget.empty())
        {
            derivedSspiTarget = clientOnly_ ? security_->GetServerIdentity(address) : security_->GetPeerIdentity(address);
            sspiTargetPtr = &derivedSspiTarget;
        }

        auto location = namedSendTargets_.find(key);
        if (location != namedSendTargets_.end())
        {
            target = location->second.lock();
            if (target)
            {
                foundExistingTarget = true;
                if (!target->HasId() && !targetId.empty())
                {
                    target->UpdateId(targetId);
                }

                target->UpdateSspiTargetIfNeeded(*sspiTargetPtr);
            }
            else
            {
                // If locking the weak_ptr failed, we need to create a new target
                target = std::make_shared<TcpSendTarget>(*this, handler_, address, targetId, *sspiTargetPtr, instance, false, security_);
                location->second = std::weak_ptr<TcpSendTarget>(target);
            }
        }
        else
        {
            target = std::make_shared<TcpSendTarget>(*this, handler_, address, targetId, *sspiTargetPtr, instance, false, security_);
            namedSendTargets_.insert(NamedSendTargetPair(key, std::weak_ptr<TcpSendTarget>(target)));
            trace.Named_TargetAdded(
                traceId_,
                target->TraceId(),
                target->LocalAddress(),
                target->Address(),
                namedSendTargets_.size(),
                TcpSendTarget::GetObjCount());
        }
    }

    if (foundExistingTarget)
    {
        target->UpdateInstanceIfNeeded(instance);
        return target;
    }

    if (target)
    {
        target->Start();
    }

    return target;
}

// This is to be called only by the TcpSendTarget's destructor to cleanup weak_ptr
void TcpDatagramTransport::RemoveSendTargetEntry(TcpSendTarget const & target)
{
    {
        AcquireWriteLock grab(lock_);

        if (target.IsAnonymous())
        {
            if (anonymousSendTargets_.erase(&target) == 0) return;
        }
        else
        {
            if (namedSendTargets_.erase(target.Address()) == 0) return;
        }
    }

    trace.TargetRemoved(traceId_, target.TraceId(), target.LocalAddress(), target.Address());
}

ISendTarget::SPtr TcpDatagramTransport::Resolve(
    wstring const & address,
    wstring const & targetId,
    wstring const & sspiTarget,
    uint64 instance)
{
    if (!(starting_ || started_))
    {
        AcquireReadLock grab(lock_);
        Invariant(starting_ || started_);
    }

    return InnerResolveTarget(address, targetId, sspiTarget, true, instance);
}

Common::ErrorCode TcpDatagramTransport::SendOneWay(
    ISendTarget::SPtr const & target,
    MessageUPtr && message,
    TimeSpan expiration,
    TransportPriority::Enum priority)
{
    if (!started_)
    {
        AcquireReadLock grab(lock_);
        Invariant(started_);
    }

    return target->SendOneWay(
        std::move(message),
        (expiration != TimeSpan::MaxValue) ? expiration : outgoingMessageExpiration_,
        priority);
}

ErrorCode TcpDatagramTransport::SetDynamicListenPortForHostName(std::vector<IListenSocket::SPtr> & listenSockets)
{
    int trialMax = TransportConfig::GetConfig().DynamicListenPortTrialMax;
    int trial = 0;
    for (; trial < trialMax; ++ trial)
    {
        // There should be at most two dynamic endpoints, one for IPv4, one for IPv6
        for (int dynamicEndpointIndex = 0; dynamicEndpointIndex < 2; ++ dynamicEndpointIndex)
        {
            // Choose dynamic port with one of the two entries
            Endpoint & dynamicEndpoint = listenEndpoints_[dynamicEndpointIndex];
            WriteInfo(
                Constants::TcpTrace, traceId_,
                "getting actual listen port of dynmic endpoint [{0}]: '{1}'",
                dynamicEndpointIndex, dynamicEndpoint);

            auto listenSocket = CreateListenSocket(dynamicEndpoint);
            ErrorCode error = listenSocket->Open();

            if (error.IsSuccess())
            {
                listenSockets.push_back(move(listenSocket));

                if (listenEndpoints_.size() == 1)
                {
                    // There is no other dynamic endpoint, we are done!
                    return error;
                }

#ifdef PLATFORM_UNIX
                // Linux does not allow sharing listen port between IPv4 and IPv6 "any" address
                if (!listenEndpoints_.front().IsLoopback())
                {
                    WriteError(
                        Constants::TcpTrace, traceId_,
                        "skip the other listen endpoint as Linux does not allow sharing port between IPv4 and IPv6 ANY address");

                    return error;
                }
#endif
            }
            else if (error.IsWin32Error(WSAEAFNOSUPPORT))
            {
                WriteWarning(
                    Constants::TcpTrace, traceId_,
                    "endpoint {0}'s protocol is not supported",
                    dynamicEndpoint);

                continue;
            }
            else
            {
                return error;
            }

            unsigned short chosenPort = listenSockets[0]->ListenEndpoint().Port;
            WriteInfo(
                Constants::TcpTrace, traceId_,
                "dynmic endpoint {0} bound to {1}",
                dynamicEndpoint, listenSockets[0]->ListenEndpoint());

            int theOtherIndex = (~dynamicEndpointIndex) & 1;
            Endpoint & theOtherEndpoint = listenEndpoints_[theOtherIndex];
            WriteInfo(
                Constants::TcpTrace, traceId_,
                "trying port {0} on the other dynamic endpoint [{1}]: '{2}'",
                chosenPort, theOtherIndex, theOtherEndpoint);

            theOtherEndpoint.Port = chosenPort;
            listenSocket = CreateListenSocket(theOtherEndpoint);
            error = listenSocket->Open();
            if (error.IsSuccess())
            {
                listenSockets.push_back(move(listenSocket));
                return error;
            }

            if (error.IsWin32Error(WSAEAFNOSUPPORT))
            {
                WriteWarning(
                    Constants::TcpTrace, traceId_,
                    "endpoint {0}'s protocol is not supported",
                    theOtherEndpoint);

                return ErrorCodeValue::Success; // At least the other protocol is supported
            }

            if (!error.IsError(ErrorCodeValue::AddressAlreadyInUse))
            {
                return error;
            }

            WriteWarning(
                Constants::TcpTrace, traceId_,
                "{0} is already in use, will try the other protocol for dynamic port",
                theOtherEndpoint);

            // Clean up for next retry
            listenSockets.clear();
            listenEndpoints_[0].Port = 0;
            listenEndpoints_[1].Port = 0;
        }

        Sleep(static_cast<DWORD>(TransportConfig::GetConfig().DynamicListenPortRetryDelay.TotalMilliseconds()));
    }

    WriteError(
        Constants::TcpTrace, traceId_,
        "failed to choose dynamic port for {0} in {1} trials",
        listenAddress_, trial);

    return ErrorCodeValue::OperationFailed;
}

void TcpDatagramTransport::UpdateListenAddress(Endpoint const & endpoint)
{
    wstring host, inputPort;
    auto error = TcpTransportUtility::TryParseHostPortString(listenAddress_, host, inputPort);
    Invariant(error.IsSuccess());
    listenAddress_ = listenOnHostname_ ? wformatString("{0}:{1}", host, endpoint.Port) : endpoint.ToString();
}

ErrorCode TcpDatagramTransport::CheckConfig()
{
    TransportConfig & config = TransportConfig::GetConfig();
    if (!ResolveOptions::TryParse(config.ResolveOption, hostnameResolveOption_))
    {
        WriteError(
            Constants::TcpTrace, traceId_,
            "failed to parse configured address type {0}",
            config.ResolveOption);

        return ErrorCodeValue::OperationFailed;
    }

    EnsureSendQueueCapacity();
    return ErrorCode();
}

ErrorCode TcpDatagramTransport::Start(bool completeStart)
{
    ErrorCode error;
    {
        AcquireWriteLock grab(lock_);

        if (starting_ || started_ || stopping_)
            return ErrorCode(ErrorCodeValue::InvalidState);

        starting_ = true;

        error = CheckConfig();
        if (!error.IsSuccess()) return error;
    }

    vector<IListenSocket::SPtr> listenSockets;
    KFinally([&]
    {
        for(auto const & listenSocket : listenSockets)
        {
            if (listenSocket)
            {
                //there was a failure, abort listen socket before destruction
                listenSocket->Abort();
            }
        }
    });

    if (!clientOnly_)
    {
        Endpoint listenEndpoint;

        // Address string with IPv4/IPv6 ?
        ErrorCode parseError = TcpTransportUtility::TryParseEndpointString(this->listenAddress_, listenEndpoint);
        if (parseError.IsSuccess())
        {
            this->listenEndpoints_.push_back(listenEndpoint);
        }
        else if (!parseError.IsWin32Error(WSAEINVAL))
        {
            WriteError(
                Constants::TcpTrace, traceId_,
                "encountered unexpected error {0} when parsing listen address {1}",
                parseError, listenAddress_);

            return parseError;
        }
        else
        {
            error = TcpTransportUtility::TryResolveHostNameAddress(listenAddress_, hostnameResolveOption_, listenEndpoints_);
            if (!error.IsSuccess())
            {
                WriteError(
                    Constants::TcpTrace, traceId_,
                    "failed to resolve {0} as address type {1}: {2}",
                    listenAddress_, TransportConfig::GetConfig().ResolveOption, error);

                return error;
            }

            listenOnHostname_ = true;

            vector<wstring> parts;
            wstring delimiter = L":";
            StringUtility::Split(listenAddress_, parts, delimiter);
            if ((parts.size() != 2) || parts[1].empty())
            {
                WriteError(
                    Constants::TcpTrace, traceId_,
                    "listen address {0} is not in 'address:port' format",
                    listenAddress_);

                return ErrorCodeValue::InvalidAddress;
            }

            if (StringUtility::AreEqualCaseInsensitive(parts[0], L"localhost"))
            {
                for (auto iter = listenEndpoints_.cbegin(); iter != listenEndpoints_.cend(); ++ iter)
                {
                    if (!iter->IsLoopback())
                    {
                        WriteError(
                            Constants::TcpTrace, traceId_,
                            "{0} is not loopback address, mapping it to 'localhost' opens security holes",
                            *iter);

                        return ErrorCodeValue::OperationFailed;
                    }
                };
            }
            else
            {
                // input listen address has non-localhost hostname, we will listen on "any" address for allowed protocols

                if (!TcpTransportUtility::IsLocalEndpoint(listenEndpoints_[0]))
                {
                    WriteError(
                        Constants::TcpTrace, traceId_,
                        "listen address {0} contains a non-local hostname",
                        this->listenAddress_);

                    return ErrorCodeValue::InvalidAddress;
                }

                listenEndpoints_.clear();

                wstringstream stringStream(parts[1]);
                unsigned short port;
                if((stringStream >> port).fail())
                {
                    WriteError(
                        Constants::TcpTrace, traceId_,
                        "listen address {0} is not in 'address:port' format",
                        listenAddress_);

                    return ErrorCodeValue::InvalidAddress;
                }

                #pragma warning(disable : 24002) // We need to support IPv4 and IPv6 seperately
                if (Sockets::IsIPv4Supported() && (hostnameResolveOption_ != ResolveOptions::IPv6))
                {
                    SOCKADDR_IN sockAddrIn = {};
                    sockAddrIn.sin_family = AF_INET;
                    sockAddrIn.sin_port = htons(port);
                    sockAddrIn.sin_addr = in4addr_any;

                    Endpoint anyEndpoint((::sockaddr&)sockAddrIn);
                    listenEndpoints_.push_back(anyEndpoint);
                    WriteInfo(
                        Constants::TcpTrace, traceId_,
                        "will listen on 'any' address {0}",
                        anyEndpoint.ToString());
                }

                if (Sockets::IsIPv6Supported() && (hostnameResolveOption_ != ResolveOptions::IPv4))
                {
                    SOCKADDR_IN6 sockAddrIn = {};
                    sockAddrIn.sin6_family = AF_INET6;
                    sockAddrIn.sin6_port = htons(port);
                    sockAddrIn.sin6_addr = in6addr_any;

                    Endpoint anyEndpoint((::sockaddr&)sockAddrIn);
                    listenEndpoints_.push_back(anyEndpoint);
                    WriteInfo(
                        Constants::TcpTrace, traceId_,
                        "will listen on 'any' address {0}",
                        anyEndpoint.ToString());
                }
            }
        }

        ASSERT_IF(listenEndpoints_.empty(), "We should have at least one listen point");

        bool listenOnDynamicPort = listenEndpoints_.front().Port == 0;

        if (listenOnHostname_ && listenOnDynamicPort)
        {
            error = SetDynamicListenPortForHostName(listenSockets);
            if (!error.IsSuccess())
            {
                return error;
            }
        }
        else
        {
            for (auto iter = this->listenEndpoints_.cbegin(); iter != this->listenEndpoints_.cend(); ++ iter)
            {
                auto listenSocket = CreateListenSocket(*iter);
                error = listenSocket->Open();

                if (error.IsWin32Error(WSAEAFNOSUPPORT))
                {
                    WriteWarning(
                        Constants::TcpTrace, traceId_,
                        "listen address {0}'s protocol is not supported",
                        iter->ToString());

                    continue;
                }

                if (!error.IsSuccess())
                {
                    return error;
                }

                // Must update transport listen address before start listening, otherwise
                // we may send out incorrect address in outgoing ListenInstance message
                listenSockets.push_back(move(listenSocket));
            }
        }

        if (listenSockets.empty())
        {
            WriteError(Constants::TcpTrace, traceId_, "no listener opened successfully");
            return ErrorCodeValue::OperationFailed;
        }
    }

    {
        AcquireWriteLock grab(lock_);

        if (started_)
        {
            WriteInfo(Constants::TcpTrace, traceId_, "already started by another thread");
            return ErrorCode::Success();
        }

        if (stopping_) // There has been a racing between Start() and Stop()
        {
            WriteError(Constants::TcpTrace, traceId_, "Stop() has been called before Start() completes");
            return ErrorCodeValue::InvalidOperation;
        }

        for (auto const & listenSocket : listenSockets)
        {
            UpdateListenAddress(listenSocket->ListenEndpoint());
        }

        listenSockets_ = move(listenSockets);
        if (completeStart)
        {
            error = CompleteStart_CallerHoldingLock();
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        StartTimers_CallerHoldingWLock();
    }

    trace.Started(traceId_, listenAddress_, instance_);
    return ErrorCode::Success();
}

void TcpDatagramTransport::EnsureMessageHandlerOnSendTargets_CallerHoldingLock()
{
    auto sendTargets = CreateSendTargetSnapshot_CallerHoldingLock();

    // Some named targets may have been constructed before handler_ was set
    Invariant(handler_);
    for (auto const & target : sendTargets)
    {
        target->SetMessageHandler(handler_);
    }
}

ErrorCode TcpDatagramTransport::CompleteStart_CallerHoldingLock()
{
    if (!handler_)
    {
        WriteInfo(Constants::TcpTrace, traceId_, "no message handler set at starting, will use dummy handler");
        TcpDatagramTransportWPtr wptr = shared_from_this(); // Capture wptr so that dummy handler does not prevent destruction
        SetMessageHandler_CallerHoldingLock([wptr, this](MessageUPtr & message, ISendTarget::SPtr const & sender)
        {
            if (wptr.lock())
            {
                WriteInfo(
                    Constants::TcpTrace, traceId_,
                    "dummy handler: dropping message {0}, Actor = {1}, Action = '{2}', from {3}",
                    message->TraceId(),
                    message->Actor,
                    message->Action,
                    sender->Address());
            }
        });
        WriteInfo(Constants::TcpTrace, traceId_, "no message handler set at starting");
    }

    EnsureMessageHandlerOnSendTargets_CallerHoldingLock();

    ErrorCode error;
    if (!clientOnly_)
    {
        for (auto const & listenSocket : listenSockets_)
        {
            error = listenSocket->SubmitAccept();
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        WriteInfo("CompleteStart", traceId_, "start accepting connections");
    }

    starting_ = false;
    started_ = true;

    return error;
}

vector<TcpSendTargetSPtr> TcpDatagramTransport::CreateSendTargetSnapshot_CallerHoldingLock()
{
    vector<TcpSendTargetSPtr> sendTargets;
    sendTargets.reserve(namedSendTargets_.size() + anonymousSendTargets_.size());

    for (auto const & pair : namedSendTargets_)
    {
        auto target = pair.second.lock();
        if (target)
        {
            sendTargets.emplace_back(move(target));
        }
    }

    for (auto const & pair : anonymousSendTargets_)
    {
        auto target = pair.second.lock();
        if (target)
        {
            sendTargets.emplace_back(move(target));
        }
    }

    return sendTargets;
}

ErrorCode TcpDatagramTransport::CompleteStart()
{
    AcquireWriteLock grab(lock_);

    if (!starting_)
    {
        WriteError(Constants::TcpTrace, traceId_, "not in starting state, started_ = {0}, stopping_ = {1}", started_, stopping_);
        return ErrorCodeValue::InvalidState;
    }

    return CompleteStart_CallerHoldingLock();
}

// This is to verify we are not leaking SendTargets and weak_ptr's
size_t TcpDatagramTransport::SendTargetCount() const
{
    AcquireReadLock grab(lock_);

    return namedSendTargets_.size() + anonymousSendTargets_.size();
}

bool TcpDatagramTransport::IsClientOnly() const
{
    return clientOnly_;
}

wstring const & TcpDatagramTransport::ListenAddress() const
{
    return listenAddress_;
}

wstring const & TcpDatagramTransport::get_IdString() const
{
    return this->id_;
}

void TcpDatagramTransport::DisableThrottle()
{
    Invariant(started_ == false);
    this->throttle_ = nullptr;
}

Throttle* TcpDatagramTransport::GetThrottle()
{
    return throttle_;
}

void TcpDatagramTransport::AllowThrottleReplyMessage()
{
    this->allowThrottleReplyMessage_ = true;
}

bool TcpDatagramTransport::AllowedToThrottleReply() const
{
    return allowThrottleReplyMessage_;
}

void TcpDatagramTransport::StartTimers_CallerHoldingWLock()
{
    StartListenerStateTraceTimer_CallerHoldingWLock();
    StartConnectionValicationTimer_CallerHoldingWLock();
    StartSendQueueCheckTimer_CallerHoldingWLock();
    StartCertMonitorTimerIfNeeded_CallerHoldingWLock();
}

void TcpDatagramTransport::CancelTimers()
{
    if (listenerStateTraceTimer_)
    {
        listenerStateTraceTimer_->Cancel();
    }

    if (connectionValidationTimer_)
    {
        connectionValidationTimer_->Cancel();
    }

    if (sendQueueCheckTimer_)
    {
        sendQueueCheckTimer_->Cancel();
    }

    if (certMonitorTimer_)
    {
        certMonitorTimer_->Cancel();
    }
}

void TcpDatagramTransport::Test_Reset()
{
    vector<NamedSendTargetPair> namedSendTargets;
    vector<AnonymousSendTargetPair> anonymousSendTargets;
    {
        AcquireWriteLock grab(lock_);

        namedSendTargets.reserve(namedSendTargets_.size());
        namedSendTargets = vector<NamedSendTargetPair>(namedSendTargets_.begin(), namedSendTargets_.end());

        anonymousSendTargets.reserve(anonymousSendTargets_.size());
        anonymousSendTargets = vector<AnonymousSendTargetPair>(anonymousSendTargets_.begin(), anonymousSendTargets_.end());
    }

    for (auto & entry : namedSendTargets)
    {
        auto target = entry.second.lock();
        if (target)
        {
            target->Reset();
        }
    }

    for (auto & entry : anonymousSendTargets)
    {
        auto target = entry.second.lock();
        if (target)
        {
            target->Reset();
        }
    }
}

void TcpDatagramTransport::Stop(TimeSpan timeout)
{
    vector<TcpSendTargetSPtr> sendTargets;
    vector<IListenSocket::SPtr> listenSockets;
    {
        AcquireWriteLock grab(lock_);

        if (stopping_) return;

        stopping_ = true;
        trace.Stopped(traceId_, listenAddress_);

        RemoveMessageHandlerCallerHoldingLock();

        connectionAcceptedHandler_ = nullptr;
        disconnectEvent_.Close();
        connectionFaultHandler_ = nullptr;
        security_->RemoveClaimsHandler();

        sendTargets = CreateSendTargetSnapshot_CallerHoldingLock();

        listenSockets = std::move(this->listenSockets_);
    }

    CancelTimers();

    for (auto const & listenSocket : listenSockets)
    {
        listenSocket->Close();
    }

    // close all the send targets we know about
    for (auto const & target : sendTargets)
    {
       target->Close(); //connections will be aborted
    }

    if (timeout == TimeSpan::Zero) return; // no need to wait for cleanup completion 

    WriteInfo(Constants::TcpTrace, traceId_, "Stop: wait for cleanup completion");
    TimeoutHelper th(timeout);
    while (!th.IsExpired)
    {
        {
            AcquireReadLock grab(lock_);

            if (namedSendTargets_.empty() && anonymousSendTargets_.empty())
            {
                WriteInfo(Constants::TcpTrace, traceId_, "Stop: cleanup completed: target count = 0");
                return;
            }
        }

        size_t connectionCount = 0;
        for (auto const & target : sendTargets)
        {
            connectionCount += target->ConnectionCount();
            if (connectionCount > 0) break;
        }

        if (connectionCount == 0)
        {
            WriteInfo(Constants::TcpTrace, traceId_, "Stop: cleanup completed: connection count = 0");
            return;
        }

        Sleep(30);
    }

    WriteInfo(
        Constants::TcpTrace, traceId_,
        "namedSendTargets_.size = {0}, anonymousSendTargets_.size = {1}",
        namedSendTargets_.size(), anonymousSendTargets_.size());

    for (auto const & target : sendTargets)
    {
        auto connectionCount =  target->ConnectionCount();
        if (connectionCount > 0)
        {
            WriteInfo(
                Constants::TcpTrace, traceId_,
                "Stop: target {0}: connectionCount = {1}",
                TextTracePtrAs(target.get(), ISendTarget), connectionCount);
        }
    }

    TRACE_AND_TESTASSERT(WriteError, Constants::TcpTrace, traceId_, "Stop did not complete within {0}", timeout);
}

TcpDatagramTransportSPtr TcpDatagramTransport::CreateClient(std::wstring const & id, wstring const & owner)
{
    return make_shared<TcpDatagramTransport>(id, owner);
}

std::shared_ptr<TcpDatagramTransport> TcpDatagramTransport::Create(std::wstring const & address, std::wstring const & id, wstring const & owner)
{
    auto result = make_shared<TcpDatagramTransport>(address, id, owner);
    AcceptThrottle::GetThrottle()->AddListener(result);
    return result;
}

wstring const & TcpDatagramTransport::Owner() const
{
    return owner_;
}

void TcpDatagramTransport::SubmitAccept(IListenSocket & listenSocket)
{
    AcquireWriteLock grab(lock_);

    if (stopping_)
    {
        return;
    }

    listenSocket.SubmitAccept();
}

TcpConnectionSPtr TcpDatagramTransport::OnConnectionAccepted(Socket & socket, Endpoint const & remoteEndpoint)
{
    ConnectionAcceptedHandler handler = nullptr;
    std::shared_ptr<TcpSendTarget> anonymousSendTarget;
    {
        AcquireWriteLock grab(lock_);

        if (!stopping_)
        {
            handler = connectionAcceptedHandler_;

            anonymousSendTarget = std::make_shared<TcpSendTarget>(
                *this,
                handler_,
                remoteEndpoint.ToString(),
                L"",
                L"",
                0,
                true,
                security_);

            this->anonymousSendTargets_.insert(
                AnonymousSendTargetPair(
                    anonymousSendTarget.get(),
                    std::weak_ptr<TcpSendTarget>(anonymousSendTarget)));

            trace.Anonymous_TargetAdded(
                traceId_,
                anonymousSendTarget->TraceId(),
                anonymousSendTarget->LocalAddress(),
                anonymousSendTarget->Address(),
                anonymousSendTargets_.size(),
                TcpSendTarget::GetObjCount());
        }
    }

    if (anonymousSendTarget)
    {
        auto connection = anonymousSendTarget->AddAcceptedConnection(socket);

        if (connection && handler)
        {
            handler(*anonymousSendTarget);
        }

        return connection;
    }

    return nullptr;
}

void TcpDatagramTransport::AcceptComplete(IListenSocket & listenSocket, ErrorCode const & error)
{
    if (test_acceptDisabled_)
    {
        WriteInfo(Constants::TcpTrace, traceId_, "test_acceptDisabled_: close accepted socket");
        listenSocket.AcceptedSocket().Close(SocketShutdown::Both);
        return;
    }

    if (error.IsSuccess())
    {
        auto const & remoteEndpoint = listenSocket.AcceptedRemoteEndpoint();
        Socket accepted(std::move(listenSocket.AcceptedSocket()));
        trace.ConnectionAccepted(traceId_, ListenAddress(), remoteEndpoint.ToString());
        if (AcceptThrottle::GetThrottle()->OnConnectionAccepted(
            *this, listenSocket, accepted, remoteEndpoint, listenSocket.ListenEndpoint().Port))
        {
            SubmitAccept(listenSocket);
        }

        return;
    }

    // Post error handling to thread pool to avoid deadlock
    try
    {
        auto thisSPtr = shared_from_this();
        Threadpool::Post([thisSPtr, error, &listenSocket]() { thisSPtr->OnAcceptFailure(error, listenSocket); });
    }
    catch (bad_weak_ptr const &)
    {
        // destructor calls Stop()
        WriteInfo(
            Constants::TcpTrace, traceId_,
            "{0}: destructing, no need to handle accept failure {1}",
            ListenAddress(),
            error);
    }
}

void TcpDatagramTransport::OnAcceptFailure(ErrorCode const & error, IListenSocket & listenSocket)
{
    {
        AcquireReadLock grab(lock_);
        if (stopping_)
        {
            WriteInfo(Constants::TcpTrace, traceId_, "{0}: Accept completed with error {1}", ListenAddress(), error);
            return;
        }
    }

    WriteWarning(Constants::TcpTrace, traceId_, "{0}: Accept failed: error {1}", ListenAddress(), error);
    
    SubmitAcceptWithDelay(listenSocket, TransportConfig::GetConfig().AcceptRetryDelay);
}

void TcpDatagramTransport::SubmitAcceptWithDelay(IListenSocket & listenSocket, TimeSpan delay)
{
    auto thisSPtr = shared_from_this();
    Threadpool::Post(
        [thisSPtr, &listenSocket]()
        { 
            thisSPtr->SubmitAccept(listenSocket);
        },
        delay);
}

void TcpDatagramTransport::Test_DisableAccept()
{
    AcquireWriteLock grab(lock_);

    test_acceptDisabled_ = true;

    for (auto const & listenSocket : listenSockets_)
    {
        listenSocket->SuspendAccept();
    }
}

bool TcpDatagramTransport::Test_AcceptSuspended() const
{
    AcquireReadLock grab(lock_);

    for (auto const & listenSocket : listenSockets_)
    {
        if (listenSocket->AcceptSuspended())
        {
            return true;
        }
    }

    return false;
}

// Both suspend and resume must happen under lock_ to avoid losing accept loop
void TcpDatagramTransport::ResumeAcceptIfNeeded()
{
    bool falure = false;
    {
        AcquireWriteLock grab(lock_);

        if (stopping_) return;

        if (test_acceptDisabled_) return;

        for (auto & listenSocket : listenSockets_)
        {
            if (!listenSocket->AcceptSuspended()) continue;

            ErrorCode error = listenSocket->SubmitAccept();
            if (error.IsSuccess())
            {
                continue;
            }

            trace.AcceptResumeFailed(traceId_, listenAddress_, error, TransportConfig::GetConfig().AcceptRetryDelay);
            falure = true;
        }
    }

    if (!falure) return;

    auto thisSPtr = shared_from_this();
    Threadpool::Post(
        [thisSPtr]() { thisSPtr->ResumeAcceptIfNeeded(); },
        TransportConfig::GetConfig().AcceptRetryDelay);
}

void TcpDatagramTransport::OnMessageReceived(
    MessageUPtr && message,
    IConnection & connection,
    ISendTarget::SPtr const & target)
{
    Invariant(message->Actor == Actor::Transport);

    if (message->Action.empty())
    {
        OnListenInstanceMessage(*message, connection, target);
    }
    else
    {
        WriteWarning(
            Constants::TcpTrace, traceId_,
            "{0}: actor = {1}, unknown action = {2}, dropping message {3}",
            listenAddress_, message->Actor, message->Action, message->TraceId());
    }
}

void TcpDatagramTransport::OnConnectionFault(ISendTarget const & target, ErrorCode const & fault)
{
    ConnectionFaultHandler handler = nullptr;
    {
        AcquireReadLock grab(lock_);

        if (stopping_) return;

        handler = connectionFaultHandler_;
    }

    if (handler)
    {
        handler(target, fault);
    }

    disconnectEvent_.Fire(DisconnectEventArgs(&target, fault));
}

void TcpDatagramTransport::SetConnectionAcceptedHandler(ConnectionAcceptedHandler const & handler)
{
    AcquireWriteLock grab(lock_);

    if (stopping_) return;

    ASSERT_IF(connectionAcceptedHandler_, "Handler already set");
    connectionAcceptedHandler_ = handler;
}

void TcpDatagramTransport::RemoveConnectionAcceptedHandler()
{
    AcquireWriteLock grab(lock_);
    connectionAcceptedHandler_ = nullptr;
}

IDatagramTransport::DisconnectHHandler TcpDatagramTransport::RegisterDisconnectEvent(DisconnectEventHandler eventHandler)
{
    AcquireWriteLock lock(lock_);

    if (stopping_) return EventT<DisconnectEventArgs>::InvalidHHandler;

    return disconnectEvent_.Add(eventHandler);
}

bool TcpDatagramTransport::UnregisterDisconnectEvent(DisconnectHHandler hHandler)
{
    AcquireWriteLock lock(lock_);
    return disconnectEvent_.Remove(hHandler);
}

void TcpDatagramTransport::SetConnectionFaultHandler(ConnectionFaultHandler const & handler)
{
    AcquireWriteLock grab(lock_);

    if (stopping_) return;

    ASSERT_IF(connectionFaultHandler_, "Handler already set");
    connectionFaultHandler_ = handler;
}

void TcpDatagramTransport::RemoveConnectionFaultHandler()
{
    AcquireWriteLock grab(lock_);
    connectionFaultHandler_ = nullptr;
}

void TcpDatagramTransport::DisableSecureSessionExpiration()
{
    ASSERT_IF(started_, "DisableSecureSessionExpiration must be set before starting");
    this->security_->DisableSecureSessionExpiration();
}

ULONG TcpDatagramTransport::PerTargetSendQueueLimit() const
{
    return perTargetSendQueueSizeLimitInBytes_;
}

TransportSecuritySPtr TcpDatagramTransport::Security() const
{
    AcquireReadLock grab(lock_);
    return security_;
}

void TcpDatagramTransport::EnsureSendQueueCapacity()
{
    auto minSendQueueCapacity = security_->MaxOutgoingFrameSize() * TransportConfig::GetConfig().MinSendQueueCapacityInMessageCount;
    if ((0 < perTargetSendQueueSizeLimitInBytes_) && (perTargetSendQueueSizeLimitInBytes_ < minSendQueueCapacity))
    {
        perTargetSendQueueSizeLimitInBytes_ = minSendQueueCapacity;
        WriteInfo(
            Constants::TcpTrace, traceId_,
            "adjusted perTargetSendQueueSizeLimitInBytes_ to {0} to accommodate at least {1} message at max size {2}",
            perTargetSendQueueSizeLimitInBytes_,
            TransportConfig::GetConfig().MinSendQueueCapacityInMessageCount,
            security_->MaxOutgoingFrameSize());
    }
}

void TcpDatagramTransport::SetRecvLimit(vector<TcpSendTargetSPtr> const & sendTargets, uint maxFrameSize)
{
    for (auto const & target : sendTargets)
    {
        target->SetRecvLimit(maxFrameSize);
    }
}

void TcpDatagramTransport::SetSendLimits(vector<TcpSendTargetSPtr> const & sendTargets, uint maxFrameSize, uint sendQueueLimit)
{
    for (auto const & target : sendTargets)
    {
        target->SetSendLimits(maxFrameSize, sendQueueLimit);
    }
}

ErrorCode TcpDatagramTransport::SetPerTargetSendQueueLimit(ULONG limitInBytes)
{
    WriteInfo(
        Constants::TcpTrace, traceId_,
        "updating PerTargetSendQueueLimit from {0} to {1}",
        perTargetSendQueueSizeLimitInBytes_, limitInBytes);

    uint maxFrameSize = 0;
    uint perTargetSendQueueSizeLimitInBytes = 0;
    vector<TcpSendTargetSPtr> sendTargets;
    {
        AcquireWriteLock grab(lock_);

        perTargetSendQueueSizeLimitInBytes_ = limitInBytes;
        EnsureSendQueueCapacity();

        maxFrameSize = security_->MaxOutgoingFrameSize();
        perTargetSendQueueSizeLimitInBytes = perTargetSendQueueSizeLimitInBytes_;
        sendTargets = CreateSendTargetSnapshot_CallerHoldingLock();
    }

    SetSendLimits(sendTargets, maxFrameSize, perTargetSendQueueSizeLimitInBytes);
    return ErrorCode();
}

ErrorCode TcpDatagramTransport::SetOutgoingMessageExpiration(TimeSpan expiration)
{
    if (started_)
    {
        WriteError(
            Constants::TcpTrace, traceId_,
            "{0}: outgoingMessageExpiration_ must be set before starting",
            listenAddress_);

        return ErrorCodeValue::InvalidOperation;
    }

    outgoingMessageExpiration_ = (expiration > TimeSpan::Zero) ? expiration : TimeSpan::MaxValue;
    trace.OutgoingMessageExpirationSet(traceId_, outgoingMessageExpiration_);
    return ErrorCode();
}

void TcpDatagramTransport::SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata && metadata)
{
    AcquireWriteLock grab(lock_);

    if (stopping_) return;

    security_->SetClaimsRetrievalMetadata(move(metadata));
}

void TcpDatagramTransport::SetClaimsRetrievalHandler(TransportSecurity::ClaimsRetrievalHandler const & handler)
{
    AcquireWriteLock grab(lock_);

    if (stopping_) return;

    security_->SetClaimsRetrievalHandler(handler);
}

void TcpDatagramTransport::RemoveClaimsRetrievalHandler()
{
    AcquireWriteLock grab(lock_);
    security_->RemoveClaimsRetrievalHandler();
}

void TcpDatagramTransport::SetClaimsHandler(TransportSecurity::ClaimsHandler const & handler)
{
    AcquireWriteLock grab(lock_);

    if (stopping_) return;

    security_->SetClaimsHandler(handler);
}

void TcpDatagramTransport::RemoveClaimsHandler()
{
    AcquireWriteLock grab(lock_);
    security_->RemoveClaimsHandler();
}

void TcpDatagramTransport::SetMaxIncomingFrameSize(ULONG maxFrameSize)
{
    WriteInfo(Constants::TcpTrace, traceId_, "SetMaxIncomingFrameSize({0})", maxFrameSize);

    uint maxFrameSizeRetrieved = 0;
    vector<TcpSendTargetSPtr> sendTargets;
    {
        AcquireWriteLock grab(lock_);

        security_->SetMaxIncomingFrameSize(maxFrameSize);
        maxFrameSizeRetrieved = security_->MaxIncomingFrameSize();
        sendTargets = CreateSendTargetSnapshot_CallerHoldingLock();
    }

    SetRecvLimit(sendTargets, maxFrameSizeRetrieved);
}

void TcpDatagramTransport::SetMaxOutgoingFrameSize(ULONG maxFrameSize)
{
    WriteInfo(Constants::TcpTrace, traceId_, "SetMaxOutgoingFrameSize({0})", maxFrameSize);

    uint maxFrameSizeRetrieved = 0;
    uint perTargetSendQueueSizeLimitInBytes = 0;
    vector<TcpSendTargetSPtr> sendTargets;
    {
        AcquireWriteLock grab(lock_);

        security_->SetMaxOutgoingFrameSize(maxFrameSize);
        EnsureSendQueueCapacity();

        maxFrameSizeRetrieved = security_->MaxOutgoingFrameSize();
        perTargetSendQueueSizeLimitInBytes = perTargetSendQueueSizeLimitInBytes_;
        sendTargets = CreateSendTargetSnapshot_CallerHoldingLock();
    }

    SetSendLimits(sendTargets, maxFrameSizeRetrieved, perTargetSendQueueSizeLimitInBytes);
}

TimeSpan TcpDatagramTransport::KeepAliveTimeout() const
{
    return keepAliveTimeout_;
}

void TcpDatagramTransport::SetKeepAliveTimeout(TimeSpan timeout)
{
    ASSERT_IF(started_, "keepAliveTimeout_ must be set before starting");

    keepAliveTimeout_ = timeout;
    trace.TcpKeepAliveTimeoutSet(traceId_, keepAliveTimeout_.TotalSeconds());
    if ((keepAliveTimeout_ > TimeSpan::Zero) && (connectionIdleTimeout_ > TimeSpan::Zero) && (keepAliveTimeout_ > connectionIdleTimeout_))
    {
        WriteWarning(
            Constants::TcpTrace, traceId_, 
            "{0}: keepAliveTimeout_ {1} is greater than connectionIdleTimeout_ {2}, keepalive probably will not work",
            listenAddress_, keepAliveTimeout_, connectionIdleTimeout_);
    }
}

uint64 TcpDatagramTransport::GetObjCount()
{
    return objCount_.load();
}

uint TcpDatagramTransport::RecvBufferSize() const
{
    return recvBufferSize_;
}

void TcpDatagramTransport::SetRecvBufferSize(uint size)
{
    recvBufferSize_ = size;
}

TimeSpan TcpDatagramTransport::ConnectionOpenTimeout() const
{
    return connectionOpenTimeout_;
}

void TcpDatagramTransport::SetConnectionOpenTimeout(TimeSpan timeout)
{
    connectionOpenTimeout_ = timeout;
}

wstring const & TcpDatagramTransport::TraceId() const
{
    return traceId_;
}

void TcpDatagramTransport::DisableListenInstanceMessage()
{
    shouldSendListenInstance_ = false;
    WriteInfo(Constants::TcpTrace, traceId_, "ListenInstance message disabled"); 
} 

void TcpDatagramTransport::RefreshIssuersIfNeeded()
{
    /* The list here is only refreshed if public keys of the respective issuer certificates change in the computer certificate store. 
       For config upgrades, this code path is not executed - the config handlers are triggered and a new TransportSecurity object is set.
    */

    X509IdentitySet latestRemoteIssuerPublicKeys;
    bool isRemoteIssuerChanged = false;
    SecuritySettings newSettings;

    {
        AcquireReadLock grab(lock_); // read lock for reading security_

        // Check if new remote issuers are added to certificate store
        if (!security_->Settings().RemoteCertIssuers().IsEmpty())
        {
            X509IdentitySet currentRemoteIssuerPublicKeys = security_->Settings().RemoteX509Names().GetAllIssuers();
            security_->Settings().RemoteCertIssuers().GetPublicKeys(latestRemoteIssuerPublicKeys);

            if (currentRemoteIssuerPublicKeys != latestRemoteIssuerPublicKeys)
            {
                isRemoteIssuerChanged = true;
                newSettings = SecuritySettings(security_->Settings());
            }
        }
    } // release ReadLock since its not required anymore. SetSecurity acquires a WriteLock.

    if (isRemoteIssuerChanged)
    {
        newSettings.SetRemoteCertIssuerPublicKeys(latestRemoteIssuerPublicKeys);
        if (security_->Settings().IsClientRoleInEffect())
        {
            newSettings.SetAdminClientCertIssuerPublicKeys(latestRemoteIssuerPublicKeys);
        }      
        SetSecurity(newSettings);
    }
}
