// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class ReplicatedStoreWrapper 
        {
            DENY_COPY(ReplicatedStoreWrapper);

        public:
            ReplicatedStoreWrapper(
                __in Store::IReplicatedStore * store,
                Store::PartitionedReplicaId const & partitionedReplicaId);

            ~ReplicatedStoreWrapper();

            __declspec(property(get=get_Store)) Store::IReplicatedStore const * Store;
            Store::IReplicatedStore const * get_Store() const { return store_; }

            void SetThrottleCallback(Store::ThrottleCallback const & throttleCallback);

            Common::ErrorCode CreateTransaction(
                Common::ActivityId const & activityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            Common::ErrorCode CreateSimpleTransaction(
                Common::ActivityId const & activityId,
                __out Store::IStoreBase::TransactionSPtr & tx);

            Store::StoreTransaction CreateTransaction(
                Common::ActivityId const & activityId);

            Common::ErrorCode Insert(
                __in Store::IStoreBase::TransactionSPtr & tx,
                Store::StoreData const & storeData) const;

            Common::ErrorCode Update(
                __in Store::IStoreBase::TransactionSPtr & tx,
                Store::StoreData const & storeData) const;

            Common::ErrorCode Delete(
                __in Store::IStoreBase::TransactionSPtr & tx,
                Store::StoreData const & storeData) const;
            
            Common::ErrorCode ReadExact(
                Common::ActivityId const & activityId,
                __in Store::StoreData & storeData);

            template <class T>
            Common::ErrorCode ReadPrefix(
                __in Store::StoreTransaction & tx,
                std::wstring const & prefix,
                __inout std::vector<T> & entries);

            static Common::AsyncOperationSPtr BeginCommit(
                Store::ReplicaActivityId const & replicaActivityId,
                PrepareTransactionCallback const & prepareTxCallback,
                Common::AsyncCallback const & callback, 
                Common::AsyncOperationSPtr const & parent);
            static Common::ErrorCode EndCommit(
                Common::AsyncOperationSPtr const & operation);

            static bool ShowsInconsistencyBetweenMemoryAndStore(Common::ErrorCode const & error);
            
        private:

            class CommitTxAsyncOperation;
            
            Store::IReplicatedStore * store_;
            Store::PartitionedReplicaId const & partitionedReplicaId_;
        };

        template <class T>
        Common::ErrorCode ReplicatedStoreWrapper::ReadPrefix(
            __in Store::StoreTransaction & tx,
            std::wstring const & prefix,
            __inout std::vector<T> & entries)
        {
            T dummy;
            auto error = tx.ReadPrefix<T>(
                dummy.Type,
                prefix,
                /*out*/entries);
            if (error.IsError(ErrorCodeValue::NotFound))
            {
                // Consider success
                error = ErrorCode(ErrorCodeValue::Success);
            }

            return error;
        }
    }
}
