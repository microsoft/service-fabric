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
            class DeployedServiceReplicaUtility
            {

            public:
                DeployedServiceReplicaUtility();

                ErrorCode GetReplicaQueryResult(
                    Infrastructure::EntityEntryBaseSPtr const & entityEntry,
                    std::wstring const & applicationName,
                    std::wstring const & serviceManifestName,
                    __out ServiceModel::DeployedServiceReplicaQueryResult & replicaQueryResult);

                ErrorCode GetReplicaQueryResult(
                    Infrastructure::EntityEntryBaseSPtr const & entityEntry,
                    std::wstring const & serviceManifestName,
                    int64 const & replicaId,
                    __out std::wstring & runtimeId,
                    __out FailoverUnitDescription & failoverUnitDescription,
                    __out ReplicaDescription & replicaDescription,
                    __out std::wstring & hostId,
                    __out ServiceModel::DeployedServiceReplicaQueryResult & replicaQueryResult);

                Common::AsyncOperationSPtr BeginGetReplicaDetailQuery(
                    ReconfigurationAgent & ra,
                    std::wstring const activityId,
                    Infrastructure::EntityEntryBaseSPtr const & entityEntry,
                    wstring const & runtimeId,
                    FailoverUnitDescription const & failoverUnitDescription,
                    ReplicaDescription const & replicaDescription,
                    wstring const & hostId,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent);

                Common::ErrorCode EndGetReplicaDetailQuery(
                    Common::AsyncOperationSPtr const & asyncOperation,
                    __out ServiceModel::DeployedServiceReplicaDetailQueryResult & replicaDetailQueryResult);

            private:
                ErrorCode GetDeployedServiceReplicaQueryResult(
                    Infrastructure::ReadOnlyLockedFailoverUnitPtr & entityEntry,
                    std::wstring const & applicationName,
                    std::wstring const & serviceManifestName,
                    __out ServiceModel::DeployedServiceReplicaQueryResult & replicaQueryResult);
            };
        }
    }
}
