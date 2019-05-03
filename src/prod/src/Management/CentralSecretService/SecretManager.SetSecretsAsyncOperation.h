// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretManager::SetSecretsAsyncOperation
            : public SecretManager::SecretsAsyncOperation
        {
        public:
            SetSecretsAsyncOperation(
                Crypto const & crypto,
                Store::IReplicatedStore * replicatedStore,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                std::vector<Secret> const & secrets,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId);

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const & asyncOperation,
                __out std::vector<Secret> & secrets);

        protected:
            virtual Common::StringLiteral const & get_TraceComponent() const override;

            virtual void OnStart(AsyncOperationSPtr const & thisSPtr) override;

        private:
            Crypto const & crypto_;
            std::vector<Secret> secrets_;

            static Common::StringLiteral const TraceComponent;

            Common::ErrorCode ValidateSecrets();
            void CompleteStoreOperation(Common::AsyncOperationSPtr const & operationSPtr, bool expectedCompletedSynchronously);

            bool TryAddOrUpdateSecretValueToStoreTransaction(
                __inout Store::StoreTransaction & storeTx,
                __inout Secret & secret,
                SecretMetadataDataItem & existingResource,
                __inout ErrorCode & error);

            bool TryAddOrUpdateSecretResourceToStoreTransaction(
                __inout Store::StoreTransaction & storeTx,
                Secret const & secret,
                __inout SecretMetadataDataItem & existingResource,
                bool resourceExists,
                __inout ErrorCode & error);
        };
    }
}