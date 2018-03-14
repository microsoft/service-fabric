// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace Infrastructure
        {
            template<typename T>
            class EntityMap 
            {
                DENY_COPY(EntityMap);

            public:
                typedef typename EntityTraits<T>::IdType IdType;
                typedef typename EntityTraits<T>::DataType DataType;
                typedef std::shared_ptr<DataType> DataTypeSPtr;
                typedef EntityState<T> EntityStateType;
                typedef std::map<IdType, EntityEntryBaseSPtr> InMemoryState;
                typedef CommitDescription<T> CommitDescriptionType;
                typedef std::shared_ptr<EntityPreCommitNotificationSink<T>> EntityPreCommitNotificationSinkSPtrType;

                EntityMap(
                    ReconfigurationAgent & ra,
                    Infrastructure::IClock & clock, 
                    Diagnostics::RAPerformanceCounters & perfCounters, 
                    EntityPreCommitNotificationSinkSPtrType const & preCommitNotificationSink) :
                    clock_(clock),
                    perfCounters_(perfCounters),
                    preCommitNotificationSink_(preCommitNotificationSink),
                    ra_(ra)
                {
                }

                void Open(InMemoryState && entries)
                {
                    // Open the store on this node
                    entities_ = std::move(entries);
                }

                void Close()
                {
                }

                EntityEntryBaseSPtr GetOrCreateEntityMapEntry(IdType const & entityId, bool createFlag)
                {
                    {
                        Common::AcquireReadLock grab(lock_);

                        auto it = entities_.find(entityId);
                        if (it != entities_.end())
                        {
                            return it->second;
                        }
                        else if (!createFlag)
                        {
                            return nullptr;
                        }
                    }

                    // Do not perform the heap alloc inside the write lock
                    auto entry = EntityTraits<T>::Create(entityId, ra_);
                    std::pair<typename InMemoryState::iterator, bool> insertResult;

                    {
                        Common::AcquireWriteLock grab(lock_);

                        auto valueToInsert = std::make_pair(entityId, std::move(entry));
                        insertResult = entities_.insert(std::move(valueToInsert));
                    }

                    if (insertResult.second)
                    {
                        perfCounters_.NumberOfEntitiesInLFUM.Increment();
                    }

                    return insertResult.first->second;
                }

                EntityEntryBaseSPtr GetEntry(IdType const& entityId) const
                {
                    Common::AcquireReadLock grab(lock_);

                    auto it = entities_.find(entityId);
                    if (it == entities_.end())
                    {
                        return nullptr;
                    }

                    return it->second;
                }

                Common::AsyncOperationSPtr BeginCommit(
                    EntityEntryBaseSPtr const & entry,
                    CommitDescriptionType const & commitDescriptionBase,
                    EntityStateType & state,
                    IdType const & id,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) 
                {
                    return BeginUpdateLockedEntity(entry, commitDescriptionBase, state, id, timeout, callback, parent);
                }

                Common::ErrorCode EndCommit(
                    Common::AsyncOperationSPtr const & operation, 
                    __out Diagnostics::CommitEntityPerformanceData & commitPerfData)
                {
                    return EndUpdateLockedEntity(operation, commitPerfData);
                }

                bool IsEmpty() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return entities_.empty();
                }

                size_t GetCount() const
                {
                    Common::AcquireReadLock grab(lock_);
                    return entities_.size();
                }

                EntityEntryBaseSPtr Test_AddFailoverUnit(IdType const & id, EntityEntryBaseSPtr const & entry)
                {
                    entities_.insert(std::make_pair(id, entry));
                    return entry;
                }

                template<typename Pred>
                Infrastructure::EntityEntryBaseList GetEntries(Pred pred) const
                {
                    Common::AcquireReadLock grab(lock_);

                    Infrastructure::EntityEntryBaseList v;

                    for (auto const & it : entities_)
                    {
                        if (pred(it.second))
                        {
                            v.push_back(it.second);
                        }
                    }

                    return v;
                }

                Infrastructure::EntityEntryBaseList GetAllEntries() const
                {
                    Common::AcquireReadLock grab(lock_);

                    Infrastructure::EntityEntryBaseList v;
                    v.reserve(entities_.size());

                    for (auto const & it : entities_)
                    {
                        v.push_back(it.second);
                    }

                    return v;
                }

            private:
                Common::AsyncOperationSPtr BeginUpdateLockedEntity(
                    EntityEntryBaseSPtr const & entry,
                    CommitDescriptionType const & commitDescription,
                    EntityStateType & state,
                    IdType const & id,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent)
                {
                    ASSERT_IF(commitDescription.Type.IsInMemoryOperation && commitDescription.Type.StoreOperationType == Storage::Api::OperationType::Insert, "As of now only delete/update can be in-memory");

                    return Common::AsyncOperation::CreateAndStart<CommitAsyncOperation>(
                        *this,
                        state,
                        commitDescription,
                        id,
                        entry,
                        timeout,
                        callback,
                        parent);
                }

                Common::ErrorCode EndUpdateLockedEntity(
                    Common::AsyncOperationSPtr const & operation,
                    __out Diagnostics::CommitEntityPerformanceData & commitPerfData)
                {
                    auto casted = Common::AsyncOperation::End<CommitAsyncOperation>(operation);

                    commitPerfData = casted->CommitPerfData;

                    return casted->Error;
                }

                class CommitAsyncOperation : public Common::AsyncOperation
                {
                    DENY_COPY(CommitAsyncOperation);
                public:
                    CommitAsyncOperation(
                        EntityMap & entityMap,
                        EntityStateType & entityState,
                        CommitDescriptionType const & commitDescription,
                        IdType const & id,
                        EntityEntryBaseSPtr const & entry,
                        Common::TimeSpan, //timeout
                        Common::AsyncCallback const & callback,
                        Common::AsyncOperationSPtr const & parent) :
                        Common::AsyncOperation(callback, parent),
                        entityMap_(entityMap),
                        commitDescription_(commitDescription),
                        entry_(std::move(entry)),
                        state_(entityState),
                        id_(id)
                    {
                    }

                    __declspec(property(get = get_CommitPerfData)) Diagnostics::CommitEntityPerformanceData const & CommitPerfData;
                    Diagnostics::CommitEntityPerformanceData const & get_CommitPerfData() const { return commitPerfData_; }

                protected:
                    void OnStart(Common::AsyncOperationSPtr const & thisSPtr) override
                    {
                        if (commitDescription_.Type.IsInMemoryOperation)
                        {
                            entityMap_.perfCounters_.InMemoryCommitsPerSecond.Increment();
                            UpdateMapOnCommitComplete(true);
                            TryComplete(thisSPtr);
                            return;
                        }

                        entityMap_.perfCounters_.CommitsPerSecond.Increment();

                        std::vector<byte> bytes;
                        auto error = Serialize(bytes);
                        if (!error.IsSuccess())
                        {
                            TryComplete(thisSPtr, error);
                            return;
                        }

                        commitPerfData_.OnStoreCommitStart(entityMap_.clock_);

                        auto op = entry_->BeginCommit(
                            commitDescription_.Type.StoreOperationType,
                            std::move(bytes),
                            timeout_,
                            [this](Common::AsyncOperationSPtr const & innerOp)
                            {
                                if (!innerOp->CompletedSynchronously)
                                {
                                    FinishStoreOperation(innerOp);
                                }
                            },
                            thisSPtr);

                        if (op->CompletedSynchronously)
                        {
                            FinishStoreOperation(op);
                        }
                    }

                private:
                    void FinishStoreOperation(
                        Common::AsyncOperationSPtr const & storeOperation)
                    {
                        auto error = entry_->EndCommit(storeOperation);
                        AssertOnInconsistency(storeOperation->Parent, error);

                        commitPerfData_.OnStoreCommitEnd(entityMap_.clock_);

                        if (!error.IsSuccess())
                        {
                            entityMap_.perfCounters_.NumberOfCommitFailures.Increment();
                        }

                        UpdateMapOnCommitComplete(error.IsSuccess());

                        TryComplete(storeOperation->Parent, error);
                    }

                    void AssertOnInconsistency(
                        Common::AsyncOperationSPtr const & thisSPtr,
                        Common::ErrorCode const & error)
                    {
                        if (error.IsSuccess())
                        {
                            return;
                        }

                        /*
                            The RA infrastructure provides several guarantees.

                            If any of these guarantees are violated and the store is attempting to perform a persistence which fails
                            it is better to go down than to continue execution in an unknown state

                            - The RA can never delete a key that it has not inserted
                            - The RA cannot update a key that does not exist
                            - RA cannot insert the same key twice
                        */

                        bool shouldAssert = false;

                        switch (commitDescription_.Type.StoreOperationType)
                        {
                        case Storage::Api::OperationType::Delete:
                            shouldAssert = error.IsError(Common::ErrorCodeValue::StoreRecordNotFound);                            
                            break;

                        case Storage::Api::OperationType::Insert:
                            shouldAssert = error.IsError(Common::ErrorCodeValue::StoreWriteConflict);
                            break;

                        case Storage::Api::OperationType::Update:
                            shouldAssert = error.IsError(Common::ErrorCodeValue::StoreRecordNotFound);
                            break;
                        }

                        if (shouldAssert)
                        {
                            // Complete the async op otherwise the async op will throw in the destructor and the stack will be lost
                            TryComplete(thisSPtr, error);
                            Common::Assert::CodingError(
                                "Inconsistency on commit. id = {0}. OpType = {1}. Err = {2}", 
                                id_, 
                                static_cast<int>(commitDescription_.Type.StoreOperationType),
                                error);
                        }
                    }


                    void UpdateMapOnCommitComplete(bool wasSuccess)
                    {
                        if (!wasSuccess)
                        {
                            return;
                        }

                        auto operationType = commitDescription_.Type.StoreOperationType;

                        // Success, commit in-memory changes
                        if (operationType == Storage::Api::OperationType::Delete)
                        {
                            state_.Delete();
                            entityMap_.perfCounters_.NumberOfEntitiesInLFUM.Decrement();

                            {
                                Common::AcquireWriteLock grab(entityMap_.lock_);
                                size_t count = entityMap_.entities_.erase(id_);
                                ASSERT_IF(count != 1, "Must have erased the entity here");
                            }
                        }
                        else if (operationType == Storage::Api::OperationType::Insert ||
                            operationType == Storage::Api::OperationType::Update)
                        {
                            state_.SetData(commitDescription_.Data);
                        }
                        else
                        {
                            Common::Assert::CodingError("unknown store op type {0}", static_cast<int>(operationType));
                        }
                    }

                    Common::ErrorCode Serialize(__out std::vector<byte> & bytes)
                    {
                        T const * item = commitDescription_.Data.get();
                        switch (commitDescription_.Type.StoreOperationType)
                        {
                        case Storage::Api::OperationType::Delete:
                            ASSERT_IF(item != nullptr, "Null item not provided for delete at {0}", id_);
                            return Common::ErrorCode::Success();

                        case Storage::Api::OperationType::Insert:
                        case Storage::Api::OperationType::Update:
                            ASSERT_IF(item == nullptr, "Null item provided for insert/update at {0}", id_);
                            return Common::FabricSerializer::Serialize(item, bytes);

                        default:
                            Common::Assert::CodingError("Unknown type {0}", static_cast<int>(commitDescription_.Type.StoreOperationType));
                        }
                    }

                    IdType id_;
                    EntityMap & entityMap_;
                    CommitDescriptionType commitDescription_;
                    EntityEntryBaseSPtr entry_;
                    EntityStateType & state_;
                    Common::TimeSpan timeout_;

                    Diagnostics::CommitEntityPerformanceData commitPerfData_;
                };

                // FailoverUnit map based on fuid
                InMemoryState entities_;
                Diagnostics::RAPerformanceCounters & perfCounters_;
                Infrastructure::IClock & clock_;
                mutable Common::RwLock lock_;
                EntityPreCommitNotificationSinkSPtrType preCommitNotificationSink_;
                ReconfigurationAgent & ra_;
            };
        }
    }
}

