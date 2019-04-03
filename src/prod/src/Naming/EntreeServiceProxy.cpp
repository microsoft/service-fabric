// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "AzureActiveDirectory/wrapper.server/ServerWrapper.h"

using namespace std;
using namespace Api;
using namespace Common;
using namespace Transport;
using namespace ClientServerTransport;
using namespace ServiceModel;
using namespace Naming;
using namespace Client;
using namespace SystemServices;
using namespace Query;
using namespace Management::TokenValidationService;

static const StringLiteral RequestProcessingTraceComponent("EntreeServiceProxy.RequestProcessing");
static const StringLiteral RequestProcessingFailedTraceComponent("EntreeServiceProxy.RequestProcessingFailed");

static const StringLiteral LifeCycleTraceComponent("LifeCycle");
#define GET_RC( ResourceName ) Common::StringResource::Get( IDS_NAMING_##ResourceName )

#define ADD_DEFAULT_CONFIGBASED_RBAC_HANDLER(MessageAction, RoleNameInConfig)                   \
{                                                                                               \
    roleBasedAccessCheckMap_[MessageAction] =                                                   \
    [this](MessageUPtr &message){ return this->DefaultActionHeaderBasedAccessCheck(message, ClientAccessConfig::GetConfig().RoleNameInConfig); }; \
}                                                                                               \

#define ADD_RBAC_HANDLER(MessageAction, roles)                                                  \
{                                                                                               \
    roleBasedAccessCheckMap_[MessageAction] =                                                   \
        [this](MessageUPtr &message){ return this->DefaultActionHeaderBasedAccessCheck(message, roles); }; \
}

#define ACCESS_CHECK( Header, MessageType, Operation )                                          \
if (Header.Action == MessageType::Operation##Action)                                      \
{                                                                                               \
    return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().Operation##Roles);       \
}                                                                                               \

#define NS_ADMIN_ACCESS_CHECK( Operation ) ADD_RBAC_HANDLER( NamingTcpMessage::Operation##Action, RoleMask::Enum::Admin )
#define NS_ALL_ACCESS_CHECK( Operation ) ADD_RBAC_HANDLER( NamingMessage::Operation##Action, RoleMask::Enum::All )
#define NS_ROLE_ACCESS_CHECK( Operation, Role ) ADD_DEFAULT_CONFIGBASED_RBAC_HANDLER( NamingTcpMessage::Operation##Action, Role##Roles )
#define NS_ACCESS_CHECK( Operation ) NS_ROLE_ACCESS_CHECK( Operation, Operation )
#define CM_ACCESS_CHECK( Operation ) ACCESS_CHECK( forwardMessageHeader, ClusterManagerTcpMessage, Operation )
#define DOCKER_COMPOSE_ACCESS_CHECK( Operation ) ACCESS_CHECK( forwardMessageHeader, ContainerOperationTcpMessage, Operation )
#define HM_ACCESS_CHECK( Operation ) ACCESS_CHECK( forwardMessageHeader, HealthManagerTcpMessage, Operation )
#define IS_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, InfrastructureServiceTcpMessage, Operation )
#define FS_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, FileStoreServiceTcpMessage, Operation )
#define FAS_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, FaultAnalysisServiceTcpMessage, Operation )
#define FUOS_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, UpgradeOrchestrationServiceTcpMessage, Operation )
#define VOLUME_MANAGER_ACCESS_CHECK( Operation ) ACCESS_CHECK( forwardMessageHeader, VolumeOperationTcpMessage, Operation )
#define FCSS_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, CentralSecretServiceMessage, Operation )
#define GATEWAY_RESOURCE_MANAGER_ACCESS_CHECK( Operation ) ACCESS_CHECK( routingAgentHeader, GatewayResourceManagerTcpMessage, Operation )

class EntreeServiceProxy::ClientRequestJobItem : public JobItem<EntreeServiceProxy>
{
public:
    ClientRequestJobItem(
        MessageUPtr &&message,
        ReceiverContextUPtr &&context)
        : message_(move(message))
        , context_(move(context))
    {
    }

    virtual void Process(EntreeServiceProxy &owner)
    {
        owner.ProcessClientMessage(move(message_), context_->ReplyTarget);
    }

private:
    MessageUPtr message_;
    ReceiverContextUPtr context_;
};

EntreeServiceProxy::EntreeServiceProxy(
    ComponentRoot const & root,
    wstring const & listenAddress,
    ProxyToEntreeServiceIpcChannel &proxyToEntreeServiceTransport,
    SecuritySettings const& securitySettings)
    : HealthReportingTransport(root)
    , NodeTraceComponent(proxyToEntreeServiceTransport.Instance)
    , namingConfig_(NamingConfig::GetConfig())
    , listenAddress_(listenAddress)
    , instanceString_(proxyToEntreeServiceTransport.Instance.ToString())
    , proxyToEntreeServiceTransport_(proxyToEntreeServiceTransport)
    , initialSecuritySettings_(securitySettings)
    , fileTransferGatewayUPtr_()
    , roleBasedAccessCheckMapLock_()
    , perfCounters_()
    , connectedClientCount_(0)
    , connectedClientCountLock_()
    , isAadServiceEnabled_(false)
{
    WriteNoise(LifeCycleTraceComponent, TraceId, "constructor called");

    AddRoleBasedAccessControlHandlers();
    LoadIsAadServiceEnabled();
}

EntreeServiceProxy::~EntreeServiceProxy()
{
    WriteNoise(LifeCycleTraceComponent, TraceId, "destructor called");
}

ErrorCode EntreeServiceProxy::OnOpen()
{
    WriteNoise(LifeCycleTraceComponent, TraceId, "Entering OnOpen");

    auto error = InitializeExternalChannel();
    if (!error.IsSuccess())
    {
        WriteWarning(
            LifeCycleTraceComponent,
            TraceId,
            "Failed to initialize external channel error : {0}",
            error);
        return error;
    }

    fileTransferGatewayUPtr_ = make_unique<FileTransferGateway>(
        Instance,
        receivingChannel_,
        demuxer_,
        this->Root);

    error = fileTransferGatewayUPtr_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            LifeCycleTraceComponent, 
            TraceId,
            "FileTransferGateway failed to open with error {0}", 
            error);

        return error;
    }

    if (error.IsSuccess())
    {
        perfCounters_ = GatewayPerformanceCounters::CreateInstance(this->Instance.ToString());
    }

    WriteInfo(
        LifeCycleTraceComponent,
        TraceId,
        "OpenComplete, listenAddress {0}, error : {1}",
        listenAddress_,
        error);

    return error;
}

void EntreeServiceProxy::OnConnectionAccepted(ISendTarget const & replyTarget)
{
    AcquireWriteLock lock(connectedClientCountLock_);

    auto max = ServiceModelConfig::GetConfig().MaxClientConnections;
    auto count = ++connectedClientCount_;

    if (perfCounters_)
    {
        perfCounters_->NumberOfClientConnections.Increment();
    }

    if (count <= max)
    {
        WriteInfo(
            LifeCycleTraceComponent,
            TraceId,
            "accepted client connection: target={0} total={1}/{2}",
            replyTarget.Id(),
            count,
            max);
    }
    else
    {
        WriteWarning(
            LifeCycleTraceComponent,
            TraceId,
            "exceeded client connection limit: target={0} max={1}",
            replyTarget.Id(),
            max);
        
        const_cast<ISendTarget&>(replyTarget).Reset();
    }
}

void EntreeServiceProxy::OnConnectionFault(ISendTarget const & replyTarget, ErrorCode const & error)
{
    AcquireWriteLock lock(connectedClientCountLock_);

    auto count = --connectedClientCount_;

    if (perfCounters_)
    {
        perfCounters_->NumberOfClientConnections.Decrement();
    }

    WriteInfo(
        LifeCycleTraceComponent,
        TraceId,
        "faulted client connection: target={0} error={1} total={2}",
        replyTarget.Id(),
        error,
        count);
}

ErrorCode EntreeServiceProxy::SetSecurity(Transport::SecuritySettings const & securitySettings)
{
    if (securitySettings.SecurityProvider() == SecurityProvider::None)
    {
        isClientRoleInEffect_ = false;
    }

    auto error = receivingChannel_->SetSecurity(securitySettings);
    if (error.IsSuccess())
    {
        WriteInfo(
            LifeCycleTraceComponent,
            TraceId,
            "Set security settings succeeded, new value = {0}",
            securitySettings);
    }
    else
    {
        WriteWarning(
            LifeCycleTraceComponent,
            TraceId,
            "Set security settings failed: error = {0}, new value {1}",
            error, 
            securitySettings);
    }

    return error;
}

ErrorCode EntreeServiceProxy::OnClose()
{
    WriteNoise(LifeCycleTraceComponent, TraceId, "Entering OnClose");

    ErrorCode error;
    if (fileTransferGatewayUPtr_)
    {
        error = fileTransferGatewayUPtr_->Close();
    }

    if (error.IsSuccess() && notificationManagerProxyUPtr_)
    {
        error = notificationManagerProxyUPtr_->Close();
    }

    if(error.IsSuccess() && demuxer_)
    {
        demuxer_->UnregisterMessageHandler(Actor::NamingGateway);
        error = demuxer_->Close();
        if (!error.IsSuccess())
        {
            WriteWarning(
                LifeCycleTraceComponent, 
                TraceId,
                "Demuxer failed to close with error {0}", 
                error);
        }
    }

    if (requestReply_)
    {
        requestReply_->Close();
    }

    this->receivingChannel_->Stop();

    WriteInfo(LifeCycleTraceComponent, TraceId, "Leaving OnClose");
    return error;
}

void EntreeServiceProxy::OnAbort()
{
    WriteNoise(LifeCycleTraceComponent, TraceId, "Entering OnAbort");

    if (fileTransferGatewayUPtr_)
    {
        fileTransferGatewayUPtr_->Abort();
    }

    if (notificationManagerProxyUPtr_)
    {
        notificationManagerProxyUPtr_->Abort();
    }

    if(demuxer_)
    {
        demuxer_->UnregisterMessageHandler(Actor::NamingGateway);
        demuxer_->Abort();
    }

    if (requestReply_)
    {
        requestReply_->Close();
    }

    if (receivingChannel_)
    {            
        receivingChannel_->Stop();
    }

    WriteInfo(LifeCycleTraceComponent, TraceId, "Leaving OnAbort");
}

void EntreeServiceProxy::AddRoleBasedAccessControlHandlers()
{
    AcquireWriteLock writeLock(roleBasedAccessCheckMapLock_);

    //
    // Default implementation of access check to directly compare the action in the message against the user role 
    // configured for that action
    //
    NS_ACCESS_CHECK( CreateName )
    NS_ACCESS_CHECK( DeleteName )
    NS_ACCESS_CHECK( NameExists )
    NS_ACCESS_CHECK( EnumerateProperties )
    NS_ACCESS_CHECK( ResolveNameOwner )
    NS_ACCESS_CHECK( ResolvePartition )
    NS_ACCESS_CHECK( ResolveService )
    NS_ACCESS_CHECK( PrefixResolveService )
    NS_ACCESS_CHECK( ResolveSystemService)
    NS_ACCESS_CHECK( CreateService )
    NS_ACCESS_CHECK( UpdateService )
    NS_ACCESS_CHECK( DeleteService )
    NS_ACCESS_CHECK( GetServiceDescription )
    NS_ACCESS_CHECK( DeactivateNode )
    NS_ACCESS_CHECK( ActivateNode )
    NS_ACCESS_CHECK( NodeStateRemoved )
    NS_ACCESS_CHECK( RecoverPartitions )
    NS_ACCESS_CHECK( RecoverPartition )
    NS_ACCESS_CHECK( RecoverServicePartitions )
    NS_ACCESS_CHECK( RecoverSystemPartitions )
    NS_ACCESS_CHECK( ResetPartitionLoad )
    NS_ACCESS_CHECK( ToggleVerboseServicePlacementHealthReporting )
    NS_ACCESS_CHECK( ReportFault )
    NS_ACCESS_CHECK( CreateNetwork)
    NS_ACCESS_CHECK( DeleteNetwork)

    //
    // Role names do not match action names
    //
    NS_ROLE_ACCESS_CHECK( PingRequest, Ping )
    NS_ROLE_ACCESS_CHECK( EnumerateSubNames, EnumerateSubnames )
    NS_ROLE_ACCESS_CHECK( DeactivateNodesBatchRequest, DeactivateNodesBatch )
    NS_ROLE_ACCESS_CHECK( RemoveNodeDeactivationsRequest, RemoveNodeDeactivations )
    NS_ROLE_ACCESS_CHECK( GetNodeDeactivationStatusRequest, GetNodeDeactivationStatus )
    NS_ROLE_ACCESS_CHECK( ServiceLocationChangeListener, GetServiceDescription )
    NS_ROLE_ACCESS_CHECK( RegisterServiceNotificationFilterRequest, ServiceNotifications )
    NS_ROLE_ACCESS_CHECK( UnregisterServiceNotificationFilterRequest, ServiceNotifications )
    NS_ROLE_ACCESS_CHECK( NotificationClientConnectionRequest, ServiceNotifications )
    NS_ROLE_ACCESS_CHECK( NotificationClientSynchronizationRequest, ServiceNotifications )
    NS_ROLE_ACCESS_CHECK( StartNode, NodeControl )

    //
    // Always admin - not configurable
    //
    NS_ADMIN_ACCESS_CHECK( CreateSystemServiceRequest )
    NS_ADMIN_ACCESS_CHECK( DeleteSystemServiceRequest )
    NS_ADMIN_ACCESS_CHECK( ResolveSystemServiceRequest )

    //
    // Internal only, so allow all roles.
    //
    NS_ALL_ACCESS_CHECK( ForwardToTvsMessage )

    //
    // Requests that need custom access check based on other data on the message.
    //
    roleBasedAccessCheckMap_[NamingMessage::ForwardMessageAction] = [this](MessageUPtr &message) { return this->ForwardToServiceAccessCheck(message); };
    roleBasedAccessCheckMap_[NamingMessage::ForwardToFileStoreMessageAction] = [this](MessageUPtr &message) { return this->ForwardToFileStoreAccessCheck(message); };
    roleBasedAccessCheckMap_[NamingTcpMessage::PropertyBatchAction] = [this](MessageUPtr &message) { return this->PropertyBatchAccessCheck(message); };
    roleBasedAccessCheckMap_[QueryTcpMessage::QueryAction] = [this](MessageUPtr &message) { return this->QueryAccessCheck(message); };

    //
    // Access checks performed in FileTransferGateway.cpp.
    // These are placeholder macros for the diff_acls.pl verification script.
    //
    // PLACEHOLDER_ACCESS_CHECK(FileContent)
    // PLACEHOLDER_ACCESS_CHECK(FileDownload)
    //
}

void EntreeServiceProxy::ProcessClientMessage(
    MessageUPtr && message,
    ISendTarget::SPtr const & replyTarget)
{
    ErrorCode error(ErrorCodeValue::Success);
    if (!NamingUtility::ValidateClientMessage(message, RequestProcessingTraceComponent, InstanceString))
    {
        WriteWarning(
            RequestProcessingTraceComponent,
            TraceId,
            "Validation of client message failed [{0}]",
            *message);

        ReplyWithError(replyTarget, message->MessageId, ErrorCodeValue::InvalidMessage);
        return;
    }

    auto activityHeader = FabricActivityHeader::FromMessage(*message);
    auto timeoutHeader = TimeoutHeader::FromMessage(*message);
    auto action = message->Action;
    TimeoutHelper timeoutHelper(timeoutHeader.Timeout);

    error = NamingUtility::CheckClientTimeout(timeoutHeader.Timeout, RequestProcessingFailedTraceComponent, InstanceString);
    if (error.IsSuccess() && !AccessCheck(message))
    {
        error = ErrorCodeValue::AccessDenied;
        WriteWarning(
            RequestProcessingTraceComponent,
            TraceId,
            "Request with activity Id = {0}, action = {1} received from {2} is not authorized",
            activityHeader.ActivityId,
            action,
            replyTarget->Address());
    }

    if (error.IsSuccess())
    {
        WriteNoise(
            RequestProcessingTraceComponent,
            TraceId,
            "Entree Service proxy received a client operation message {0} from {1}. activity id = {2} , timeout = {3}",
            message,
            replyTarget->Address(),
            activityHeader,
            timeoutHelper.OriginalTimeout);

        if (perfCounters_)
        {
            perfCounters_->NumberOfOutstandingClientRequests.Increment();
        }

        MessageId id = message->MessageId;
        auto inner = AsyncOperation::CreateAndStart<RequestAsyncOperationBase>(
            *this,
            std::move(message),
            replyTarget,
            timeoutHelper.GetRemainingTime(),
            [this, id, activityHeader, replyTarget, action](AsyncOperationSPtr const & asyncOperation)
            {
                OnProcessClientMessageComplete(id, activityHeader, replyTarget, asyncOperation, action, false);
            },
            Root.CreateAsyncOperationRoot());

        OnProcessClientMessageComplete(id, activityHeader, replyTarget, inner, action, true);
    }

    if (!error.IsSuccess())
    {
        ReplyWithError(replyTarget, message->MessageId, move(error));
    }
}

void EntreeServiceProxy::OnProcessClientMessageComplete(
    MessageId requestId,
    FabricActivityHeader const & activityHeader,
    ISendTarget::SPtr const & replyTarget,
    AsyncOperationSPtr const & asyncOperation,
    wstring const & action,
    bool expectedCompletedSynchronously)
{
    if (expectedCompletedSynchronously != asyncOperation->CompletedSynchronously)
    {
        return;
    }

    MessageUPtr reply;
    auto error = RequestAsyncOperationBase::End(asyncOperation, /*out*/reply);

    if (error.IsSuccess())
    {
        if (reply.get() != nullptr)
        {
            WriteNoise(
                RequestProcessingTraceComponent,
                TraceId,
                "{0}: ProcessClientOperation for action {1} replying with {2}",
                activityHeader.ActivityId,
                action,
                reply);
            reply->Headers.Add(RelatesToHeader(requestId));
            receivingChannel_->SendOneWay(replyTarget, std::move(reply));
        }
        else
        {
            // It is a bug for the reply to be null, but handle it gracefully
            // instead of crashing the process. Return an error that is not
            // expected and not automatically retried by the system internally.
            //
            WriteError(
                RequestProcessingFailedTraceComponent,
                TraceId,
                "{0}: ProcessClientOperation for action '{1}' reply is null",
                activityHeader.ActivityId,
                action);
            ReplyWithError(replyTarget, requestId, ErrorCodeValue::OperationFailed);
        }
    }
    else
    {
        WriteInfo(
            RequestProcessingFailedTraceComponent,
            TraceId,
            "ProcessClientOperation for action '{0}' encountered error {1}. activity id = {2}",
            action,
            error,
            activityHeader);

        ReplyWithError(replyTarget, requestId, move(error));
    }

    if (perfCounters_)
    {
        perfCounters_->NumberOfOutstandingClientRequests.Decrement();
    }    
}

void EntreeServiceProxy::ReplyWithError(
    ISendTarget::SPtr const & replyTarget, 
    MessageId const & requestId,
    ErrorCode && error)
{
    MessageUPtr reply = NamingMessage::GetClientOperationFailure(requestId, move(error));
    receivingChannel_->SendOneWay(replyTarget, std::move(reply));
}

IClaimsTokenAuthenticator * EntreeServiceProxy::CreateClaimsTokenAuthenticator()
{
    if (SecurityConfig::GetConfig().UseTestClaimsAuthenticator)
    {
        testTokenAuthenticator_ = make_shared<TestTokenAuthenticator>();
        return &(*testTokenAuthenticator_);
    }
    else
    {
        return this;
    }
}

void EntreeServiceProxy::ClaimsHandler(
    __in IClaimsTokenAuthenticator *tokenAuthenticator,
    wstring const& token,
    SecurityContextSPtr const& context)
{
    if (AzureActiveDirectory::ServerWrapper::IsAadEnabled() && !isAadServiceEnabled_)
    {
        auto selfRoot = this->Root.CreateComponentRoot();
        Threadpool::Post([this, selfRoot, token, context] { this->ValidateAadToken(token, context); });
    }
    else
    {
        tokenAuthenticator->BeginAuthenticate(
            token,
            SecurityConfig::GetConfig().NegotiationTimeout,
            [this, context, tokenAuthenticator](AsyncOperationSPtr const& operation)
            {
                this->OnAuthenticateComplete(tokenAuthenticator, operation, context);
            },
            this->Root.CreateAsyncOperationRoot());
    }
}

void EntreeServiceProxy::LoadIsAadServiceEnabled()
{
    isAadServiceEnabled_ = AzureActiveDirectory::ServerWrapper::IsAadServiceEnabled();
}

void EntreeServiceProxy::ValidateAadToken(
    wstring const & jwt,
    SecurityContextSPtr const & context)
{
    wstring claims;
    TimeSpan expiration;
    auto error = AzureActiveDirectory::ServerWrapper::ValidateClaims(
        jwt,
        claims, // out
        expiration); // out

    SecuritySettings::RoleClaims roleClaims;

    if (error.IsSuccess())
    {
        error = SecuritySettings::StringToRoleClaims(claims, roleClaims);
    }

    context->CompleteClientAuth(error, roleClaims, expiration);
}

void EntreeServiceProxy::OnAuthenticateComplete(
    __in IClaimsTokenAuthenticator *tokenAuthenticator,
    AsyncOperationSPtr const& operation,
    SecurityContextSPtr const& context)
{
    TimeSpan expirationTime;
    vector<Claim> claimList;
    SecuritySettings::RoleClaims roleClaims;

    auto error = tokenAuthenticator->EndAuthenticate(operation, expirationTime, claimList);
    if (error.IsSuccess())
    {
        if (!SecuritySettings::ClaimListToRoleClaim(claimList, roleClaims))
        {
            WriteWarning(
                RequestProcessingFailedTraceComponent,
                TraceId,
                "ClaimListToRoleClaim failed for claimlist: {0}",
                claimList);

            error = ErrorCodeValue::InvalidCredentials; // TODO: bharatn: create new error code InvalidClaims
        }
    }

    context->CompleteClientAuth(error, roleClaims, expirationTime);
}

AsyncOperationSPtr EntreeServiceProxy::BeginAuthenticate(
    std::wstring const& inputToken,
    Common::TimeSpan const timeout,
    Common::AsyncCallback const& callback,
    Common::AsyncOperationSPtr const& parent)
{
    auto tokenMessage = TokenValidationServiceTcpMessage::GetValidateToken(
        ValidateTokenMessageBody(inputToken))->GetTcpMessage();

    tokenMessage->Headers.Replace(TimeoutHeader(timeout));
    ServiceRoutingAgentMessage::WrapForForwardingToTokenValidationService(*tokenMessage);

    return BeginProcessRequest(
        move(tokenMessage),
        ISendTarget::SPtr(),
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceProxy::EndAuthenticate(
    AsyncOperationSPtr const& operation,
    __out TimeSpan &expirationTime,
    __out vector<Claim> &claimsList)
{
    MessageUPtr replyMessage;
    auto error = EndProcessRequest(operation, replyMessage);

    if (error.IsSuccess())
    {
        TokenValidationMessage body;
        if (replyMessage->GetBody(body))
        {
            error = body.Error;
            claimsList = move(body.ClaimsResult.ClaimsList);
            expirationTime = body.TokenExpiryTime;
        }
        else
        {
            error = ErrorCode::FromNtStatus(replyMessage->GetStatus());
        }
    }

    return error;
}

AsyncOperationSPtr EntreeServiceProxy::BeginProcessRequest(
    MessageUPtr && message,
    ISendTarget::SPtr const &,
    TimeSpan const &timeout,
    AsyncCallback const &callback, 
    AsyncOperationSPtr const &parent)
{
    return proxyToEntreeServiceTransport_.BeginSendToEntreeService(
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceProxy::EndProcessRequest(
    AsyncOperationSPtr const &operation,
    __out MessageUPtr &reply)
{
    return proxyToEntreeServiceTransport_.EndSendToEntreeService(
        operation,
        reply);
}

AsyncOperationSPtr EntreeServiceProxy::BeginReport(
    MessageUPtr && message,
    TimeSpan timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    SystemServices::ServiceRoutingAgentMessage::WrapForForwarding(*message);
    message->Headers.Add(FabricActivityHeader(Guid::NewGuid()));
    return proxyToEntreeServiceTransport_.BeginSendToEntreeService(
        move(message),
        timeout,
        callback,
        parent);
}

ErrorCode EntreeServiceProxy::EndReport(
    AsyncOperationSPtr const & operation,
    __out MessageUPtr & reply)
{
    return proxyToEntreeServiceTransport_.EndSendToEntreeService(
        operation,
        reply);
}

bool EntreeServiceProxy::AccessCheck(MessageUPtr &message)
{
    ASSERT_IF(
        isClientRoleInEffect_  && !message->SecurityContextPtr(),
        "SecurityContext missing on message {0}",
        message->TraceId());

    AcquireReadLock readLock(roleBasedAccessCheckMapLock_);

    auto const &action = message->Action;
    auto iter = roleBasedAccessCheckMap_.find(action);
    if (iter == roleBasedAccessCheckMap_.end())
    {
        WriteWarning(
            RequestProcessingFailedTraceComponent, 
            TraceId,
            "Unknown message action: '{0}'", 
            action);

        return false;
    }

    return iter->second(message);
}

bool EntreeServiceProxy::DefaultActionHeaderBasedAccessCheck(
    MessageUPtr & receivedMessage, 
    RoleMask::Enum requiredRoles)
{
    return receivedMessage->IsInRole(requiredRoles);
}

bool EntreeServiceProxy::PropertyBatchAccessCheck(MessageUPtr & receivedMessage)
{
    NamePropertyOperationBatch batchRequest;
    if (!receivedMessage->GetBody(batchRequest))
    {
        WriteWarning(
            RequestProcessingFailedTraceComponent, 
            TraceId,
            "{0} property batch request missing body : {1}", 
            FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
            *receivedMessage);

        return false;
    }

    if (batchRequest.IsReadonly())
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().PropertyReadBatchRoles);
    }

    return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().PropertyWriteBatchRoles);
}

bool EntreeServiceProxy::ForwardToFileStoreAccessCheck(MessageUPtr &receivedMessage)
{
    Transport::ForwardMessageHeader forwardMessageHeader;
    if (!receivedMessage->Headers.TryReadFirst(forwardMessageHeader))
    {
        WriteWarning(
            RequestProcessingFailedTraceComponent,
            TraceId,
            "{0} ForwardMessageHeader missing: Action:{1} messageId:{2}",
            FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
            receivedMessage->Action,
            *receivedMessage);

        return false;
    }

    if (forwardMessageHeader.Actor == Actor::ServiceRoutingAgent &&
        forwardMessageHeader.Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
    {
        ServiceRoutingAgentHeader routingAgentHeader;
        if (receivedMessage->Headers.TryReadFirst(routingAgentHeader))
        {
            return RoutingAgentAccessCheck(receivedMessage, routingAgentHeader);
        }
        else
        {
            WriteWarning(
                RequestProcessingFailedTraceComponent,
                TraceId,
                "{0} ServiceRouteRequestAction message is missing ServiceRoutingAgentHeader during access check",
                FabricActivityHeader::FromMessage(*receivedMessage).ActivityId);

            // Fall through to checking for Admin if it is not possible to
            // perform the fine-grained check. The message is not valid anyway.
        }
    }

    WriteNoise(
        RequestProcessingTraceComponent,
        TraceId,
        "{0} found system action in forwardMessageHeader : {1}", 
        FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
        forwardMessageHeader.Action);

    return receivedMessage->IsInRole(Transport::RoleMask::Admin);

}

bool EntreeServiceProxy::ForwardToServiceAccessCheck(MessageUPtr &receivedMessage)
{
    Transport::ForwardMessageHeader forwardMessageHeader;
    if (!receivedMessage->Headers.TryReadFirst(forwardMessageHeader))
    {
        WriteWarning(
            RequestProcessingFailedTraceComponent,
            TraceId,
            "{0} ForwardMessageHeader missing: {1}", 
            FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
            *receivedMessage);

        return false;
    }

    if (forwardMessageHeader.Actor == Actor::ServiceRoutingAgent &&
        forwardMessageHeader.Action == ServiceRoutingAgentMessage::ServiceRouteRequestAction)
    {
        ServiceRoutingAgentHeader routingAgentHeader;
        if (receivedMessage->Headers.TryReadFirst(routingAgentHeader))
        {
            return RoutingAgentAccessCheck(receivedMessage, routingAgentHeader);
        }
        else
        {
            WriteWarning(
                RequestProcessingTraceComponent,
                TraceId,
                "{0} ServiceRouteRequestAction message is missing ServiceRoutingAgentHeader during access check",
                FabricActivityHeader::FromMessage(*receivedMessage).ActivityId);

            return false;
        }
    }

    CM_ACCESS_CHECK(ProvisionApplicationType)
    CM_ACCESS_CHECK(UpgradeApplication)
    CM_ACCESS_CHECK(MoveNextUpgradeDomain)
    CM_ACCESS_CHECK(RollbackApplicationUpgrade)
    CM_ACCESS_CHECK(GetUpgradeStatus)
    CM_ACCESS_CHECK(UnprovisionApplicationType)
    CM_ACCESS_CHECK(ReportUpgradeHealth)

    CM_ACCESS_CHECK(CreateApplication)
    CM_ACCESS_CHECK(DeleteApplication)
    CM_ACCESS_CHECK(CreateService)
    CM_ACCESS_CHECK(CreateServiceFromTemplate)
    CM_ACCESS_CHECK(DeleteService)

    CM_ACCESS_CHECK(ProvisionFabric)
    CM_ACCESS_CHECK(UpgradeFabric)
    CM_ACCESS_CHECK(MoveNextFabricUpgradeDomain)
    CM_ACCESS_CHECK(RollbackFabricUpgrade)
    CM_ACCESS_CHECK(GetFabricUpgradeStatus)
    CM_ACCESS_CHECK(UnprovisionFabric)
    CM_ACCESS_CHECK(ReportFabricUpgradeHealth)

    CM_ACCESS_CHECK(StartInfrastructureTask)
    CM_ACCESS_CHECK(FinishInfrastructureTask)

    DOCKER_COMPOSE_ACCESS_CHECK(CreateComposeDeployment)
    DOCKER_COMPOSE_ACCESS_CHECK(DeleteComposeDeployment)
    DOCKER_COMPOSE_ACCESS_CHECK(UpgradeComposeDeployment)

    VOLUME_MANAGER_ACCESS_CHECK(CreateVolume)
    VOLUME_MANAGER_ACCESS_CHECK(DeleteVolume)

    HM_ACCESS_CHECK(ReportHealth)

    WriteInfo(
        RequestProcessingTraceComponent,
        TraceId,
        "{0} found system action in forwardMessageHeader: {1}", 
        FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
        forwardMessageHeader.Action);

    return receivedMessage->IsInRole(Transport::RoleMask::Admin);
}

bool EntreeServiceProxy::RoutingAgentAccessCheck(MessageUPtr &receivedMessage, ServiceRoutingAgentHeader const & routingAgentHeader)
{
    switch (routingAgentHeader.Actor)
    {
    case Actor::IS:
        {
            IS_ACCESS_CHECK(InvokeInfrastructureCommand)
            IS_ACCESS_CHECK(InvokeInfrastructureCommand)
            IS_ACCESS_CHECK(InvokeInfrastructureQuery)
        }
    case Actor::FileStoreService:
        {
            FS_ACCESS_CHECK(GetStagingLocation)
            FS_ACCESS_CHECK(Upload)
            FS_ACCESS_CHECK(Delete)
            FS_ACCESS_CHECK(List)
            FS_ACCESS_CHECK(GetStoreLocation)
            FS_ACCESS_CHECK(InternalList)
        }
    case Actor::NM:
        {
            return true;
        }
    case Actor::FAS:
        {
            FAS_ACCESS_CHECK(StartPartitionDataLoss)
            FAS_ACCESS_CHECK(StartPartitionQuorumLoss)
            FAS_ACCESS_CHECK(StartPartitionRestart)
            FAS_ACCESS_CHECK(GetPartitionDataLossProgress)
            FAS_ACCESS_CHECK(GetPartitionQuorumLossProgress)
            FAS_ACCESS_CHECK(GetPartitionRestartProgress)
            FAS_ACCESS_CHECK(CancelTestCommand)

            FAS_ACCESS_CHECK(StartChaos)
            FAS_ACCESS_CHECK(StopChaos)
            FAS_ACCESS_CHECK(GetChaosReport)

            FAS_ACCESS_CHECK(StartNodeTransition)
            FAS_ACCESS_CHECK(GetNodeTransitionProgress)
        }
        case Actor::CSS:
        {
            FCSS_ACCESS_CHECK(GetSecrets)
        }
        case Actor::UOS:
        {
            FUOS_ACCESS_CHECK(StartClusterConfigurationUpgrade)
            FUOS_ACCESS_CHECK(GetClusterConfigurationUpgradeStatus)
            FUOS_ACCESS_CHECK(GetClusterConfiguration)
            FUOS_ACCESS_CHECK(GetUpgradesPendingApproval)
            FUOS_ACCESS_CHECK(StartApprovedUpgrades)
            FUOS_ACCESS_CHECK(GetUpgradeOrchestrationServiceState)
            FUOS_ACCESS_CHECK(SetUpgradeOrchestrationServiceState)
        }
        case Actor::GatewayResourceManager:
        {
            GATEWAY_RESOURCE_MANAGER_ACCESS_CHECK(CreateGatewayResource)
            GATEWAY_RESOURCE_MANAGER_ACCESS_CHECK(DeleteGatewayResource)
        }

        break;
    }

    WriteNoise(
        RequestProcessingTraceComponent,
        TraceId,
        "{0} found system action in routing agent header: {1}+{2}",
        FabricActivityHeader::FromMessage(*receivedMessage).ActivityId,
        routingAgentHeader.Actor,
        routingAgentHeader.Action);

    return receivedMessage->IsInRole(Transport::RoleMask::Admin);
}

bool EntreeServiceProxy::QueryAccessCheck(MessageUPtr &receivedMessage)
{
    std::wstring queryName;
    if (!QueryGateway::TryGetQueryName(*receivedMessage, queryName))
    {
        WriteWarning(
            RequestProcessingTraceComponent,
            TraceId,
            "{0} QueryName not found in QueryMessage",
            FabricActivityHeader::FromMessage(*receivedMessage).ActivityId);

        return false;
    }

    if(queryName == QueryNames::ToString(QueryNames::StopNode)/*Both RestartNode and StopNode use this query name*/)
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().NodeControlRoles);
    }
    else if (queryName == QueryNames::ToString(QueryNames::RestartDeployedCodePackage))
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().CodePackageControlRoles);
    }
    else if (queryName == QueryNames::ToString(QueryNames::DeployServicePackageToNode))
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().PredeployPackageToNodeRoles);
    }
    else if (queryName == QueryNames::ToString(QueryNames::AddUnreliableTransportBehavior) || 
        queryName == QueryNames::ToString(QueryNames::RemoveUnreliableTransportBehavior))
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().UnreliableTransportControlRoles);
    }        
    else if ((queryName == QueryNames::ToString(QueryNames::MovePrimary))
        || (queryName == QueryNames::ToString(QueryNames::MoveSecondary)))
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().MoveReplicaControlRoles);
    }
    else if (queryName == QueryNames::ToString(QueryNames::InvokeContainerApiOnNode))
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().InvokeContainerApiRoles);
    }
    else
    {
        return receivedMessage->IsInRole(ClientAccessConfig::GetConfig().QueryRoles);
    }
}

ErrorCode EntreeServiceProxy::InitializeExternalChannel()
{
    this->receivingChannel_ = Transport::DatagramTransportFactory::CreateTcp(
        listenAddress_, 
        InstanceString,
        L"EntreeServiceProxy");        

    // Enable keep alive on listening side also, so that non-responsive clients can be detected
    receivingChannel_->SetKeepAliveTimeout(TimeSpan::FromSeconds(ClientConfig::GetConfig().KeepAliveIntervalInSeconds));
    receivingChannel_->EnableInboundActivityTracing();
    ReceivingChannel->AllowThrottleReplyMessage();

    NamingUtility::SetCommonTransportProperties(*receivingChannel_);

    auto error = SetSecurity(this->initialSecuritySettings_);
    if (!error.IsSuccess())
    {
        return error;
    }

    auto tokenAuthenticator = this->CreateClaimsTokenAuthenticator();

    this->receivingChannel_->SetClaimsHandler(
        [this, tokenAuthenticator](wstring const& token, SecurityContextSPtr const& context)
        {
            this->ClaimsHandler(tokenAuthenticator, token, context);
        });

    SecurityConfig::GetConfig();

    if (AzureActiveDirectory::ServerWrapper::IsAadEnabled())
    {
        this->receivingChannel_->SetClaimsRetrievalMetadata(ClaimsRetrievalMetadata::CreateForAAD());
    }

    demuxer_ = make_unique<Demuxer>(this->Root, receivingChannel_);

    clientRequestQueue_ = make_unique<CommonJobQueue<EntreeServiceProxy>>(
        L"EntreeServiceProxy",
        *this,
        false,
        0,
        nullptr,
        ServiceModelConfig::GetConfig().RequestQueueSize);

    auto selfRoot = this->Root.CreateComponentRoot();
    this->demuxer_->RegisterMessageHandler(
        Actor::NamingGateway,
        [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
        {
            if (!clientRequestQueue_->Enqueue(make_unique<ClientRequestJobItem>(message->Clone(), move(receiverContext))))
            {
                WriteWarning(
                    RequestProcessingFailedTraceComponent,
                    TraceId,
                    "Dropping message because JobQueue is full, QueueSize : {0}", clientRequestQueue_->GetQueueLength());
            }
        },
        true /* Dispatch on transport thread */);

    error = demuxer_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            LifeCycleTraceComponent, 
            TraceId,
            "Demuxer failed to open with error {0}", 
            error);
        return error;
    }

    requestReply_ = make_shared<RequestReply>(Root, receivingChannel_);
    requestReply_->Open();
    demuxer_->SetReplyHandler(*requestReply_);

    notificationManagerProxyUPtr_ = make_unique<ServiceNotificationManagerProxy>(
        this->Root,
        this->Instance,
        requestReply_,
        proxyToEntreeServiceTransport_,
        receivingChannel_);

    //
    // Connection accepted and faulted handlers should be set before the listener open, since we are tracking
    // the connection count.
    //
    notificationManagerProxyUPtr_->SetConnectionAcceptedHandler([this, selfRoot](ISendTarget const & sendTarget)
        {
            this->OnConnectionAccepted(sendTarget);
        });
    
    notificationManagerProxyUPtr_->SetConnectionFaultHandler([this, selfRoot](ISendTarget const & sendTarget, ErrorCode const & error)
        {
            this->OnConnectionFault(sendTarget, error);
        });
    
    error = notificationManagerProxyUPtr_->Open();
    if (!error.IsSuccess())
    {
        WriteWarning(
            LifeCycleTraceComponent, 
            TraceId,
            "Notification manager proxy open failed with {0}", 
            error);

        return error;
    }


    //
    // !! Any component that needs to be opened/started before we start listening, should be started before this !!
    //
    error = receivingChannel_->Start();
    if (!error.IsSuccess())
    {
        return error;
    }

    //Read back the address in case dynamic ports used
    listenAddress_ = receivingChannel_->ListenAddress();

    return ErrorCodeValue::Success;
}
