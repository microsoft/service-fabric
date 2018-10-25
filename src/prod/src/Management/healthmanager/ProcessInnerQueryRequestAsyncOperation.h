// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ProcessInnerQueryRequestAsyncOperation
            : public Common::TimedAsyncOperation
            , public IQueryProcessor
            , public Store::ReplicaActivityTraceComponent<Common::TraceTaskCodes::HealthManager>
        {
            DENY_COPY(ProcessInnerQueryRequestAsyncOperation)
        public:
            ProcessInnerQueryRequestAsyncOperation(
                __in EntityCacheManager & entityManager,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::ActivityId const & activityId,
                Common::ErrorCode const & passedThroughError,
                Query::QueryNames::Enum queryName,
                ServiceModel::QueryArgumentMap const & queryArgs,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent);

            virtual ~ProcessInnerQueryRequestAsyncOperation();

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out Transport::MessageUPtr & reply);

            void StartProcessQuery(Common::AsyncOperationSPtr const & thisSPtr);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

        private:
            Common::ErrorCode GetNodeIdFromQueryArguments(__out NodeHealthId & nodeId);
            Common::ErrorCode GetStringQueryArgument(
                std::wstring const & queryParameterName,
                bool acceptEmpty,
                __inout std::wstring & queryParameterValue);

            Common::ErrorCode GetStringQueryArgument(
                std::wstring const & queryParameterName,
                bool acceptEmpty,
                bool isOptionalArg,
                __inout std::wstring & queryParameterValue);

            //
            // Health state chunk queries
            //
            Common::ErrorCode CreateClusterHealthChunkQueryContext(
                __in QueryRequestContext & context);

            //
            // Entity queries
            //
            Common::ErrorCode CreateClusterQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateNodeQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreatePartitionQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateReplicaQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateServiceQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateApplicationQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateDeployedApplicationQueryContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateDeployedServicePackageQueryContext(
                __in QueryRequestContext & context);

            //
            // List queries
            //
            Common::ErrorCode CreateNodeQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreatePartitionQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateReplicaQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateServiceQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateApplicationQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateApplicationUnhealthyEvaluationContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateDeployedApplicationQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateDeployedApplicationsOnNodeQueryListContext(
                __in QueryRequestContext & context);
            Common::ErrorCode CreateDeployedServicePackageQueryListContext(
                __in QueryRequestContext & context);

            Common::ErrorCode passedThroughError_;
            EntityCacheManager & entityManager_;
            Query::QueryNames::Enum queryName_;
            ServiceModel::QueryArgumentMap queryArgs_;
            Transport::MessageUPtr reply_;
            int64 startTime_;
        };
    }
}

