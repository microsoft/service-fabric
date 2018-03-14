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
            /*
                The generic entity entry

                EntityTraits<T> must be specialized
            */
            template<typename T>
            class EntityEntry : public EntityEntryBase
            {
                DENY_COPY(EntityEntry);

            public:
                typedef typename EntityTraits<T>::IdType IdType;
                typedef std::shared_ptr<typename EntityTraits<T>::DataType> DataSPtrType;
                typedef typename EntityTraits<T>::DataType * DataPtrType;
                typedef EntityState<T> EntityStateType;      
                typedef EntityScheduler<T> EntitySchedulerType;
                typedef LockedEntityPtr<T> LockedEntityPtrType;
                typedef ReadOnlyLockedEntityPtr<T> ReadOnlyLockedEntityPtrType;
                typedef EntityMap<T> EntityMapType;

                EntityEntry(
                    IdType const & id, 
                    Storage::Api::IKeyValueStoreSPtr const & store) : 
                    EntityEntryBase(Common::formatString(id)),
                    id_(id),
                    persistedId_(EntityTraits<T>::RowType, Common::wformatString(id)),
                    store_(store)
                {
                    ASSERT_IF(store == nullptr, "cannot create entity without store");
                }

                EntityEntry(
                    IdType const & id, 
                    Storage::Api::IKeyValueStoreSPtr const & store, 
                    DataSPtrType && data) :
                    EntityEntryBase(Common::formatString(id)),
                    id_(id),
                    state_(std::move(data)),
                    persistedId_(EntityTraits<T>::RowType, Common::wformatString(id)),
                    store_(store)
                {
                    ASSERT_IF(store == nullptr, "cannot create entity without store");
                }

                __declspec(property(get = get_Id)) IdType const & Id;
                IdType const & get_Id() const { return id_; }

                __declspec(property(get = get_PersistenceId)) std::wstring const & PersistenceId;
                std::wstring const & get_PersistenceId() const { return persistedId_.Id; }

                __declspec(property(get = get_State)) EntityStateType & State;
                EntityStateType & get_State() { return state_; }

                __declspec(property(get = get_Scheduler)) EntitySchedulerType & Scheduler;
                EntitySchedulerType & get_Scheduler() { return scheduler_; }

                EntityMapType & GetEntityMap(ReconfigurationAgent & ra)
                {
                    return EntityTraits<T>::GetEntityMap(ra);
                }

                void AddEntityIdToMessage(Transport::Message & message) const override
                {
                    return EntityTraits<T>::AddEntityIdToMessage(message, id_);
                }

                Common::AsyncOperationSPtr BeginExecuteJobItem(
                    EntityEntryBaseSPtr const & thisSPtr,
                    EntityJobItemBaseSPtr const & jobItem,
                    ReconfigurationAgent & ra,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return Common::AsyncOperation::CreateAndStart<ScheduleAndExecuteEntityJobItemAsyncOperation<T>>(
                        thisSPtr,
                        jobItem,
                        ra,
                        callback,
                        parent);
                }

                Common::ErrorCode EndExecuteJobItem(
                    Common::AsyncOperationSPtr const & op,
                    __out std::vector<MultipleEntityWorkSPtr> & completedWork) override
                {
                    auto casted = Common::AsyncOperation::End<ScheduleAndExecuteEntityJobItemAsyncOperation<T>>(op);
                    casted->GetCompletedWork(completedWork);
                    return casted->Error;
                }

                LockedEntityPtrType Test_CreateLock()
                {
                    return LockedEntityPtrType(state_);
                }

                ReadOnlyLockedEntityPtrType CreateReadLock() const
                {
                    return ReadOnlyLockedEntityPtrType(state_);
                }

                Common::ErrorCode Test_Commit(
                    EntityEntryBaseSPtr const & thisSPtr,
                    LockedEntityPtrType & lock, 
                    ReconfigurationAgent & ra)
                {
                    auto desc = lock.CreateCommitDescription();

                    Common::ManualResetEvent ev;
                    Diagnostics::CommitEntityPerformanceData commitPerfData;

                    auto op = GetEntityMap(ra).BeginCommit(
                        thisSPtr,
                        desc,
                        state_,
                        id_,
                        ra.Config.MaxEseCommitWaitDuration,
                        [&ev](Common::AsyncOperationSPtr const &)
                        {
                            ev.Set();
                        },
                        ra.Root.CreateAsyncOperationRoot());

                    ev.WaitOne();

                    return GetEntityMap(ra).EndCommit(op, commitPerfData);
                }

                Common::AsyncOperationSPtr BeginCommit(
                    Storage::Api::OperationType::Enum operationType,
                    Storage::Api::RowData && bytes,
                    Common::TimeSpan const timeout,
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent) override
                {
                    return store_->BeginStoreOperation(
                        operationType,
                        persistedId_,
                        std::move(bytes),
                        timeout,
                        callback,
                        parent);
                }

                Common::ErrorCode EndCommit(Common::AsyncOperationSPtr const & operation)
                {
                    return store_->EndStoreOperation(operation);
                }

            private:
                IdType id_;
                mutable EntityStateType state_;
                EntitySchedulerType scheduler_;
                Storage::Api::RowIdentifier persistedId_;
                Storage::Api::IKeyValueStoreSPtr store_;
            };
        }
    }
}
