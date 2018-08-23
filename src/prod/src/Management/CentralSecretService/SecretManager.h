// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretManager
            : public Common::ComponentRoot
            , public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::CentralSecretService>
        {
            using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::CentralSecretService>::TraceId;
            using PartitionedReplicaTraceComponent<Common::TraceTaskCodes::CentralSecretService>::get_TraceId;
        public:
            SecretManager(
                Store::IReplicatedStore * replicatedStore,
                Store::PartitionedReplicaId const & partitionedReplicaId);

            Common::ErrorCode GetSecrets(
                std::vector<SecretReference> const & secretReferences,
                bool includeValue,
                Common::ActivityId const & activityId,
                __out std::vector<Secret> & secrets);

            Common::AsyncOperationSPtr SetSecretsAsync(
                std::vector<Secret> const & secrets,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            Common::AsyncOperationSPtr RemoveSecretsAsync(
                std::vector<SecretReference> const & secretReferences,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            static std::wstring ToResourceName(Secret const & secret);
            static std::wstring ToResourceName(SecretReference const & secretRef);

            static Common::ErrorCode FromResourceName(__in std::wstring const & resourceName, __out SecretReference & secretRef);
        private:
            Crypto crypto_;
            Store::IReplicatedStore * replicatedStore_;
        };

        using SecretManagerUPtr = std::unique_ptr<SecretManager>;
    }
}
