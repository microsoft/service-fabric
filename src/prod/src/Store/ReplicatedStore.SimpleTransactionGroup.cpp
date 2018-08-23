// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    static atomic_uint64 NextMigrationTxKey;

    StringLiteral const TraceComponent("SimpleTransactionGroup");

    ReplicatedStore::SimpleTransactionGroup::SimpleTransactionGroup(
        __in Store::ReplicatedStore & store,
        shared_ptr<TransactionReplicator> const & txReplicator,
        __in TransactionSPtr && innerTransaction,
        IReplicatedStoreTxEventHandlerWPtr const & txEventHandler,
        Common::ActivityId const & activityId,
        int commitBatchingSizeLimit)
        : ComponentRoot()
        , ReplicaActivityTraceComponent(store.PartitionedReplicaId, activityId)
        , storeRoot_(store.Root.CreateComponentRoot())
        , replicatedStore_(store)
        , txReplicatorSPtr_(txReplicator)
        , innerTransactionSPtr_(move(innerTransaction))
        , txEventHandler_(txEventHandler)
        , commitBatchingSizeLimit_(commitBatchingSizeLimit)
        , creationError_(ErrorCodeValue::Success)
        , rolledback_(false)
        , closed_(false)
        , transactionLock_()
        , lock_()
        , replicationMap_()
        , replicationOperations_()
        , replicationSize_(0)
        , commitMap_()
        , committedTxCount_(0)
        , operationLSN_(0)
        , migrationTxKey_(++NextMigrationTxKey)
    {
        WriteInfo(
            TraceComponent, 
            "{0}: SimpleTransactionGroup::ctor", 
            this->TraceId);

        auto txHandler = txEventHandler_.lock();
        if (txHandler.get() != nullptr)
        {
            txHandler->OnCreateTransaction(this->ActivityId, this->MigrationTxKey);
        }
    }

    ReplicatedStore::SimpleTransactionGroup::~SimpleTransactionGroup()
    {
        WriteInfo(
            TraceComponent, 
            "{0}: SimpleTransactionGroup::~dtor", 
            this->TraceId);

        auto txHandler = txEventHandler_.lock();
        if (txHandler.get() != nullptr)
        {
            txHandler->OnReleaseTransaction(this->ActivityId, this->MigrationTxKey);
        }
    }

    TransactionBaseSPtr ReplicatedStore::SimpleTransactionGroup::CreateSimpleTransaction(Common::ActivityId const & activityId)
    {
        AcquireWriteLock grab(lock_);

        if (!this->CanCreateTransaction())
        {
            return nullptr;
        }

        auto txSPtr = make_shared<SimpleTransaction>(*this, txReplicatorSPtr_, activityId);

        auto result = commitMap_.insert(pair<Common::ActivityId, AsyncOperationSPtr>(txSPtr->ActivityId, nullptr));
        ASSERT_IFNOT(result.second, "activityId already exists");

        WriteNoise(
            TraceComponent, 
            "{0}: SimpleTransactionGroup::CreateSimpleTransaction: simple tx = {1}, total tx = {2}", 
            this->TraceId,
            activityId,
            commitMap_.size());

        return txSPtr;
    }

    void ReplicatedStore::SimpleTransactionGroup::Close()
    {
        bool doCommit;
        {
            AcquireWriteLock grab(lock_);

            WriteInfo(
                TraceComponent, 
                "{0}: SimpleTransactionGroup::Close: rolledback_ = {1}, total tx = {2}, committed tx = {3}", 
                this->TraceId,
                rolledback_,
                commitMap_.size(),
                committedTxCount_);

            closed_ = true;

            doCommit = (!rolledback_ && committedTxCount_ == commitMap_.size() && committedTxCount_ > 0);
        }

        if (doCommit)
        {
            BeginCommit();
        }
    }

    void ReplicatedStore::SimpleTransactionGroup::AddCommitAsyncOperation(
        Common::ActivityId const & activityId,
        AsyncOperationSPtr const & commitAsyncOperation)
    {
        ASSERT_IFNOT(commitAsyncOperation, "commitAsyncOperation");

        bool doCommit = false;
        bool shouldComplete = false;
        {
            AcquireWriteLock grab(lock_);

            if (rolledback_)
            {
                shouldComplete = true;
            }
            else
            {
                auto existing = commitMap_.find(activityId);
                if (existing == commitMap_.end())
                {
                    Assert::CodingError("activityId is not existing");
                }
                else
                {
                    ASSERT_IF(existing->second, "duplicate commit on activityId");
                }

                commitMap_[activityId] = commitAsyncOperation;
                ++committedTxCount_;

                ASSERT_IF(committedTxCount_ > commitMap_.size(), "too many commit");

                WriteInfo(
                    TraceComponent, 
                    "{0}: SimpleTransactionGroup::AddCommitAsyncOperation: simple tx = {1}, total tx = {2}, committed Tx = {3}, closed = {4}", 
                    this->TraceId,
                    activityId,
                    commitMap_.size(),
                    committedTxCount_,
                    closed_);

                doCommit = (closed_ && committedTxCount_ == commitMap_.size() && committedTxCount_ > 0);
            }
        }

        if (shouldComplete)
        {
            commitAsyncOperation->TryComplete(commitAsyncOperation, ErrorCodeValue::StoreOperationCanceled);
        }
        else if (doCommit)
        {
            BeginCommit();
        }
    }

    void ReplicatedStore::SimpleTransactionGroup::BeginCommit()
    {
        size_t replicationSize = 0;
        size_t committedTxCount = 0;
        size_t commitAsyncOperationsSize = 0;

        {
            AcquireReadLock grab(lock_);

            committedTxCount = committedTxCount_;
            commitAsyncOperationsSize = commitMap_.size();
            replicationSize = replicationSize_;
        }

        ASSERT_IFNOT(committedTxCount == commitAsyncOperationsSize, "committedTxCount_");
        ASSERT_IFNOT(commitAsyncOperationsSize > 0, "commitMap_.size()");

        if (txReplicatorSPtr_)
        {
            WriteInfo(
                TraceComponent, 
                "{0}: SimpleTransactionGroup::BeginCommit: data size = {1}, total tx = {2}", 
                this->TraceId,
                replicationSize,
                commitAsyncOperationsSize);

            auto inner = txReplicatorSPtr_->BeginReplicate(
                *this,
                [this](AsyncOperationSPtr const & operation) { this->FinishCommit(operation, false); },
                this->CreateAsyncOperationRoot());
            this->FinishCommit(inner, true);
        }
        else
        {
            this->PostCompletions(ErrorCodeValue::ObjectClosed, 0);
        }
    }

    void ReplicatedStore::SimpleTransactionGroup::FinishCommit(
        AsyncOperationSPtr const & operation,
        bool completeSynchronously)
    {
        if (operation->CompletedSynchronously != completeSynchronously)
        {
            return;
        }

        auto const & thisSPtr = operation->Parent;

        ::FABRIC_SEQUENCE_NUMBER operationLSN = FABRIC_INVALID_SEQUENCE_NUMBER;

        auto error = txReplicatorSPtr_->EndReplicate(operation, operationLSN);

        if (error.IsSuccess())
        {
            auto txHandler = txEventHandler_.lock();
            if (txHandler.get() != nullptr)
            {
                error = txHandler->OnCommit(this->ActivityId, this->MigrationTxKey);
            }
        }

        thisSPtr->TryComplete(thisSPtr, error);

        this->PostCompletions(error, operationLSN);
    }

    void ReplicatedStore::SimpleTransactionGroup::PostCompletions(ErrorCode const & error, ::FABRIC_SEQUENCE_NUMBER operationLSN)
    {
        auto snap = make_shared<CommittedOperationMap>();
        {
            AcquireWriteLock grab(lock_);

            operationLSN_ = operationLSN;

            snap->swap(commitMap_);
        }

        this->ReleaseInnerTransaction();

        WriteInfo(
            TraceComponent, 
            "{0}: SimpleTransactionGroup::FinishCommit: total tx = {1} result = {2} batchPeriod = {3}ms",
            this->TraceId,
            snap->size(),
            error,
            replicatedStore_.CommitBatchingPeriod);

        auto & store = replicatedStore_;
        auto storeRoot = storeRoot_->CreateComponentRoot();

        // Post tx group completion with some delay so that the ESE commit callback returns
        // and releases the underlying ESE transaction. Otherwise, if a new transaction group
        // is started within the context of this callback (before the underlying ESE transaction 
        // is released), then the new transaction group will still see the uncommitted view 
        // of the previous completing transaction group, potentially causing sequence number
        // check conflicts for the application.
        //
        Threadpool::Post([&store, storeRoot, snap, error]()
            {
                store.CloseCurrentSimpleTransactionGroup();

                for (auto const & item : *snap)
                {
                    auto const & operation = item.second;
                    if (operation)
                    {
                        operation->TryComplete(operation, error);
                    }
                }
            },
            TimeSpan::FromMilliseconds(store.CommitBatchingPeriod));
    }

    ErrorCode ReplicatedStore::SimpleTransactionGroup::TryGetInnerTransaction(__out TransactionSPtr & txSPtr)
    {
        AcquireWriteLock lock(transactionLock_);

        return this->TryGetInnerTransactionCallerHoldsLock(txSPtr);
    }

    ErrorCode ReplicatedStore::SimpleTransactionGroup::TryGetInnerTransactionCallerHoldsLock(__out TransactionSPtr & txSPtr)
    {
        txSPtr = innerTransactionSPtr_;

        return (txSPtr ? ErrorCodeValue::Success : ErrorCodeValue::StoreTransactionNotActive);
    }

    void ReplicatedStore::SimpleTransactionGroup::ReleaseInnerTransaction()
    {
        TransactionSPtr toDestruct;
        {
            AcquireWriteLock lock(transactionLock_);

            toDestruct = move(innerTransactionSPtr_);
        }
    }
 
    ErrorCode ReplicatedStore::SimpleTransactionGroup::AddReplicationOperation(
        ReplicationOperation && operation, 
        Common::ActivityId activityId)
    {
        // Note: callers must ensure that the transactionLock_ is held when calling this to serialize access
        // of simple transactions in the group
        //
        AcquireWriteLock grab(lock_);
        
        // Continue adding operations after close. The last simple transaction to commit will commit the entire batch.
        //
        if (rolledback_)
        {
            return ErrorCodeValue::StoreOperationCanceled;
        }

        auto typeKeyPair = std::make_pair(operation.Type, operation.Key);
        
        auto existing = replicationMap_.find(typeKeyPair);
        if (existing == replicationMap_.end())
        {
            WriteNoise(
                TraceComponent, 
                "{0}: SimpleTransactionGroup::AddReplicationOperation: type = {1}, key = {2}, activity {3}", 
                this->TraceId,
                typeKeyPair.first,
                typeKeyPair.second,
                activityId);

            replicationMap_.insert(make_pair(move(typeKeyPair), move(activityId)));
    
            replicationSize_ += operation.Size;

            replicationOperations_.push_back(move(operation));

            return ErrorCodeValue::Success;
        }
        else if (existing->second != activityId)
        {
            WriteError(
                TraceComponent, 
                "{0}: SimpleTransactionGroup::AddReplicationOperation: write conflict!! writing {1} to {2} with type = {3}, key = {4}", 
                this->TraceId,
                activityId,
                existing->second,
                typeKeyPair.first,
                typeKeyPair.second);

            return ErrorCodeValue::StoreWriteConflict;
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    void ReplicatedStore::SimpleTransactionGroup::Rollback(Common::ActivityId const & activityId)
    {
        CommittedOperationMap commitAsyncOperations;
        int committedTxCount = 0;

        {
            AcquireWriteLock grab(lock_);

            if (rolledback_)
            {
                return;
            }

            rolledback_ = true;
            closed_ = true;
            committedTxCount = static_cast<int>(committedTxCount_);
            commitAsyncOperations.swap(commitMap_);
        }

        WriteInfo(
            TraceComponent, 
            "{0}: SimpleTransactionGroup::Rollback: simple tx = {1}, total tx = {2}, committed tx = {3}", 
            this->TraceId,
            activityId,
            commitAsyncOperations.size(),
            committedTxCount);

        replicatedStore_.OnRollbackSimpleTransactionGroup(this);

        for (auto iterator = commitAsyncOperations.begin(); iterator != commitAsyncOperations.end(); iterator++)
        {
            if (iterator->second)
            {
                iterator->second->TryComplete(iterator->second, ErrorCodeValue::StoreOperationCanceled);
            }
        }
    }

    bool ReplicatedStore::SimpleTransactionGroup::IsBatchLimitExceeded() const
    {
        return (replicationSize_ > commitBatchingSizeLimit_);
    }

    bool ReplicatedStore::SimpleTransactionGroup::CanCreateTransaction() const
    {
        return !(rolledback_ || closed_ || this->IsBatchLimitExceeded());
    }
    
    vector<ReplicationOperation> && ReplicatedStore::SimpleTransactionGroup::MoveReplicationOperations()
    {
        // transactionLock_ is needed here to protect the replicationOperations_ member which can
        // also be accessed during Verify* in the Insert/Update/Delete calls on the replicated store

        AcquireExclusiveLock lock(transactionLock_);

        return move(replicationOperations_);
    }
}
