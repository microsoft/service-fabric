// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace CentralSecretService
    {
        class SecretManager::SecretsAsyncOperation
            : public Common::TimedAsyncOperation
            , public Common::TextTraceComponent<Common::TraceTaskCodes::CentralSecretService>
        {
        public:
            SecretsAsyncOperation(
                Store::IReplicatedStore * replicatedStore,
                Store::PartitionedReplicaId const & partitionedReplicaId,
                Common::TimeSpan const & timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent,
                Common::ActivityId const & activityId);

            virtual ~SecretsAsyncOperation();

        protected:
            __declspec(property(get = get_ActivityId)) Common::ActivityId const & ActivityId;
            Common::ActivityId const & get_ActivityId() const { return activityId_; }

            __declspec(property(get = get_PartitionedReplicaId)) Store::PartitionedReplicaId const & PartitionedReplicaId;
            Store::PartitionedReplicaId const & get_PartitionedReplicaId() const { return partitionedReplicaId_; }

            __declspec(property(get = get_ReplicatedStore)) Store::IReplicatedStore * ReplicatedStore;
            Store::IReplicatedStore * get_ReplicatedStore() const { return replicatedStore_; }

            __declspec(property(get = get_TraceComponent)) Common::StringLiteral const & TraceComponent;
            virtual Common::StringLiteral const & get_TraceComponent() const = 0;

            virtual void OnTimeout(AsyncOperationSPtr const &) override;
            virtual void OnStart(AsyncOperationSPtr const &) override;
            virtual void OnCompleted() override;
            virtual void OnCancel() override;

            Store::StoreTransaction CreateTransaction();
            void Complete(Common::AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & errorCode);

        private:
            Store::IReplicatedStore * replicatedStore_;
            Store::PartitionedReplicaId const partitionedReplicaId_;
            Common::ActivityId const activityId_;
        };
    }
}
