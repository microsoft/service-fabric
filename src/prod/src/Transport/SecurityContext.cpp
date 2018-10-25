// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace std;

namespace
{
    const StringLiteral TraceType("SecurityContext");
    atomic_uint64 objCount(0);
}

SecurityContextSPtr SecurityContext::Create(
    IConnectionSPtr const & connection,
    TransportSecuritySPtr const & transportSecurity,
    wstring const & targetName,
    ListenInstance localListenInstance)
{
    SecurityContextSPtr result;

    if (SecurityContextSsl::Supports(transportSecurity->SecurityProvider))
    {
        result = make_shared<SecurityContextSsl>(connection, transportSecurity, targetName, localListenInstance);
    }
#ifndef PLATFORM_UNIX
    else if (SecurityContextWin::Supports(transportSecurity->SecurityProvider))
    {
        result = make_shared<SecurityContextWin>(connection, transportSecurity, targetName, localListenInstance);
    }
#endif

    if (result)
    {
        result->Initialize();
    }

    return result;
}

SecurityContext::SecurityContext(
    IConnectionSPtr const & connection,
    TransportSecuritySPtr const & transportSecurity,
    wstring const & targetName,
    ListenInstance localListenInstance)
    : connection_(connection)
    , id_(connection->TraceId())
    , inbound_(connection->IsInbound())
    , shouldSendServerAuthHeader_(connection->IsInbound())
    , shouldCheckForIncomingServerAuthHeader_(!connection->IsInbound())
    , remotePromisedToSendConnectionAuthStatus_(false)
    , transportSecurity_(transportSecurity)
    , targetName_(targetName)
    , securityRequirements_(transportSecurity->SecurityRequirements)
    , negotiationState_(S_FALSE)
    , connectionAuthorizationState_(STATUS_PENDING)
    , roleMask_(transportSecurity_->Settings().DefaultRemoteRole(inbound_))
    , sessionExpiration_(StopwatchTime::MaxValue)
    , framingProtectionEnabled_(transportSecurity_->Settings().FramingProtectionEnabled())
    , outgoingSecNegoHeader_(framingProtectionEnabled_, localListenInstance, connection->IncomingFrameSizeLimit())
{
    SecInvalidateHandle(&hSecurityContext_);

    if (inbound_)
    {
        securityRequirements_ |= (ASC_REQ_ALLOCATE_MEMORY | ASC_REQ_MUTUAL_AUTH | ASC_REQ_REPLAY_DETECT);

        if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign 
            || transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::Sign) 
        {
            securityRequirements_ |= ASC_REQ_INTEGRITY;
        }

        if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign
            || transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign) 
        {
            securityRequirements_ |= (ASC_REQ_INTEGRITY | ASC_REQ_CONFIDENTIALITY);
        }
    }
    else
    {
        securityRequirements_ |= (ISC_REQ_ALLOCATE_MEMORY | ISC_REQ_REPLAY_DETECT);

        if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::Sign
            || transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::Sign) 
        {
            securityRequirements_ |= ISC_REQ_INTEGRITY;
        }

        if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign
            || transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign) 
        {
            securityRequirements_ |= (ISC_REQ_INTEGRITY | ISC_REQ_CONFIDENTIALITY);
        }
    }

    CreateSessionExpirationTimer();
}

void SecurityContext::Initialize()
{
    OnInitialize();
    trace.SecurityContextCreated(
        id_,
        transportSecurity_->SecurityProvider,
        TraceThis,
        ++objCount,
        inbound_,
        framingProtectionEnabled_,
        credentials_.front()->Expiration().ToString());
}

ErrorCode SecurityContext::FaultToReport() const
{
    wstring errorMsg = wformatString(
        "{0}: {1}",
        inbound_ ? ErrorCodeValue::ConnectionDenied : ErrorCodeValue::ServerAuthenticationFailed,
        ErrorCodeValue::Enum(connectionAuthorizationState_));

    if (IsConnectionAuthorizationFailureRetryable())
    {
        return ErrorCode(ErrorCodeValue::Enum(connectionAuthorizationState_), move(errorMsg));
    }

    // FabricClient stops retrying on the following error code values.
    return ErrorCode(
        inbound_ ? ErrorCodeValue::ConnectionDenied : ErrorCodeValue::ServerAuthenticationFailed,
        move(errorMsg));
}

void SecurityContext::CreateSessionExpirationTimer()
{
    static const StringLiteral SessionExpirationTimerTag("SessionExpiration");
    auto connectionWPtr = connection_;
    sessionExpirationTimer_ = Timer::Create(
        SessionExpirationTimerTag,
        [connectionWPtr, this](TimerSPtr const&)
        {
            if (auto connection = connectionWPtr.lock())
            {
                sessionExpiration_ = StopwatchTime::MaxValue;
                connection->OnSessionExpired();
            }
        });
}

SecurityContext::~SecurityContext()
{
    trace.SecurityContextDestructing(id_, TraceThis, --objCount);

    sessionExpirationTimer_->Cancel();

    if (SecIsValidHandle(&hSecurityContext_))
    {
        DeleteSecurityContext(&hSecurityContext_);
    }
}

_Use_decl_annotations_
vector<byte> SecurityContext::GetMergedMessageBody(MessageUPtr const & inputMessage)
{
    vector<byte> merged;
    if (!inputMessage)
    {
        return merged;
    }

    size_t securityTokenSize = 0;
    vector<const_buffer> inputBufferList;
    inputMessage->GetBody(inputBufferList);
    for (auto const & buffer : inputBufferList)
    {
        securityTokenSize += buffer.size();
    }

    merged.resize(securityTokenSize);
    size_t offset = 0;

    for (auto const & buffer : inputBufferList)
    {
        KMemCpySafe(merged.data() + offset, merged.size() - offset, buffer.buf, buffer.len);
        offset += buffer.len;
    }

    return merged;
}

void SecurityContext::SearchForServerAuthHeaderOnFirstIncomingMessage(MessageUPtr const & incomingNegoMessage)
{
    if (!shouldCheckForIncomingServerAuthHeader_ || !incomingNegoMessage) return;

    shouldCheckForIncomingServerAuthHeader_ = false;

    ServerAuthHeader serverAuthHeader;
    if (incomingNegoMessage->Headers.TryReadFirst(serverAuthHeader))
    {
        remotePromisedToSendConnectionAuthStatus_ = serverAuthHeader.WillSendConnectionAuthStatus();
        WriteInfo(TraceType, id_, "ServerAuthHeader received: {0}", serverAuthHeader);

        if (transportSecurity_)
        {
            auto metadata = serverAuthHeader.TakeClaimsRetrievalMetadata();
            if (metadata)
            {
                transportSecurity_->SetClaimsRetrievalMetadata(move(*metadata));
            }
        }
    }
}

void SecurityContext::AddServerAuthHeaderIfNeeded(Message & message)
{
    if (!shouldSendServerAuthHeader_) return;

    ServerAuthHeader header(true);

    if (inbound_ && transportSecurity_ && ((securityRequirements_ & ASC_REQ_MUTUAL_AUTH) == 0))
    {
        auto const & metadata = transportSecurity_->GetClaimsRetrievalMetadata();

        if (metadata)
        {
            if (metadata->IsValid)
            {
                WriteInfo(TraceType, id_, "sending claims retrieval metadata: {0}", *metadata);
                header.SetClaimsRetrievalMetadata(metadata);
            }
            else
            {
                TRACE_AND_TESTASSERT(WriteWarning, TraceType, id_, "invalid claims retrieval metadata: {0}", *metadata);
            }
        }
    }

    message.Headers.Add(header);
    shouldSendServerAuthHeader_ = false;
}

bool SecurityContext::ShouldWaitForConnectionAuthStatus() const
{
    bool shouldWait = transportSecurity_->IsClientOnly() && remotePromisedToSendConnectionAuthStatus_;
    WriteInfo(
        TraceType, id_,
        "ShouldWaitForConnectionAuthStatus: IsClientOnly = {0}, remotePromisedToSendConnectionAuthStatus_ = {1}, shouldWait = {2}",
        transportSecurity_->IsClientOnly(), remotePromisedToSendConnectionAuthStatus_, shouldWait); 

    return shouldWait;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContext::NegotiateSecurityContext(MessageUPtr const & input, MessageUPtr & output)
{
    output.reset();

    SECURITY_STATUS state = negotiationState_;
    if ((state != S_FALSE) && (state != SEC_I_CONTINUE_NEEDED))
    {
        WriteWarning(
            TraceType, id_,
            "unexpected security negotiation state 0x{0:x}",
            (uint)state);

        return state;
    }

    SearchForServerAuthHeaderOnFirstIncomingMessage(input);

    // Setup output buffers
    SecBuffer outputBuffers[1];

    SecBufferDesc outputBufferDesc;
    outputBufferDesc.ulVersion = SECBUFFER_VERSION;
    outputBufferDesc.cBuffers = 1;
    outputBufferDesc.pBuffers = outputBuffers;

    outputBuffers[0].BufferType = SECBUFFER_TOKEN;
    outputBuffers[0].cbBuffer = 0;
    outputBuffers[0].pvBuffer = nullptr;

    negotiationState_ = OnNegotiateSecurityContext(input, &outputBufferDesc, output);
    if (FAILED(negotiationState_))
    {
        trace.SecurityNegotiationFailed(id_, transportSecurity_->SecurityProvider, targetName_, (uint32)negotiationState_);
        return negotiationState_;
    }

    SetNegotiationStartIfNeeded();
    if (output) AddServerAuthHeaderIfNeeded(*output);

    trace.SecurityNegotiationStatus(id_, transportSecurity_->SecurityProvider, targetName_, (uint32)negotiationState_);
    if (negotiationState_ == SEC_I_CONTINUE_NEEDED)
    {
        return negotiationState_;
    }

    if (NegotiationSucceeded())
    {
        SecPkgContext_NegotiationInfo negotiationInfo = {};

#ifdef PLATFORM_UNIX
        trace.SecurityNegotiationSucceeded(
            id_,
            transportSecurity_->SecurityProvider,
            targetName_,
            L"",
            0,
            contextExpiration_.ToString());
#else
        SECURITY_STATUS status = QueryAttributes(SECPKG_ATTR_NEGOTIATION_INFO, &negotiationInfo);
        if (FAILED(status) && (status != SEC_E_UNSUPPORTED_FUNCTION))
        {
            WriteError(TraceType, id_, "QueryAttributes(SECPKG_ATTR_NEGOTIATION_INFO) failed: 0x{0:x}", (uint)status);
            return status;
        }

        KFinally([&negotiationInfo] { FreeContextBuffer(negotiationInfo.PackageInfo); });

        trace.SecurityNegotiationSucceeded(
            id_,
            transportSecurity_->SecurityProvider,
            targetName_,
            negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->Name : L"",
            negotiationInfo.PackageInfo ? negotiationInfo.PackageInfo->fCapabilities : 0,
            contextExpiration_.ToString());
#endif

        state = OnNegotiationSucceeded(negotiationInfo);
        if (FAILED(state))
        {
            negotiationState_ = state;
        }

        ScheduleSessionExpirationIfNeeded();

        connectionAuthorizationState_ = AuthorizeRemoteEnd();

        if (FAILED(connectionAuthorizationState_))
        {
            trace.SecureSessionAuthorizationFailed(id_, (ErrorCodeValue::Enum)connectionAuthorizationState_);
            return connectionAuthorizationState_;
        }

        return connectionAuthorizationState_;
    }

    TRACE_AND_TESTASSERT(WriteError, TraceType, id_, "unexpected state=0x{0:x}, ending negotiation...", (uint)state);
    return E_FAIL;
}

bool SecurityContext::IsSessionExpirationEnabled() const
{
    if (!transportSecurity_->SessionExpirationEnabled())
    {
        WriteInfo(TraceType, id_, "session expiration disabled");
        return false;
    }

    if (inbound_ && transportSecurity_->Settings().IsRemotePeer())
    {
        WriteInfo(TraceType, id_, "session expiration disabled on server side in peer-to-peer mode");
        return false;
    }

    return true;
}

void SecurityContext::ScheduleSessionExpirationIfNeeded()
{
    if (!IsSessionExpirationEnabled()) return;

    TimeSpan expirationFromNow = transportSecurity_->Settings().SessionDuration();

    // Add minor randomness to session expiration if necessary
    auto threshold = TimeSpan::FromMinutes(10);
    if (expirationFromNow >= threshold)
    {
        auto randomVariance = TimeSpan::FromMilliseconds(Random().NextDouble() * threshold.TotalMilliseconds());
        expirationFromNow = expirationFromNow + randomVariance;
    }

    ScheduleSessionExpiration(expirationFromNow);
}

void SecurityContext::ScheduleSessionExpiration(TimeSpan expiration)
{
    if (!IsSessionExpirationEnabled()) return;

    auto newExpiration = Stopwatch::Now() + expiration;
    if ((Stopwatch::Now() < sessionExpiration_) && (sessionExpiration_ <= newExpiration))
    {
        WriteInfo(
            TraceType, id_,
            "ScheduleSessionExpiration: no-op: current expiration {0} <= new expiration = {1}",
            sessionExpiration_, newExpiration);
        return;
    }

    sessionExpiration_ = Stopwatch::Now() + expiration;
    sessionExpirationTimer_->Change(expiration);
    WriteInfo(TraceType, id_, "session will expire in {0}", expiration);
}

PCtxtHandle SecurityContext::IscInputContext()
{
    return (!NegotiationStarted() ? nullptr : &hSecurityContext_);
}

PCtxtHandle SecurityContext::AscInputContext()
{
    return (!NegotiationStarted() ? nullptr : &hSecurityContext_);
}

wchar_t* SecurityContext::TargetName() const
{
    if (targetName_.empty())
    {
        return nullptr;
    }

    return const_cast<wchar_t*>(targetName_.c_str());
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContext::QueryAttributes(ULONG ulAttribute, void* pBuffer)
{
#ifdef PLATFORM_UNIX
    return SEC_E_OK;
#else
    TESTASSERT_IF(!NegotiationSucceeded(), "Security context negotiation is incomplete or has failed.");
    return ::QueryContextAttributes(&hSecurityContext_, ulAttribute, pBuffer);
#endif
}

_Use_decl_annotations_
MessageUPtr SecurityContext::CreateSecurityNegoMessage(PSecBufferDesc pBuffers)
{
    if (FAILED(negotiationState_) || (pBuffers->pBuffers[0].cbBuffer == 0))
    {
        return nullptr;
    }

    std::vector<Common::const_buffer> buffers;
    for (unsigned i = 0; i < pBuffers->cBuffers; i++)
    {
        buffers.push_back(Common::const_buffer(pBuffers->pBuffers[i].pvBuffer, pBuffers->pBuffers[i].cbBuffer));
    }

    auto message = std::unique_ptr<Message>(new Message(buffers, FreeSecurityBuffers, nullptr));
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActorHeader(Actor::SecurityContext));

    return message;
}

void SecurityContext::FreeSecurityBuffers(vector<Common::const_buffer> const & buffers, void *)
{
    for (size_t i = 0; i < buffers.size(); i++)
    {
        ::FreeContextBuffer(buffers[i].buf);
    }
}

void SecurityContext::FreeEncryptedBuffer(std::vector<Common::const_buffer> const &, void * state)
{
    std::unique_ptr<std::vector<byte>> poi(static_cast<std::vector<byte> *>(state));
}

void SecurityContext::FreeByteBuffer(std::vector<Common::const_buffer> const &, void * state)
{
    delete state;
}

TransportSecurity const & SecurityContext::TransportSecurity() const
{
    return *transportSecurity_;
}

bool SecurityContext::IsInRole(RoleMask::Enum allowedRoles, RoleMask::Enum effectiveRole)
{
    auto mask = roleMask_;
    // todo, post V2, client ID information may not always be in securityContext_, it could be in a message
    // header added by naming gateway, we should first check such a message header before checking roleMask_
    if (mask == RoleMask::Enum::Admin && effectiveRole != RoleMask::Enum::None)
    {
        // effective role is populate to indicate cases where an securitycontext with admin role
        // needs to perform some operation as a specific lower privileged role. Currently this is 
        // used by httpgateway.
        mask = effectiveRole;
    }

    return (mask & allowedRoles) != 0;
}

bool SecurityContext::AuthenticateRemoteByClaims() const
{
    return false;
}

bool SecurityContext::ShouldPerformClaimsRetrieval() const
{
    return false;
}

bool SecurityContext::NegotiationStarted() const
{
    return SecIsValidHandle(&hSecurityContext_);
}

// mark the start of security negotiation
void SecurityContext::SetNegotiationStartIfNeeded()
{
#ifdef PLATFORM_UNIX
    if (!SecIsValidHandle(&hSecurityContext_))
    {
        hSecurityContext_.valid_ = true;
    }
#endif
}

MessageUPtr SecurityContext::CreateConnectionAuthMessage(ErrorCode const & authStatus)
{
    ConnectionAuthMessage mb(authStatus.Message, roleMask_);
    MessageUPtr message = make_unique<Message>(mb);
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActorHeader(Actor::TransportSendTarget));
    message->Headers.Add(ActionHeader(*Constants::ConnectionAuth));
    message->Headers.Add(FaultHeader(authStatus.ReadValue(), true));

    WriteInfo(
        TraceType, id_,
        "connection auth status message created: {0}:{1}",
        authStatus,
        mb);

    return message;
}

void SecurityContext::AddSecurityNegotiationHeader(Message & message)
{
    WriteInfo(TraceType, id_, "Add SecurityNegotiationHeader({0}) to message {1}", outgoingSecNegoHeader_, message.TraceId());
    message.Headers.Add(outgoingSecNegoHeader_);
}

void SecurityContext::AddVerificationHeaders(Message & message)
{
    AddSecurityNegotiationHeader(message);
    //todo, leikong, add other headers like the following
    //AddServerAuthHeaderIfNeeded(message);
}

void SecurityContext::OnRemoteFrameSizeLimit()
{
    if (auto connection = connection_.lock())
    {
        connection->OnRemoteFrameSizeLimit(incomingSecNegoHeader_.MaxIncomingFrameSize());
    }
}

void SecurityContext::TryGetIncomingSecurityNegotiationHeader(Message & message)
{
    if (NegotiationSucceeded())
    {
        shouldCheckVerificationHeaders_ = false;
        WriteInfo(
            TraceType, id_,
            "First incoming message {0} is already protected, will skip SecurityNegotiationHeader comparison",
            message.TraceId());

        if (message.Headers.TryReadFirst(incomingSecNegoHeader_))
        {
            WriteInfo(
                TraceType, id_,
                "TryGetIncomingSecurityNegotiationHeader: SecurityNegotiationHeader({0}) found on first incoming message {1}",
                incomingSecNegoHeader_,
                message.TraceId());

            OnRemoteFrameSizeLimit();
        }

        return;
    }

    if (message.Headers.TryReadFirst(incomingSecNegoHeader_))
    {
        WriteInfo(
            TraceType, id_,
            "TryGetIncomingSecurityNegotiationHeader: SecurityNegotiationHeader({0}) found on first incoming message {1}",
            incomingSecNegoHeader_,
            message.TraceId());

        OnRemoteFrameSizeLimit();

        if (incomingSecNegoHeader_.FramingProtectionEnabled())
        {
            EnableFramingProtection();
        }
        else
        {
            WriteInfo(
                TraceType, id_,
                "TryGetIncomingSecurityNegotiationHeader: FramingProtectionEnabled=false on remote side, inbound_ = {0}, SecurityProvider = {1}",
                inbound_,
                transportSecurity_->SecurityProvider);

            if (inbound_ && (transportSecurity_->SecurityProvider == SecurityProvider::Kerberos))
            {
                //MSODS uses Kerberos instead of Negotiate security provider
                //With Kerberos, security negotiation process can complete with a single message from client to server,
                //no server-to-client negotiation message is sent. Thus, a client with FramingProtectionEnabled set to false
                //will not have a chance to know if server side has FramingProtectionEnabled set true decryption will fail 
                //later due to inconsistency of FramingProtectionEnabled. Server side should disable framing protection in
                //such a case.
                DisableFramingProtection();
            }
        }

        return;
    }

    //SecurityNegotiationHeader is not found on first negotiation message, thus it is not affecting
    //negotiation process, thus there is no need to verify it after negotiation completion
    shouldCheckVerificationHeaders_ = false;
    WriteInfo(
        TraceType, id_,
        "SecurityNegotiationHeader not found on first negotiation message {0}, will skip comparison",
        message.TraceId());
}

bool SecurityContext::CheckVerificationHeadersIfNeeded(Message & message)
{
    if (!shouldCheckVerificationHeaders_) return true;
    shouldCheckVerificationHeaders_ = false;

    SecurityNegotiationHeader secNegoHeaderForVerification;
    if (message.Headers.TryReadFirst(secNegoHeaderForVerification))
    {
        WriteInfo(
            TraceType, id_,
            "CheckVerificationHeaders: SecurityNegotiationHeader({0}) found on first secured incoming message {1}",
            secNegoHeaderForVerification,
            message.TraceId());

        auto matchedPreviouslyReceived = secNegoHeaderForVerification == incomingSecNegoHeader_;
        textTrace.WriteTrace(
            matchedPreviouslyReceived? LogLevel::Info : LogLevel::Error,
            TraceType,
            id_,
            "CheckVerificationHeaders: SecurityNegotiationHeader: matchedPreviouslyReceived = {0}",
            matchedPreviouslyReceived);

        if (!matchedPreviouslyReceived) return false;
    }
    else
    {
        WriteInfo(
            TraceType, id_,
            "CheckVerificationHeaders: SecurityNegotiationHeader not found on first secured incoming message {0}",
            message.TraceId());
    }

    //todo, leikong, add verification of other headers sent unprotected during negotiation
    return true;
}

void SecurityContext::EnableFramingProtection()
{
    framingProtectionEnabled_ = true;
    WriteInfo(TraceType, id_, "FramingProtectionEnabled set true");
}

void SecurityContext::DisableFramingProtection()
{
    framingProtectionEnabled_ = false;
    WriteInfo(TraceType, id_, "FramingProtectionEnabled set false");
}

void SecurityContext::ReportSecurityApiSlow(wchar_t const* api, Common::TimeSpan duration, Common::TimeSpan threshold)
{
    auto reportCode = SystemHealthReportCode::FabricNode_SecurityApiSlow;
    auto description = wformatString(
        "call duration = {0}, configuration Setting Security/SlowApiThreshold = {1}",
        duration, threshold);

    IDatagramTransport::RerportHealth(reportCode, api, description, SecurityConfig::GetConfig().SlowApiHealthReportTtl);
}

ConnectionAuthMessage::ConnectionAuthMessage()
{
}

ConnectionAuthMessage::ConnectionAuthMessage(wstring const & errorMessage, RoleMask::Enum roleGranted)
    : errorMessage_(errorMessage)
    , roleGranted_(roleGranted)
{
}

wstring const & ConnectionAuthMessage::ErrorMessage() const
{
    return errorMessage_;
}

wstring && ConnectionAuthMessage::TakeMessage()
{
    return move(errorMessage_);
}

void ConnectionAuthMessage::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << errorMessage_ << L", role granted: " << roleGranted_;
}
