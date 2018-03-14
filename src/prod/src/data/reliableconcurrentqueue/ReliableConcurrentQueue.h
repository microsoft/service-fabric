// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../tstore/TestStateSerializer.h"

namespace Data
{
    using namespace TStore;

    namespace Collections
    {
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
                __in HashFunctionType func,
                __in KUriView & name,
                __in FABRIC_STATE_PROVIDER_ID stateProviderId,
                __in Data::StateManager::IStateSerializer<TValue>& valueStateSerializer,
                __out SPtr & result);

            ktl::Awaitable<void> EnqueueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __in TValue value,
                __in Common::TimeSpan timeout,
                __in ktl::CancellationToken const & cancellationToken) override;

            ktl::Awaitable<NTSTATUS> TryDequeueAsync(
                __in TxnReplicator::TransactionBase& replicatorTransaction,
                __out TValue& value,
                __in ktl::CancellationToken const & cancellationToken) override;

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
            LONG64 id_;
            KSharedPtr<ConcurrentDictionary2<LONG64, KSharedPtr<IEnumerator<LONG64>>>> enumeratorDictionarySPtr_;

        private:
            LONG64 GetNextId();
        };

        template <typename TValue>
        NTSTATUS ReliableConcurrentQueue<TValue>::Create(
            __in KAllocator& allocator,
            __in PartitionedReplicaId const & traceId,
            __in HashFunctionType func,
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

            SPtr output = _new(TEST_TAG, allocator) ReliableConcurrentQueue(traceId, *keyComparerSPtr, func, name, stateProviderId, *keySerializerSPtr, valueStateSerializer);

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
            KSharedPtr<IStoreTransaction<LONG64, TValue>> storeTransaction = nullptr;
            this->CreateOrFindTransaction(replicatorTransaction, storeTransaction);

            // todo : implement, get largest key after recovery of store.
            // todo : ChangeRole->P, get largest key in Store.
            LONG64 id = GetNextId();

            co_await this->AddAsync(*storeTransaction, id, value, timeout, cancellationToken);
        }

        template <typename TValue>
        ktl::Awaitable<NTSTATUS> ReliableConcurrentQueue<TValue>::TryDequeueAsync(
            __in TxnReplicator::TransactionBase& replicatorTransaction,
            __out TValue& value,
            __in ktl::CancellationToken const & cancellationToken)
        {
            // Using timeout as a local variable for now. Will change the implementation to a blocking queue as required.
            // Currently the timeout is 0.
            auto timeout = Common::TimeSpan::FromSeconds(0.0f);

            KSharedPtr<IStoreTransaction<LONG64, TValue>> storeTransaction = nullptr;
            this->CreateOrFindTransaction(replicatorTransaction, storeTransaction);
            
            KSharedPtr<IEnumerator<LONG64>> enumerator = nullptr;
            
#ifdef QUEUE_CACHE_ENUMERATOR
            // Disabled due to leaking enumerators

            // Try and get a cached enumerator.
            bool gotCachedEnumerator = enumeratorDictionarySPtr_->TryGetValue(replicatorTransaction.TransactionId, enumerator);

            if (!gotCachedEnumerator)
            {
#endif
                // If no cached enumerator exists, create one and cache it. 
                // todo : Remove the enumerator once we see unlock for the transaction
                storeTransaction->set_ReadIsolationLevel(StoreTransactionReadIsolationLevel::Snapshot);
                enumerator = co_await this->CreateKeyEnumeratorAsync(*storeTransaction);

#ifdef QUEUE_CACHE_ENUMERATOR
                // Disabled due to leaking enumerators
                enumeratorDictionarySPtr_->TryAdd(replicatorTransaction.TransactionId, enumerator);
            }
#endif

            do
            {
                if (enumerator->MoveNext())
                {
                    LONG64 current = enumerator->Current();

                    // ConditionalGetAsync 
                    KeyValuePair<LONG64, TValue> result;
                    bool gotValue = co_await this->ConditionalGetAsync(*storeTransaction, current, timeout, result, cancellationToken);

                    // todo : Add assert that the value version is always lowest.

                    if (gotValue)
                    {
                        // If we can get the value for the key, then remove the value from the store.
                        bool removed = co_await this->ConditionalRemoveAsync(*storeTransaction, current, timeout, cancellationToken);

                        if (removed)
                        {
                            // todo enumerator->Dispose();
                            value = result.Value;

                            // return success
                            co_return STATUS_SUCCESS;
                        }
                        else
                        {
                            co_return STATUS_UNSUCCESSFUL;
                            // todo : Some other transaction might have removed the value. We cant assert
                        }
                    }
                }
#ifdef QUEUE_CACHE_ENUMERATOR
                else if (gotCachedEnumerator)
                {
                    // If we got a cached enumerator, create another enumerator and try to dequeue an item.
                    // If we cant get an element in the second attempt too, then return false
                    gotCachedEnumerator = false;

                    enumeratorDictionarySPtr_->Remove(replicatorTransaction.TransactionId);
                    enumerator = co_await this->CreateKeyEnumeratorAsync(*storeTransaction);
                    enumeratorDictionarySPtr_->TryAdd(replicatorTransaction.TransactionId, enumerator);
                }
#endif
                else
                {
                    // nothing to do
                    co_return STATUS_UNSUCCESSFUL;
                }
            } while (true);

            // todo : Add assert => Code should be unreachable

            // if the code reaches here, then none of the keys could be dequeued. Return failure code.
            co_return STATUS_UNSUCCESSFUL;
        }

#pragma endregion IReliableConcurrentQueue implementation

        template <typename TValue>
        LONG64 ReliableConcurrentQueue<TValue>::GetNextId()
        {
            return InterlockedIncrement64(&id_);
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
            enumeratorDictionarySPtr_ = nullptr;

            // todo : fix the first argument for concurrent dictionary (size)
            NTSTATUS status = ConcurrentDictionary2<LONG64, KSharedPtr<IEnumerator<LONG64>>>::Create(10000, func, keyComparer, this->GetThisAllocator(), enumeratorDictionarySPtr_);
            Diagnostics::Validate(status);
        }

        template <typename TValue>
        ReliableConcurrentQueue<TValue>::~ReliableConcurrentQueue()
        {
        }
    }
}
