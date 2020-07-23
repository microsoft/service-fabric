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
        {
        public:
            SecretManager(
                Store::IReplicatedStore * replicatedStore,
                Store::PartitionedReplicaId const & partitionedReplicaId);

            __declspec(property(get = get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
            Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return partitionedReplicaId_; }

            __declspec(property(get = get_ReplicatedStore)) Store::IReplicatedStore * ReplicatedStore;
            Store::IReplicatedStore * get_ReplicatedStore() const { return replicatedStore_; }

            __declspec(property(get = get_Crypto)) Crypto const & CryptoAgent;
            Crypto const & get_Crypto() const { return crypto_; }

            Common::AsyncOperationSPtr BeginGetSecrets(
                std::vector<SecretReference> const & secretReferences,
                bool includeValue,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            Common::ErrorCode EndGetSecrets(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<Secret> & secrets);

            Common::AsyncOperationSPtr BeginSetSecrets(
                std::vector<Secret> const & secrets,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            Common::ErrorCode EndSetSecrects(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<Secret> & secrets);

            Common::AsyncOperationSPtr BeginRemoveSecrets(
                std::vector<SecretReference> const & secretReferences,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            Common::ErrorCode EndRemoveSecrets(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<SecretReference> & secretReferences);

            Common::AsyncOperationSPtr BeginGetSecretVersions(
                std::vector<SecretReference> const & secretReferences,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const& parent,
                Common::ActivityId const & activityId);

            Common::ErrorCode EndGetSecretVersions(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<SecretReference> & secretReferences);

            static std::wstring ToResourceName(Secret const & secret);
            static std::wstring ToResourceName(SecretReference const & secretRef);

            static Common::ErrorCode FromResourceName(__in std::wstring const & resourceName, __out SecretReference & secretRef);

        private:
            Crypto const crypto_;
            Store::PartitionedReplicaId const partitionedReplicaId_;
            Store::IReplicatedStore * replicatedStore_;


            class AutoRetryAsyncOperation;

            class SecretsAsyncOperation;
            class GetSecretsAsyncOperation;
            class SetSecretsAsyncOperation;
            class RemoveSecretsAsyncOperation;
            class GetSecretVersionsAsyncOperation;
        };

        using SecretManagerUPtr = std::unique_ptr<SecretManager>;
    }
}
