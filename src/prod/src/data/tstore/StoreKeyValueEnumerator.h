// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define COMPONENTKEYENUMERATOR_TAG 'eaCK'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class StoreKeyValueEnumerator
            : public KObject<StoreKeyValueEnumerator<TKey, TValue>>
            , public KShared<StoreKeyValueEnumerator<TKey, TValue>>
            , public IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>
        {
            K_FORCE_SHARED(StoreKeyValueEnumerator)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator)

        public:
            static NTSTATUS Create(
                __in IStore<TKey, TValue> & store,
                __in IComparer<TKey> & keyComparer,
                __in IEnumerator<TKey> & keys,
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in ReadMode readMode,
                __in KAllocator & allocator,
                __out KSharedPtr<IAsyncEnumerator<KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>>> & result)
            {
                result = _new(COMPONENTKEYENUMERATOR_TAG, allocator) StoreKeyValueEnumerator(store, keyComparer, keys, storeTransaction, readMode);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> GetCurrent() override
            {
                if (isDone_)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                return current_;
            }

            ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) override
            {
                if (isDone_)
                {
                    co_return false;
                }
                
                while (keysEnumeratorSPtr_->MoveNext())
                {
                    TKey key = keysEnumeratorSPtr_->Current();
                    
                    // TODO:
                    // Since the output sequence should not have duplicate keys, we check for them here
                    // Ideally, there should be no duplicates in the input
                    if (isPreviousSet_ && keyComparerSPtr_->Compare(key, previousKey_) == 0)
                    {
                        continue;
                    }

                    KSharedPtr<Store<TKey, TValue>> storeSPtr = static_cast<Store<TKey, TValue> *>(storeSPtr_.RawPtr());

                    KeyValuePair<LONG64, TValue> kvpair;
                    bool exists = co_await storeSPtr->TryGetValueAsync(
                        *storeTransactionSPtr_, 
                        key, 
                        Common::TimeSpan::FromSeconds(Constants::EnumerationGetValueTimeoutSeconds), 
                        kvpair, 
                        readMode_,
                        ktl::CancellationToken::None);

                    if (exists)
                    {
                        current_ = KeyValuePair<TKey, KeyValuePair<LONG64, TValue>>(key, kvpair);
                        co_return true;
                    }
                    
                  cancellationToken.ThrowIfCancellationRequested();
                }

                isDone_ = true;
                co_return false;
            }

            void Reset() override
            {
                // TODO: IEnumerator also needs a Reset() method
                throw ktl::Exception(STATUS_NOT_IMPLEMENTED);
            }

            void Dispose() override
            {
                Close();
            }

            void Close()
            {
                keysEnumeratorSPtr_ = nullptr;
                storeTransactionSPtr_ = nullptr;
                storeSPtr_ = nullptr;
            }

        private:
            StoreKeyValueEnumerator(
                __in IStore<TKey, TValue> & store,
                __in IComparer<TKey> & keyComparer,
                __in IEnumerator<TKey> & keys,
                __in IStoreTransaction<TKey, TValue> & storeTransaction,
                __in ReadMode readMode) :
                keysEnumeratorSPtr_(&keys),
                storeTransactionSPtr_(&storeTransaction),
                storeSPtr_(&store),
                keyComparerSPtr_(&keyComparer),
                readMode_(readMode)
            {
            }

            KSharedPtr<IEnumerator<TKey>> keysEnumeratorSPtr_;
            KSharedPtr<IStoreTransaction<TKey, TValue>> storeTransactionSPtr_;
            KSharedPtr<IStore<TKey, TValue>> storeSPtr_;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
            ReadMode readMode_;

            bool isDone_ = false;
            KeyValuePair<TKey, KeyValuePair<LONG64, TValue>> current_;
            
            bool isPreviousSet_ = false;
            TKey previousKey_;
        };

        template<typename TKey, typename TValue>
        StoreKeyValueEnumerator<TKey, TValue>::~StoreKeyValueEnumerator()
        {
        }
    }
}
