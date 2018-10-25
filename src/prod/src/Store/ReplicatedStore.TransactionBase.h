// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::TransactionBase
        : public IStoreBase::TransactionBase
        , public ReplicaActivityTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
        DENY_COPY(TransactionBase);

    public:
        TransactionBase(
            __in Store::ReplicatedStore & store,
            std::shared_ptr<TransactionReplicator> const &,
            __in Common::ActivityId const & activityId);

        virtual ~TransactionBase();

        __declspec(property(get=get_TrackerId)) uint64 TrackerId;
        uint64 get_TrackerId() const { return trackerId_; }
        
        __declspec(property(get=get_MigrationTxKey)) uint64 MigrationTxKey;
        virtual uint64 get_MigrationTxKey() const = 0;

        __declspec(property(get=get_CreationTime)) Common::DateTime CreationTime;
        Common::DateTime get_CreationTime() const { return creationTime_; }
        
        __declspec(property(get=get_IsActive)) bool IsActive;
        bool get_IsActive() const { return active_.load(); }    
        
        __declspec(property(get=get_ReplicatedStore)) Store::ReplicatedStore & ReplicatedStore;
        Store::ReplicatedStore & get_ReplicatedStore() const { return replicatedStore_; }

        __declspec(property(get=get_TxReplicator)) std::shared_ptr<TransactionReplicator> const & TxReplicator;
        std::shared_ptr<TransactionReplicator> const & get_TxReplicator() const { return txReplicatorSPtr_; }

        __declspec(property(get=get_IsEnumerationSupported)) bool IsEnumerationSupported;
        virtual bool get_IsEnumerationSupported() const = 0;

        virtual Common::ErrorCode TryGetInnerTransaction(__out TransactionSPtr &) = 0;
        virtual bool TryForceReleaseInnerTransaction();
        virtual bool TryReleaseInnerTransaction() = 0;

        virtual void OnRollback() = 0;
        virtual void AcquireLock() = 0;
        virtual void ReleaseLock() = 0;

        virtual Common::ErrorCode AddReplicationOperation(ReplicationOperation && operation) = 0;
        virtual bool ContainsDelete(std::wstring const & type, std::wstring const & key) = 0;
        virtual bool ContainsOperation(std::wstring const & type, std::wstring const & key) = 0;
        virtual std::vector<ReplicationOperation> && MoveReplicationOperations() = 0;

        Common::ErrorCode Commit(__out ::FABRIC_SEQUENCE_NUMBER &, Common::TimeSpan const timeout) override;
        void Rollback() override;
        void Abort() override;
        Common::ErrorCode CheckAborted() override;

        void OnConstructEnumeration() const { ++enumerationCount_; }
        void OnDestructEnumeration() const { --enumerationCount_; }

        void OperationFailed(Common::ErrorCode const &);
        
    protected:

        template <class TDerivedTransaction>
        class ReplicateAsyncOperation : public Common::AsyncOperation
        {
        public:
            ReplicateAsyncOperation(
                __in TDerivedTransaction & owner,
                Common::TimeSpan const timeout,
                Common::AsyncCallback const & callback,
                Common::AsyncOperationSPtr const & parent)
                : Common::AsyncOperation(callback, parent)
                , owner_(owner)
                , timeout_(timeout)
                , lsn_(0)
            {
            }

            static Common::ErrorCode End(
                Common::AsyncOperationSPtr const &, 
                __out ::FABRIC_SEQUENCE_NUMBER &);

            void OnStart(Common::AsyncOperationSPtr const &);

        private:

            void OnReplication(Common::AsyncOperationSPtr const & operation);
            void OnReplicateCompleted(Common::AsyncOperationSPtr const & operation, bool expectedCompletedSynchronously);

            TDerivedTransaction & owner_;
            Common::TimeSpan timeout_;
            ::FABRIC_SEQUENCE_NUMBER lsn_;
        };

    protected:

        void OnDtor();
        virtual void OnOperationFailed(Common::ErrorCode const &) = 0;

        Store::ReplicatedStore & replicatedStore_;
        std::shared_ptr<TransactionReplicator> txReplicatorSPtr_;

        // Used by TransactionTracker to track outstanding transactions
        // during primary demotion
        //
        uint64 trackerId_;
        Common::DateTime creationTime_;

        mutable int enumerationCount_;

        // Transaction is active until committed or rolled back
        //
        Common::atomic_bool active_;

        // Transaction is aborted if an error is encountered.
        // Prevents user from retrying operations on the same 
        // transaction.
        // 
        Common::atomic_bool aborted_;

        bool isForceReleased_;
    };

    typedef std::shared_ptr<ReplicatedStore::TransactionBase> TransactionBaseSPtr;
    typedef std::weak_ptr<ReplicatedStore::TransactionBase> TransactionBaseWPtr;
}
