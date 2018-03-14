// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Query
        {
            class ReplicaDetailQueryHandler : public IQueryHandler
            {

            public:
                ReplicaDetailQueryHandler(
                    ReconfigurationAgent & ra,
                    QueryUtility queryUtility,
                    DeployedServiceReplicaUtility deployedServiceReplicaUtility);

                ReplicaDetailQueryHandler(const ReplicaDetailQueryHandler & handler);

                void ProcessQuery(
                    Federation::RequestReceiverContextUPtr && context,
                    ServiceModel::QueryArgumentMap const & queryArgs,
                    std::wstring const & activityId) override;

            private:
                static Common::ErrorCode ValidateAndGetParameters(
                    ServiceModel::QueryArgumentMap const & queryArgs,
                    __out std::wstring & serviceManifestName,
                    __out FailoverUnitId & failoverUnitId,
                    __out int64 & replicaId);

                void OnCompleteSendResponse(
                    std::wstring const & activityId,
                    Federation::RequestReceiverContext & context,
                    Common::AsyncOperationSPtr const& operation,
                    ServiceModel::DeployedServiceReplicaQueryResult & replicaQueryResult);

            private:
                ReconfigurationAgent & ra_;
                QueryUtility queryUtility_;
                DeployedServiceReplicaUtility deployedServiceReplicaUtility_;
            };
        }
    }
}
