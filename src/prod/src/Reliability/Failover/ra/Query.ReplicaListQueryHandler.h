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
            class ReplicaListQueryHandler : public IQueryHandler
            {
            public:
                ReplicaListQueryHandler(
                    ReconfigurationAgent & ra,
                    QueryUtility queryUtility,
                    DeployedServiceReplicaUtility deployedServiceReplicaUtility);

                ReplicaListQueryHandler(const ReplicaListQueryHandler& handler);

                void ProcessQuery(
                    Federation::RequestReceiverContextUPtr && context,
                    ServiceModel::QueryArgumentMap const & queryArgs,
                    std::wstring const & activityId) override;
            private:
                static Common::ErrorCode ValidateAndGetParameters(
                    ServiceModel::QueryArgumentMap const & queryArgs,
                    __out std::wstring & applicationName,
                    __out std::wstring & serviceManifestName,
                    __out FailoverUnitId & failoverUnitId,
                    __out bool & isFailoverUnitProvided);
            private:
                ReconfigurationAgent & ra_;
                QueryUtility queryUtility_;
                DeployedServiceReplicaUtility deployedServiceReplicaUtility_;
            };
        }
    }
}
