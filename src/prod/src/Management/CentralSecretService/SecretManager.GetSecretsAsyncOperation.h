// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretManager::GetSecretsAsyncOperation
            : public SecretManager::SecretsAsyncOperation
        {
        public:
            GetSecretsAsyncOperation(
                Crypto const & crypto,
                Store::IReplicatedStore * replicatedStore,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                std::vector<SecretReference> const & secretReferences,
                bool const & includeValue,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<Secret> & secrets);

        protected:
            virtual void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override;

            virtual Common::StringLiteral const & get_TraceComponent() const override;

        private:
            Crypto const & crypto_;
            bool const includeValue_;
            std::vector<SecretReference> const secretReferences_;
            std::vector<Secret> readSecrets_;

            static Common::StringLiteral const TraceComponent;

            Common::ErrorCode ValidateSecretReferences();

            // retrieval helpers
            ErrorCode ListSecrets(Store::StoreTransaction const & storeTx, vector<Secret> & retrievedSecretResources);
        };
    }
}