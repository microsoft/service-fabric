// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EntreeService 
      : public IGateway
      , public Common::AsyncFabricComponent
      , public Client::HealthReportingTransport
      , public Common::TextTraceComponent<Common::TraceTaskCodes::NamingGateway>        
    {
        DENY_COPY(EntreeService)

    public:
        EntreeService(
            __in Federation::IRouter & innerRingCommunication,
            __in Reliability::ServiceResolver & fmClient,
            __in Reliability::ServiceAdminClient & adminClient,
            IGatewayManagerSPtr const &gatewayManager,
            Federation::NodeInstance const & instance,
            std::wstring const & listenAddress,
            Common::FabricNodeConfigSPtr const& nodeConfig,
            Transport::SecuritySettings const & zombieModeSecuritySettings,
            Common::ComponentRoot const & root,
            bool isInZombieMode = false);

        ~EntreeService();

        __declspec(property(get=get_NamingServiceCuids)) std::vector<Reliability::ConsistencyUnitId> const & NamingServiceCuids;
        std::vector<Reliability::ConsistencyUnitId> const & get_NamingServiceCuids() const { return properties_->NamingCuidCollection.NamingServiceCuids; }

        __declspec (property(get=get_Properties)) GatewayProperties & Properties;
        GatewayProperties & get_Properties() const { return *properties_; }

        __declspec (property(get = get_Instance)) Federation::NodeInstance Instance;
        Federation::NodeInstance get_Instance() const { return properties_->Instance; }

        Common::ErrorCode SetSecurity(Transport::SecuritySettings const & securitySettings);

        // IGateway functions

        Federation::NodeId get_Id() const;

        Common::AsyncOperationSPtr BeginProcessRequest(
            Transport::MessageUPtr && message, 
            Common::TimeSpan const,
            Common::AsyncCallback const &, 
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndProcessRequest(
            Common::AsyncOperationSPtr const &,
            __out Transport::MessageUPtr &);

        bool RegisterGatewayMessageHandler(
            Transport::Actor::Enum actor,
            BeginHandleGatewayRequestFunction const& beginFunction,
            EndHandleGatewayRequestFunction const& endFunction);

        void UnRegisterGatewayMessageHandler(
            Transport::Actor::Enum actor);

        virtual Common::AsyncOperationSPtr BeginReport(
            Transport::MessageUPtr && message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndReport(
            Common::AsyncOperationSPtr const & operation,
            __out Transport::MessageUPtr & reply);

        __declspec (property(get=get_EntreeServiceListenAddress)) std::wstring const& EntreeServiceListenAddress;
        std::wstring const& get_EntreeServiceListenAddress() const { return listenAddress_; }

    protected:
        Common::AsyncOperationSPtr OnBeginOpen(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndOpen(Common::AsyncOperationSPtr const &) override;

        Common::AsyncOperationSPtr OnBeginClose(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) override;
        Common::ErrorCode OnEndClose(Common::AsyncOperationSPtr const &) override;

        void OnAbort();

        virtual Common::AsyncOperationSPtr BeginDispatchGatewayMessage(
            Transport::MessageUPtr &&message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        virtual Common::ErrorCode EndDispatchGatewayMessage(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Transport::MessageUPtr & reply);

        virtual Common::AsyncOperationSPtr BeginProcessQuery(
            Query::QueryNames::Enum queryName, 
            ServiceModel::QueryArgumentMap const & queryArgs, 
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode EndProcessQuery(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Transport::MessageUPtr & reply);

        Common::AsyncOperationSPtr EntreeService::BeginGetQueryListOperation(
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EntreeService::EndGetQueryListOperation(
            Common::AsyncOperationSPtr const & operation, 
            __out Transport::MessageUPtr & reply);

        Common::AsyncOperationSPtr EntreeService::BeginGetApplicationNameOperation(
            ServiceModel::QueryArgumentMap const & queryArgs,
            Common::ActivityId const & activityId,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        Common::ErrorCode EntreeService::EndGetApplicationNameOperation(
            Common::AsyncOperationSPtr const & operation, 
            __out Transport::MessageUPtr & reply);

        virtual Common::AsyncOperationSPtr BeginForwardMessage(
            std::wstring const & childAddressSegment, 
            std::wstring const & childAddressSegmentMetadata, 
            Transport::MessageUPtr & message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        virtual Common::ErrorCode EndForwardMessage(
            Common::AsyncOperationSPtr const & asyncOperation, 
            __out Transport::MessageUPtr & reply);
        Common::AsyncOperationSPtr StartSendQueryToNodeOperation(
            std::wstring const & nodeName,
            Transport::MessageUPtr & message,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
                               
    private:
        class OpenAsyncOperation;
        class CloseAsyncOperation;
        class RequestAsyncOperationBase;
        class CompletedRequestAsyncOperation;
        class AdminRequestAsyncOperationBase;
        class NamingRequestAsyncOperationBase;
        class ServiceResolutionAsyncOperationBase;
        class CreateNameAsyncOperation;
        class CreateServiceAsyncOperation;
        class UpdateServiceAsyncOperation;
        class DeleteNameAsyncOperation;
        class DeleteServiceAsyncOperation;
        class EnumeratePropertiesAsyncOperation;
        class EnumerateSubnamesAsyncOperation;
        class ForwardToFileStoreServiceAsyncOperation;
        class ForwardToServiceOperation;
        class GetServiceDescriptionAsyncOperation;
        class NameExistsAsyncOperation;
        class PingAsyncOperation;
        class PropertyBatchAsyncOperation;
        class DeactivateNodeAsyncOperation;
        class ActivateNodeAsyncOperation;
        class DeactivateNodesBatchAsyncOperation;
        class RemoveNodeDeactivationsAsyncOperation;
        class GetNodeDeactivationStatusAsyncOperation;
        class NodeStateRemovedAsyncOperation;
        class RecoverPartitionsAsyncOperation;
        class RecoverPartitionAsyncOperation;
        class RecoverServicePartitionsAsyncOperation;
        class RecoverSystemPartitionsAsyncOperation;
        class ResetPartitionLoadAsyncOperation;
        class ToggleVerboseServicePlacementHealthReportingAsyncOperation;
        class ReportFaultAsyncOperation;
        class FMQueryAsyncOperation;
        class ResolveNameOwnerAsyncOperation;
        class ResolvePartitionAsyncOperation;
        class ResolveServiceAsyncOperation;
        class ServiceLocationNotificationAsyncOperation;
        class SendToNodeOperation;
        class QueryRequestOperation;
        class ProcessGatewayMessageAsyncOperation;
        class ForwardToTokenValidationServiceAsyncOperation;
        class StartNodeAsyncOperation;
        class ProcessServiceNotificationRequestAsyncOperation;
        class CreateSystemServiceAsyncOperation;
        class DeleteSystemServiceAsyncOperation;
        class ResolveSystemServiceAsyncOperation;
        class ResolveSystemServiceGatewayAsyncOperation;
        class PrefixResolveServiceAsyncOperation;
        class CreateNetworkAsyncOperation;
        class DeleteNetworkAsyncOperation;
        class Test_TestNamespaceManagerAsyncOperation;

        class ProcessQueryAsyncOperation;
        class GetQueryListAsyncOperation;
        class GetApplicationNameAsyncOperation;

        typedef std::function<Common::AsyncOperationSPtr(
            __in GatewayProperties &,
            __in Transport::MessageUPtr &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &)> ProcessRequestHandler;

        void InitializeHandlers(bool);

        void AddHandler(
            std::map<std::wstring, ProcessRequestHandler> & temp,
            std::wstring const &, 
            ProcessRequestHandler const &);

        template <class TAsyncOperation>
        static Common::AsyncOperationSPtr CreateHandler(
            __in GatewayProperties &,
            __in Transport::MessageUPtr &,
            Common::TimeSpan,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);

        Common::ErrorCode InitializeReceivingChannel();
        void RegisterReceivingChannelMessageHandlers();
        void RegisterQueryHandlers();

        void ProcessZombieModeMessage(
            Transport::MessageUPtr && message,
            Transport::ISendTarget::SPtr const & replyTarget);

        void OnProcessZombieModeMessageComplete(
            Transport::MessageId,
            Transport::FabricActivityHeader const &,
            Transport::ISendTarget::SPtr const &,
            Common::AsyncOperationSPtr const &,
            bool);

        Common::AsyncOperationSPtr StartQueryOperation(
            Transport::MessageUPtr && message, 
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback, 
            Common::AsyncOperationSPtr const & parent);

        NamingConfig const & namingConfig_;
        GatewayPropertiesSPtr properties_;        

        std::wstring listenAddress_;
        std::map<std::wstring, ProcessRequestHandler> requestHandlers_;

        //
        // *** Sub-components
        //

        IGatewayManagerSPtr gatewayManager_;

        // Query
        //
        Query::QueryMessageHandlerUPtr queryMessageHandler_;
        Query::QueryGatewayUPtr queryGateway_;        

        // System service messaging
        //
        SystemServices::SystemServiceResolverSPtr systemServiceResolver_;

        // File Upload
        //
        FileTransferGatewayUPtr fileTransferGateway_;

        // Claims Token Validation
        //
        typedef std::pair<BeginHandleGatewayRequestFunction, EndHandleGatewayRequestFunction> GatewayMessageHandler;
        std::map<Transport::Actor::Enum, GatewayMessageHandler> registeredHandlersMap_;
        Common::RwLock registeredHandlersMapLock_;

        //
        // Transport datastructures used when in Zombie mode.
        //
        Transport::IDatagramTransportSPtr zombieModeTransport_;
        Transport::DemuxerUPtr zombieModeDemuxer_;
        Transport::SecuritySettings zombieModeSecuritySettings_;

        // Service Push-Notifications (v3.0)
        //
        ServiceNotificationManagerUPtr serviceNotificationManager_;
    };

    typedef std::shared_ptr<EntreeService> EntreeServiceUPtr;
}
