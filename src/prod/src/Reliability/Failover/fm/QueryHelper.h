// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace FailoverManagerComponent
    {
        class FMQueryHelper : public Common::FabricComponent
        {
            DENY_COPY(FMQueryHelper);

        public:
            FMQueryHelper(__in FailoverManager & fm);
            virtual ~FMQueryHelper();
            Common::AsyncOperationSPtr BeginProcessQueryMessage(
                Transport::Message & message,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);
            Common::ErrorCode EndProcessQueryMessage(
                Common::AsyncOperationSPtr const & asyncOperation,
                _Out_ Transport::MessageUPtr & reply);

            Common::TimeSpan timeout_;

        protected:
            virtual Common::ErrorCode OnOpen();
            virtual Common::ErrorCode OnClose();
            virtual void OnAbort();
        private:
            FailoverManager & fm_;

            Query::QueryMessageHandlerUPtr queryMessageHandler_;


            ServiceModel::ServicePartitionInformation FMQueryHelper::CheckPartitionInformation(Reliability::ConsistencyUnitDescription const & consistencyUnitDescription);
            void FMQueryHelper::InsertPartitionIntoQueryResultList(LockedFailoverUnitPtr const & failoverUnitPtr, ServiceModel::ServicePartitionInformation const & servicePartitionInformation, std::vector<ServiceModel::ServicePartitionQueryResult> & partitionQueryResultList);

            Common::ErrorCode GetNodesList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetSystemServicesList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetServicesList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetServiceGroupsMembersList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetServicePartitionList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetServicePartitionReplicaList(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetClusterLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetPartitionLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetNodeLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetReplicaLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetUnplacedReplicaInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetApplicationLoadInformation(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetServiceName(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode GetReplicaListByServiceNames(ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);
            Common::ErrorCode ProcessNIMQuery(Query::QueryNames::Enum queryname, ServiceModel::QueryArgumentMap const & queryArgs, ServiceModel::QueryResult & queryResult, Common::ActivityId const & activityId);

            void ConvertToLoadMetricReport(std::vector<LoadBalancingComponent::LoadMetricStats> const &, __out std::vector<ServiceModel::LoadMetricReport> &);

            bool IsMatch(FABRIC_QUERY_SERVICE_REPLICA_STATUS replicaStatus, DWORD replicaStatusFilter);
			bool IsMatch(FABRIC_QUERY_NODE_STATUS nodeStatus, DWORD nodeStatusFilter);

            void InsertReplicaInfoForPartition(LockedFailoverUnitPtr const & failoverUnitPtr, bool isFilterByPartitionName, std::wstring const & partitionNameFilter, ServiceModel::ReplicasByServiceQueryResult &replicaList);

            Common::ErrorCode FMQueryHelper::GetServiceNameAndCurrentPrimaryNode(Reliability::FailoverUnitId const & failoverUnitId, Common::ActivityId const &, std::wstring & serviceName, Federation::NodeId & currentPrimaryNodeId);

            class ProcessQueryAsyncOperation;

            Common::AsyncOperationSPtr BeginProcessQuery(
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndProcessQuery(
                Common::AsyncOperationSPtr const & operation,
                _Out_ Transport::MessageUPtr & reply);

            class MovePrimaryAsyncOperation;

            Common::AsyncOperationSPtr BeginMovePrimaryOperation(
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndMovePrimaryOperation(
                Common::AsyncOperationSPtr const & operation);

            class MoveSecondaryAsyncOperation;

            Common::AsyncOperationSPtr BeginMoveSecondaryOperation(
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::ActivityId const & activityId,
                Common::TimeSpan timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            Common::ErrorCode EndMoveSecondaryOperation(
                Common::AsyncOperationSPtr const & operation);

            bool TryGetLockedFailoverUnit(
                FailoverUnitId const& failoverUnitId,
                Common::ActivityId const & activityId,
                __out LockedFailoverUnitPtr & failoverUnitPtr
                ) const;

            Common::ErrorCode VerifyNodeExistsAndIsUp(Federation::NodeId const & nodeId)const;
        };
    }
}