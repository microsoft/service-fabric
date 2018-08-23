// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::SimpleTransactionGroup
        : public Common::ComponentRoot
        , public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(SimpleTransactionGroup);

    public:

        using ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::TraceId;
        using ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>::get_TraceId;

        SimpleTransactionGroup(
            __in ReplicatedStore & store,
            std::shared_ptr<TransactionReplicator> const &,
            __in TransactionSPtr && innerTransaction,
            IReplicatedStoreTxEventHandlerWPtr const &,
            Common::ActivityId const & activityId,
            int commitBatchingSizeLimit);
        virtual ~SimpleTransactionGroup();

        __declspec(property(get=get_MigrationTxKey)) uint64 MigrationTxKey;
        uint64 get_MigrationTxKey() const { return migrationTxKey_; }

        __declspec(property(get=get_ReplicatedStore)) Store::ReplicatedStore & ReplicatedStore;
        __declspec(property(get=get_ReplicationOperations)) std::vector<ReplicationOperation> const & ReplicationOperations;
        __declspec(property(get=get_OperationLSN)) ::FABRIC_SEQUENCE_NUMBER OperationLSN;

        Store::ReplicatedStore & get_ReplicatedStore() const { return replicatedStore_; }

        // Note: this accessor is not thread safe, so caller has to ensure proper synchronization
        std::vector<ReplicationOperation> const & get_ReplicationOperations() const { return replicationOperations_; }

        ::FABRIC_SEQUENCE_NUMBER get_OperationLSN() const { return operationLSN_; }

        TransactionBaseSPtr CreateSimpleTransaction(Common::ActivityId const & activityId);
        
        Common::ErrorCode TryGetInnerTransaction(__out TransactionSPtr &);
        Common::ErrorCode TryGetInnerTransactionCallerHoldsLock(__out TransactionSPtr &);
        void ReleaseInnerTransaction();

        Common::ErrorCode AddReplicationOperation(ReplicationOperation && operation, Common::ActivityId activityId);

        void AddCommitAsyncOperation(
            Common::ActivityId const & activityId,
            Common::AsyncOperationSPtr const & commitAsyncOperation);
        
        void Close();

        void BeginCommit();

        void FinishCommit(
            Common::AsyncOperationSPtr const & asyncOperation,
            bool completeSynchronously);

        void Rollback(Common::ActivityId const & activityId);

        virtual void AcquireLock() { transactionLock_.AcquireExclusive(); }
        virtual void ReleaseLock() { transactionLock_.ReleaseExclusive(); }

        virtual std::vector<ReplicationOperation> && MoveReplicationOperations();

    private:
        typedef std::unordered_map<Common::ActivityId, Common::AsyncOperationSPtr, Common::ActivityId::Hasher> CommittedOperationMap;

        // used to detect write conflict, which is a result of misuse of simple transaction.
        //
        typedef std::pair<std::wstring, std::wstring> TypeKeyPair;
        struct Hasher
        {
            size_t operator() (TypeKeyPair const & key) const 
            { 
                return Common::StringUtility::GetHash(key.first) + Common::StringUtility::GetHash(key.second);
            }

            bool operator() (TypeKeyPair const & left, TypeKeyPair const & right) const 
            { 
                return (left.first == right.first && left.second == right.second);
            }
        };

        bool IsBatchLimitExceeded() const;
        bool CanCreateTransaction() const;
        void PostCompletions(Common::ErrorCode const &, ::FABRIC_SEQUENCE_NUMBER);

        Common::ComponentRootSPtr storeRoot_;
        Store::ReplicatedStore & replicatedStore_;
        std::shared_ptr<TransactionReplicator> txReplicatorSPtr_;
        TransactionSPtr innerTransactionSPtr_;
        IReplicatedStoreTxEventHandlerWPtr txEventHandler_;
        int commitBatchingSizeLimit_;
        Common::ErrorCode creationError_;
        bool rolledback_;
        bool closed_;

        // Lock used to synchronize access across different simple transactions
        // by the replicated store
        RWLOCK(StoreSimpleTransactionGroupTransaction, transactionLock_);

        // Lock used to synchronize access to the simple transaction group
        // data structures
        RWLOCK(StoreSimpleTransactionGroup, lock_);
        
        std::unordered_map<TypeKeyPair, Common::ActivityId, Hasher> replicationMap_;

        // writes from all simple transactions.
        std::vector<ReplicationOperation> replicationOperations_;
        size_t replicationSize_;

        // map of replicate async operations of all simple transactions.
        CommittedOperationMap commitMap_;
        size_t committedTxCount_;
        
        ::FABRIC_SEQUENCE_NUMBER operationLSN_;

        uint64 migrationTxKey_;
    };
}
