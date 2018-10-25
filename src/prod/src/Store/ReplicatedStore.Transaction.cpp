// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("Transaction");

    ReplicatedStore::Transaction::Transaction(
        __in Store::ReplicatedStore & store,
        shared_ptr<TransactionReplicator> const & txReplicator,
        TransactionSPtr const & innerTransactionSPtr,
        IReplicatedStoreTxEventHandlerWPtr const & txEventHandler,
        Common::ActivityId const & activityId,
        Store::TransactionSettings const & settings)
        : TransactionBase(store, txReplicator, activityId)
        , innerTransactionSPtr_(innerTransactionSPtr)
        , txEventHandler_(txEventHandler)
        , txLock_()
        , isReadOnly_(false)
        , replicationOperations_()
        , liveKeys_()
        , deleteKeys_()
        , replicationOperationsLock_()
        , txSettings_(settings)
        , storeRoot_(store.Root.shared_from_this())
    {
        WriteNoise(
            TraceComponent, 
            "{0} ctor: id={1} this={2}",
            this->TraceId,
            this->TrackerId,
            static_cast<void*>(this));

        auto txHandler = txEventHandler_.lock();
        if (txHandler.get() != nullptr)
        {
            txHandler->OnCreateTransaction(this->ActivityId, this->TrackerId);
        }
    }

    ReplicatedStore::Transaction::~Transaction()
    {
        WriteNoise(
            TraceComponent, 
            "{0} ~dtor: id={1} this={2}",
            this->TraceId,
            this->TrackerId,
            static_cast<void*>(this));

        auto txHandler = txEventHandler_.lock();
        if (txHandler.get() != nullptr)
        {
            txHandler->OnReleaseTransaction(this->ActivityId, this->TrackerId);
        }

        this->OnDtor();
    }

    ErrorCode ReplicatedStore::Transaction::TryGetInnerTransaction(__out TransactionSPtr & txSPtr)
    {
        AcquireExclusiveLock lock(txLock_);

        txSPtr = innerTransactionSPtr_;

        return (txSPtr ? ErrorCodeValue::Success : ErrorCodeValue::StoreTransactionNotActive);
    }

    bool ReplicatedStore::Transaction::TryReleaseInnerTransaction()
    {
        TransactionSPtr toDestruct;
        {
            AcquireExclusiveLock lock(txLock_);

            toDestruct = move(innerTransactionSPtr_);
        }

        // Do not explicitly rollback since replication
        // may have already started. If so, allow the local
        // commit to finish.
        //
        bool released = (toDestruct.get() != nullptr);
        if (released)
        {
            // Setting the aborted flag prevents further read/write usage
            // via TransactionBase::CheckAborted()
            //
            this->Abort();
        }
        return released;
    }

    void ReplicatedStore::Transaction::OnRollback()
    {
        TransactionSPtr toRollback;
        if (this->TryGetInnerTransaction(toRollback).IsSuccess())
        {
            toRollback->Rollback();
        }
    }

    ErrorCode ReplicatedStore::Transaction::AddReplicationOperation(ReplicationOperation && operation) 
    { 
        if (operation.Operation == ReplicationOperationType::Delete)
        {
            deleteKeys_.insert(make_pair(operation.Type, operation.Key));
        }
        else
        {
            liveKeys_.insert(make_pair(operation.Type, operation.Key));
        }

        replicationOperations_.push_back(move(operation)); 

        return ErrorCodeValue::Success; 
    };

    bool ReplicatedStore::Transaction::ContainsDelete(wstring const & type, wstring const & key)
    {
        return (deleteKeys_.find(make_pair(type, key)) != deleteKeys_.end());
    }

    bool ReplicatedStore::Transaction::ContainsOperation(wstring const & type, wstring const & key)
    {
        return (liveKeys_.find(make_pair(type, key)) != liveKeys_.end());
    }

    vector<ReplicationOperation> && ReplicatedStore::Transaction::MoveReplicationOperations()
    {
        AcquireExclusiveLock lock(replicationOperationsLock_);

        isReadOnly_ = replicationOperations_.empty();
        liveKeys_.clear();
        deleteKeys_.clear();

        return move(replicationOperations_);
    }

    AsyncOperationSPtr ReplicatedStore::Transaction::BeginCommit(
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & root)
    {
        return AsyncOperation::CreateAndStart<ReplicateAsyncOperation<Transaction>>(
            *this,
            timeout,
            callback,
            root);
    }

    ErrorCode ReplicatedStore::Transaction::EndCommit(
        AsyncOperationSPtr const & operation,
        __out ::FABRIC_SEQUENCE_NUMBER & operationLSN)
    {
        auto error = ReplicateAsyncOperation<Transaction>::End(operation, operationLSN);

        // If the application is using a non-zero low watermark for simple transactions,
        // then it's possible that a regular tx and a tx group are both created with the
        // same snapshot of the database as the low watermark is crossed.
        //
        // Updates made by this regular tx won't be visible to a tx group that was
        // created before this regular tx commits. To avoid conflicts (i.e. sequence number
        // check failures), flush the current simple tx group to make sure subsequent
        // simple transactions attach to a new tx group that sees all updates made
        // by this regular tx.
        //
        if (!isReadOnly_ && error.IsSuccess())
        {
            this->ReplicatedStore.CloseCurrentSimpleTransactionGroup();

            auto txHandler = txEventHandler_.lock();
            if (txHandler.get() != nullptr)
            {
                error = txHandler->OnCommit(this->ActivityId, this->TrackerId);
            }
        }
        
        return error;
    }

    void ReplicatedStore::Transaction::OnOperationFailed(ErrorCode const &)
    {
        this->Abort();
    }

    // *** Hasher

    size_t ReplicatedStore::Transaction::Hasher::operator() (KeyTrackerEntry const & key) const
    {
        return StringUtility::GetHash(wformatString("{0}:{1}", key.first, key.second));
    }

    bool ReplicatedStore::Transaction::Hasher::operator() (KeyTrackerEntry const & left, KeyTrackerEntry const & right) const
    {
        return (left.first == right.first && left.second == right.second);
    }
}
