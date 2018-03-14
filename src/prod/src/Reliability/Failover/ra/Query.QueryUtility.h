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
            class QueryUtility
            {

            public:
                QueryUtility();

                vector<Infrastructure::EntityEntryBaseSPtr> GetAllEntityEntries(
                    ReconfigurationAgent & ra) const;

                vector<Infrastructure::EntityEntryBaseSPtr> GetEntityEntriesByFailoverUnitId(
                    ReconfigurationAgent & ra,
                    FailoverUnitId const & ftId) const;

                static void SendErrorResponseForQuery(
                    ReconfigurationAgent & ra,
                    std::wstring const & activityId,
                    Common::ErrorCode const & error,
                    Federation::RequestReceiverContext & context);

                static void SendResponseForGetServiceReplicaListDeployedOnNodeQuery(
                    ReconfigurationAgent & ra,
                    std::wstring const & activityId,
                    std::vector<ServiceModel::DeployedServiceReplicaQueryResult> && items,
                    Federation::RequestReceiverContext & context);

                static void SendResponseForGetDeployedServiceReplicaDetailQuery(
                    ReconfigurationAgent & ra,
                    ServiceModel::DeployedServiceReplicaDetailQueryResult & deployedServiceReplicaDetailQueryResult,
                    Federation::RequestReceiverContext & context);
            };
        }
    }
}
