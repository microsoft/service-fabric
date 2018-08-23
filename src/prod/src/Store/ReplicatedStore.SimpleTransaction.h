// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStore::SimpleTransaction : public TransactionBase
    {
        DENY_COPY(SimpleTransaction);

    public:
        SimpleTransaction(
            __in SimpleTransactionGroup &,
            std::shared_ptr<TransactionReplicator> const &,
            Common::ActivityId const &);

        virtual ~SimpleTransaction();

        uint64 get_MigrationTxKey() const override { return parent_.MigrationTxKey; }
        
        bool get_IsEnumerationSupported() const override { return false; };

        Common::ErrorCode TryGetInnerTransaction(__out TransactionSPtr &) override;
        bool TryReleaseInnerTransaction() override;

        void OnRollback() override;

        // Different SimpleTransactions will be using the same underlying SimpleTransactionGroup
        // from different threads
        //
        void AcquireLock() override { parent_.AcquireLock(); }
        void ReleaseLock() override { parent_.ReleaseLock(); }

        Common::ErrorCode AddReplicationOperation(ReplicationOperation && operation) override;

        // SimpleTransactionGroup will perform write conflict checks
        //
        bool ContainsDelete(std::wstring const &, std::wstring const &) override { return false; }
        bool ContainsOperation(std::wstring const &, std::wstring const &) override { return false; }

        std::vector<ReplicationOperation> && MoveReplicationOperations() override { return parent_.MoveReplicationOperations(); };

        Common::AsyncOperationSPtr BeginCommit(
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & state) override;

        Common::ErrorCode EndCommit(
            Common::AsyncOperationSPtr const & asyncOperation,
            __out ::FABRIC_SEQUENCE_NUMBER & commitSequenceNumber) override;

        void AddToParent(Common::AsyncOperationSPtr const & pendingReplication);

    protected:

        void OnOperationFailed(Common::ErrorCode const &) override;

    private:

        Common::ComponentRootSPtr parentRoot_;
        SimpleTransactionGroup & parent_;
        Common::atomic_bool isReleased_;
    };
}
