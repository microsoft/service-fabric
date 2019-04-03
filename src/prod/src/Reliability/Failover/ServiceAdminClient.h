// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ServiceAdminClient
    {
        DENY_COPY(ServiceAdminClient);

    public:
        explicit ServiceAdminClient(FederationWrapper & transport);

        __declspec (property(get=get_TraceId)) std::wstring const& TraceId;
        std::wstring const& get_TraceId() const { return transport_.InstanceIdStr; }

        __declspec (property(get=get_Transport)) FederationWrapper & Transport;
        FederationWrapper & get_Transport() const { return transport_; }

        Common::AsyncOperationSPtr BeginCreateService(
            ServiceDescription const & service,
            std::vector<ConsistencyUnitDescription> const& consistencyUnitDescriptions,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateService(Common::AsyncOperationSPtr const & operation, __out CreateServiceReplyMessageBody & body);

        Common::AsyncOperationSPtr BeginUpdateService(
            std::wstring const& serviceName,
            Naming::ServiceUpdateDescription const& serviceUpdateDescription,
            Transport::FabricActivityHeader const& activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);
        Common::AsyncOperationSPtr BeginUpdateService(
            ServiceDescription const& serviceDescription,
            std::vector<ConsistencyUnitDescription> && addedCuids,
            std::vector<ConsistencyUnitDescription> && removedCuids,
            Transport::FabricActivityHeader const& activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);
        Common::ErrorCode EndUpdateService(Common::AsyncOperationSPtr const& operation, __out UpdateServiceReplyMessageBody & body);

        Common::AsyncOperationSPtr BeginDeleteService(
            std::wstring const & serviceName,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timespan,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::AsyncOperationSPtr BeginDeleteService(
            std::wstring const & serviceName,
            bool const isForce,
            uint64 instanceId,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timespan,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteService(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginQuery(                        
            Transport::FabricActivityHeader const & activityHeader,
            Query::QueryAddressHeader const & addressHeader,
            Query::QueryRequestMessageBodyInternal const & requestMessageBody,
            bool isTargetFMM,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode ServiceAdminClient::EndQuery(Common::AsyncOperationSPtr const & operation, Transport::MessageUPtr & reply);

        Common::AsyncOperationSPtr BeginDeactivateNodes(
            std::map<Federation::NodeId, NodeDeactivationIntent::Enum> &&,
            std::wstring const & batchId,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndDeactivateNodes(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginRemoveNodeDeactivations(
            std::wstring const & batchId,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndRemoveNodeDeactivations(Common::AsyncOperationSPtr const &);

        Common::AsyncOperationSPtr BeginGetNodeDeactivationStatus(
            std::wstring const & batchId,
            Transport::FabricActivityHeader const &,
            Common::TimeSpan const,
            Common::AsyncCallback const &,
            Common::AsyncOperationSPtr const &);
        Common::ErrorCode EndGetNodeDeactivationsStatus(
            Common::AsyncOperationSPtr const &,
            __out NodeDeactivationStatus::Enum &,
            __out std::vector<NodeProgress> &);

        Common::AsyncOperationSPtr BeginNodeStateRemoved(
            Federation::NodeId nodeId,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndNodeStateRemoved(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRecoverPartitions(
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRecoverPartitions(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRecoverPartition(
            PartitionId const& partitionId,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRecoverPartition(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRecoverServicePartitions(
            std::wstring const& serviceName,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRecoverServicePartitions(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginRecoverSystemPartitions(
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndRecoverSystemPartitions(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginGetServiceDescription(
            std::wstring const& serviceName,
            Transport::FabricActivityHeader const& activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);
        Common::AsyncOperationSPtr BeginGetServiceDescriptionWithCuids(
            std::wstring const& serviceName,
            Transport::FabricActivityHeader const& activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);
        Common::ErrorCode EndGetServiceDescription(Common::AsyncOperationSPtr const& operation, __out ServiceDescriptionReplyMessageBody & body);

        Common::AsyncOperationSPtr BeginResetPartitionLoad(
            PartitionId const& partitionId,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndResetPartitionLoad(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginToggleVerboseServicePlacementHealthReporting(
            bool enabled,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndToggleVerboseServicePlacementHealthReporting(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginCreateNetwork(
            ServiceModel::ModelV2::NetworkResourceDescription const &networkDescription,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndCreateNetwork(Common::AsyncOperationSPtr const & operation);

        Common::AsyncOperationSPtr BeginDeleteNetwork(
            ServiceModel::DeleteNetworkDescription const &deleteNetworkDescription,
            Transport::FabricActivityHeader const & activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);
        Common::ErrorCode EndDeleteNetwork(Common::AsyncOperationSPtr const & operation);

    private:

        Common::AsyncOperationSPtr BeginGetServiceDescriptionInternal(
            std::wstring const& serviceName,
            bool includeCuids,
            Transport::FabricActivityHeader const& activityHeader,
            Common::TimeSpan timeout,
            Common::AsyncCallback const& callback,
            Common::AsyncOperationSPtr const& parent);

    private:

        class NodeDeactivationBaseAsyncOperation;
        class DeactivateNodesAsyncOperation;
        class RemoveNodeDeactivationsAsyncOperation;
        class GetNodeDeactivationStatusAsyncOperation;

        class NodeStateRemovedAsycOperation : public Common::AsyncOperation
        {
            DENY_COPY(NodeStateRemovedAsycOperation);

        public:

            NodeStateRemovedAsycOperation(
                ServiceAdminClient & serviceAdminClient,
                Federation::NodeId nodeId,
                Transport::FabricActivityHeader const& activityHeader,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

        private:

            static void OnRequestToFMMCompleted(Common::AsyncOperationSPtr const& requestOperation);
            static void OnRequestToFMCompleted(Common::AsyncOperationSPtr const& requestOperation);

            static Common::ErrorCode OnRequestCompleted(Common::ErrorCode thisError, Common::ErrorCode otherError);

            void Complete(Common::AsyncOperationSPtr const& opertion, Common::ErrorCode error);

            ServiceAdminClient & serviceAdminClient_;
            Federation::NodeId nodeId_;
            Transport::FabricActivityHeader const activityHeader_;
            Common::TimeSpan const timeout_;

            bool isRequestToFMMComplete_;
            bool isRequestToFMComplete_;

            Common::ErrorCode errorFMM_;
            Common::ErrorCode errorFM_;

            Common::ExclusiveLock lockObject_;
        };

        class RecoverPartitionsAsyncOperation : public Common::AsyncOperation
        {
            DENY_COPY(RecoverPartitionsAsyncOperation);

        public:

            RecoverPartitionsAsyncOperation(
                ServiceAdminClient & serviceAdminClient,
                Transport::FabricActivityHeader const& activityHeader,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

            static Common::ErrorCode End(Common::AsyncOperationSPtr const& operation);

        protected:

            void OnStart(Common::AsyncOperationSPtr const& thisSPtr);

            ServiceAdminClient & serviceAdminClient_;
            Transport::FabricActivityHeader const activityHeader_;

        private:

            virtual Transport::MessageUPtr CreateRequestMessage() const = 0;

            static void OnRequestToFMMCompleted(Common::AsyncOperationSPtr const& requestOperation);
            static void OnRequestToFMCompleted(Common::AsyncOperationSPtr const& requestOperation);

            static Common::ErrorCode OnRequestCompleted(Common::ErrorCode thisError, Common::ErrorCode otherError);

            void Complete(Common::AsyncOperationSPtr const& opertion, Common::ErrorCode error);

            Common::TimeSpan const timeout_;

            bool isRequestToFMMComplete_;
            bool isRequestToFMComplete_;

            Common::ErrorCode errorFMM_;
            Common::ErrorCode errorFM_;

            Common::ExclusiveLock lockObject_;
        };

        class RecoverAllPartitionsAsyncOperation : public RecoverPartitionsAsyncOperation
        {
            DENY_COPY(RecoverAllPartitionsAsyncOperation);

        public:

            RecoverAllPartitionsAsyncOperation(
                ServiceAdminClient & serviceAdminClient,
                Transport::FabricActivityHeader const& activityHeader,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

        private:

            virtual Transport::MessageUPtr CreateRequestMessage() const;
        };

        class RecoverSystemPartitionsAsyncOperation : public RecoverPartitionsAsyncOperation
        {
            DENY_COPY(RecoverSystemPartitionsAsyncOperation);

        public:

            RecoverSystemPartitionsAsyncOperation(
                ServiceAdminClient & serviceAdminClient,
                Transport::FabricActivityHeader const& activityHeader,
                Common::TimeSpan timeout,
                Common::AsyncCallback const& callback,
                Common::AsyncOperationSPtr const& parent);

        private:

            virtual Transport::MessageUPtr CreateRequestMessage() const;
        };

        FederationWrapper & transport_;
    };
}
