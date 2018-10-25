// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::Transaction : public TransactionBase
    {
        DENY_COPY(Transaction);

    public:
        Transaction(
            __in Store::ReplicatedStore &,
            std::shared_ptr<TransactionReplicator> const &,
            TransactionSPtr const & innerTransactionSPtr,
            IReplicatedStoreTxEventHandlerWPtr const &,
            Common::ActivityId const &,
            Store::TransactionSettings const & settings = Store::TransactionSettings());

        virtual ~Transaction();

        __declspec(property(get=get_TransactionSettings)) Store::TransactionSettings const & TransactionSettings;
        Store::TransactionSettings const & get_TransactionSettings() const { return txSettings_; }

        uint64 get_MigrationTxKey() const override { return this->TrackerId; }

        bool get_IsEnumerationSupported() const override { return true; };

        Common::ErrorCode TryGetInnerTransaction(__out TransactionSPtr &) override;
        bool TryReleaseInnerTransaction() override;

        void OnRollback() override;

        // Multi-threaded transactions are not supported, but we need to be resilient to incorrect usage,
        // returning errors instead of crashing/asserting
        //
        void AcquireLock() override { replicationOperationsLock_.AcquireExclusive(); }
        void ReleaseLock() override { replicationOperationsLock_.ReleaseExclusive(); }

        Common::ErrorCode AddReplicationOperation(ReplicationOperation && operation) override;
        bool ContainsDelete(std::wstring const & type, std::wstring const & key) override;
        bool ContainsOperation(std::wstring const & type, std::wstring const & key) override;
        std::vector<ReplicationOperation> && MoveReplicationOperations() override;

        Common::AsyncOperationSPtr BeginCommit(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) override;

        Common::ErrorCode EndCommit(
            Common::AsyncOperationSPtr const &,
            __out ::FABRIC_SEQUENCE_NUMBER &) override;

    protected:

        void OnOperationFailed(Common::ErrorCode const &) override;

    private:

        typedef std::pair<std::wstring, std::wstring> KeyTrackerEntry;

        struct Hasher
        {
            size_t operator() (KeyTrackerEntry const & key) const; 
            bool operator() (KeyTrackerEntry const & left, KeyTrackerEntry const & right) const;
        };

        typedef std::unordered_set<KeyTrackerEntry, Hasher, Hasher> KeyTracker;

        TransactionSPtr innerTransactionSPtr_;
        IReplicatedStoreTxEventHandlerWPtr txEventHandler_;
        Common::ExclusiveLock txLock_;

        bool isReadOnly_;
        std::vector<ReplicationOperation> replicationOperations_;
        KeyTracker liveKeys_;
        KeyTracker deleteKeys_;
        Common::ExclusiveLock replicationOperationsLock_;
        Store::TransactionSettings txSettings_;

        // Allow pending transactions after close
        Common::ComponentRootSPtr storeRoot_;
    };
}
