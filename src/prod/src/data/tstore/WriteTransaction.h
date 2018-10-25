// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TStoreTests
{
   using namespace ktl;
   using namespace Data::TStore;
   using namespace TxnReplicator;

   template<typename TKey, typename TValue>
   class WriteTransaction :
      public KObject<WriteTransaction<TKey, TValue>>,
      public KShared<WriteTransaction<TKey, TValue>>
   {
      K_FORCE_SHARED(WriteTransaction)

   public:
      __declspec(property(get = get_TransactionSPtr)) TxnReplicator::Transaction::SPtr TransactionSPtr;
      TxnReplicator::Transaction::SPtr get_TransactionSPtr() const
      {
         return transactionSPtr_;
      }

      __declspec(property(get = get_StoreTransactionSPtr)) KSharedPtr<Data::TStore::StoreTransaction<TKey, TValue>>  StoreTransactionSPtr;
      KSharedPtr<Data::TStore::StoreTransaction<TKey, TValue>>  get_StoreTransactionSPtr() const
      {
         return storeTransactionSPtr_;
      }

      __declspec(property(get = get_StateProviderSPtr))  KSharedPtr<Data::TStore::Store<TKey, TValue>>  StateProviderSPtr;
      KSharedPtr<Data::TStore::Store<TKey, TValue>>  get_StateProviderSPtr() const
      {
         return stateProviderSPtr_;
      }

      static NTSTATUS
         Create(
            __in Data::TStore::Store<TKey, TValue>& stateProvider,
            __in TxnReplicator::Transaction& transaction,
            __in Data::TStore::StoreTransaction<TKey, TValue> const & storeTransaction,
            __in KAllocator& allocator,
            __out SPtr& result)
      {
         NTSTATUS status;

         SPtr output = _new(STORE_TAG, allocator) WriteTransaction(stateProvider, transaction, storeTransaction);

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

      ktl::Awaitable<LONG64> CommitAsync()
      {
         NTSTATUS status = co_await transactionSPtr_->CommitAsync();
         Diagnostics::Validate(status);

         co_return transactionSPtr_->CommitSequenceNumber;
      }

      ktl::Awaitable<LONG64> AbortAsync()
      {
         co_await suspend_never();
         NTSTATUS status = transactionSPtr_->Abort();
         Diagnostics::Validate(status);
         co_return 0;
      }

      void Dispose()
      {
          transactionSPtr_->Dispose();
      }

   private:
      WriteTransaction(
         __in Data::TStore::Store<TKey, TValue>& stateProvider,
         __in TxnReplicator::Transaction& transaction,
         __in Data::TStore::StoreTransaction<TKey, TValue> const & storeTransaction);

      // Hardcoded for tstore now
      KSharedPtr<Data::TStore::Store<TKey, TValue>> stateProviderSPtr_;
      TxnReplicator::Transaction::SPtr transactionSPtr_;
      KSharedPtr<Data::TStore::StoreTransaction<TKey, TValue>> storeTransactionSPtr_;
   };

   template <typename TKey, typename TValue>
   WriteTransaction<TKey, TValue>::WriteTransaction(
      __in Data::TStore::Store<TKey, TValue>& stateProvider,
      __in TxnReplicator::Transaction& transaction,
      __in Data::TStore::StoreTransaction<TKey, TValue> const & storeTransaction)
      :stateProviderSPtr_(&stateProvider),
      transactionSPtr_(&transaction)
   {
      KSharedPtr<Data::TStore::StoreTransaction<TKey, TValue> const> storeTransactionCSPtr = &storeTransaction;
      storeTransactionSPtr_ = (const_cast<Data::TStore::StoreTransaction<TKey, TValue>*>(storeTransactionCSPtr.RawPtr()));
   }

   template <typename TKey, typename TValue>
   WriteTransaction<TKey, TValue>::~WriteTransaction()
   {
        transactionSPtr_->Dispose();
   }
}
