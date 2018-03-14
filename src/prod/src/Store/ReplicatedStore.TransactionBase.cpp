// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    static atomic_uint64 NextTrackerId;

    StringLiteral const TraceComponent("Transaction");

    //
    // TransactionBase
    //

    ReplicatedStore::TransactionBase::TransactionBase(
        __in Store::ReplicatedStore & store,
        shared_ptr<TransactionReplicator> const & txReplicator,
        __in Common::ActivityId const & activityId)
        : ILocalStore::TransactionBase()
        , ReplicaActivityTraceComponent(store.PartitionedReplicaId, activityId)
        , replicatedStore_(store)
        , txReplicatorSPtr_(txReplicator)
        , trackerId_(++NextTrackerId)
        , creationTime_(DateTime::Now())
        , enumerationCount_(0)
        , active_(true)
        , aborted_(false)
        , isForceReleased_(false)
    {
    }

    ReplicatedStore::TransactionBase::~TransactionBase()
    {
    }

    void ReplicatedStore::TransactionBase::OnDtor()
    {
        if (enumerationCount_ != 0)
        {
            // Our own usage should ensure proper resource clean-up by releasing enumerations
            // before transactions, but do not assert when user applications (using KVS)
            // forget to Dispose managed transactions, which may cause enumerations to
            // destruct before transactions when GC occurs.
            //
            TRACE_INFO_AND_TESTASSERT(
                TraceComponent,
                "{0} enumerations still exist that are using this replicated transaction: count = {1}", 
                this->TraceId, 
                enumerationCount_);
        }

        this->Rollback();

        // If this transaction is the last object keeping the replicated store alive (via storeRoot_),
        // then ensure that the EseLocalStore transaction destructs and decrements the transaction
        // count before the EseLocalStore destructor runs. This is because EseLocalStore will 
        // assert that the transaction count is 0 in its destructor.
        //
        if (this->TryReleaseInnerTransaction())
        {
            this->ReplicatedStore.UntrackAndFinishTransaction(*this);
        }
    }

    bool ReplicatedStore::TransactionBase::TryForceReleaseInnerTransaction()
    {
        isForceReleased_ = this->TryReleaseInnerTransaction();

        return isForceReleased_;
    }

    ErrorCode ReplicatedStore::TransactionBase::Commit(__out ::FABRIC_SEQUENCE_NUMBER & sequenceNumber, TimeSpan const timeout)
    {
        ManualResetEvent waiter;
        auto operation = BeginCommit(
            timeout,
            [&](AsyncOperationSPtr const &) { waiter.Set(); },
            this->ReplicatedStore.Root.CreateAsyncOperationRoot());

        waiter.WaitOne();

        return EndCommit(operation, sequenceNumber);
    }

    void ReplicatedStore::TransactionBase::Rollback()
    {
        // Execute only if commit or Rollback has never been called 
        if (active_.exchange(false))
        {
            this->OnRollback();
        }
    }

    void ReplicatedStore::TransactionBase::Abort()
    {
        aborted_.store(true);
    }

    ErrorCode ReplicatedStore::TransactionBase::CheckAborted()
    {
        if (aborted_.load())
        {
            WriteWarning(
                TraceComponent, 
                "{0} transaction aborted: forced={1}",
                this->TraceId,
                isForceReleased_);

            return isForceReleased_ ? ErrorCodeValue::NotPrimary : ErrorCodeValue::TransactionAborted;
        }
        else
        {
            return ErrorCodeValue::Success;
        }
    }

    void ReplicatedStore::TransactionBase::OperationFailed(ErrorCode const & error)
    {
        // Enables implementation of more efficient TryInsert(), TryUpdate(), and TryDelete() patterns.
        //
        if (!error.IsError(ErrorCodeValue::StoreRecordAlreadyExists) && !error.IsError(ErrorCodeValue::StoreRecordNotFound))
        {
            this->OnOperationFailed(error);

            if (error.IsError(ErrorCodeValue::StoreFatalError))
            {
                replicatedStore_.ReportTransientFault(error);
            }
        }
    }

    //
    // TransactionBase::ReplicateAsyncOperation<TDerivedTransaction>
    //

    template <class TDerivedTransaction>
    ErrorCode ReplicatedStore::TransactionBase::ReplicateAsyncOperation<TDerivedTransaction>::End(
        AsyncOperationSPtr const & operation, 
        __out ::FABRIC_SEQUENCE_NUMBER & lsn)
    {
        auto casted = AsyncOperation::End<ReplicateAsyncOperation>(operation);

        if (casted->Error.IsSuccess())
        {
            lsn = casted->lsn_;
        }

        return casted->Error;
    }

    template <class TDerivedTransaction>
    void ReplicatedStore::TransactionBase::ReplicateAsyncOperation<TDerivedTransaction>::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        auto const & txReplicator = owner_.TxReplicator;

        if (txReplicator)
        {
            AsyncOperationSPtr operation;

            if (owner_.active_.exchange(false))
            {
                operation = txReplicator->BeginReplicate(
                    owner_, 
                    timeout_, 
                    [this](AsyncOperationSPtr const & thisSPtr) { this->OnReplicateCompleted(thisSPtr, false); },
                    thisSPtr);

                this->OnReplication(operation);
            }
            else
            {
                // An already committed/rolled back transaction should not accept commit again
                //
                operation = txReplicator->BeginReplicate(
                    ErrorCodeValue::StoreTransactionNotActive, 
                    owner_.ReplicaActivityId,
                    [this](AsyncOperationSPtr const & thisSPtr) { this->OnReplicateCompleted(thisSPtr, false); },
                    thisSPtr);
            }

            this->OnReplicateCompleted(operation, true);
        }
        else
        {
            this->TryComplete(thisSPtr, ErrorCodeValue::ObjectClosed);
        }
    }

    template <class TDerivedTransaction>
    void ReplicatedStore::TransactionBase::ReplicateAsyncOperation<TDerivedTransaction>::OnReplicateCompleted(
        AsyncOperationSPtr const & operation, 
        bool expectedCompletedSynchronously)
    {
        if (operation->CompletedSynchronously != expectedCompletedSynchronously) { return; }

        auto error = owner_.TxReplicator->EndReplicate(operation, lsn_);

        switch (error.ReadValue())
        {
        case ErrorCodeValue::Success:
        case ErrorCodeValue::Timeout:
        case ErrorCodeValue::StoreTransactionNotActive:
            //
            // Timeout is not considered a failure during a commit.
            //
            // StoreTransactionNotActive indicates the tx has either already commited
            // or rolled back.
            //
            break;

        default:
            owner_.OnRollback();
            break;
        }

        this->TryComplete(operation->Parent, error);
    }

    template <class TDerivedTransaction>
    void ReplicatedStore::TransactionBase::ReplicateAsyncOperation<TDerivedTransaction>::OnReplication(
        AsyncOperationSPtr const &)
    {
        // intentional no-op
    }

    template <>
    void ReplicatedStore::TransactionBase::ReplicateAsyncOperation<ReplicatedStore::SimpleTransaction>::OnReplication(
        AsyncOperationSPtr const & operation)
    {
        // Okay to add after replication operation is started since underlying transaction group waits
        // for all associated simple transactions to commit before flushing. The last simple transaction
        // to commit triggers the flush.
        //
        owner_.AddToParent(operation);
    }

    template class ReplicatedStore::TransactionBase::ReplicateAsyncOperation<ReplicatedStore::Transaction>;
    template class ReplicatedStore::TransactionBase::ReplicateAsyncOperation<ReplicatedStore::SimpleTransaction>;
}
