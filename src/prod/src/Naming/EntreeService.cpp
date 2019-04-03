// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Naming
{
    using namespace Common;
    using namespace Client;
    using namespace Transport;
    using namespace Query;
    using namespace Reliability;
    using namespace Federation;
    using namespace std;
    using namespace ServiceModel;
    using namespace SystemServices;
    using namespace Management;
    using namespace Api;
    using namespace Management::TokenValidationService;
    using namespace ClientServerTransport;

#define GET_RC( ResourceName ) \
    Common::StringResource::Get( IDS_NAMING_##ResourceName )

    StringLiteral const LifeCycleTraceComponent("LifeCycle");
    StringLiteral const RequestProcessingTraceComponent("RequestProcessing");
    StringLiteral const RequestProcessingFailedTraceComponent("RequestProcessingFailed");
    const GatewayEventSource Events;

    EntreeService::EntreeService(
        __in IRouter & innerRingCommunication,
        __in ServiceResolver & fmClient,
        __in Reliability::ServiceAdminClient & adminClient,
        IGatewayManagerSPtr const &gatewayManager,
        NodeInstance const & instance,
        wstring const & listenAddress,
        Common::FabricNodeConfigSPtr const& nodeConfig,
        Transport::SecuritySettings const & zombieModeSecuritySettings,
        ComponentRoot const & root,
        bool isInZombieMode)
        : HealthReportingTransport(root)
        , TextTraceComponent()
        , namingConfig_(NamingConfig::GetConfig())
        , properties_()
        , listenAddress_(listenAddress)
        , requestHandlers_()
        , gatewayManager_(gatewayManager)
        , queryMessageHandler_()
        , queryGateway_()
        , serviceNotificationManager_()
        , zombieModeSecuritySettings_(zombieModeSecuritySettings)
    {
        WriteNoise(LifeCycleTraceComponent, "{0} : constructor called", instance);

        auto tempQueryMessageHandler = make_unique<QueryMessageHandler>(this->Root, L"/");
        auto tempQueryGateway = make_unique<Query::QueryGateway>(*tempQueryMessageHandler);
        auto tempSystemServiceResolver = SystemServiceResolver::Create(*tempQueryGateway, fmClient, root);
        auto tempServiceNotificationManager = make_unique<ServiceNotificationManager>(
            instance,
            nodeConfig->InstanceName,
            fmClient.Cache,
            Events,
            root);
        auto tempProperties = make_shared<GatewayProperties>(
            instance,
            innerRingCommunication,
            adminClient,
            fmClient,
            *tempQueryGateway,
            tempSystemServiceResolver,
            *tempServiceNotificationManager,
            *this,
            Events,
            nodeConfig,
            zombieModeSecuritySettings,
            root,
            isInZombieMode);

        this->InitializeHandlers(isInZombieMode);

        queryMessageHandler_.swap(tempQueryMessageHandler);
        queryGateway_.swap(tempQueryGateway);
        systemServiceResolver_.swap(tempSystemServiceResolver);
        serviceNotificationManager_.swap(tempServiceNotificationManager);
        properties_ = move(tempProperties);
    }

    EntreeService::~EntreeService()
    {
        WriteNoise(LifeCycleTraceComponent, "{0} : destructor called", Instance);
    }

    class EntreeService::OpenAsyncOperation : public TimedAsyncOperation
    {
    public:

        OpenAsyncOperation(
            EntreeService & entreeService,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & asyncOperationParent)
            : TimedAsyncOperation(timeout, callback, asyncOperationParent)
            , entreeService_(entreeService)
        {
        }

        using AsyncOperation::End;

    protected:

        void OnStart(AsyncOperationSPtr const & thisSPtr) override
        {
            WriteNoise(LifeCycleTraceComponent, "{0} : entering EntreeService::OpenAsyncOperation::OnStart", entreeService_.Instance);

            auto error = entreeService_.InitializeReceivingChannel();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    LifeCycleTraceComponent,
                    "{0}: InitializeReceivingChannel failed with error {1}",
                    entreeService_.Instance,
                    error);
                TryComplete(thisSPtr, error);
                return;
            }

            if (!entreeService_.Properties.IsInZombieMode)
            {
                entreeService_.RegisterQueryHandlers();
                error = entreeService_.queryMessageHandler_->Open();
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        LifeCycleTraceComponent,
                        "{0}: QueryMessageHandler failed to open with error {1}",
                        entreeService_.Properties.Instance,
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }

                error = entreeService_.Properties.BroadcastEventManager.Open();
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        LifeCycleTraceComponent,
                        "{0}: BroadcastEventManager failed to open with error {1}",
                        entreeService_.Properties.Instance,
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }

                error = entreeService_.serviceNotificationManager_->Open(entreeService_.Properties.ReceivingChannel);
                if (!error.IsSuccess())
                {
                    WriteWarning(
                        LifeCycleTraceComponent,
                        "{0}: ServiceNotificationManager failed to open with error {1}",
                        entreeService_.Properties.Instance,
                        error);
                    TryComplete(thisSPtr, error);
                    return;
                }

                if (entreeService_.gatewayManager_)
                {
                    if (!entreeService_.gatewayManager_->RegisterGatewaySettingsUpdateHandler())
                    {
                        WriteWarning(
                            LifeCycleTraceComponent,
                            "RegisterGatewaySettingsUpdateHandler failed");

                        TryComplete(thisSPtr, ErrorCodeValue::InvalidConfiguration);
                        return;
                    }

                    auto op = entreeService_.gatewayManager_->BeginActivateGateway(
                        entreeService_.Properties.ReceivingChannel->TransportListenAddress,
                        RemainingTime,
                        [this](AsyncOperationSPtr const &operation)
                        {
                            ErrorCode error = entreeService_.gatewayManager_->EndActivateGateway(operation);
                            if (!error.IsSuccess())
                            {
                                WriteWarning(
                                    LifeCycleTraceComponent,
                                    "{0}: GatewayManager failed to open with error {1}",
                                    entreeService_.Properties.Instance,
                                    error);
                            }
                            FinishOpen(operation->Parent, error);
                        },
                        thisSPtr);

                    return;
                }
            } // end if !IsInZombieMode

            FinishOpen(thisSPtr, ErrorCodeValue::Success);
        }

        void OnCompleted() override
        {
            Events.GatewayOpenComplete(entreeService_.Properties.Instance, entreeService_.listenAddress_, this->Error);
        }

    private:

        void FinishOpen(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
        {
            if (error.IsSuccess())
            {
                entreeService_.RegisterReceivingChannelMessageHandlers();
            }

            TryComplete(thisSPtr, error);
        }

        EntreeService & entreeService_;
        TimeSpan timeout_;
    };

    class EntreeService::CloseAsyncOperation : public TimedAsyncOperation
    {
    public:

        CloseAsyncOperation(
            EntreeService & entreeService,
            TimeSpan timeout,
            AsyncCallback const & callback,
            AsyncOperationSPtr const & asyncOperationParent)
            : TimedAsyncOperation(timeout, callback, asyncOperationParent)
            , entreeService_(entreeService)
        {
        }

        using AsyncOperation::End;

    protected:

        void OnStart(AsyncOperationSPtr const & thisSPtr) override
        {
            entreeService_.WriteNoise(
                LifeCycleTraceComponent,
                "{0} : entering EntreeService::CloseAsyncOperation::OnStart",
                entreeService_.Properties.Instance);

            ErrorCode error;
            if (!entreeService_.Properties.IsInZombieMode)
            {
                if (entreeService_.gatewayManager_)
                {
                    auto op = entreeService_.gatewayManager_->BeginDeactivateGateway(
                        RemainingTime,
                        [this, error](AsyncOperationSPtr operation)
                        {
                            // Even if the request to deactivate the gateway failed for some reason, close of naming should continue.
                            entreeService_.gatewayManager_->EndDeactivateGateway(operation);
                            entreeService_.gatewayManager_.reset();
                            FinishClose(operation->Parent, error);
                        },
                        thisSPtr);
                    return;
                }
            }

            FinishClose(thisSPtr, error);
        }

        void OnCompleted() override
        {
            WriteNoise(LifeCycleTraceComponent, "{0} : completing close: {1}", entreeService_.Properties.Instance, this->Error);
        }

    private:
        void FinishClose(AsyncOperationSPtr const & thisSPtr, ErrorCode error)
        {
            if (!entreeService_.Properties.IsInZombieMode)
            {
                if (entreeService_.serviceNotificationManager_)
                {
                    error = ErrorCode::FirstError(error, entreeService_.serviceNotificationManager_->Close());
                }

                error = ErrorCode::FirstError(error, entreeService_.Properties.BroadcastEventManager.Close());

                if (entreeService_.queryMessageHandler_)
                {
                    error = ErrorCode::FirstError(error, entreeService_.queryMessageHandler_->Close());
                }

                if (entreeService_.Properties.ReceivingChannel)
                {
                    entreeService_.Properties.ReceivingChannel->UnregisterMessageHandler(Transport::Actor::NamingGateway);
                    error = ErrorCode::FirstError(error, entreeService_.Properties.ReceivingChannel->Close());
                }
            }
            else
            {
                if (entreeService_.zombieModeTransport_)
                {
                    entreeService_.zombieModeTransport_->Stop();
                    entreeService_.zombieModeDemuxer_->UnregisterMessageHandler(Actor::NamingGateway);
                    error = ErrorCode::FirstError(error, entreeService_.zombieModeDemuxer_->Close());
                }
            }

            TryComplete(thisSPtr, error);
        }

        EntreeService & entreeService_;
        TimeSpan timeout_;
    };

    AsyncOperationSPtr EntreeService::OnBeginOpen(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<OpenAsyncOperation>(*this, timeout, callback, parent);
    }

    ErrorCode EntreeService::OnEndOpen(AsyncOperationSPtr const & operation)
    {
        return OpenAsyncOperation::End(operation);
    }

    AsyncOperationSPtr EntreeService::OnBeginClose(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, timeout, callback, parent);
    }

    ErrorCode EntreeService::OnEndClose(AsyncOperationSPtr const & operation)
    {
        return CloseAsyncOperation::End(operation);
    }

    void EntreeService::OnAbort()
    {
        WriteNoise(LifeCycleTraceComponent, "{0} : entering OnAbort", Instance);

        if (!Properties.IsInZombieMode)
        {
            if (queryMessageHandler_)
            {
                queryMessageHandler_->Abort();
            }

            if (serviceNotificationManager_)
            {
                serviceNotificationManager_->Close();
            }

            this->Properties.BroadcastEventManager.Abort();

            if (gatewayManager_)
            {
                gatewayManager_->AbortGateway();
            }

            if (this->Properties.ReceivingChannel)
            {
                this->Properties.ReceivingChannel->UnregisterMessageHandler(Actor::NamingGateway);
                this->Properties.ReceivingChannel->Abort();
            }
        }
        else
        {
            if (zombieModeTransport_)
            {
                zombieModeTransport_->Stop();
                zombieModeDemuxer_->Abort();
            }
        }

        WriteNoise(LifeCycleTraceComponent, "{0} : leaving OnAbort", Instance);
    }

    NodeId EntreeService::get_Id() const
    {
        return Instance.Id;
    }

    void EntreeService::InitializeHandlers(bool isInZombieMode)
    {
        map<wstring, ProcessRequestHandler> t;

        if (!isInZombieMode)
        {
            this->AddHandler(t, NamingTcpMessage::PingRequestAction, CreateHandler<PingAsyncOperation>);
            this->AddHandler(t, QueryTcpMessage::QueryAction, CreateHandler<QueryRequestOperation>);
            this->AddHandler(t, NamingMessage::ForwardToTvsMessageAction, CreateHandler<ForwardToTokenValidationServiceAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::CreateNameAction, CreateHandler<CreateNameAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::DeleteNameAction, CreateHandler<DeleteNameAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::NameExistsAction, CreateHandler<NameExistsAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::EnumerateSubNamesAction, CreateHandler<EnumerateSubnamesAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::PropertyBatchAction, CreateHandler<PropertyBatchAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::EnumeratePropertiesAction, CreateHandler<EnumeratePropertiesAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::ResolveNameOwnerAction, CreateHandler<ResolveNameOwnerAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ResolvePartitionAction, CreateHandler<ResolvePartitionAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ResolveServiceAction, CreateHandler<ResolveServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ResolveSystemServiceAction, CreateHandler<ResolveSystemServiceGatewayAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::PrefixResolveServiceAction, CreateHandler<PrefixResolveServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ServiceLocationChangeListenerAction, CreateHandler<ServiceLocationNotificationAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::CreateServiceAction, CreateHandler<CreateServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::UpdateServiceAction, CreateHandler<UpdateServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::DeleteServiceAction, CreateHandler<DeleteServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::GetServiceDescriptionAction, CreateHandler<GetServiceDescriptionAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::DeactivateNodeAction, CreateHandler<DeactivateNodeAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ActivateNodeAction, CreateHandler<ActivateNodeAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::DeactivateNodesBatchRequestAction, CreateHandler<DeactivateNodesBatchAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::RemoveNodeDeactivationsRequestAction, CreateHandler<RemoveNodeDeactivationsAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::GetNodeDeactivationStatusRequestAction, CreateHandler<GetNodeDeactivationStatusAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::NodeStateRemovedAction, CreateHandler<NodeStateRemovedAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::RecoverPartitionsAction, CreateHandler<RecoverPartitionsAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::RecoverPartitionAction, CreateHandler<RecoverPartitionAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::RecoverServicePartitionsAction, CreateHandler<RecoverServicePartitionsAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::RecoverSystemPartitionsAction, CreateHandler<RecoverSystemPartitionsAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ReportFaultAction, CreateHandler<ReportFaultAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ResetPartitionLoadAction, CreateHandler<ResetPartitionLoadAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ToggleVerboseServicePlacementHealthReportingAction, CreateHandler<ToggleVerboseServicePlacementHealthReportingAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::CreateNetworkAction, CreateHandler<CreateNetworkAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::DeleteNetworkAction, CreateHandler<DeleteNetworkAsyncOperation>);

            this->AddHandler(t, NamingMessage::ForwardMessageAction, CreateHandler<ForwardToServiceOperation>);
            this->AddHandler(t, NamingMessage::ForwardToFileStoreMessageAction, CreateHandler<ForwardToFileStoreServiceAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::CreateSystemServiceRequestAction, CreateHandler<CreateSystemServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::DeleteSystemServiceRequestAction, CreateHandler<DeleteSystemServiceAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::ResolveSystemServiceRequestAction, CreateHandler<ResolveSystemServiceAsyncOperation>);

            this->AddHandler(t, NamingTcpMessage::RegisterServiceNotificationFilterRequestAction, CreateHandler<ProcessServiceNotificationRequestAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::UnregisterServiceNotificationFilterRequestAction, CreateHandler<ProcessServiceNotificationRequestAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::NotificationClientConnectionRequestAction, CreateHandler<ProcessServiceNotificationRequestAsyncOperation>);
            this->AddHandler(t, NamingTcpMessage::NotificationClientSynchronizationRequestAction, CreateHandler<ProcessServiceNotificationRequestAsyncOperation>);

            // todo surface this as ipc transport thing
            this->AddHandler(t, NamingMessage::DisconnectClientAction, CreateHandler<ProcessServiceNotificationRequestAsyncOperation>);
        }

        WriteInfo(LifeCycleTraceComponent, "EntreeService initializing with zombie mode: {0}", isInZombieMode);

        // Always handle Ping and StartNode
        this->AddHandler(t, NamingTcpMessage::PingRequestAction, CreateHandler<PingAsyncOperation>);
        this->AddHandler(t, NamingTcpMessage::StartNodeAction, CreateHandler<StartNodeAsyncOperation>);

        requestHandlers_.swap(t);
    }

    void EntreeService::AddHandler(map<wstring, ProcessRequestHandler> & temp, wstring const & action, ProcessRequestHandler const & handler)
    {
        temp.insert(std::make_pair(action, handler));
    }

    template <class TAsyncOperation>
    AsyncOperationSPtr EntreeService::CreateHandler(
        __in GatewayProperties & properties,
        __in Transport::MessageUPtr & request,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<TAsyncOperation>(
            properties,
            move(request),
            timeout,
            callback,
            parent);
    }

    AsyncOperationSPtr EntreeService::BeginProcessRequest(
        MessageUPtr && request,
        Common::TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        auto const & action = request->Action;

        auto it = requestHandlers_.find(action);
        if (it != requestHandlers_.end())
        {
            return (it->second)(this->Properties, request, timeout, callback, parent);
        }
        else
        {
            WriteWarning(
                RequestProcessingTraceComponent,
                "{0}: Unknown message action: '{1}'",
                Instance,
                action);

            return AsyncOperation::CreateAndStart<CompletedRequestAsyncOperation>(
                this->Properties,
                FabricActivityHeader::FromMessage(*request),
                Properties.IsInZombieMode ? ErrorCodeValue::NodeIsStopped : ErrorCodeValue::InvalidMessage,
                callback,
                parent);
        }
    }

    ErrorCode EntreeService::EndProcessRequest(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply)
    {
        auto error = RequestAsyncOperationBase::End(operation, /*out*/reply);
        return NamingErrorCategories::ToClientError(move(error));
    }

    void EntreeService::ProcessZombieModeMessage(
        MessageUPtr && message,
        ISendTarget::SPtr const & replyTarget)
    {
        ErrorCode error(ErrorCodeValue::Success);

        if (!NamingUtility::ValidateClientMessage(message, RequestProcessingFailedTraceComponent, Instance.ToString()))
        {
            WriteWarning(
                RequestProcessingTraceComponent,
                "{0}: validation of client message failed [{1}]",
                Instance,
                *message);

            error = ErrorCodeValue::InvalidMessage;
        }

        if (error.IsSuccess())
        {
            auto activityHeader = FabricActivityHeader::FromMessage(*message);
            auto timeoutHeader = TimeoutHeader::FromMessage(*message);
            auto timeout = timeoutHeader.Timeout;

            error = NamingUtility::CheckClientTimeout(timeout, RequestProcessingFailedTraceComponent, Instance.ToString());
            if (error.IsSuccess())
            {
                WriteNoise(
                    RequestProcessingTraceComponent,
                    "{0}: Entree Service received a client operation message {1} from {2}. activity id = {3} , timeout = {4}",
                    Instance,
                    message,
                    replyTarget->Address(),
                    activityHeader,
                    timeout);

                MessageId id = message->MessageId;
                auto inner = BeginProcessRequest(
                    std::move(message),
                    timeout,
                    [this, id, activityHeader, replyTarget](AsyncOperationSPtr const & asyncOperation)
                {
                    OnProcessZombieModeMessageComplete(id, activityHeader, replyTarget, asyncOperation, false);
                },
                    this->Root.CreateAsyncOperationRoot());
                OnProcessZombieModeMessageComplete(id, activityHeader, replyTarget, inner, true);
            }
        }

        if (!error.IsSuccess())
        {
            MessageUPtr reply = NamingMessage::GetClientOperationFailure(message->MessageId, move(error));
            zombieModeTransport_->SendOneWay(replyTarget, std::move(reply));
        }
    }

    void EntreeService::OnProcessZombieModeMessageComplete(
        MessageId requestId,
        FabricActivityHeader const & activityHeader,
        ISendTarget::SPtr const & replyTarget,
        AsyncOperationSPtr const & asyncOperation,
        bool expectedCompletedSynchronously)
    {
        if (expectedCompletedSynchronously != asyncOperation->CompletedSynchronously)
        {
            return;
        }

        MessageUPtr reply;
        auto error = EndProcessRequest(asyncOperation, /*out*/reply);

        if (error.IsSuccess())
        {
            if (reply)
            {
                WriteNoise(
                    RequestProcessingTraceComponent,
                    "{0}: ProcessClientOperation replying with {1}",
                    activityHeader.ActivityId,
                    reply);
                reply->Headers.Add(RelatesToHeader(requestId));
                zombieModeTransport_->SendOneWay(replyTarget, std::move(reply));
            }
            else
            {
                // It is a bug for the reply to be null, but handle it gracefully
                // instead of crashing the process. Return an error that is not
                // expected and not automatically retried by the system internally.
                //
                WriteError(
                    RequestProcessingTraceComponent,
                    "{0}: ProcessClientOperation reply is null",
                    activityHeader.ActivityId);
                MessageUPtr opFailureReply = NamingMessage::GetClientOperationFailure(requestId, move(error));
                zombieModeTransport_->SendOneWay(replyTarget, std::move(opFailureReply));
            }
        }
        else
        {
            WriteInfo(
                RequestProcessingFailedTraceComponent,
                "ProcessClientOperation encountered error {0}. activity id = {1}",
                error,
                activityHeader);

            MessageUPtr opFailureReply = NamingMessage::GetClientOperationFailure(requestId, move(error));
            zombieModeTransport_->SendOneWay(replyTarget, std::move(opFailureReply));
        }
    }

    bool EntreeService::RegisterGatewayMessageHandler(
        Transport::Actor::Enum actor,
        BeginHandleGatewayRequestFunction const& beginFunction,
        EndHandleGatewayRequestFunction const& endFunction)
    {
        AcquireWriteLock writeLock(registeredHandlersMapLock_);

        if (registeredHandlersMap_.count(actor) != 0) { return false; }

        registeredHandlersMap_[actor] = std::make_pair(beginFunction, endFunction);

        return true;
    }

    void EntreeService::UnRegisterGatewayMessageHandler(
        Transport::Actor::Enum actor)
    {
        AcquireWriteLock writeLock(registeredHandlersMapLock_);
        if (registeredHandlersMap_.count(actor) != 0)
        {
            registeredHandlersMap_.erase(actor);
        }

        return;
    }

    AsyncOperationSPtr EntreeService::BeginDispatchGatewayMessage(
        MessageUPtr &&message,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ProcessGatewayMessageAsyncOperation>(
            move(message),
            *this,
            timeout,
            callback,
            parent);
    }

    ErrorCode EntreeService::EndDispatchGatewayMessage(
        AsyncOperationSPtr const& operation,
        __out MessageUPtr & replyMessage)
    {
        return ProcessGatewayMessageAsyncOperation::End(operation, replyMessage);
    }

    AsyncOperationSPtr EntreeService::BeginProcessQuery(
        QueryNames::Enum queryName,
        QueryArgumentMap const & queryArgs,
        ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<ProcessQueryAsyncOperation>(
            *this,
            queryName,
            queryArgs,
            activityId,
            timeout,
            callback,
            parent);
    }

    ErrorCode EntreeService::EndProcessQuery(
        AsyncOperationSPtr const & operation,
        _Out_ MessageUPtr & reply)
    {
        return ProcessQueryAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr EntreeService::BeginGetQueryListOperation(
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<GetQueryListAsyncOperation>(
            queryGateway_,
            timeout,
            callback,
            parent);
    }

    ErrorCode EntreeService::EndGetQueryListOperation(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply)
    {
        return GetQueryListAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr EntreeService::BeginGetApplicationNameOperation(
        ServiceModel::QueryArgumentMap const & queryArgs,
        ActivityId const & activityId,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        return AsyncOperation::CreateAndStart<GetApplicationNameAsyncOperation>(
            this->Properties,
            queryArgs,
            activityId,
            timeout,
            callback,
            parent);
    }

    ErrorCode EntreeService::EndGetApplicationNameOperation(
        AsyncOperationSPtr const & operation,
        __out MessageUPtr & reply)
    {
        return GetApplicationNameAsyncOperation::End(operation, reply);
    }

    AsyncOperationSPtr EntreeService::BeginForwardMessage(
        wstring const & childAddressSegment,
        wstring const & childAddressSegmentMetadata,
        MessageUPtr & message,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::FMAddressSegment))
        {
            return AsyncOperation::CreateAndStart<FMQueryAsyncOperation>(this->Properties, std::move(message), false, timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::FMMAddressSegment))
        {
            return AsyncOperation::CreateAndStart<FMQueryAsyncOperation>(this->Properties, std::move(message), true, timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::CMAddressSegment))
        {
            message->Headers.Add(ForwardMessageHeader(Transport::Actor::CM, QueryTcpMessage::QueryAction));
            return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::HMAddressSegment))
        {
            message->Headers.Add(ForwardMessageHeader(Transport::Actor::HM, QueryTcpMessage::QueryAction));
            return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::RMAddressSegment))
        {
            ServiceRoutingAgentMessage::SetForwardingHeaders(
                *message,
                Transport::Actor::RM,
                QueryTcpMessage::QueryAction,
                *ServiceTypeIdentifier::RepairManagerServiceTypeId);

            return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::HostingAddressSegment))
        {
            message->Headers.Replace(Transport::ActorHeader(Transport::Actor::Hosting));
            return StartSendQueryToNodeOperation(childAddressSegmentMetadata, message, timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::TestabilityAddressSegment))
        {
            message->Headers.Replace(Transport::ActorHeader(Transport::Actor::TestabilitySubsystem));
            return StartSendQueryToNodeOperation(childAddressSegmentMetadata, message, timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::RAAddressSegment))
        {
            message->Headers.Replace(Transport::ActorHeader(Transport::Actor::RA));
            return StartSendQueryToNodeOperation(childAddressSegmentMetadata, message, timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::TSAddressSegment))
        {
            ServiceRoutingAgentMessage::SetForwardingHeaders(
                *message,
                Transport::Actor::FAS,
                QueryTcpMessage::QueryAction,
                *ServiceTypeIdentifier::FaultAnalysisServiceTypeId);

            return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
        }
        else if (Common::StringUtility::AreEqualCaseInsensitive(childAddressSegment, QueryAddresses::GRMAddressSegment))
        {
            ServiceRoutingAgentMessage::SetForwardingHeaders(
                *message,
                Transport::Actor::GatewayResourceManager,
                QueryTcpMessage::QueryAction,
                *ServiceTypeIdentifier::GatewayResourceManagerServiceTypeId);

            NamingUri serviceName;
            NamingUri::TryParse(*SystemServiceApplicationNameHelper::PublicGatewayResourceManagerName, serviceName);
            message->Headers.Replace(ServiceTargetHeader(serviceName));

            return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
        }
        else
        {
            WriteWarning(RequestProcessingTraceComponent, "{0}: Invalid query argument or incorrect query configuration", this->Properties.Instance);
            return AsyncOperation::CreateAndStart<CompletedRequestAsyncOperation>(
                this->Properties,
                (message ? FabricActivityHeader::FromMessage(*message) : FabricActivityHeader()),
                ErrorCodeValue::InvalidArgument,
                callback,
                parent);
        }
    }

    AsyncOperationSPtr EntreeService::StartSendQueryToNodeOperation(
        wstring const & nodeName,
        MessageUPtr & message,
        TimeSpan timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
    {
        Federation::NodeId nodeId;
        ErrorCode errorCode = Federation::NodeIdGenerator::GenerateFromString(nodeName, nodeId);
        if (!errorCode.IsSuccess())
        {
            WriteWarning(
                RequestProcessingTraceComponent,
                "{0}: Failed to generated node Id from node name {1}. Error = {2}",
                this->Properties.Instance,
                nodeName,
                errorCode);

            return AsyncOperation::CreateAndStart<CompletedRequestAsyncOperation>(
                this->Properties,
                (message ? FabricActivityHeader::FromMessage(*message) : FabricActivityHeader()),
                ErrorCodeValue::InvalidArgument,
                callback,
                parent);
        }
        else
        {
            // Routed messages to a node can get NodeIsNotExactMatch error if the instance id of the node changes
            // and that change hasn't been propagated to this node. Query layer doesn't care about Node Instance id,
            // and federation layer should retry on such errors. Marking the message as Idempotent, enables that.
            message->Idempotent = true;
            return AsyncOperation::CreateAndStart<SendToNodeOperation>(this->Properties, std::move(message), timeout, callback, parent, nodeId);
        }
    }

    ErrorCode EntreeService::EndForwardMessage(
        AsyncOperationSPtr const & asyncOperation,
        _Out_ MessageUPtr & reply)
    {
        ErrorCode error = RequestAsyncOperationBase::End(asyncOperation, /*out*/reply);
        return error;
    }

    Common::AsyncOperationSPtr EntreeService::BeginReport(
        Transport::MessageUPtr && message,
        Common::TimeSpan timeout,
        Common::AsyncCallback const & callback,
        Common::AsyncOperationSPtr const & parent)
    {
        message->Headers.Add(ForwardMessageHeader(message->Actor, message->Action));
        message->Headers.Add(FabricActivityHeader(Guid::NewGuid()));
        return AsyncOperation::CreateAndStart<ForwardToServiceOperation>(this->Properties, std::move(message), timeout, callback, parent);
    }

    Common::ErrorCode EntreeService::EndReport(
        Common::AsyncOperationSPtr const & operation,
        __out Transport::MessageUPtr & reply)
    {
        return RequestAsyncOperationBase::End(operation, /*out*/reply);
    }

    void EntreeService::RegisterQueryHandlers()
    {
        queryMessageHandler_->RegisterQueryForwardHandler(
            [this](wstring const & childAddressSegment, wstring const & childAddressSegmentMetadata, MessageUPtr & message, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        { return this->BeginForwardMessage(childAddressSegment, childAddressSegmentMetadata, message, timeout, callback, parent); },
        [this](AsyncOperationSPtr const & asyncOperation, __out MessageUPtr & reply)
        { return this->EndForwardMessage(asyncOperation, reply); });

        queryMessageHandler_->RegisterQueryHandler(
            [this](QueryNames::Enum queryName, QueryArgumentMap const & queryArgs, ActivityId const & activityId, TimeSpan timeout, AsyncCallback const & callback, AsyncOperationSPtr const & parent)
        { return this->BeginProcessQuery(queryName, queryArgs, activityId, timeout, callback, parent); },
        [this](AsyncOperationSPtr const & asyncOperation, __out MessageUPtr & reply)
        { return this->EndProcessQuery(asyncOperation, reply); });
    }

    ErrorCode EntreeService::InitializeReceivingChannel()
    {
        ErrorCode error(ErrorCodeValue::Success);
        if (!Properties.IsInZombieMode)
        {
            this->Properties.ReceivingChannel = make_shared<EntreeServiceTransport>(
                this->Root,
                listenAddress_,
                this->Properties.Instance,
                this->Properties.NodeName,
                Events);

            this->Properties.SystemServiceClient = ServiceDirectMessagingClient::Create(
                *this->Properties.ReceivingChannel->GetInnerTransport(),
                *systemServiceResolver_,
                this->Root);

            error = this->Properties.ReceivingChannel->Open();
            if (!error.IsSuccess())
            {
                WriteWarning(
                    LifeCycleTraceComponent,
                    "{0}: Transport open failed, error = {1}",
                    Instance,
                    error);
            }

            listenAddress_ = Properties.ReceivingChannel->TransportListenAddress;
        }
        else
        {
            zombieModeTransport_ = DatagramTransportFactory::CreateTcp(
                listenAddress_,
                L"",
                L"EntreeService");

            zombieModeTransport_->SetKeepAliveTimeout(TimeSpan::FromSeconds(
                ClientConfig::GetConfig().KeepAliveIntervalInSeconds));

            zombieModeTransport_->SetConnectionIdleTimeout(TimeSpan::MaxValue);

            zombieModeTransport_->DisableSecureSessionExpiration();

            zombieModeTransport_->EnableInboundActivityTracing();

            zombieModeTransport_->AllowThrottleReplyMessage();
            NamingUtility::SetCommonTransportProperties(*zombieModeTransport_);

            error = SetSecurity(zombieModeSecuritySettings_);
            if (!error.IsSuccess()) { return error; }

            zombieModeDemuxer_ = make_unique<Demuxer>(this->Root, zombieModeTransport_);
            error = zombieModeDemuxer_->Open();
            if (!error.IsSuccess()) { return error; }

            error = zombieModeTransport_->Start();
            if (!error.IsSuccess()) { return error; }

            listenAddress_ = zombieModeTransport_->ListenAddress();
        }

        return error;
    }

    ErrorCode EntreeService::SetSecurity(Transport::SecuritySettings const & securitySettings)
    {
        if (!Properties.IsInZombieMode)
        {
            return ErrorCodeValue::Success;
        }

        auto error = this->zombieModeTransport_->SetSecurity(securitySettings);
        if (error.IsSuccess())
        {
            WriteInfo(
                LifeCycleTraceComponent,
                "{0}: set security settings succeeded, new value = {1}",
                Instance,
                securitySettings);
        }
        else
        {
            WriteError(
                LifeCycleTraceComponent,
                "{0}: set security settings failed: error = {1}, new value {2}",
                Instance, error, securitySettings);
        }

        return error;
    }

    void EntreeService::RegisterReceivingChannelMessageHandlers()
    {
        auto selfRoot = this->Root.CreateComponentRoot();

        // Debugging aid for scale scenario in Linux
        auto propertiesCopy = properties_;

        if (!Properties.IsInZombieMode)
        {
            this->Properties.ReceivingChannel->RegisterMessageHandler(
                Actor::NamingGateway,
                [this, selfRoot, propertiesCopy](MessageUPtr &message, TimeSpan const timeout, AsyncCallback const &callback, AsyncOperationSPtr const &parent)
                {
                    return this->BeginProcessRequest(move(message), timeout, callback, parent);
                },
                [this, selfRoot, propertiesCopy](AsyncOperationSPtr const &operation, __out MessageUPtr &reply)
                {
                    return this->EndProcessRequest(operation, reply);
                });
        }
        else
        {
            // Transport demuxer calls message->Clone() when dispatching on a different thread.
            this->zombieModeDemuxer_->RegisterMessageHandler(
                Actor::NamingGateway,
                [this, selfRoot](MessageUPtr & message, ReceiverContextUPtr & receiverContext)
                {
                    this->ProcessZombieModeMessage(move(message), receiverContext->ReplyTarget);
                },
                false);
        }
    }
}
