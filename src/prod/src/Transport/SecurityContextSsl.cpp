// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace AccessControl;
using namespace std;

static const StringLiteral TraceType("SecurityContextSsl");
static const Global<ThumbprintSet> emptyThumbprintSet = make_global<ThumbprintSet>();

SecurityContextSsl::SecurityContextSsl(
    IConnectionSPtr const & connection,
    TransportSecuritySPtr const & transportSecurity,
    wstring const & targetName,
    ListenInstance localListenInstance)
    : SecurityContext(connection, transportSecurity, targetName, localListenInstance)
{
    if (inbound_)
    {
        securityRequirements_ |= ASC_REQ_STREAM;
    }
    else
    {
        securityRequirements_ |= ISC_REQ_STREAM;
    }
}

bool SecurityContextSsl::Supports(SecurityProvider::Enum provider)
{
    return provider == SecurityProvider::Ssl || provider == SecurityProvider::Claims;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::CheckClientAuthHeader(MessageUPtr const & inputMessage)
{
    if (!inbound_ || !transportSecurity_->Settings().ClaimBasedClientAuthEnabled() || NegotiationStarted())
    {
        return SEC_E_OK;
    }

    ClientAuthHeader clientAuthHeader;
    if (!inputMessage->Headers.TryReadFirst(clientAuthHeader))
    {
        return SEC_E_OK;
    }

    if (clientAuthHeader.ProviderType() != SecurityProvider::Claims)
    {
        WriteError(TraceType, id_, "ignoring unexpected provider type in ClientAuthHeader: {0}", clientAuthHeader);
        return E_FAIL;
    }

    securityRequirements_ &= (~ASC_REQ_MUTUAL_AUTH);
    WriteInfo(TraceType, id_, "cleared mutual auth bit in requirement per incoming ClientAuthHeader: {0}", clientAuthHeader);

#ifdef PLATFORM_UNIX

    SSL_set_verify(ssl_.get(), SSL_VERIFY_NONE, CertVerifyCallbackStatic);

#endif

    return SEC_E_OK;
}

MessageUPtr SecurityContextSsl::CreateClaimsTokenErrorMessage(ErrorCode const & error)
{
    ConnectionAuthMessage mb(error.Message);
    MessageUPtr message = make_unique<Message>(mb);
    message->Headers.Add(MessageIdHeader());
    message->Headers.Add(ActorHeader(Actor::TransportSendTarget));
    message->Headers.Add(ActionHeader(*Constants::ClaimsTokenError));
    return message;
}

bool SecurityContextSsl::AuthenticateRemoteByClaims() const
{
    return inbound_ && ((securityRequirements_ & ASC_REQ_MUTUAL_AUTH) == 0) && transportSecurity_->Settings().ClaimBasedClientAuthEnabled();
}

bool SecurityContextSsl::ShouldPerformClaimsRetrieval() const
{
    // AAD implicitly uses claims (not explicitly configured in cluster manifest). The client does not know that
    // it needs to provide a claims token until told by the server upon receiving claims retrieval metadata.
    // Optimistically send the client auth header, which will be ignored by the server if it's not configured for claims.
    //
    // Allow clients to provide client cert (local/service clients) even when cluster is configured for claims.
    //
    return (transportSecurity_->ClaimsRetrievalHandlerFunc() != nullptr && transportSecurity_->SecurityProvider != SecurityProvider::Ssl);
}

void SecurityContextSsl::CompleteClaimsRetrieval(ErrorCode const & error, std::wstring const & claimsToken)
{
    auto owner = connection_.lock();
    if (!owner)
    {
        WriteInfo(TraceType, id_, "ignoring claims retrieval completion as owner is gone");
        return;
    }

    owner->CompleteClaimsRetrieval(error, claimsToken);
}

void SecurityContextSsl::CompleteClientAuth(ErrorCode const & error, SecuritySettings::RoleClaims const & clientClaims, Common::TimeSpan expiration)
{
    auto owner = connection_.lock();
    if (!owner)
    {
        WriteInfo(TraceType, id_, "ignoring client auth completion as owner is gone");
        return;
    }

    if (!error.IsSuccess())
    {
        WriteWarning(TraceType, id_, "claim handler completed with failure: {0}", error);
        connectionAuthorizationState_ = error.ToHResult();

        // todo, leikong, replace token-error message with connection-denied message after all customers upgrade client 
        // side to 3.1 or above, and all token-error message related code including receiving side should be removed.
        // token-error message is sent here to be backward compatible with 2.2 clients.
        owner->SendOneWay_Dedicated(CreateClaimsTokenErrorMessage(error), TimeSpan::MaxValue);
        owner->ResumeReceive(false);
        owner->ReportFault(error);
        owner->Close();
        return;
    }

    WriteInfo(TraceType, id_, "completing client auth with '{0}'", clientClaims);
    clientClaims_ = clientClaims;

    if (!clientClaims.IsInRole(transportSecurity_->Settings().ClientClaims()))
    {
        WriteWarning(
            TraceType, id_,
            "client claims '{0}' does not meet requirement '{1}'",
            clientClaims, transportSecurity_->Settings().ClientClaims());
        connectionAuthorizationState_ = STATUS_ACCESS_DENIED;
        auto fault = FaultToReport();

        owner->SendOneWay_Dedicated(
            CreateConnectionAuthMessage(fault),
            TransportConfig::GetConfig().CloseDrainTimeout);

        owner->ResumeReceive(false);
        owner->ReportFault(fault);
        owner->Close();
        return;
    }

    connectionAuthorizationState_ = SEC_E_OK;
    WriteInfo(
        TraceType, id_,
        "client claims '{0}' meets client role requirements '{1}'",
        clientClaims, transportSecurity_->Settings().ClientClaims());

    if (clientClaims.IsInRole(transportSecurity_->Settings().AdminClientClaims()))
    {
        roleMask_ = RoleMask::Admin;
        WriteInfo(
            TraceType, id_,
            "client claims '{0}' meets admin role requirements '{1}'",
            clientClaims, transportSecurity_->Settings().AdminClientClaims());
    }

    owner->SendOneWay_Dedicated(CreateConnectionAuthMessage(ErrorCode()), TimeSpan::MaxValue);

    owner->ResumeReceive();
    ScheduleSessionExpiration(expiration);
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::OnNegotiateSecurityContext(
    MessageUPtr const & inputMessage,
    PSecBufferDesc pOutput,
    MessageUPtr & outputMessage)
{
    outputMessage.reset();

    SECURITY_STATUS status = CheckClientAuthHeader(inputMessage);
    if (FAILED(status))
    {
        return status;
    }

    vector<byte> input = GetMergedMessageBody(inputMessage);

    bool shouldAddClientAuthHeader = false;
    if (!inbound_)
    {
        // ClientAuthHeader needs to be added onto the first negotiation message if claim based authentication is used on clients
        //
        shouldAddClientAuthHeader = !NegotiationStarted() && ((transportSecurity_->SecurityProvider == SecurityProvider::Claims) || this->ShouldPerformClaimsRetrieval());
    }

#ifdef PLATFORM_UNIX
    if (!input.empty())
    {
        //LINUXTODO consider avoid copying
        //BUF_MEM bm = { .data = (char*)input.data(), .max = input.size(), .length = input.size() };
        //BIO_set_mem_buf(inBio_, &bm, BIO_NOCLOSE);
        auto retval = BIO_write(inBio_, input.data(), input.size());
        Invariant(retval == input.size());
    }

    auto retval = SSL_do_handshake(ssl_.get());
    if (retval == 1)
    {
        negotiationState_ = SEC_E_OK;
        WriteInfo(TraceType, id_, "SSL_do_handshake:1, handshake was successfully completed");
    }
    else
    {
        auto sslErr = SSL_get_error(ssl_.get(), retval);
        WriteInfo(TraceType, id_, "SSL_do_handshake:{0}, SSL_get_error:{1}", retval, sslErr);

        if (sslErr == SSL_ERROR_WANT_READ) // SSL_ERROR_WANT_WRITE is not possible with our socket layer 
        {
            negotiationState_ = SEC_I_CONTINUE_NEEDED;
        }
        else
        {
            negotiationState_ = E_FAIL;
        }
    }

    char* data = nullptr;
    SecBuffer& output = pOutput->pBuffers[0];
    output.cbBuffer = BIO_ctrl_pending(outBio_);
    if (output.cbBuffer > 0)
    {
        output.pvBuffer = new BYTE[output.cbBuffer];
        auto read = BIO_read(outBio_, output.pvBuffer, output.cbBuffer);//LINUXTODO avoid memory copy
        Invariant(read == output.cbBuffer);
    }

#else
    SecBuffer secBufferIn[2] = {0};
    SecBufferDesc secBufDescriptorIn;

    secBufferIn[0].BufferType = SECBUFFER_TOKEN;
    secBufferIn[0].cbBuffer = static_cast<ULONG>(input.size());
    secBufferIn[0].pvBuffer = input.data();

    secBufferIn[1].BufferType = SECBUFFER_EMPTY;
    secBufferIn[1].cbBuffer = 0;
    secBufferIn[1].pvBuffer = nullptr;

    secBufDescriptorIn.ulVersion = SECBUFFER_VERSION;
    secBufDescriptorIn.cBuffers = 2;
    secBufDescriptorIn.pBuffers = secBufferIn;

    TimeStamp timestamp;
    if (inbound_)
    {
        auto api = [&]
        {
            negotiationState_ = ::AcceptSecurityContext(
                svrCredentials_.front()->Credentials(),
                AscInputContext(),
                &secBufDescriptorIn,
                securityRequirements_,
                0,
                &hSecurityContext_,
                pOutput,
                &fContextAttrs_,
                &timestamp);
        };

        CheckCallDuration<TraceTaskCodes::Transport>(
            api,
            SecurityConfig::GetConfig().SlowApiThreshold,
            TraceType,
            id_,
            L"AcceptSecurityContext",
            [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });
    }
    else
    {
        auto api = [&]
        {
            negotiationState_ = ::InitializeSecurityContext(
                credentials_.front()->Credentials(),
                IscInputContext(),
                TargetName(),
                securityRequirements_,
                0,
                0,
                &secBufDescriptorIn,
                0,
                &hSecurityContext_,
                pOutput,
                &fContextAttrs_,
                &timestamp);
        };

        CheckCallDuration<TraceTaskCodes::Transport>(
            api,
            SecurityConfig::GetConfig().SlowApiThreshold,
            TraceType,
            id_,
            L"InitializeSecurityContext",
            [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });
    }

    contextExpiration_ = DateTime((FILETIME const&)timestamp); // Schannel returns expiration in UTC
#endif

    outputMessage = CreateSecurityNegoMessage(pOutput);

    if (!inbound_)
    {
        if (shouldAddClientAuthHeader)
        {
            Invariant(outputMessage != nullptr);
            WriteInfo(TraceType, id_, "adding client auth header");

            outputMessage->Headers.Add(ClientAuthHeader(this->ShouldPerformClaimsRetrieval() 
                    ? SecurityProvider::Claims 
                    : transportSecurity_->SecurityProvider));
        }
        else
        {
            WriteInfo(TraceType, id_, "skipping client auth header: {0}", transportSecurity_->SecurityProvider);
        }
    }

    return negotiationState_;
}

SECURITY_STATUS SecurityContextSsl::ProcessClaimsMessage(MessageUPtr & message)
{
    if (!ConnectionAuthorizationPending() || claimsMessageReceived_)
    {
        return SEC_E_OK;
    }

    if (message->Actor != Actor::Transport)
    {
        WriteError(TraceType, id_, "only transport messages are allowed while client auth is pending, incoming: {0}", *message); 
        return E_FAIL;
    }

    ClaimsMessage claimsMessage;
    if (!message->GetBody(claimsMessage))
    {
        WriteError(TraceType, id_, "failed to get ClaimsMessage from {0}: 0x{1:x}", *message, message->Status);

        return E_FAIL;
    }

    claimsMessageReceived_ = true;

    TransportSecurity::ClaimsHandler claimsHandler = transportSecurity_->ClaimsHandlerFunc();
    if (claimsHandler == nullptr)
    {
        WriteError(TraceType, id_, "failed to get handler for ClaimsMessage {0} '{1}'", message->TraceId(), claimsMessage);
        return E_FAIL;
    }

    auto connection = connection_.lock();
    if (!connection)
    {
        WriteError(TraceType, id_, "failed to lock connection_ for claims message");
        return E_FAIL;
    }

    connection->SuspendReceive([connection, claimsHandler, claimsMessage, this] { claimsHandler(claimsMessage.Claims(), shared_from_this()); });
    WriteInfo(TraceType, id_, "received ClaimsMessage {0} '{1}', will schedule claims handler", message->TraceId(), claimsMessage);

    message.reset();

    return SEC_E_OK;
}

#ifdef PLATFORM_UNIX

ErrorCode SecurityContextSsl::Encrypt(void const* buffer, size_t len)
{
    auto retval = SSL_write(ssl_.get(), buffer, len);
    ErrorCode error;
    if (retval <= 0)
    {
        error = cryptUtil_.GetOpensslErr();
        WriteWarning(TraceType, id_, "SSL_write failed: {0}", error);
    }

    return error;
}

ByteBuffer2 SecurityContextSsl::EncryptFinal()
{
    return BioMemToByteBuffer2(outBio_);
}

SECURITY_STATUS SecurityContextSsl::DecodeMessage(MessageUPtr & message)
{
    message;
    Assert::CodingError("not implemented");
}

SECURITY_STATUS SecurityContextSsl::DecodeMessage(bique<byte> & receiveQueue, bique<byte> & decrypted)
{
    // prepare data for decryption
    size_t totalBufferUsed = receiveQueue.end() - receiveQueue.begin();
    auto iter = receiveQueue.begin();
    auto endIter = receiveQueue.end();
    while (iter < endIter)
    {
        size_t chunkSize = (iter + iter.fragment_size() <= endIter)? iter.fragment_size() : (endIter - iter);
        AddDataToDecrypt(&(*iter), chunkSize); 
        iter += chunkSize;
    }

    receiveQueue.truncate_before(endIter);

    // decryption
    auto beforeCapacity = decrypted.capacity();
    decrypted.reserve_back(totalBufferUsed + DecryptedPendingMax());
    WriteNoise(
        TraceType, id_, 
        "DecodeMessage: inputTotal = {0}, DecryptedPendingMax = {1}, decrypted: size = {2}, capacity: {3} -> {4}",
        totalBufferUsed,
        DecryptedPendingMax(),
        decrypted.size(),
        beforeCapacity,
        decrypted.capacity());

    auto decryptIter = decrypted.end();
    auto decryptLimit = decrypted.uninitialized_end();
    Invariant(decryptIter < decryptLimit);
    size_t decryptedTotal = 0; 
    SECURITY_STATUS status = SEC_E_OK;

    for(;;)
    {
        int64 bufLen = decryptIter.fragment_end() - &(*decryptIter);
        auto bufLenBefore = bufLen;
        status = Decrypt(&(*decryptIter), bufLen);
        WriteTrace(
            FAILED(status)? LogLevel::Info : LogLevel::Noise,
            TraceType, id_, 
            "Decrypt: status = {0:x}, bufLenBefore = {1}, bufLenAfter = {2}, decryptedTotal = {3}",
            (uint)status,
            bufLenBefore,
            bufLen,
            decryptedTotal);

        if (status != SEC_E_OK) break;

        if (bufLen == 0)
        {
            WriteNoise(TraceType, id_, "Decrypt: bufLen == 0");
            break;
        }

        decrypted.no_fill_advance_back(bufLen);
        decryptIter += bufLen;
        decryptedTotal += bufLen;
        Invariant(decryptIter < decryptLimit);
    }

    if (decryptedTotal) return SEC_E_OK; 

    return status;
}

void SecurityContextSsl::AddDataToDecrypt(void const* buffer, size_t len)
{
    //LINUXTODO do we need to clean up inBio_ or SSL_read will clean up?
    auto written = BIO_write(inBio_, buffer, len);
    ASSERT_IFNOT(
        written == len,
        "BIO_write returned {0}, input len = {1}, error = {2}",
        written,
        len,
        cryptUtil_.GetOpensslErr().Message);

    WriteNoise(TraceType, id_, "AddDataToDecrypt: written = {0}, BIO_ctrl_pending = {1}", written, BIO_ctrl_pending(inBio_));
}

constexpr size_t SecurityContextSsl::DecryptedPendingMax()
{
    //LINUXTODO, can we avoid hardcoding this?
    return 16*1024; // TLS record size max 
}

SECURITY_STATUS SecurityContextSsl::Decrypt(void* buffer, _Inout_ int64& len)
{
    auto pending = BIO_ctrl_pending(inBio_);
    auto bufferSize = len;
    len = SSL_read(ssl_.get(), buffer, len);
    if (len < 0)
    {
        auto err = errno;
        auto sslError = SSL_get_error(ssl_.get(), len);
        if (sslError == SSL_ERROR_WANT_READ)
        {
            WriteNoise(TraceType, id_, "Decrypt: SSL_ERROR_WANT_READ, BIO_ctrl_pending = {0}", BIO_ctrl_pending(inBio_));
            return STATUS_PENDING;
        }

        if (sslError == SSL_ERROR_SYSCALL)
        {
            WriteNoise(TraceType, id_, "Decrypt: SSL_ERROR_SYSCALL, errno = {0}, BIO_ctrl_pending = {1}", err, BIO_ctrl_pending(inBio_));
            return STATUS_PENDING;
        }

        auto ec = LinuxCryptUtil().GetOpensslErr();
        WriteNoise(TraceType, id_, "Decrypt: bufferSize = {0}, SSL_get_error = {1}, errno = {2}, {3}", bufferSize, sslError, err, ec.Message);
        return E_FAIL;
    }

    return SEC_E_OK;
}

void SecurityContextSsl::OnInitialize()
{
    vector<SecurityCredentialsSPtr> dummy;
    transportSecurity_->GetCredentials(credentials_, dummy);
    contextExpiration_ = credentials_.front()->Expiration();

    ssl_ = SslUPtr(SSL_new(credentials_.front()->SslCtx()));
    WriteInfo(TraceType, id_, "created ssl_ {0:x}", TextTracePtr(ssl_.get()));
    Invariant(ssl_);
    SSL_set_app_data(ssl_.get(), this);

    SSL_set_info_callback(ssl_.get(), SslInfoCallbackStatic);

    // CheckClientAuthHeader() will reset the flags if claims authentication is requested
    //
    SSL_set_verify(ssl_.get(), SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, CertVerifyCallbackStatic);

    auto param = SSL_get0_param(ssl_.get());
    LinuxCryptUtil::ApplyCrlCheckingFlag(param, transportSecurity_->Settings().X509CertChainFlags());

    inBio_ = BIO_new(BIO_s_mem());
    Invariant(inBio_);
    BIO_set_mem_eof_return(inBio_, -1);

    outBio_ = BIO_new(BIO_s_mem()); 
    Invariant(outBio_);
    BIO_set_mem_eof_return(outBio_, -1);

    SSL_set_bio(ssl_.get(), inBio_, outBio_);

    if (inbound_)
    {
        SSL_set_accept_state(ssl_.get());
    }
    else
    {
        SSL_set_connect_state(ssl_.get());
    }
}

int SecurityContextSsl::CertVerifyCallbackStatic(int preverify_ok, X509_STORE_CTX* ctx)
{
    auto ssl = (SSL*)X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
    auto thisPtr = (SecurityContextSsl*)SSL_get_app_data(ssl);
    Invariant(thisPtr);
    return thisPtr->CertVerifyCallback(preverify_ok, ctx);
}

int SecurityContextSsl::CertVerifyCallback(int preverify_ok, X509_STORE_CTX* ctx)
{
    X509* cert = X509_STORE_CTX_get_current_cert(ctx);
    int depth = X509_STORE_CTX_get_error_depth(ctx);
    int err = X509_STORE_CTX_get_error(ctx);

    auto subjectName = X509_NAME_oneline(X509_get_subject_name(cert), nullptr, 0);
    Invariant(subjectName);
    KFinally([=] { free(subjectName); });
    auto issuerName = X509_NAME_oneline(X509_get_issuer_name(cert), nullptr, 0);
    Invariant(issuerName);
    KFinally([=] { free(issuerName); });
    uint64 certSerialNo = ASN1_INTEGER_get(cert->cert_info->serialNumber);

    if (preverify_ok)
    {
        WriteInfo(
            TraceType, id_,
            "CertVerifyCallback(preverify_ok={0}) called on certificate(subject='{1}', issuer='{2}', serial={3:x}) at depth {4}",
            preverify_ok, subjectName, issuerName, certSerialNo, depth);
        return preverify_ok;
    }

    WriteInfo(
        TraceType, id_,
        "CertVerifyCallback(preverify_ok={0}) called on certificate(subject='{1}', issuer='{2}', serial={3:x}) at depth {4}, error={5}:{6}",
        preverify_ok, subjectName, issuerName, certSerialNo, depth, err, X509_verify_cert_error_string(err));

    if (err == X509_V_ERR_INVALID_PURPOSE)
    {
        WriteInfo(TraceType, id_, "ignore X509_V_ERR_INVALID_PURPOSE as serverAuth and clientAuth can be used interchangably in our system");
        return 1;
    }

    if (LinuxCryptUtil::BenignCertErrorForAuthByThumbprintOrPubKey(err) || (err == X509_V_ERR_UNABLE_TO_GET_CRL))
    {
        // Error listed above may be acceptable:
        // 1. self-signed certificate or untrusted root is ok if allow list contains thumbprints or public keys
        // 2. crl retrieval failure is ok if our config allows it
        // AuthorizeRemoteEnd will later decide if encountered error can be ignored
        certChainErrors_.emplace_back(err, depth);
        return 1;
    }

    return 0;
}

void SecurityContextSsl::SslInfoCallbackStatic(const SSL* ssl, int where, int ret)
{
    auto thisPtr = (SecurityContextSsl*)SSL_get_app_data(ssl);
    Invariant(thisPtr);
    Invariant(ssl == thisPtr->ssl_.get());
    thisPtr->SslInfoCallback(where, ret);
}

void SecurityContextSsl::SslInfoCallback(int where, int ret)
{
    const char *str = "other";
    if (where & SSL_ST_CONNECT)
    {
        str = "SSL_ST_CONNECT";
    }
    else if (where & SSL_ST_ACCEPT)
    {
        str = "SSL_ST_ACCEPT";
    }

    if (where & SSL_CB_LOOP)
    {
        WriteInfo(TraceType, id_, "SslInfoCallback: {0:x}:{1}:{2}", where, str, SSL_state_string_long(ssl_.get())); 
    }
    else if (where & SSL_CB_ALERT)
    {
        str = (where & SSL_CB_READ)? "read":"write";
        WriteInfo(
            TraceType, id_,
            "SslInfoCallback: alert: {0}:{1}:{2}",
            str,
            SSL_alert_type_string_long(ret),
            SSL_alert_desc_string_long(ret));
    }
    else if (where & SSL_CB_EXIT)
    {
        if (ret <= 0)
        {
            WriteInfo(
                TraceType, id_,
                "SslInfoCallback: {0} in {1}",
                str,
                SSL_state_string_long(ssl_.get()));
        }
    }
}

SECURITY_STATUS SecurityContextSsl::AuthorizeRemoteEnd()
{
    if(AuthenticateRemoteByClaims())
    {
        WriteInfo(TraceType, id_, "skipping cert verification, remote will be authenticated by claims");
        return STATUS_PENDING;
    }

    auto verifyResult = SSL_get_verify_result(ssl_.get());
    WriteInfo(TraceType, id_, "SSL_get_verify_result(): {0}", verifyResult);
    //LINUXTODO use SSL_get_verify_result to avoid duplicate chain validation

    //SSL_get_peer_cert_chain does not increment ref count on the chain, which is fine for
    //the auth checking here, but if we need to use it afterwards, e.g. AccessCheck(), we
    //will increment ref count in case the stack gets reset by renegotiation
    //https://www.openssl.org/docs/manmaster/ssl/SSL_get_peer_cert_chain.html
    auto remoteCertChain = SSL_get_peer_cert_chain(ssl_.get());
    if (!remoteCertChain)
    {
        WriteInfo(TraceType, id_, "SSL_get_peer_cert_chain returned null");
        return E_FAIL;
    }

    WriteNoise(TraceType, id_, "SSL_get_peer_cert_chain returned {0} certificates", sk_X509_num(remoteCertChain));
    X509StackShallowUPtr clientCertStack; //Use "Shallow" as SSL_get_peer_cert_chain does not 
    //increase ref count on X509 objects in the returned stack

    remoteCertContext_ = CertContextUPtr(SSL_get_peer_certificate(ssl_.get()));
    if (!remoteCertContext_)
    {
        WriteInfo(TraceType, id_, "SSL_get_peer_certificate returned null");
        return E_FAIL;
    }

    TryAuthenticateRemoteAsPeer();

    if (inbound_)
    {
        clientCertStack = X509StackShallowUPtr(sk_X509_dup(remoteCertChain));
        sk_X509_insert(clientCertStack.get(), remoteCertContext_.get(), 0);
        remoteCertChain = clientCertStack.get();
    }

    auto err = cryptUtil_.VerifyCertificate(
        id_,
        remoteCertChain,
        transportSecurity_->Settings().X509CertChainFlags(),
        transportSecurity_->Settings().ShouldIgnoreCrlOfflineError(inbound_),
        remoteAuthenticatedAsPeer_,
        transportSecurity_->Settings().RemoteCertThumbprints(),
        transportSecurity_->Settings().RemoteX509Names(),
        true,
        &remoteCommonName_,
        &certChainErrors_);

    if (!err.IsSuccess())
    {
        return err.ToHResult(); 
    }

    if (transportSecurity_->Settings().IsClientRoleInEffect())
    {
        auto adminRoleStatus = cryptUtil_.VerifyCertificate(
            id_,
            remoteCertChain,
            transportSecurity_->Settings().X509CertChainFlags(),
            transportSecurity_->Settings().ShouldIgnoreCrlOfflineError(inbound_),
            remoteAuthenticatedAsPeer_,
            transportSecurity_->Settings().AdminClientCertThumbprints(),
            transportSecurity_->Settings().AdminClientX509Names(),
            false,
            &remoteCommonName_,
            &certChainErrors_);

        if (adminRoleStatus.IsSuccess())
        {
            roleMask_ = RoleMask::Admin;
        }
    }

    WriteInfo(
        TraceType, id_, "cert auth completed: IsClientRoleInEffect = {0}, RoleMask = {1}",
        transportSecurity_->Settings().IsClientRoleInEffect(), roleMask_);

    return SEC_E_OK; 
}

SECURITY_STATUS SecurityContextSsl::EncodeMessage(MessageUPtr & message)
{
    Assert::CodingError("NotImplemented");
    return E_FAIL;
}

SECURITY_STATUS SecurityContextSsl::EncodeMessage(TcpFrameHeader const &, Message &, ByteBuffer2 &)
{
    Assert::CodingError("NotImplemented");
    return E_FAIL;
}

bool SecurityContextSsl::AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const
{
    //LINUXTODO
    return false;
}

SECURITY_STATUS SecurityContextSsl::OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const &)
{
//    if (AuthenticateRemoteByClaims()) return SEC_E_OK;
//
//    auto remoteCertChain = SSL_get_peer_cert_chain(ssl_.get());
//    if (!remoteCertChain)
//    {
//        WriteWarning(TraceType, id_, "OnNegotiationSucceeded: SSL_get_peer_cert_chain returned null");
//        return E_FAIL;
//    }
//
//    X509_chain_up_ref(remoteCertChain);
//    remoteCertContext_.reset(remoteCertChain);
    return SEC_E_OK;
}

#else

void SecurityContextSsl::OnInitialize()
{
    RtlZeroMemory(&streamSizes_, sizeof(streamSizes_));
    transportSecurity_->GetCredentials(credentials_, svrCredentials_);

    if (transportSecurity_->SecurityProvider == SecurityProvider::Claims) return;

    WriteInfo(
        TraceType, id_,
        "OnInitialize: credentials_ = {0}",
        SecurityCredentials::VectorsToString(credentials_));
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::DecodeMessage(MessageUPtr & message)
{
    SECURITY_STATUS status = SEC_E_OK;
    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        SecurityHeader securityHeader;
        bool foundSecurityHeader = message->Headers.TryReadFirst(securityHeader);
        if (!foundSecurityHeader)
        {
            WriteError(TraceType, id_, "failed to get security header, message status = 0x{0:x}", (DWORD)(message->Status));
            return NT_SUCCESS(message->Status) ? E_FAIL : message->Status;
        }

        if ((securityHeader.MessageHeaderProtectionLevel != transportSecurity_->MessageHeaderProtectionLevel) ||
            (securityHeader.MessageBodyProtectionLevel != transportSecurity_->MessageBodyProtectionLevel))
        {
            WriteError(
                TraceType, id_,
                "protection level mismatch: local: {0}-{1}, incoming: {2}-{3}",
                transportSecurity_->MessageHeaderProtectionLevel,
                transportSecurity_->MessageBodyProtectionLevel,
                securityHeader.MessageHeaderProtectionLevel,
                securityHeader.MessageBodyProtectionLevel);

            return E_FAIL;
        }

        ByteBique decrypted(securityHeader.HeaderData.size());
        status = DecryptMessageHeaders(securityHeader.HeaderData, decrypted);
        if (FAILED(status))
        {
            return status;
        }

        // todo, leikong, after decryption, the continuous input buffer may be broken up to multiple pieces,
        // due to the removal of SSL header and trails. Currently, the storage of MessageHeaders must be 
        // ByteBiqueRange, thus we have to copy decrypted data to ByteBique in DecryptMessageHeaders. If
        // MessageHeaders allows buffer list input, then we can avoid the extra copying and the extra heap
        // allocation made for ByteBique.
        ByteBiqueRange decryptedRange(decrypted.begin(), decrypted.end(), true);
        message->Headers.Replace(std::move(decryptedRange));
    }

    if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        status = DecryptMessageBody(message);
    }

    if (!FAILED(status))
    {
        if (!CheckVerificationHeadersIfNeeded(*message)) return E_FAIL;

        status = ProcessClaimsMessage(message);
    }

    return status;
}

SECURITY_STATUS SecurityContextSsl::DecodeMessage(bique<byte> & receiveQueue, bique<byte> & decrypted)
{
    auto iter = receiveQueue.begin();
    auto endIter = receiveQueue.end();
    auto cipherTextLength = endIter - iter;
    if (cipherTextLength > numeric_limits<uint>::max())
    {
        WriteError(TraceType, id_, "ciphertext too long: {0}", cipherTextLength);
        return STATUS_UNSUCCESSFUL;
    }

    if (cipherTextLength > 0)
    {
        decrypted.reserve_back(cipherTextLength); // decrypted text length <= cipherTextLength
    }

    byte* decryptInput = nullptr;
    uint bytesToDecrypt = 0;
    bool shouldMergeInput = false;
    const auto tlsRecordSize = (uint)decryptionInputMergeBuffer_.size();

    while (iter < endIter)
    {
        // set up a single buffer for decryption, SSPI(SChannel) decryption does not allow multiple input buffers
        auto fragmentSize = iter.fragment_size();
        auto fragmentEnd = iter + fragmentSize;
        auto remaining = endIter - iter;
        auto chunkSize = (uint)((fragmentEnd <= endIter) ? fragmentSize : remaining);

        if (shouldMergeInput)
        {
            shouldMergeInput = false;

            decryptionInputMergeBuffer_.ResetAppendCursor();
            decryptionInputMergeBuffer_.append(decryptInput, bytesToDecrypt);

            // no need to copy a full chunk, just enough to make at least one complete TLS record
            auto toCopy = std::min(chunkSize, tlsRecordSize - bytesToDecrypt);
            decryptionInputMergeBuffer_.append(&(*iter), toCopy);

#if DBG && 0
            WriteNoise(TraceType, id_, "decrypt: input merging: {0} | {1}", bytesToDecrypt, toCopy);
#endif

            decryptInput = decryptionInputMergeBuffer_.data();
            bytesToDecrypt += toCopy;
            iter += toCopy;
        }
        else
        {
            decryptInput = &(*iter);
            bytesToDecrypt = chunkSize;
            iter += chunkSize;
        }

#if DBG && 0
        WriteNoise(
            TraceType, id_,
            "decrypt: input: {0:l}",
            ConstBuffer(decryptInput, bytesToDecrypt));
#endif

        SecBuffer buffers[4] = {};
        SECURITY_STATUS status = SspiDecrypt(decryptInput, bytesToDecrypt, buffers);
        if (status == SEC_E_INCOMPLETE_MESSAGE)
        {
            if (iter >= endIter)
            {
                receiveQueue.truncate_before(receiveQueue.end() - bytesToDecrypt);
                return STATUS_PENDING;
            }

            if (decryptInput == decryptionInputMergeBuffer_.data())
            {
                WriteError(TraceType, id_, "decrypt: SEC_E_INCOMPLETE_MESSAGE is unexpected with merge buffer used, bytesToDecrypt = {0}, tlsRecordSize = {1}", bytesToDecrypt, tlsRecordSize);
                return STATUS_UNSUCCESSFUL;
            }

            shouldMergeInput = true;
            continue;
        }

        if (status != SEC_E_OK)
        {
            WriteError(TraceType, id_, "decrypt failed: 0x{0:x}", (uint)status);
            return status;
        }

        auto srcBegin = (byte*)(buffers[1].pvBuffer);
        uint srcLength = buffers[1].cbBuffer;
#if DBG && 0
        WriteNoise(
            TraceType, id_,
            "decrypt: output: {0:l}",
            ConstBuffer(srcBegin, srcLength));
#endif
        // todo, leikong, consider reducing copy, is it possible to do in-place-decryption most of time and only copy when required?
        decrypted.append(srcBegin, srcLength);

        // check for extra input left by decryption
        int  bufferIndex = 3;
        while ((buffers[bufferIndex].BufferType != SECBUFFER_EXTRA) && (bufferIndex-- != 0));

        if (bufferIndex >= 0)
        {
#if DBG && 0
            WriteNoise(TraceType, id_, "decrypt: {0} bytes input left untouched", buffers[bufferIndex].cbBuffer);
#endif
            iter -= buffers[bufferIndex].cbBuffer;
        }
    }

    receiveQueue.truncate_before(receiveQueue.end());
    return SEC_E_OK;
}

#if 0

// In some cases, this simple "merge all input buffers and then decrypt" code below actually performs
// better than the optimized version above, where mering is only done when absolutely needed. 

SECURITY_STATUS SecurityContextSsl::DecodeMessage(bique<byte> & receiveQueue, bique<byte> & decrypted)
{
    const auto sslRecordSize = (uint)decryptionInputMergeBuffer_.size();
    const auto receiveChunkSize = TransportConfig::GetConfig().SslReceiveChunkSize;

    auto iter = receiveQueue.begin();
    auto endIter = receiveQueue.end();
    byte* decryptBegin = nullptr;
    uint bytesToDecrypt = 0;
    SECURITY_STATUS status = SEC_E_OK;

    auto cipherTextLength = endIter - iter;
    if (cipherTextLength > numeric_limits<uint>::max())
    {
        WriteError(TraceType, id_, "ciphertext too long: {0}", cipherTextLength);
        return STATUS_UNSUCCESSFUL;
    }

    if (cipherTextLength > 0)
    {
        decrypted.reserve_back(cipherTextLength); // decrypted text length <= cipherTextLength
    }

    //todo, leikong, avoid the following copying when possible
    ByteBuffer2 inputMerged(cipherTextLength);
    while (iter < endIter)
    {
        auto fragmentSize = iter.fragment_size();
        auto fragmentEnd = iter + fragmentSize;
        auto remaining = endIter - iter;
        auto chunkSize = (uint)((fragmentEnd <= endIter) ? fragmentSize : remaining);
        inputMerged.append(&(*iter), chunkSize);
        iter += chunkSize;
    }

    inputMerged.SetSizeAfterAppend();
    decryptBegin = inputMerged.data();
    bytesToDecrypt = (uint)inputMerged.size();

    for (;;)
    {
        WriteNoise(
            TraceType, id_,
            "decrypt: input: {0:l}",
            ConstBuffer(decryptBegin, bytesToDecrypt));

        SecBuffer buffers[4] = {};
        status = SspiDecrypt(decryptBegin, bytesToDecrypt, buffers);
        if (status == SEC_E_INCOMPLETE_MESSAGE)
        {
            receiveQueue.truncate_before(receiveQueue.end() - bytesToDecrypt);
            return STATUS_PENDING;
        }

        if (status != SEC_E_OK)
        {
            WriteError(TraceType, id_, "decrypt failed: 0x{0:x}", (uint)status);
            return status;
        }

        auto srcBegin = (byte*)(buffers[1].pvBuffer);
        uint srcLength = buffers[1].cbBuffer;
        WriteNoise(
            TraceType, id_,
            "decrypt: output: {0:l}",
            ConstBuffer(srcBegin, srcLength));

        decrypted.append(srcBegin, srcLength);

        // check for extra input left by decryption
        int  bufferIndex = 3;
        while ((buffers[bufferIndex].BufferType != SECBUFFER_EXTRA) && (bufferIndex-- != 0));

        if (bufferIndex >= 0)
        {
            WriteNoise(TraceType, id_, "decrypt: {0} byte input left by previous decrypt", buffers[bufferIndex].cbBuffer);
            bytesToDecrypt = buffers[bufferIndex].cbBuffer;
            decryptBegin = inputMerged.end() - buffers[bufferIndex].cbBuffer;
            continue;
        }

        // no input left by previous decrypt
        receiveQueue.truncate_before(receiveQueue.end());
        break;
    }

    return status;
}

#endif

SECURITY_STATUS SecurityContextSsl::AuthorizeRemoteEnd()
{
    if(AuthenticateRemoteByClaims())
    {
        WriteInfo(TraceType, id_, "skipping cert verification, remote will be authenticated by claims");
        return STATUS_PENDING;
    }

    TryAuthenticateRemoteAsPeer();

    SECURITY_STATUS status = VerifyCertificate(
        id_,
        remoteCertContext_.get(),
        transportSecurity_->Settings().X509CertChainFlags(),
        transportSecurity_->Settings().ShouldIgnoreCrlOfflineError(inbound_),
        remoteAuthenticatedAsPeer_,
        transportSecurity_->Settings().RemoteCertThumbprints(),
        transportSecurity_->Settings().RemoteX509Names(),
        true,
        &remoteCommonName_);

    if (status != SEC_E_OK)
    {
        return status;
    }

    if (transportSecurity_->Settings().IsClientRoleInEffect())
    {
        SECURITY_STATUS adminRoleStatus = VerifyCertificate(
            id_,
            remoteCertContext_.get(),
            transportSecurity_->Settings().X509CertChainFlags(),
            transportSecurity_->Settings().ShouldIgnoreCrlOfflineError(inbound_),
            remoteAuthenticatedAsPeer_,
            transportSecurity_->Settings().AdminClientCertThumbprints(),
            transportSecurity_->Settings().AdminClientX509Names(),
            false,
            &remoteCommonName_);

        if (adminRoleStatus == SEC_E_OK)
        {
            roleMask_ = RoleMask::Admin;
        }
    }

    WriteInfo(
        TraceType, id_,
        "cert auth completed: IsClientRoleInEffect = {0}, RoleMask = {1}",
        transportSecurity_->Settings().IsClientRoleInEffect(), roleMask_);

    return status;
}

void SecurityContextSsl::TraceCertificate(
    wstring const & id,
    PCCERT_CONTEXT certContext,
    Thumbprint const & issuerCertThumbprint)
{
    const DWORD nameType = CERT_X500_NAME_STR|CERT_NAME_STR_NO_PLUS_FLAG;
    const DWORD nameCapacity = 512;
    wchar_t subject[nameCapacity];
    wchar_t issuer[nameCapacity];
    subject[0] = 0;
    issuer[0] = 0;

    DWORD subjectNameLen = ::CertNameToStr(
        certContext->dwCertEncodingType,
        &(certContext->pCertInfo->Subject),
        nameType,
        subject,
        nameCapacity);
    if (subjectNameLen == 1)
    {
        WriteWarning(TraceType, id, "CertNameToStr failed with 0x{0:x}", ::GetLastError());
    }

    DWORD issuerNameLen = ::CertNameToStr(
        certContext->dwCertEncodingType,
        &(certContext->pCertInfo->Issuer),
        nameType,
        issuer,
        nameCapacity);
    if (issuerNameLen == 1)
    {
        WriteWarning(TraceType, id, "CertNameToStr failed with 0x{0:x}", ::GetLastError());
    }

    Thumbprint certThumbprint;
    if (!certThumbprint.Initialize(certContext).IsSuccess())
    {
        WriteWarning(TraceType, id, "failed to get certificate thumbprint");
    }

    WriteInfo(
        TraceType, id,
        "incoming cert: thumbprint = {0}, subject='{1}', issuer='{2}', issuerCertThumbprint={3}, NotBefore={4}, NotAfter={5}",
        certThumbprint,
        WStringLiteral(subject, subject + subjectNameLen - 1),
        WStringLiteral(issuer, issuer + issuerNameLen - 1),
        issuerCertThumbprint,
        DateTime(certContext->pCertInfo->NotBefore),
        DateTime(certContext->pCertInfo->NotAfter));
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::GetCertChainContext(
    wstring const & id,
    PCCERT_CONTEXT certContext,
    LPSTR usage,
    DWORD certChainFlags,
    CertChainContextUPtr & certChainContext,
    Common::X509Identity::SPtr* issuerPubKey,
    Common::X509Identity::SPtr* issuerCertThumbprint)
{
    CERT_CHAIN_PARA chainPara = {0};
    chainPara.cbSize = sizeof(CERT_CHAIN_PARA);
    chainPara.RequestedUsage.dwType = USAGE_MATCH_TYPE_OR;
    chainPara.RequestedUsage.Usage.cUsageIdentifier = 1;
    chainPara.RequestedUsage.Usage.rgpszUsageIdentifier = &usage;

    SecurityConfig & securityConfig = SecurityConfig::GetConfig();
    if (securityConfig.CrlRetrivalTimeout > TimeSpan::Zero)
    {
        chainPara.dwUrlRetrievalTimeout = (DWORD)securityConfig.CrlRetrivalTimeout.TotalMilliseconds();
    }

    PCCERT_CHAIN_CONTEXT certChainContextPtr = nullptr;
    SECURITY_STATUS status = SEC_E_OK;
    BOOL retval = FALSE;
    auto api = [&]
    {
        retval = ::CertGetCertificateChain(
            nullptr,
            certContext,
            nullptr,
            certContext->hCertStore,
            &chainPara,
            certChainFlags | CERT_CHAIN_CACHE_END_CERT | CERT_CHAIN_REVOCATION_ACCUMULATIVE_TIMEOUT,
            nullptr,
            &certChainContextPtr);
    };

    CheckCallDuration<TraceTaskCodes::Transport>(
        api,
        SecurityConfig::GetConfig().SlowApiThreshold,
        TraceType,
        id,
        L"CertGetCertificateChain",
        [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });

    if (!retval)
    {
        DWORD lastError = ::GetLastError();
        WriteWarning(TraceType, id, "CertGetCertificateChain failed: 0x{0:x}", lastError);
        certChainContext.reset();
        return HRESULT_FROM_WIN32(lastError);
    }

    certChainContext.reset(certChainContextPtr);
    WriteInfo(
        TraceType, id,
        "usage: {0}, cert chain trust status: info = {1:x}, error = {2:x}, chainSize = {3}, SelfSigned = {4}",
        usage,
        certChainContext->TrustStatus.dwInfoStatus,
        certChainContext->TrustStatus.dwErrorStatus,
        certChainContext->rgpChain[0]->cElement,
        CryptoUtility::IsCertificateSelfSigned(certChainContext.get()));

    uint nonFatalError =
        CERT_TRUST_IS_NOT_VALID_FOR_USAGE | //key usage is not enforced, as cluster/replication cert is used for both client and server auth due to peer-to-peer nature
        CERT_TRUST_IS_UNTRUSTED_ROOT |
        CERT_TRUST_IS_PARTIAL_CHAIN |
        CERT_TRUST_REVOCATION_STATUS_UNKNOWN |
        CERT_TRUST_IS_OFFLINE_REVOCATION;

    if ((certChainContext->TrustStatus.dwErrorStatus) & (~nonFatalError))
    {
        WriteInfo(TraceType, id, "CertGetCertificateChain: TrustStatus.dwErrorStatus has more than nonFatalError({0:x})", nonFatalError);
        if ((certChainContext->TrustStatus.dwErrorStatus) & CERT_TRUST_IS_REVOKED) return CRYPT_E_REVOKED;
        if ((certChainContext->TrustStatus.dwErrorStatus) & CERT_TRUST_IS_NOT_TIME_VALID) return CERT_E_EXPIRED;
        return E_FAIL;
    }

    if (issuerPubKey)
    {
        Invariant(issuerCertThumbprint);

        size_t issuerCertIndex = 0;
        if (CryptoUtility::GetCertificateIssuerChainIndex(certChainContext.get(), issuerCertIndex))
        {
            auto cContext = certChainContext->rgpChain[0]->rgpElement[issuerCertIndex]->pCertContext;
            *issuerPubKey = make_shared<X509PubKey>(cContext);
            Thumbprint::SPtr thumbprint;
            auto error = Thumbprint::Create(cContext, thumbprint);
            if (!error.IsSuccess())
            {
                WriteError(TraceType, id, "failed to get issuer thumbprint: {0}", error);
                return error.ToHResult();
            }

            *issuerCertThumbprint = thumbprint;
        }
        else
        {
            *issuerCertThumbprint = make_shared<Thumbprint>(); // default constructed Thumbprint will not match any real thumbprint value
        }
    }

    return status;
}

SECURITY_STATUS SecurityContextSsl::TryMatchCertThumbprint(
    wstring const & id,
    PCCERT_CONTEXT certContext,
    Thumbprint const & incoming,
    SECURITY_STATUS certChainErrorStatus,
    uint certChainFlags,
    bool onlyCrlOfflineEncountered,
    ThumbprintSet const & thumbprintSet)
{
    bool matched = false;
    SECURITY_STATUS status = S_FALSE;

    auto match = thumbprintSet.Value().find(incoming);
    if (match != thumbprintSet.Value().cend())
    {
        matched = true;
        WriteInfo(
            TraceType, id,
            "incoming cert thumbprint {0} matched, CertChainShouldBeVerified={1}, certChainErrorStatus=0x{2:x}",
            incoming,
            match->CertChainShouldBeVerified(),
            (unsigned int)certChainErrorStatus);

        status = match->CertChainShouldBeVerified()? certChainErrorStatus : SEC_E_OK;
    }

    CrlOfflineErrCache::GetDefault().Update(incoming, certContext, certChainFlags, onlyCrlOfflineEncountered, matched);
    return status;
}

_Use_decl_annotations_ SECURITY_STATUS SecurityContextSsl::VerifyAsServerAuthCert(
    wstring const & id,
    PCCERT_CONTEXT certContext,
    PCCERT_CHAIN_CONTEXT certChainContext,
    Common::Thumbprint const & certThumbprint,
    X509Identity::SPtr const & issuerPubKey,
    X509Identity::SPtr const & issuerCertThumbprint,
    SecurityConfig::X509NameMap const & x509NamesToMatch,
    DWORD certChainFlags,
    bool shouldIgnoreCrlOffline,
    bool onlyCrlOfflineEncountered,
    wstring * nameMatched)
{
    CERT_CHAIN_POLICY_STATUS policyStatus = { 0 };
    policyStatus.cbSize = sizeof(policyStatus);
    SSL_EXTRA_CERT_CHAIN_POLICY_PARA extraPolicyPara = { 0 };
    extraPolicyPara.cbStruct = sizeof(SSL_EXTRA_CERT_CHAIN_POLICY_PARA);
    extraPolicyPara.dwAuthType = AUTHTYPE_SERVER;

    CERT_CHAIN_POLICY_PARA policyInput = { 0 };
    policyInput.cbSize = sizeof(policyInput);
    policyInput.dwFlags = 0;
    policyInput.pvExtraPolicyPara = &extraPolicyPara;

    for (auto matchTarget = x509NamesToMatch.CBegin(); matchTarget != x509NamesToMatch.CEnd(); ++matchTarget)
    {
        if (!SecurityConfig::X509NameMap::MatchIssuer(issuerPubKey, issuerCertThumbprint, matchTarget))
            continue;

        // here we're verifying the chain policy again, this time to match a name. In this case,
        // we are explicitly allowing untrusted CAs for name-based validation with pinned issuers.
        if (!matchTarget->second.IsEmpty())
        {
            policyInput.dwFlags = CERT_CHAIN_POLICY_ALLOW_UNKNOWN_CA_FLAG;
        }

        extraPolicyPara.pwszServerName = const_cast<wchar_t*>(matchTarget->first.c_str());
        BOOL retval = FALSE;
        auto api = [&]
        {
            retval = CertVerifyCertificateChainPolicy(
                CERT_CHAIN_POLICY_SSL,
                certChainContext,
                &policyInput,
                &policyStatus);
        };

        CheckCallDuration<TraceTaskCodes::Transport>(
            api,
            SecurityConfig::GetConfig().SlowApiThreshold,
            TraceType,
            id,
            L"CertVerifyCertificateChainPolicy",
            [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });

        if (!retval)
        {
            DWORD lastError = ::GetLastError();
            WriteInfo(TraceType, id, "CertVerifyCertificateChainPolicy failed: 0x{0:x}", lastError);
            continue;
        }

        CrlOfflineErrCache::GetDefault().Test_CheckCrl(certChainFlags, (uint&)(policyStatus.dwError));

        if (policyStatus.dwError)
        {
            // CRYPT_E_REVOCATION_OFFLINE means certificate revocation service providers are offline, e.g. it can be caused by network
            // outage. It is one of the least fatal errors in certificate verification. It will only be returned if there are no other
            // errors. It is okay to ignore such an error when client machines verify server certificates, but not recommended to be
            // ignored when server machines verify client certificates. This is the case for smart card logon deployed at MS.
            if (onlyCrlOfflineEncountered)
            {
                //We only care about CRL offline error for certificates in our allow list
                //CRYPT_E_REVOCATION_OFFLINE is only reported when there is no other error, so name match succeeded 
                CrlOfflineErrCache::GetDefault().AddError(certThumbprint, certContext, certChainFlags);
            }

            if (onlyCrlOfflineEncountered && shouldIgnoreCrlOffline)
            {
                WriteWarning(TraceType, id, "ignore error 0x{0:x}:certificate revocation list offline", policyStatus.dwError);
            }
            else
            {
                if (policyStatus.dwError != CERT_E_CN_NO_MATCH)
                {
                    WriteWarning(
                        TraceType, id,
                        "CertVerifyCertificateChainPolicy('{0}') failed with policyStatus: {1}",
                        matchTarget->first,
                        policyStatus.dwError);
                }
                continue;
            }
        }

        if (matchTarget->second.IsEmpty())
        {
            WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_SERVER", matchTarget->first);
        }
        else
        {
            WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_SERVER, issuer pinned to {1}", matchTarget->first, matchTarget->second);
        }

        if (nameMatched)
        {
            *nameMatched = matchTarget->first;
        }

        return SEC_E_OK;
    }

    return ErrorCodeValue::CertificateNotMatched;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::Test_VerifyCertificate(PCCERT_CONTEXT certContext, std::wstring const & commonNameToMatch)
{
    SecurityConfig::X509NameMap names;
    names.Add(commonNameToMatch);
    return VerifyCertificate(
        L"test",
        certContext,
        SecurityConfig::GetConfig().CrlCheckingFlag,
        false,
        false,
        *emptyThumbprintSet,
        names,
        true);
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::VerifyCertificate(
    wstring const & id,
    const PCCERT_CONTEXT certContext,
    DWORD inputCertChainFlags,
    bool shouldIgnoreCrlOffline,
    bool remoteAuthenticatedAsPeer,
    ThumbprintSet const & certThumbprintsToMatch,
    SecurityConfig::X509NameMap const & x509NamesToMatch,
    bool traceCert,
    wstring * commonNameMatched)
{
    Thumbprint certThumbprint;
    auto error = certThumbprint.Initialize(certContext);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, id, "failed to get certificate thumbprint: {0}",  error);
        return error.ToHResult();
    }

    auto certChainFlags = CrlOfflineErrCache::GetDefault().MaskX509CertChainFlags(certThumbprint, inputCertChainFlags, shouldIgnoreCrlOffline);

    WriteInfo(
        TraceType, id,
        "VerifyCertificate: remoteAuthenticatedAsPeer = {0}, incoming: tp='{1}', against: certThumbprintsToMatch='{2}', x509NamesToMatch='{3}', certChainFlags=0x{4:x}, maskedFlags=0x{5:x} shouldIgnoreCrlOffline={6}",
        remoteAuthenticatedAsPeer,
        certThumbprint,
        (certThumbprintsToMatch.Value().size() < 100) ? certThumbprintsToMatch.ToString() : L"...",
        (x509NamesToMatch.Size() < 100) ? wformatString(x509NamesToMatch) : L"...",
        inputCertChainFlags,
        certChainFlags,
        shouldIgnoreCrlOffline);

    if (!remoteAuthenticatedAsPeer && x509NamesToMatch.IsEmpty() && certThumbprintsToMatch.Value().empty())
    {
        return ErrorCodeValue::CertificateNotMatched;
    }

    bool thumbprintChecked = false;
    bool issuerRetrieved = false;
    bool certTraced = false;
    X509Identity::SPtr issuerPubKey;
    X509Identity::SPtr issuerCertThumbprint;
    bool onlyCrlOfflineEncountered = false;
    if (IsClientAuthenticationCertificate(id, certContext))
    {
        CertChainContextUPtr clientAuthCertChain;
        SECURITY_STATUS status = GetCertChainContext(
            id,
            certContext,
            szOID_PKIX_KP_CLIENT_AUTH,
            certChainFlags,
            clientAuthCertChain,
            &issuerPubKey,
            &issuerCertThumbprint);

        issuerRetrieved = true;

        if (traceCert)
        {
            TraceCertificate(id, certContext, (Thumbprint const &)(*issuerCertThumbprint));
            certTraced = true;
        }

        if (FAILED(status))
        {
            return status;
        }

        auto certChainStatus = VerifySslCertChain(id, clientAuthCertChain.get(), AUTHTYPE_CLIENT, certChainFlags, shouldIgnoreCrlOffline, onlyCrlOfflineEncountered);

        if (remoteAuthenticatedAsPeer)
        {
            CrlOfflineErrCache::GetDefault().Update(certThumbprint, certContext, certChainFlags, onlyCrlOfflineEncountered, true);
            WriteInfo(TraceType, id, "VerifyCertificate: complete as remote is authenticated as peer and the chain has no fatal error");
            return SEC_E_OK;
        }

        status = TryMatchCertThumbprint(id, certContext, certThumbprint, certChainStatus, certChainFlags, onlyCrlOfflineEncountered, certThumbprintsToMatch);
        if (status == SEC_E_OK)
        {
            return status;
        }

        thumbprintChecked = true;

        // accept untrusted roots for certificates declared by subject and issuer; proceed with validation if that is the case
        if (!onlyCrlOfflineEncountered
            && FAILED(certChainStatus)
            && !SecurityContextSsl::IsExpectedCertChainTrustError(clientAuthCertChain.get(), certChainStatus, CERT_E_UNTRUSTEDROOT, CERT_TRUST_IS_UNTRUSTED_ROOT))
        {
            return certChainStatus;
        }

        wstring clientAuthCertCN;
        error = CryptoUtility::GetCertificateCommonName(certContext, clientAuthCertCN);
        if (!error.IsSuccess())
        {
            WriteWarning(TraceType, id, "GetCertificateCommonName failed: ErrorCode - {0}", error);
            return SEC_E_CERT_UNKNOWN;
        }

        SecurityConfig::X509NameMapBase::const_iterator matched;
        if (x509NamesToMatch.Match(clientAuthCertCN, issuerPubKey, issuerCertThumbprint, matched))
        {
            if (matched->second.IsEmpty())
            {
                WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_CLIENT", matched->first);
            }
            else
            {
                WriteInfo(TraceType, id, "incoming cert matched name '{0}' as AUTHTYPE_CLIENT, issuer pinned to {1}", matched->first, matched->second);
            }

            if (commonNameMatched) *commonNameMatched = matched->first;

            if (onlyCrlOfflineEncountered)
            {
                //We only care about CRL offline error for certificates in our allow list
                CrlOfflineErrCache::GetDefault().AddError(certThumbprint, certContext, certChainFlags);
            }

            if (FAILED(certChainStatus))
            {
                // accept untrusted roots with subject + (non-empty) issuer match
                if (!IsExpectedCertChainTrustError(clientAuthCertChain.get(), certChainStatus, CERT_E_UNTRUSTEDROOT, CERT_TRUST_IS_UNTRUSTED_ROOT)
                    || matched->second.IsEmpty())
                    return certChainStatus;

                WriteWarning(TraceType, id, "ignoring error 0x{0:x}: untrusted root accepted for client cert with pinned issuer", certChainStatus);
            }

            return SEC_E_OK;
        }
    }

    // When verifying as server certificate, ::CertGetCertificateChain and ::CertVerifyCertificateChainPolicy
    // do not require szOID_PKIX_KP_SERVER_AUTH exist in "Enhanced Key Usage" extension. Thus we should not check
    // if szOID_PKIX_KP_SERVER_AUTH exists in the certificate before verifying it as a server certificate, in 
    // order to be backward-compatible, since we do not check for szOID_PKIX_KP_SERVER_AUTH in previous releases.
    CertChainContextUPtr serverAuthCertChain;
    SECURITY_STATUS status = GetCertChainContext(
        id,
        certContext,
        szOID_PKIX_KP_SERVER_AUTH,
        certChainFlags,
        serverAuthCertChain,
        issuerRetrieved ? nullptr : &issuerPubKey,
        issuerRetrieved ? nullptr : &issuerCertThumbprint);

    if (traceCert && !certTraced)
    {
        TraceCertificate(id, certContext, (Thumbprint const &)(*issuerCertThumbprint));
    }

    if (FAILED(status))
    {
        return status;
    }

    auto certChainStatus = VerifySslCertChain(id, serverAuthCertChain.get(), AUTHTYPE_SERVER, certChainFlags, shouldIgnoreCrlOffline, onlyCrlOfflineEncountered);

    if (remoteAuthenticatedAsPeer)
    {
        CrlOfflineErrCache::GetDefault().Update(certThumbprint, certContext, certChainFlags, onlyCrlOfflineEncountered, true);
        WriteInfo(TraceType, id, "VerifyCertificate: complete as remote is authenticated as peer and the chain has no fatal error");
        return SEC_E_OK;
    }

    if (!thumbprintChecked)
    {
        status = TryMatchCertThumbprint(id, certContext, certThumbprint, certChainStatus, certChainFlags, onlyCrlOfflineEncountered, certThumbprintsToMatch);
        if (status == SEC_E_OK)
        {
            return status;
        }
    }

    if (!onlyCrlOfflineEncountered
        && FAILED(certChainStatus))
    {
        if (!IsExpectedCertChainTrustError(serverAuthCertChain.get(), certChainStatus, CERT_E_UNTRUSTEDROOT, CERT_TRUST_IS_UNTRUSTED_ROOT))
            return certChainStatus;

        WriteWarning(TraceType, id, "ignoring error 0x{0:x}: untrusted root accepted for server cert with pinned issuer; proceeding with verification", certChainStatus);
    }

    return VerifyAsServerAuthCert(
        id,
        certContext,
        serverAuthCertChain.get(),
        certThumbprint,
        issuerPubKey,
        issuerCertThumbprint,
        x509NamesToMatch,
        certChainFlags,
        shouldIgnoreCrlOffline,
        onlyCrlOfflineEncountered,
        commonNameMatched);
}

SECURITY_STATUS SecurityContextSsl::VerifySslCertChain(
    wstring const & id,
    PCCERT_CHAIN_CONTEXT certChainContext,
    DWORD authType,
    DWORD certChainFlags,
    bool shouldIgnoreCrlOffline,
    bool & onlyCrlOfflineEncountered)
{
    onlyCrlOfflineEncountered = false;

    CERT_CHAIN_POLICY_STATUS policyStatus = { 0 };
    policyStatus.cbSize = sizeof(policyStatus);
    SSL_EXTRA_CERT_CHAIN_POLICY_PARA extraPolicyPara = { 0 };
    extraPolicyPara.cbStruct = sizeof(SSL_EXTRA_CERT_CHAIN_POLICY_PARA);
    extraPolicyPara.dwAuthType = authType;
    extraPolicyPara.pwszServerName = NULL;

    // this chain verification is solely for the purpose of validating the authentication type,
    // and not any name match. Therefore the policy does not allow untrusted roots.
    CERT_CHAIN_POLICY_PARA policyInput = { 0 };
    policyInput.cbSize = sizeof(policyInput);
    policyInput.pvExtraPolicyPara = &extraPolicyPara;

    BOOL retval = FALSE;
    auto api = [&]
    {
        retval = CertVerifyCertificateChainPolicy(
            CERT_CHAIN_POLICY_SSL,
            certChainContext,
            &policyInput,
            &policyStatus);
    };

    CheckCallDuration<TraceTaskCodes::Transport>(
        api,
        SecurityConfig::GetConfig().SlowApiThreshold,
        TraceType,
        id,
        L"CertVerifyCertificateChainPolicy",
        [] (const wchar_t* api, TimeSpan duration, TimeSpan threshold) { ReportSecurityApiSlow(api, duration, threshold); });

    if (!retval)
    {
        DWORD lastError = ::GetLastError();
        WriteInfo(TraceType, id, "CertVerifyCertificateChainPolicy failed: 0x{0:x}", lastError);
        return HRESULT_FROM_WIN32(lastError);
    }

    CrlOfflineErrCache::GetDefault().Test_CheckCrl(certChainFlags, (uint&)(policyStatus.dwError));

    if (policyStatus.dwError)
    {
        // CRYPT_E_REVOCATION_OFFLINE means certificate revocation service providers are offline, e.g. it can be caused by network
        // outage. It is one of the least fatal errors in certificate verification. It will only be returned if there are no other
        // errors. It is okay to ignore such an error when client machines verify server certificates, but not recommended to be
        // ignored when server machines verify client certificates. This is the case for smart card logon deployed at MS.
        onlyCrlOfflineEncountered = (policyStatus.dwError == CRYPT_E_REVOCATION_OFFLINE);
        if (onlyCrlOfflineEncountered && shouldIgnoreCrlOffline)
        {
            WriteWarning(TraceType, id, "ignore error 0x{0:x}:certificate revocation list offline", policyStatus.dwError);
        }
        else
        {
            WriteInfo(TraceType, id, "CertVerifyCertificateChainPolicy({0}) failed with policy status: 0x{1:x}", authType, policyStatus.dwError);
            return policyStatus.dwError;
        }
    }

    return SEC_E_OK;
}

_Use_decl_annotations_
bool SecurityContextSsl::IsClientAuthenticationCertificate(wstring const & id, PCCERT_CONTEXT certContext)
{
    return CertCheckEnhancedKeyUsage(id, certContext, szOID_PKIX_KP_CLIENT_AUTH);
}

_Use_decl_annotations_
bool SecurityContextSsl::IsServerAuthenticationCertificate(wstring const & id, PCCERT_CONTEXT certContext)
{
    return CertCheckEnhancedKeyUsage(id, certContext, szOID_PKIX_KP_SERVER_AUTH);
}

_Use_decl_annotations_
bool SecurityContextSsl::CertCheckEnhancedKeyUsage(
    wstring const & id,
    PCCERT_CONTEXT certContext,
    LPCSTR usageIdentifier)
{
    DWORD cbUsage;

    if (CertGetEnhancedKeyUsage(certContext, 0, NULL, &cbUsage))
    {
        ScopedHeap heap;
        PCERT_ENHKEY_USAGE pEnhKeyUsage = (PCERT_ENHKEY_USAGE)heap.Allocate(cbUsage);
        if (CertGetEnhancedKeyUsage(certContext, 0, pEnhKeyUsage, &cbUsage))
        {
            for (uint i = 0; i < pEnhKeyUsage->cUsageIdentifier; ++i)
            {
                if (strcmp(pEnhKeyUsage->rgpszUsageIdentifier[i], usageIdentifier) == 0)
                {
                    return true;
                }
            }
        }
        else
        {
            WriteWarning(TraceType, id, "Getting Enhancedkey usage failed: LastError = 0x{0:x}", GetLastError());
        }
    }
    else
    {
        WriteWarning(TraceType, id, "Getting Enhancedkey usage failed: LastError = 0x{0:x}", GetLastError());
    }

    return false;
}

SECURITY_STATUS SecurityContextSsl::OnNegotiationSucceeded(SecPkgContext_NegotiationInfo const &)
{
    SECURITY_STATUS status = QueryAttributes(SECPKG_ATTR_STREAM_SIZES, &streamSizes_);
    if (status != SEC_E_OK)
    {
        WriteWarning(
            TraceType, id_,
            "OnNegotiationSucceeded: QueryAttributes(SECPKG_ATTR_STREAM_SIZES) failed: 0x{0:x}",
            (uint)status);

        return status;
    }

    decryptionInputMergeBuffer_.resize(streamSizes_.cbHeader + streamSizes_.cbMaximumMessage + streamSizes_.cbTrailer);
    WriteInfo(TraceType, id_, "decryptionInputMergeBuffer_ set to {0} bytes", decryptionInputMergeBuffer_.size());

    if (TransportConfig::GetConfig().SslReceiveChunkSize < 2* decryptionInputMergeBuffer_.size())
    {
        WriteError(
            TraceType, id_,
            "config Transport::SslReceiveChunkSize to {0} or larger, the minimum is twice SSL record size",
            2* decryptionInputMergeBuffer_.size());

        return E_FAIL;
    }

    if (!AuthenticateRemoteByClaims())
    {
        PCERT_CONTEXT remoteCertContext = nullptr;
        status =  QueryAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT, &remoteCertContext);
        remoteCertContext_.reset(remoteCertContext);
        if (FAILED(status))
        {
            WriteWarning(
                TraceType, id_,
                "OnNegotiationSucceeded: QueryAttributes(SECPKG_ATTR_REMOTE_CERT_CONTEXT) failed: 0x{0:x}",
                (uint)status);

            return status;
        }
    }

    return status;
}

bool SecurityContextSsl::AccessCheck(AccessControl::FabricAcl const & acl, DWORD desiredAccess) const
{
    if (AuthenticateRemoteByClaims())
    {
        return acl.AccessCheckClaim(clientClaims_, desiredAccess);
    }

    if (acl.AccessCheckX509(remoteCommonName_, desiredAccess))
    {
        return true;
    }

    // A certificate may have more than one common name, the one matched for connection authorization(remoteCommonName_) may not appear in ACL.
    DWORD accessGranted = 0;
    for (auto const & ace : acl.AceList())
    {
        if ((ace.Type() == FabricAceType::Allowed) && (ace.Principal().Type() == SecurityPrincipalType::X509))
        {
            SecurityPrincipalX509 & principal = (SecurityPrincipalX509 &)ace.Principal();
            SecurityConfig::X509NameMap namesToMatch;
            namesToMatch.Add(principal.CommonName(), X509IdentitySet() /* no need to check issuer as it has been done for connection authorization*/);
            if (VerifyCertificate(
                    id_,
                    remoteCertContext_.get(),
                    transportSecurity_->Settings().X509CertChainFlags(),
                    transportSecurity_->Settings().ShouldIgnoreCrlOfflineError(inbound_),
                    remoteAuthenticatedAsPeer_,
                    *emptyThumbprintSet,
                    namesToMatch,
                    false) == SEC_E_OK)
            {
                accessGranted |= ace.AccessMask();
                if ((accessGranted & desiredAccess) == desiredAccess)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::WriteSslRecord(ULONG dataLength, byte * record, ULONG & sslRecordSize)
{
    SecBuffer buffers[3];
    buffers[0].BufferType = SECBUFFER_STREAM_HEADER;
    buffers[0].cbBuffer = streamSizes_.cbHeader;
    buffers[0].pvBuffer = record;

    buffers[1].BufferType = SECBUFFER_DATA;
    buffers[1].cbBuffer = dataLength;
    buffers[1].pvBuffer = record + streamSizes_.cbHeader;

    buffers[2].BufferType = SECBUFFER_STREAM_TRAILER;
    buffers[2].cbBuffer = streamSizes_.cbTrailer;
    buffers[2].pvBuffer = record + streamSizes_.cbHeader + dataLength;

    SecBufferDesc bufferDesc;
    bufferDesc.ulVersion = SECBUFFER_VERSION;
    bufferDesc.cBuffers = 3;
    bufferDesc.pBuffers = buffers;

    SECURITY_STATUS status = ::EncryptMessage(&hSecurityContext_, 0, &bufferDesc, 0);
    if (status == SEC_E_OK)
    {
        KAssert(buffers[0].cbBuffer == streamSizes_.cbHeader);
        KAssert(buffers[1].cbBuffer == dataLength);
        sslRecordSize = buffers[0].cbBuffer + buffers[1].cbBuffer + buffers[2].cbBuffer;
    }
    else
    {
        sslRecordSize = 0;
    }

    return status;
}

class SecurityContextSsl::TemplateHelper
{
public:
    template<class TChunkIterator>
    static SECURITY_STATUS Encrypt(
        SecurityContextSsl & context,
        TChunkIterator const & inputChunkBegin,
        TChunkIterator const & inputChunkEnd,
        byte * const encrypted,
        _Inout_ ULONG & outputSize);
};

_Use_decl_annotations_
HRESULT SecurityContextSsl::GetSslOutputLimit(ULONG inputBytes, ULONG & outputLimit)
{
    outputLimit = 0;

    KAssert(streamSizes_.cbMaximumMessage > 0);
    ULONG sslRecordCount;
    HRESULT hr = ULongAdd(inputBytes, streamSizes_.cbMaximumMessage - 1, &sslRecordCount);
    if (FAILED(hr)) return hr;

    sslRecordCount /= streamSizes_.cbMaximumMessage;

    ULONG sslRecordOverheadMax = streamSizes_.cbHeader + streamSizes_.cbTrailer;
    KAssert(sslRecordOverheadMax >= streamSizes_.cbHeader); // no overflow

    hr = ULongMult(sslRecordCount, sslRecordOverheadMax, &outputLimit);
    if (FAILED(hr)) return hr;

    return ULongAdd(inputBytes, outputLimit, &outputLimit);
}

_Use_decl_annotations_
template<class TChunkIterator>
SECURITY_STATUS SecurityContextSsl::TemplateHelper::Encrypt(
    SecurityContextSsl & context,
    TChunkIterator const & inputChunkBegin,
    TChunkIterator const & inputChunkEnd,
    byte * const encrypted,
    ULONG & outputSize)
{
    SECURITY_STATUS status = SEC_E_OK;
    byte* sslRecord = encrypted;
    ULONG sslRecordSize;
    SecPkgContext_StreamSizes const & streamSizes = context.streamSizes_;
    byte* outputPtr = encrypted + streamSizes.cbHeader;
    const ULONG outputCapacity = outputSize;
    ULONG copiedToCurrentRecord = 0;

    // Pack max size SSL record when possible
    for (auto iter = inputChunkBegin; iter != inputChunkEnd; ++ iter)
    {
        byte* inputPtr = (byte*)iter->buf;

        for (ULONG remainedInThisChunk = iter->len; remainedInThisChunk > 0; )
        {
            if (remainedInThisChunk >= (streamSizes.cbMaximumMessage - copiedToCurrentRecord))
            {
                KMemCpySafe(outputPtr, encrypted + outputCapacity - outputPtr, inputPtr, streamSizes.cbMaximumMessage - copiedToCurrentRecord);

                status = context.WriteSslRecord(streamSizes.cbMaximumMessage, sslRecord, sslRecordSize);
                if (FAILED(status))
                {
                    return status;
                }

                remainedInThisChunk -= (streamSizes.cbMaximumMessage - copiedToCurrentRecord);
                inputPtr += (streamSizes.cbMaximumMessage - copiedToCurrentRecord);

                sslRecord += sslRecordSize;
                outputPtr = sslRecord + streamSizes.cbHeader;
                copiedToCurrentRecord = 0;
            }
            else
            {
                KMemCpySafe(outputPtr, encrypted + outputCapacity - outputPtr, inputPtr, remainedInThisChunk);
                outputPtr += remainedInThisChunk;
                copiedToCurrentRecord += remainedInThisChunk;
                break;
            }
        }
    }

    if (copiedToCurrentRecord == 0)
    {
        outputSize = (ULONG)(sslRecord - encrypted);
    }
    else
    {
        // extra data to encrypt, because last ssl record is not at max-size
        status = context.WriteSslRecord(copiedToCurrentRecord, sslRecord, sslRecordSize);
        outputSize = (ULONG)(sslRecord - encrypted) + sslRecordSize;
    }

    return status;
}

SECURITY_STATUS SecurityContextSsl::EncodeMessage(TcpFrameHeader const & frameHeader, Message & message, ByteBuffer2 & encrypted)
{
    ULONG outputLimit;
    auto status = GetSslOutputLimit(frameHeader.FrameLength(), outputLimit);
    if (FAILED(status)) return status;

    encrypted.resize(outputLimit);

    byte* sslRecord = encrypted.data();
    ULONG sslRecordSize;
    SecPkgContext_StreamSizes const & streamSizes = streamSizes_;
    byte* outputPtr = encrypted.data() + streamSizes.cbHeader;

    // add frame header to encryption stream
    Invariant(sizeof(frameHeader) <= streamSizes.cbMaximumMessage);
    KMemCpySafe(outputPtr, encrypted.end() - outputPtr, &frameHeader, sizeof(frameHeader));
    outputPtr += sizeof(frameHeader);
    ULONG copiedToCurrentRecord = sizeof(frameHeader);

    // Pack max size SSL record when possible
    auto iter1 = message.BeginHeaderChunks();
    auto iter2 = message.BeginBodyChunks();
    byte* inputPtr = nullptr;
    ULONG chunkSize = 0;
    for(;;)
    {
        if (iter1 != message.EndHeaderChunks())
        {
            inputPtr = (byte*)iter1->buf;
            chunkSize = iter1->len;
            ++iter1;
        }
        else if (iter2 != message.EndBodyChunks())
        {
            inputPtr = (byte*)iter2->buf;
            chunkSize = iter2->len;
            ++iter2;
        }
        else
        {
            break;
        }

        for (ULONG remainedInThisChunk = chunkSize; remainedInThisChunk > 0; )
        {
            if (remainedInThisChunk >= (streamSizes.cbMaximumMessage - copiedToCurrentRecord))
            {
                KMemCpySafe(outputPtr, encrypted.end() - outputPtr, inputPtr, streamSizes.cbMaximumMessage - copiedToCurrentRecord);

                status = WriteSslRecord(streamSizes.cbMaximumMessage, sslRecord, sslRecordSize);
                if (FAILED(status))
                {
                    return status;
                }

                remainedInThisChunk -= (streamSizes.cbMaximumMessage - copiedToCurrentRecord);
                inputPtr += (streamSizes.cbMaximumMessage - copiedToCurrentRecord);

                sslRecord += sslRecordSize;
                outputPtr = sslRecord + streamSizes.cbHeader;
                copiedToCurrentRecord = 0;
            }
            else
            {
                KMemCpySafe(outputPtr, encrypted.end() - outputPtr, inputPtr, remainedInThisChunk);
                outputPtr += remainedInThisChunk;
                copiedToCurrentRecord += remainedInThisChunk;
                break;
            }
        }
    }

    if (copiedToCurrentRecord == 0)
    {
        auto outputSize = sslRecord - encrypted.data();
        encrypted.resize(outputSize);
    }
    else
    {
        // extra data to encrypt, because last ssl record is not at max-size
        status = WriteSslRecord(copiedToCurrentRecord, sslRecord, sslRecordSize);
        auto outputSize = sslRecord - encrypted.data() + sslRecordSize;
        encrypted.resize(outputSize);
    }

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::EncodeMessage(MessageUPtr & message)
{
    SECURITY_STATUS status = SEC_E_OK;

    SecurityHeader securityHeader(transportSecurity_->MessageHeaderProtectionLevel, transportSecurity_->MessageBodyProtectionLevel);

    if (transportSecurity_->MessageHeaderProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        ULONG outputLimit;
        status = GetSslOutputLimit(message->SerializedHeaderSize(), outputLimit);
        if (FAILED(status)) return status;

        securityHeader.HeaderData.resize(outputLimit);
        ULONG outputSize = (ULONG)securityHeader.HeaderData.size();
        status = TemplateHelper::Encrypt<BiqueChunkIterator>(
            *this,
            message->BeginHeaderChunks(),
            message->EndHeaderChunks(),
            securityHeader.HeaderData.data(),
            outputSize);

        if (FAILED(status))
        {
            return status;
        }

        securityHeader.HeaderData.resize(outputSize);

        // remove headers if no encryption of body (which would create a new message)
        if (transportSecurity_->MessageBodyProtectionLevel != ProtectionLevel::EncryptAndSign)
        {
            // Headers were already compacted so replacing the existing headers is sufficient
            message->Headers.ReplaceUnsafe(ByteBiqueRange(EmptyByteBique.begin(), EmptyByteBique.end(), false));
        }
    }

    if (transportSecurity_->MessageBodyProtectionLevel == ProtectionLevel::EncryptAndSign)
    {
        ULONG outputLimit;
        status = GetSslOutputLimit(message->SerializedBodySize(), outputLimit);
        if (FAILED(status)) return status;

        byte * bodyData = new byte[outputLimit];
        ULONG outputSize = (ULONG)outputLimit;
        status = TemplateHelper::Encrypt<BufferIterator>(
            *this,
            message->BeginBodyChunks(),
            message->EndBodyChunks(),
            bodyData,
            outputSize);

        if (FAILED(status))
        {
            delete [] bodyData;
            return status;
        }

        vector<const_buffer> bufferList;
        bufferList.push_back(const_buffer(bodyData, outputSize));

        auto secureMessage = std::unique_ptr<Message>(new Message(bufferList, SecurityContext::FreeByteBuffer, bodyData));

        if (transportSecurity_->MessageHeaderProtectionLevel != ProtectionLevel::EncryptAndSign)
        {
            // If headers are encrypted they will all be in the securityHeader, otherwise we need to move them
            // to the secure message
            secureMessage->Headers.AppendFrom(message->Headers);
        }

        secureMessage->SetTraceId(message->TraceId());
        secureMessage->MoveLocalTraceContextFrom(*message);

        if(message->HasSendStatusCallback())
        {
            secureMessage->SetSendStatusCallback(message->get_SendStatusCallback());
        }
        
        message = std::move(secureMessage);
    }

    message->Headers.Add<SecurityHeader>(securityHeader);

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::SspiDecrypt(_Inout_ byte* input, ULONG inputLength, SecBuffer buffers[4])
{
    buffers[0].BufferType = SECBUFFER_DATA;
    buffers[0].pvBuffer = input;
    buffers[0].cbBuffer = inputLength;
    buffers[1].BufferType = buffers[2].BufferType = buffers[3].BufferType = SECBUFFER_EMPTY;

    SecBufferDesc bufferDesc = {SECBUFFER_VERSION, 4, buffers};

    SECURITY_STATUS status = ::DecryptMessage(&hSecurityContext_, &bufferDesc, 0, nullptr);
    if ((buffers[1].BufferType != SECBUFFER_DATA) && (status == SEC_E_OK))
    {
        WriteError(
            TraceType, id_,
            "on output, buffers[1] should contain decrypted data, type {0} is not expected",
            buffers[1].BufferType);

        return E_FAIL;
    }

    return status;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::DecryptMessageHeaders(vector<byte> & input, ByteBique & output)
{
    if (input.empty())
    {
        output.truncate_before(output.end());
        return SEC_E_OK;
    }

    output.reserve_back(input.size());
    const ULONG outputBufferSize = (ULONG)input.capacity();
    ULONG bytesToDecrypt = (ULONG)input.size();
    byte* inputCursor = input.data();
    byte* outputCursor = output.end().fragment_begin();
    byte* const outputStart = outputCursor;

    for (;;)
    {
        SecBuffer buffers[4] = {};
        SECURITY_STATUS status = SspiDecrypt(inputCursor, bytesToDecrypt, buffers);
        if (status != SEC_E_OK)
        {
            return status;
        }

        // copy decrypted data
        KMemCpySafe(outputCursor, outputStart + outputBufferSize - outputCursor, buffers[1].pvBuffer, buffers[1].cbBuffer);
        outputCursor += buffers[1].cbBuffer;

        // check for extra input
        int  bufferIndex = 3;
        while ((buffers[ bufferIndex].BufferType != SECBUFFER_EXTRA) && ( bufferIndex-- != 0));

        if (bufferIndex >= 0)
        {
            bytesToDecrypt = buffers[bufferIndex].cbBuffer;
            inputCursor = input.data() + input.size() - bytesToDecrypt;
            continue;
        }

        break;
    }

    output.no_fill_advance_back(outputCursor - output.end().fragment_begin());
    return SEC_E_OK;
}

_Use_decl_annotations_
SECURITY_STATUS SecurityContextSsl::DecryptMessageBody(MessageUPtr & message)
{
    const size_t encryptedBodySize = message->SerializedBodySize();
    if (encryptedBodySize == 0)
    {
        return SEC_E_OK;
    }

    // Get decrypted buffer count upper bound:
    // Base: one decrypted buffer per SSL record
    // Extra: decrypted record may occupy two buffers when SSL record span across two adjacent receive chunks
    const ULONG sslRecordSize = (ULONG)decryptionInputMergeBuffer_.size();
    const ULONG receiveChunkSize = (ULONG)TransportConfig::GetConfig().SslReceiveChunkSize;
    size_t decrypedBufferCountMax = 
        (encryptedBodySize +  sslRecordSize - 1) / sslRecordSize /* SSL record count */ +
        (encryptedBodySize + receiveChunkSize - 1) / receiveChunkSize + 1 /* Split SSL record count */;

    Message::BodyBuffers decryptedBuffers(decrypedBufferCountMax);

    byte* decryptBegin = nullptr;
    byte* decryptEnd = nullptr;
    ULONG bytesToDecrypt = 0;
    ULONG readFromThisChunk = 0;
    byte* sslRecordStartInFirstChunk = nullptr;
    SECURITY_STATUS status = SEC_E_OK;
    for (auto chunk = message->BeginBodyChunks(); chunk != message->EndBodyChunks(); )
    {
        const bool inputMergeBufferUsed = decryptBegin == decryptionInputMergeBuffer_.data();
        if (inputMergeBufferUsed) // incomplete message in previous decryption, need more input
        {
            readFromThisChunk = std::min(chunk->len, sslRecordSize - bytesToDecrypt);
            KMemCpySafe(
                decryptBegin + bytesToDecrypt,
                decryptionInputMergeBuffer_.data() + decryptionInputMergeBuffer_.size() - (decryptBegin + bytesToDecrypt),
                chunk->buf,
                readFromThisChunk);

            bytesToDecrypt += readFromThisChunk;
            decryptEnd = (byte*)chunk->buf + readFromThisChunk;
        }
        else if (bytesToDecrypt == 0)
        {
            decryptBegin = (byte*)chunk->buf;
            bytesToDecrypt = chunk->len;
            decryptEnd = decryptBegin + bytesToDecrypt;
            readFromThisChunk = chunk->len;
        }

        SecBuffer buffers[4] = {};
        status = SspiDecrypt(decryptBegin, bytesToDecrypt, buffers);
        if (status == SEC_E_INCOMPLETE_MESSAGE)
        {
            if (inputMergeBufferUsed)
            {
                WriteError(
                    TraceType, id_,
                    "bytesToDecrypt={0}, chunk->len={1}, decryptionInputMergeBuffer_.size()={2}, full decryption buffer contains incomplete ssl record, cannot happen as ssl receive chunk is larger than twice SSL record limit",
                    bytesToDecrypt,
                    chunk->len,
                    decryptionInputMergeBuffer_.size());

                return status;
            }

            KAssert(decryptionInputMergeBuffer_.size() > bytesToDecrypt);
            KMemCpySafe(decryptionInputMergeBuffer_.data(), decryptionInputMergeBuffer_.size(), decryptBegin, bytesToDecrypt);
            sslRecordStartInFirstChunk = decryptBegin;
            decryptBegin = decryptionInputMergeBuffer_.data();

            ++chunk;
            readFromThisChunk = 0;

            continue;
        }

        if (status != SEC_E_OK)
        {
            WriteError(TraceType, id_, "decrypt failed: 0x{0:x}", (DWORD)status);
            return status;
        }

        if (inputMergeBufferUsed)
        {
            // copy decrypted buffer back to original receive buffers
            KAssert(readFromThisChunk > 0);
            ULONG copyToFirstChunk = bytesToDecrypt - readFromThisChunk;
            ULONG copyToSecondChunk = buffers[1].cbBuffer - copyToFirstChunk;
            if (copyToFirstChunk >= buffers[1].cbBuffer)
            {
                // This is possible due to gaps of SSL header and trailer after decryption
                copyToFirstChunk = buffers[1].cbBuffer;
                copyToSecondChunk = 0;
            }

            KMemCpySafe(sslRecordStartInFirstChunk, copyToFirstChunk, buffers[1].pvBuffer, copyToFirstChunk);
            decryptedBuffers.AddBuffer(sslRecordStartInFirstChunk, copyToFirstChunk);

            if (copyToSecondChunk > 0)
            {
                KMemCpySafe(chunk->buf, chunk->len, (byte*)buffers[1].pvBuffer + copyToFirstChunk, copyToSecondChunk);
                decryptedBuffers.AddBuffer(chunk->buf, copyToSecondChunk);
            }
        }
        else
        {
            decryptedBuffers.AddBuffer(buffers[1].pvBuffer, buffers[1].cbBuffer);
        }

        // check for extra input
        int  bufferIndex = 3;
        while ((buffers[ bufferIndex].BufferType != SECBUFFER_EXTRA) && ( bufferIndex-- != 0));

        if (bufferIndex >= 0)
        {
            decryptBegin = decryptEnd - buffers[bufferIndex].cbBuffer;
            bytesToDecrypt = buffers[bufferIndex].cbBuffer;
            if (inputMergeBufferUsed)
            {
                // There could be more input in this chunk
                bytesToDecrypt += (chunk->len - readFromThisChunk);
                decryptEnd += (chunk->len - readFromThisChunk);
                readFromThisChunk = chunk->len;
            }

            continue;
        }

        bytesToDecrypt = chunk->len - readFromThisChunk;
        if (bytesToDecrypt) // More input in this chunk
        {
            KAssert(inputMergeBufferUsed);
            decryptBegin = decryptEnd;
            decryptEnd = decryptBegin + bytesToDecrypt;
            readFromThisChunk = chunk->len;
            continue;
        }

        decryptBegin = nullptr;
        KAssert(readFromThisChunk == chunk->len);
        readFromThisChunk = 0;
        ++chunk;
    }

    if (status == SEC_E_OK)
    {
        message->UpdateBodyBufferList(move(decryptedBuffers));
    }

    return status;
}

#endif

void SecurityContextSsl::TryAuthenticateRemoteAsPeer()
{
    if (!transportSecurity_->Settings().IsRemotePeer()) return;

    if (!remoteCertContext_) return;

    if (!SecurityConfig::GetConfig().ImplicitPeerTrustEnabled)
    {
        WriteInfo(TraceType, id_, "TryAuthenticateRemoteAsPeer: ImplicitPeerTrustEnabled = false");
        return;
    }

    X509PubKey remotePubKey(remoteCertContext_.get());

    auto localX509PubKey = credentials_.front()->X509PublicKey();
    if (!localX509PubKey)
    {
        WriteError(TraceType, id_, "TryAuthenticateRemoteAsPeer: local X509PublicKey is null");
        Assert::TestAssert(TraceType, id_, "local X509PublicKey cannot be null in peer-to-peer mode");
        return;
    }

    if (remotePubKey == *localX509PubKey)
    {
        WriteInfo(TraceType, id_, "TryAuthenticateRemoteAsPeer: remote public key matches local: {0}, RoleMask = {1}", remotePubKey, roleMask_);
        remoteAuthenticatedAsPeer_ = true;
        return;
    }
}
