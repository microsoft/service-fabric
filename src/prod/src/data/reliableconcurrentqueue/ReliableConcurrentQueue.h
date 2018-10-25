// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../tstore/TestStateSerializer.h"

#define RCQ_TAG 'rtCQ'

namespace Data
{
    using namespace TStore;

    namespace Collections
    {
        //
        // Start with 512 slots and we'll double in every new segment
        // In debug we start with 4 to exercise the path more often
        //
#ifdef _DEBUG
        #define QUEUE_SEGMENT_START_SIZE      0x4
#else
        #define QUEUE_SEGMENT_START_SIZE      0x200
#endif
        //
        // Maximum is 64K slots (=65536)
        // If it is too large in rare race conditions the duplicate segment initialization may take too long
        // Of course, this needs to be measured
        // In debug we use a small-ish number to force creating more segments (and smaller) and flush out 
        // issues
        //
#ifdef _DEBUG
        #define QUEUE_SEGMENT_MAX_SIZE        0x10
#else
        #define QUEUE_SEGMENT_MAX_SIZE        0x10000
#endif

        template <typename TValue>
        class ReliableConcurrentQueue
            : public TStore::Store<LONG64, TValue>
            , public IReliableConcurrentQueue<TValue>
        {
            K_FORCE_SHARED(ReliableConcurrentQueue)
            K_SHARED_INTERFACE_IMP(IReliableConcurrentQueue)

        public:
            using typename TStore::Store<LONG64, TValue>::HashFunctionType;

            static NTSTATUS Create(
                __in KAllocator& allocator,
                __in PartitionedReplicaId const & traceId,
                __in KUriView & name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer,
                __out SPtr & result);

            ktl::Awaitable<void> EnqueueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TValue value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<bool> TryDequeueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __out TValue& value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            LONG64 GetQueueCount() const override
            {
                return this->Count;
            }

            ConcurrentQueue<LONG64> &GetInternalQueue()
            {
                return *queue_;
            }

        private:
            //
            // Forwards Store Add/Remove events to the queue
            //
            class DictionaryChangeForwarder
                : public KObject<DictionaryChangeForwarder>
                , public KShared<DictionaryChangeForwarder>
                , public IDictionaryChangeHandler<LONG64, TValue>
            {
                K_FORCE_SHARED(DictionaryChangeForwarder);
                K_SHARED_INTERFACE_IMP(IDictionaryChangeHandler);

            public:
                static HRESULT Create(
                    __in KAllocator& allocator,
                    __in ReliableConcurrentQueue<TValue> *queue,
                    __out typename DictionaryChangeForwarder::SPtr& result)
                {
                    DictionaryChangeForwarder* pointer = _new(RCQ_TAG, allocator) DictionaryChangeForwarder(queue);
                    if (!pointer)
                    {
                        return E_OUTOFMEMORY;
                    }

                    result = pointer;
                    return S_OK;
                }

            public :
                //
                // IDictionaryChangeHandler methods
                //
                ktl::Awaitable<void> OnAddedAsync(
                    __in TxnReplicator::TransactionBase const& replicatorTransaction,
                    __in LONG64 key,
                    __in TValue value,
                    __in LONG64 sequenceNumber,
                    __in bool isPrimary) noexcept
                {
                    UNREFERENCED_PARAMETER(replicatorTransaction);
                    UNREFERENCED_PARAMETER(sequenceNumber);
                    UNREFERENCED_PARAMETER(value);

                    queue_->OnAddedAsyncCallback(key, isPrimary);

                    co_return;
                }

                ktl::Awaitable<void> OnUpdatedAsync(
                    __in TxnReplicator::TransactionBase const& replicatorTransaction,
                    __in LONG64 key,
                    __in TValue value,
                    __in LONG64 sequenceNumber,
                    __in bool isPrimary) noexcept
                {
                    UNREFERENCED_PARAMETER(replicatorTransaction);
                    UNREFERENCED_PARAMETER(sequenceNumber);
                    UNREFERENCED_PARAMETER(key);
                    UNREFERENCED_PARAMETER(value);
                    UNREFERENCED_PARAMETER(isPrimary);

                    CODING_ASSERT("Should never receive OnUpdatedAsync events in queue");
                }

                ktl::Awaitable<void> OnRemovedAsync(
                    __in TxnReplicator::TransactionBase const& replicatorTransaction,
                    __in LONG64 key,
                    __in LONG64 sequenceNumber,
                    __in bool isPrimary) noexcept
                {
                    UNREFERENCED_PARAMETER(replicatorTransaction);
                    UNREFERENCED_PARAMETER(sequenceNumber);

                    queue_->OnRemovedAsyncCallback(key, isPrimary);

                    co_return;
                }

                ktl::Awaitable<void> OnRebuiltAsync(__in Utilities::IAsyncEnumerator<KeyValuePair<LONG64, KeyValuePair<LONG64, TValue>>> & enumerableState) noexcept
                {
                    UNREFERENCED_PARAMETER(enumerableState);

                    CODING_ASSERT("Should never receive OnRebuildAsync event in queue as it is masked");
                }

            private:
                DictionaryChangeForwarder(__in ReliableConcurrentQueue<TValue> *queue)
                {
                    queue_ = queue;
                }

            private:
                // Use raw pointer to avoid cycles - queue derives from store which sends the notifications
                // so they have the same lifetime
                ReliableConcurrentQueue<TValue> *queue_;
            };   

            //
            // Forwards Unlock events from StoreTransaction to queue
            //
            class QueueTransaction :
                public StoreTransaction<LONG64, TValue>
            {
                K_FORCE_SHARED(QueueTransaction)

            public:
                static NTSTATUS
                    Create(
                        __in TxnReplicator::IStateProvider2 *stateProvider,
                        __in LONG64 id,
                        __in TxnReplicator::TransactionBase& replicatorTransaction,
                        __in LONG64 owner,
                        __in ConcurrentDictionary2<LONG64,  KSharedPtr<StoreTransaction<LONG64, TValue>>>& container,
                        __in IComparer<LONG64>& keyComparer,
                        __in StoreTraceComponent & traceComponent,
                        __in KAllocator& allocator,
                        __out SPtr& result)
                {
                    NTSTATUS status;
                    SPtr output = _new(STORETRANSACTION_TAG, allocator) QueueTransaction(stateProvider, id, replicatorTransaction, owner, container, keyComparer, traceComponent);

                    if (!output)
                    {
                        status = STATUS_INSUFFICIENT_RESOURCES;
                        return status;
                    }

                    status = output->Status();
                    if (!NT_SUCCESS(status))
                    {
                        return status;
                    }

                    result = Ktl::Move(output);
                    return STATUS_SUCCESS;
                }

                QueueTransaction(
                    __in TxnReplicator::IStateProvider2 *stateProvider,
                    __in LONG64 id,
                    __in TxnReplicator::TransactionBase& transaction,
                    __in LONG64 owner,
                    __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<LONG64, TValue>>>& container,
                    __in IComparer<LONG64> & keyComparer,
                    __in StoreTraceComponent & traceComponent)
                    :StoreTransaction<LONG64, TValue>(id, transaction, owner, container, keyComparer, traceComponent),
                    stateProviderSPtr_(stateProvider)
                {
                }

            protected:
                void OnClearLocks() override
                {
                    typename ReliableConcurrentQueue<TValue>::SPtr rcqSPtr = static_cast<ReliableConcurrentQueue<TValue> *>(stateProviderSPtr_.RawPtr());
                    rcqSPtr->OnUnlockedCallback(*this);
                }

            private :
                typename TxnReplicator::IStateProvider2::SPtr stateProviderSPtr_;
            };

        private:
            void OnAddedAsyncCallback(LONG64 key, bool isPrimary)
            {
                // For both primary & secondary
                UNREFERENCED_PARAMETER(isPrimary);

                if (!isPrimary)
                {
                    // Make sure ID is up-to-date
                    UpdateId(key);
                }

                NTSTATUS status = queue_->Enqueue(key);
                if (!NT_SUCCESS(status))
                {
                    throw ktl::Exception(status);
                }
            }

            void OnRemovedAsyncCallback(LONG64 key, bool isPrimary)
            {
                UNREFERENCED_PARAMETER(key);

                if (isPrimary)
                {
                    // do nothing in primary - dequeue happens immediately in transaction so in commit
                    // we have nothing to do
                }
                else
                {
                    // in secondary / recovery, we need to mutate the in-memory data structure
                    // note that apply for different transactions may be coming in in parallel which means
                    // we need to remove that specific key instead of dequeue
                    // This unfortunately introduces "gaps" which we'll need to handle as well
                    NTSTATUS status = queue_->Remove(key);
                    if (!NT_SUCCESS(status))
                    {
                        throw ktl::Exception(status);
                    }
                }
            }

            void OnRecoverKeyCallback(__in LONG64 key, __in VersionedItem<TValue>& value) override
            {
                // STORE_ASSERT(value->GetRecordKind() == RecordKind::InsertedVersion, "Should only see InsertedValue in recovery", value->GetRecordKind());
                if (value.GetRecordKind() == RecordKind::InsertedVersion)
                {
                    // Make sure ID is up-to-date
                    UpdateId(key);

                    NTSTATUS status = queue_->Enqueue(key);
                    if (!NT_SUCCESS(status))
                    {
                        throw ktl::Exception(status);
                    }
                }
            }
            
            void UpdateId(LONG64 newId)
            {
                while (true)
                {
                    LONG64 curId = id_;

                    // We've already past that
                    if (curId >= newId)
                        break;
                    
                    // Otherwise, increase to that ID
                    if (InterlockedCompareExchange64(&id_, newId, curId) == curId)
                    {
                        break;
                    }
                }
            }

            void OnUnlockedCallback(__in StoreTransaction<LONG64, TValue> &storeTransaction)
            {
                KSharedPtr<WriteSetStoreComponent<LONG64, TValue>> component = storeTransaction.GetComponent(this->GetFunc());

                NTSTATUS status;
                auto enumerator = component->GetEnumerator();
                while (enumerator->MoveNext())
                {
                    auto kv = enumerator->Current();
                    auto key = kv.Key;
                    auto versionedItem = kv.Value.LatestValue;
                    if (versionedItem->GetRecordKind() == RecordKind::DeletedVersion && 
                        versionedItem->GetVersionSequenceNumber() == 0)
                    {
                        // Dequeue that haven't yet been applied (otherwise it would have a valid version sequence number)
                        // Abort the dequeue by enqueuing the ID
                        status = queue_->Enqueue(key);
                        if (!NT_SUCCESS(status))
                        {
                            throw ktl::Exception(status);
                        }
                    }
                }
            }

        protected:
            NTSTATUS OnCreateStoreTransaction(
                __in LONG64 id,
                __in TxnReplicator::TransactionBase& transaction,
                __in LONG64 owner,
                __in ConcurrentDictionary2<LONG64, KSharedPtr<StoreTransaction<LONG64, TValue>>>& container,
                __in IComparer<LONG64> & keyComparer,
                __out typename StoreTransaction<LONG64, TValue>::SPtr& result) override
            {
                typename QueueTransaction::SPtr ptr;
                NTSTATUS status = QueueTransaction::Create(this, id, transaction, owner, container, keyComparer, *this->TraceComponent, this->GetThisAllocator(), ptr);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = ptr.RawPtr();

                return status;
            }

        private:
            FAILABLE ReliableConcurrentQueue(
                __in PartitionedReplicaId const & traceId,
                __in IComparer<LONG64>& keyComparer,
                __in HashFunctionType func,
                __in KUriView& name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Data::StateManager::IStateSerializer<LONG64>& keyStateSerializer,
                __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer);

        private:

#pragma region IStateProviderInfo implementation

            StateProviderKind GetKind() const override
            {
                return StateProviderKind::ConcurrentQueue;
            }

#pragma endregion

        private:
            LONG64 id_;
            typename ConcurrentQueue<LONG64>::SPtr queue_;
            typename DictionaryChangeForwarder::SPtr dictionaryChangeForwarder_;

        private:
            LONG64 GetNextId();
        };

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::QueueTransaction::~QueueTransaction() {}

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::DictionaryChangeForwarder::~DictionaryChangeForwarder() {}

        template <typename TValue>
        NTSTATUS ReliableConcurrentQueue<TValue>::Create(
            __in KAllocator &allocator,
            __in PartitionedReplicaId const & traceId,
            __in KUriView & name,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer,
            __out SPtr & result)
        {
            NTSTATUS status;

            KSharedPtr<LongComparer> keyComparerSPtr = nullptr;
            LongComparer::Create(allocator, keyComparerSPtr);

            KSharedPtr<TStoreTests::TestStateSerializer<LONG64>> keySerializerSPtr = nullptr;
            status = TStoreTests::TestStateSerializer<LONG64>::Create(allocator, keySerializerSPtr);
            
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            SPtr output = _new(RCQ_TAG, allocator) ReliableConcurrentQueue(traceId, *keyComparerSPtr, K_DefaultHashFunction, name, stateProviderId, *keySerializerSPtr, valueStateSerializer);

            if (!output)
            {
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            status = output->Status();
            if (!NT_SUCCESS(status))
            {
                return status;
            }

            result = Ktl::Move(output);
            return STATUS_SUCCESS;
        }

#pragma region IReliableConcurrentQueue implementation

        template <typename TValue>
        ktl::Awaitable<void> ReliableConcurrentQueue<TValue>::EnqueueAsync(
            __in TxnReplicator::TransactionBase& replicatorTransaction,
            __in TValue value,
            __in Common::TimeSpan timeout,
            __in ktl::CancellationToken const & cancellationToken)
        {
            ApiEntry();

            KSharedPtr<IStoreTransaction<LONG64, TValue>> storeTransaction = nullptr;

            this->CreateOrFindTransaction(replicatorTransaction, storeTransaction);

            LONG64 id = GetNextId();

            // Add into store - only when apply comes we'll put the items into queue
            co_await this->AddAsync(*storeTransaction, id, value, timeout, cancellationToken);
        }

        template <typename TValue>
        ktl::Awaitable<bool> ReliableConcurrentQueue<TValue>::TryDequeueAsync(
            __in TxnReplicator::TransactionBase& replicatorTransaction,
            __out TValue& value,
            __in Common::TimeSpan timeout,
            __in ktl::CancellationToken const & cancellationToken)
        {
            ApiEntry();

            KSharedPtr<IStoreTransaction<LONG64, TValue>> storeTransaction = nullptr;

            this->CreateOrFindTransaction(replicatorTransaction, storeTransaction);

            LONG64 current;
            bool succeeded;
            NTSTATUS status = queue_->TryDequeue(current, succeeded);
            if (!NT_SUCCESS(status))
            {
                throw ktl::Exception(status);
            }

            if (!succeeded)
            {
                // @TODO - Implement "Read your own write" by inspecting WriteSet
                // However this means we'll also have a small in-memory queue here to properly keep track
                // of status of each item. We can do it in a future release
                co_return false;
            }

            KeyValuePair<LONG64, TValue> result;

            // @TODO - Merge these two calls into one call
            // Retrieve the value
            bool gotValue = co_await this->ConditionalGetAsync(*storeTransaction, current, timeout, result, cancellationToken);
            ASSERT_IFNOT(gotValue, "nRCQ: ConditionalGetAsync should always succeed");

            // Remove the item
            bool removed = co_await this->ConditionalRemoveAsync(*storeTransaction, current, timeout, cancellationToken);
            ASSERT_IFNOT(removed, "nRCQ: ConditionalRemoveAsync should always succeed");

            value = result.Value;
            co_return true;
        }

#pragma endregion IReliableConcurrentQueue implementation

        template <typename TValue>
        LONG64 ReliableConcurrentQueue<TValue>::GetNextId()
        {
            return InterlockedIncrement64((LONG64 *)&id_);
        }

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::ReliableConcurrentQueue()
            : id_(0)
        {
        }

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::ReliableConcurrentQueue(
            __in PartitionedReplicaId const & traceId,
            __in IComparer<LONG64>& keyComparer,
            __in HashFunctionType func,
            __in KUriView& name,
            __in FABRIC_STATE_PROVIDER_ID stateProviderId,
            __in Data::StateManager::IStateSerializer<LONG64>& keyStateSerializer,
            __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer)
            : TStore::Store<LONG64, TValue>(traceId, keyComparer, func, name, stateProviderId, keyStateSerializer, valueStateSerializer)
            , id_(0)
        {
            NTSTATUS status;
            status = ConcurrentQueue<LONG64>::Create(this->GetThisAllocator(), QUEUE_SEGMENT_START_SIZE, QUEUE_SEGMENT_MAX_SIZE, queue_);
            if (!NT_SUCCESS(status))
            {
                this->SetConstructorStatus(status);
                return;
            }

            status = DictionaryChangeForwarder::Create(this->GetThisAllocator(), this, dictionaryChangeForwarder_);
            if (!NT_SUCCESS(status))
            {
                this->SetConstructorStatus(status);
                return;
            }

            this->set_DictionaryChangeHandler(dictionaryChangeForwarder_.RawPtr());

            // Only need Add/Remove. Not going to receive Update ever. Don't want Rebuilt events as we have
            // a special override that doesn't aggressively load values
            this->set_DictionaryChangeHandlerMask(static_cast<DictionaryChangeEventMask::Enum>(DictionaryChangeEventMask::Add | DictionaryChangeEventMask::Remove));
        }

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::~ReliableConcurrentQueue()
        {
        }
    }
}
