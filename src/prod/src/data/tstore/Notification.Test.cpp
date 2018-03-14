// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define TEST_TAG 'gtST'

inline bool StringEquals(__in KString::SPtr & one, __in KString::SPtr & two)
{
    if (one == nullptr || two == nullptr) 
    {
        return one == two;
    }

    return one->Compare(*two) == 0;
}

namespace TStoreTests
{
    using namespace ktl;

    class NotificationTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, StringStateSerializer>
    {
    public:
        NotificationTest()
        {
            Setup(3);
        }

        ~NotificationTest()
        {
            Cleanup();
        }

        KString::SPtr CreateString(LONG64 seed)
        {
            KString::SPtr key;

            wstring zeroString = wstring(L"0");
            wstring seedString = to_wstring(seed);
            wstring str = wstring(L"test_key_") + seedString;
            auto status = KString::Create(key, GetAllocator(), str.c_str());
            Diagnostics::Validate(status);
            return key;
        }

        TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr CreateTestChangeHandler()
        {
            LongComparer::SPtr longComparerSPtr = nullptr;
            auto status = LongComparer::Create(GetAllocator(), longComparerSPtr);

            KStringComparer::SPtr stringComparerSPtr = nullptr;
            status = KStringComparer::Create(GetAllocator(), stringComparerSPtr);
            
            IComparer<LONG64>::SPtr keyComparerSPtr = static_cast<IComparer<LONG64> *>(longComparerSPtr.RawPtr());
            IComparer<KString::SPtr>::SPtr valueComparerSPtr = static_cast<IComparer<KString::SPtr> *>(stringComparerSPtr.RawPtr());

            TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr handlerSPtr = nullptr;
            status = TestDictionaryChangeHandler<LONG64, KString::SPtr>::Create(*keyComparerSPtr, *valueComparerSPtr, GetAllocator(), handlerSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            return handlerSPtr;
        }

        TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr TrackNotifications(Data::TStore::Store<LONG64, KString::SPtr> & store)
        {
            auto handlerSPtr = CreateTestChangeHandler();

            IDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr dictionaryChangeHandlerSPtr = static_cast<IDictionaryChangeHandler<LONG64, KString::SPtr> *>(handlerSPtr.RawPtr());
            store.DictionaryChangeHandlerSPtr = dictionaryChangeHandlerSPtr;
            store.DictionaryChangeHandlerMask = DictionaryChangeEventMask::Enum::All;

            return handlerSPtr;
        }

        KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr>::SPtr TrackNotificationsOnAllStores()
        {
            KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr>::SPtr handlers = _new(TEST_TAG, GetAllocator()) KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr>();
            for (ULONG32 i = 0; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                auto handler = TrackNotifications(*store);
                handlers->Append(handler);
            }

            return handlers;
        }

        void AddExpectedAddNotification(
            __in KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr> & handlers, 
            __in WriteTransaction<LONG64, KString::SPtr> & transaction,
            __in LONG64 key, 
            __in KString::SPtr value, 
            __in bool secondariesOnly = false)
        {
            // Assumes primary is at 0th index
            ULONG32 startIndex = secondariesOnly ? 1 : 0;

            for (ULONG32 i = startIndex; i < handlers.Count(); i++)
            {
                auto handler = handlers[i];
                handler->AddToExpected(
                    StoreModificationType::Add,
                    key, value,
                    transaction.TransactionSPtr->CommitSequenceNumber);
            }
        }

        void AddExpectedUpdateNotification(
            __in KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr> & handlers, 
            __in WriteTransaction<LONG64, KString::SPtr> & transaction,
            __in LONG64 key, 
            __in KString::SPtr value, 
            __in bool secondariesOnly = false)
        {
            // Assumes primary is at 0th index
            ULONG32 startIndex = secondariesOnly ? 1 : 0;

            for (ULONG32 i = startIndex; i < handlers.Count(); i++)
            {
                auto handler = handlers[i];
                handler->AddToExpected(
                    StoreModificationType::Update,
                    key, value,
                    transaction.TransactionSPtr->CommitSequenceNumber);
            }
        }

        void AddExpectedRemoveNotification(
            __in KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr> & handlers, 
            __in WriteTransaction<LONG64, KString::SPtr> & transaction,
            __in LONG64 key, 
            __in bool secondariesOnly = false)
        {
            // Assumes primary is at 0th index
            ULONG32 startIndex = secondariesOnly ? 1 : 0;

            for (ULONG32 i = startIndex; i < handlers.Count(); i++)
            {
                auto handler = handlers[i];
                handler->AddToExpected(
                    StoreModificationType::Remove,
                    key, nullptr,
                    transaction.TransactionSPtr->CommitSequenceNumber);
            }
        }

        template <typename T>
        void UpdateAllDictionaryChangeHandlerMask(
            __in T mask,
            __in bool secondariesOnly = false)
        {
            // Assumes primary is at 0th index
            ULONG32 startIndex = secondariesOnly ? 1 : 0;

            for (ULONG32 i = startIndex; i < Stores->Count(); i++)
            {
                auto store = (*Stores)[i];
                store->DictionaryChangeHandlerMask = static_cast<DictionaryChangeEventMask::Enum>(mask);
            }
        }

        void ValidateDictionaryChangeHandlers(__in KSharedArray<TestDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr> & handlers)
        {
            for (ULONG32 i = 0; i < handlers.Count(); i++)
            {
                auto handler = handlers[i];
                handler->Validate();
            }
        }
        
        void RecoverWithNotifications(
            __in TestDictionaryChangeHandler<LONG64, KString::SPtr> & handler, 
            __in_opt DictionaryChangeEventMask::Enum mask = DictionaryChangeEventMask::Enum::All)
        {
            IDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr dictionaryRebuildHandler = static_cast<IDictionaryChangeHandler<LONG64, KString::SPtr> *>(&handler);
            CloseAndReOpenStore(dictionaryRebuildHandler, mask);
        }

        void CopyWithNotifications(__in Data::TStore::Store<LONG64, KString::SPtr> & secondaryStore, __in TestDictionaryChangeHandler<LONG64, KString::SPtr> & handler)
        {
            IDictionaryChangeHandler<LONG64, KString::SPtr>::SPtr dictionaryRebuildHandler = static_cast<IDictionaryChangeHandler<LONG64, KString::SPtr> *>(&handler);
            secondaryStore.DictionaryChangeHandlerSPtr = dictionaryRebuildHandler;
            // Checkpoint the Primary, to get all current state
            Checkpoint(*Store);
            CopyCheckpointToSecondary(secondaryStore);
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(NotificationTestSuite, NotificationTest)

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleAdd_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedAddNotification(*handlers, *txn, key, value);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleAdd_Masked_ShouldNotNotify)
    {
        LONG64 key = 6;
        auto value = CreateString(9);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            UpdateAllDictionaryChangeHandlerMask(~DictionaryChangeEventMask::Enum::Add);
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleUpdate_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updateValue = CreateString(14);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedUpdateNotification(*handlers, *txn, key, updateValue);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleUpdate_Masked_ShouldNotNotify)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updateValue = CreateString(14);

        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            UpdateAllDictionaryChangeHandlerMask(~DictionaryChangeEventMask::Enum::Update);
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_UpdateRemove_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updateValue = CreateString(14);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedUpdateNotification(*handlers, *txn, key, updateValue);
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_UpdateRemoveAdd_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updateValue = CreateString(14);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedUpdateNotification(*handlers, *txn, key, updateValue);
            AddExpectedRemoveNotification(*handlers, *txn, key);
            AddExpectedAddNotification(*handlers, *txn, key, value);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_UpdateUpdate_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto update1Value = CreateString(14);
        auto update2Value = CreateString(18);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, update1Value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, update2Value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedUpdateNotification(*handlers, *txn, key, update1Value);
            AddExpectedUpdateNotification(*handlers, *txn, key, update2Value);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleRemove_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleRemove_Masked_ShouldNotNotify)
    {
        LONG64 key = 6;
        auto value = CreateString(9);

        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            UpdateAllDictionaryChangeHandlerMask(~DictionaryChangeEventMask::Enum::Remove);
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemoveAdd_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            
            AddExpectedRemoveNotification(*handlers, *txn, key);
            AddExpectedAddNotification(*handlers, *txn, key, updatedValue);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemoveAdd_SingleTransaction_Masked_ShouldNotifyAddOnly)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();

            UpdateAllDictionaryChangeHandlerMask(~DictionaryChangeEventMask::Enum::Remove);
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedAddNotification(*handlers, *txn, key, updatedValue);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemoveAddUpdate_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            
            AddExpectedRemoveNotification(*handlers, *txn, key);
            AddExpectedAddNotification(*handlers, *txn, key, value);
            AddExpectedUpdateNotification(*handlers, *txn, key, updatedValue);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemoveAddUpdate_SingleTransaction_Masked_ShouldNotNotify)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            
            UpdateAllDictionaryChangeHandlerMask(DictionaryChangeEventMask::Enum::None);

            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemoveAddRemove_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        
        // Add without notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            
            AddExpectedRemoveNotification(*handlers, *txn, key);
            AddExpectedAddNotification(*handlers, *txn, key, value);
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }
        
        ValidateDictionaryChangeHandlers(*handlers);
    }


    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddUpdate_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));

            SyncAwait(txn->CommitAsync());

            AddExpectedAddNotification(*handlers, *txn, key, value);
            AddExpectedUpdateNotification(*handlers, *txn, key, updatedValue);
        }

        ValidateDictionaryChangeHandlers(*handlers);

        VerifyCountInAllStores(1);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddUpdate_MultipleTransactions_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedAddNotification(*handlers, *txn, key, value);
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedUpdateNotification(*handlers, *txn, key, updatedValue);
        }

        ValidateDictionaryChangeHandlers(*handlers);

        VerifyCountInAllStores(1);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddRemove_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();

            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedAddNotification(*handlers, *txn, key, value);
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddRemove_MultipleTransactions_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedAddNotification(*handlers, *txn, key, value);
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddUpdateRemove_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedAddNotification(*handlers, *txn, key, value);
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedUpdateNotification(*handlers, *txn, key, updatedValue);
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            AddExpectedRemoveNotification(*handlers, *txn, key);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddRemoveAdd_SingleTransaction_ShouldSucceed)
    {
        LONG64 key = 6;
        auto value = CreateString(9);
        auto updatedValue = CreateString(15);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());

            AddExpectedAddNotification(*handlers, *txn, key, value);
            AddExpectedRemoveNotification(*handlers, *txn, key);
            AddExpectedAddNotification(*handlers, *txn, key, updatedValue);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressAdd_NoPreviousState_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedAddNotification(*handlers, *txn, key1, value1, true);
            AddExpectedAddNotification(*handlers, *txn, key2, value2, true);

            AddExpectedRemoveNotification(*handlers, *txn, key2, true);
            AddExpectedRemoveNotification(*handlers, *txn, key1, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressAdd_PreviouslyRemoved_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);

        // Setup - Keys should have been removed before false progress adding
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start tracking notifications
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedAddNotification(*handlers, *txn, key1, value1, true);
            AddExpectedAddNotification(*handlers, *txn, key2, value2, true);

            AddExpectedRemoveNotification(*handlers, *txn, key2, true);
            AddExpectedRemoveNotification(*handlers, *txn, key1, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressUpdate_PreviouslyAdded_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);
        auto value1Update = CreateString(10);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);
        auto value2Update = CreateString(6);

        // Setup - Keys should be added before false progress updating
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start tracking notifications
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value1Update, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value2Update, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedUpdateNotification(*handlers, *txn, key1, value1Update, true);
            AddExpectedUpdateNotification(*handlers, *txn, key2, value2Update, true);

            AddExpectedUpdateNotification(*handlers, *txn, key2, value2, true);
            AddExpectedUpdateNotification(*handlers, *txn, key1, value1, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressUpdate_PreviouslyUpdated_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);
        auto value1Update1 = CreateString(10);
        auto value1Update2 = CreateString(11);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);
        auto value2Update1 = CreateString(6);
        auto value2Update2 = CreateString(7);

        // Setup - Keys should be added and updated before false progress updating
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value1Update1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value2Update1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start tracking notifications
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value1Update2, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value2Update2, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedUpdateNotification(*handlers, *txn, key1, value1Update2, true);
            AddExpectedUpdateNotification(*handlers, *txn, key2, value2Update2, true);

            AddExpectedUpdateNotification(*handlers, *txn, key2, value2Update1, true);
            AddExpectedUpdateNotification(*handlers, *txn, key1, value1Update1, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressRemove_PreviouslyAdded_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);

        // Setup - Keys should be added and updated before false progress updating
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start tracking notifications
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key2, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedRemoveNotification(*handlers, *txn, key1, true);
            AddExpectedRemoveNotification(*handlers, *txn, key2, true);

            AddExpectedAddNotification(*handlers, *txn, key2, value2, true);
            AddExpectedAddNotification(*handlers, *txn, key1, value1, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondaries_FalseProgressRemove_PreviouslyUpdated_ShouldSucceed)
    {
        LONG64 key1 = 6;
        auto value1 = CreateString(9);
        auto value1Update = CreateString(10);

        LONG64 key2 = 8;
        auto value2 = CreateString(5);
        auto value2Update = CreateString(6);

        // Setup - Keys should be added and updated before false progress updating
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key1, value1, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key2, value2, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key1, value1Update, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key2, value2Update, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Start tracking notifications
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key2, DefaultTimeout, CancellationToken::None));

            SyncAwait(Replicator->SimulateFalseProgressAsync(*txn->TransactionSPtr));

            SyncAwait(txn->AbortAsync());

            AddExpectedRemoveNotification(*handlers, *txn, key1, true);
            AddExpectedRemoveNotification(*handlers, *txn, key2, true);

            AddExpectedAddNotification(*handlers, *txn, key2, value2Update, true);
            AddExpectedAddNotification(*handlers, *txn, key1, value1Update, true);
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NoOperations_ShouldNotNotify)
    {
        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(txn->AbortAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(ReadStore_ShouldNotNotify)
    {
        auto existingKey = 8;
        auto value = CreateString(5);
        auto nonexistantKey = 12;

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, existingKey, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        SyncAwait(VerifyKeyExistsInStoresAsync(existingKey, nullptr, value, StringEquals));
        SyncAwait(VerifyKeyDoesNotExistInStoresAsync(nonexistantKey));

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(Enumeration_ShouldNotNotify)
    {
        auto key = 8;
        auto value = CreateString(5);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();
        
        {
            auto txn = CreateWriteTransaction();
            txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;
            auto enumeratorSPtr = SyncAwait(Store->CreateEnumeratorAsync(*txn->StoreTransactionSPtr));
            while (SyncAwait(enumeratorSPtr->MoveNextAsync(CancellationToken::None)))
            {

            }
            SyncAwait(txn->AbortAsync());
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddedRebuilt_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);

        // Create the handler, but don't register it with any store
        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            handler->AddToExpectedRebuild(key, value, txn->TransactionSPtr->CommitSequenceNumber);
        }

        Checkpoint();
        
        
        // Handler will verify successful notification
        RecoverWithNotifications(*handler);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == Stores->Count());
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_AddedRebuilt_Masked_ShouldNotNotify)
    {
        auto key = 8;
        auto value = CreateString(5);

        // Create the handler, but don't register it with any store
        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();


        // Handler will verify successful notification
        // Disable rebuild event in notification
        RecoverWithNotifications(*handler, DictionaryChangeEventMask::Enum::None);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == 0);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_UpdatedRebuilt_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);
        auto updateValue = CreateString(3);


        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Create the handler, but don't register it with any store
        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            handler->AddToExpectedRebuild(key, updateValue, txn->TransactionSPtr->CommitSequenceNumber);
        }

        Checkpoint();
        
        
        // Handler will verify successful notification
        RecoverWithNotifications(*handler);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == Stores->Count());
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_SingleKey_RemovedRebuilt_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);
        auto updateValue = CreateString(3);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        // Create the handler, but don't register it with any store
        // No expected keys
        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        Checkpoint();
        
        // Handler will verify successful notification
        RecoverWithNotifications(*handler);
        
        CODING_ERROR_ASSERT(handler->RebuildCallCount() == Stores->Count());
    }

    BOOST_AUTO_TEST_CASE(NotifySecondary_SingleKey_AddCopied_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);
        
        // Create the handler, but don't register it with any store
        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            handler->AddToExpectedRebuild(key, value, txn->TransactionSPtr->CommitSequenceNumber);
        }


        auto secondaryStore = CreateSecondary();
        CopyWithNotifications(*secondaryStore, *handler);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == 1);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondary_SingleKey_UpdateCopied_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);
        auto update = CreateString(3);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handler = CreateTestChangeHandler();

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, update, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
            handler->AddToExpectedRebuild(key, update, txn->TransactionSPtr->CommitSequenceNumber);
        }

        auto secondaryStore = CreateSecondary();
        CopyWithNotifications(*secondaryStore, *handler);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == 1);
    }

    BOOST_AUTO_TEST_CASE(NotifySecondary_SingleKey_RemoveCopied_ShouldSucceed)
    {
        auto key = 8;
        auto value = CreateString(5);

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handler = CreateTestChangeHandler();

        auto secondaryStore = CreateSecondary();
        CopyWithNotifications(*secondaryStore, *handler);

        CODING_ERROR_ASSERT(handler->RebuildCallCount() == 1);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_AddUpdateRemoveMany_Recover_Copy_ShouldSucceed)
    {
        const LONG64 numKeys = 256;

        auto changeHandlersArray = TrackNotificationsOnAllStores();
        auto rebuildHandler = CreateTestChangeHandler();

        // Add all the keys
        for (LONG64 key = 0; key < numKeys; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());

                AddExpectedAddNotification(*changeHandlersArray, *txn, key, CreateString(key));
            }
        }

        // Update all the keys, backwards
        for (LONG64 key = numKeys - 1; key >= 0; key--)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());

                AddExpectedUpdateNotification(*changeHandlersArray, *txn, key, CreateString(key + 1));
            }
        }

        // Remove all the keys
        for (LONG64 key = 0; key < numKeys; key++)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());

                AddExpectedRemoveNotification(*changeHandlersArray, *txn, key);
            }
        }

        // Add all the keys, backwards
        for (LONG64 key = numKeys - 1; key >= 0; key--)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
                AddExpectedAddNotification(*changeHandlersArray, *txn, key, CreateString(key));
                rebuildHandler->AddToExpectedRebuild(key, CreateString(key), txn->TransactionSPtr->CommitSequenceNumber);
            }
        }
        
        ValidateDictionaryChangeHandlers(*changeHandlersArray);

        // Checkpoint, Close, Re-Open, and Recover all the stores 
        Checkpoint();
        RecoverWithNotifications(*rebuildHandler);
        CODING_ERROR_ASSERT(rebuildHandler->RebuildCallCount() == Stores->Count());
        rebuildHandler->ResetRebuildCallCount();
        
        // Create a new secondary, and copy the existing state to it
        auto secondaryStore = CreateSecondary();
        CopyWithNotifications(*secondaryStore, *rebuildHandler);
        CODING_ERROR_ASSERT(rebuildHandler->RebuildCallCount() == 1);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_AddUpdateRemoveRepeated_Recover_Copy_ShouldSucceed)
    {
        const ULONG32 numIterations = 16;
        const LONG64 keysPerIteration = 32;

        auto changeHandlersArray = TrackNotificationsOnAllStores();
        auto rebuildHandler = CreateTestChangeHandler();
        
        LONG64 currentKey = -1;
        for (ULONG32 i = 0; i < numIterations; i++)
        {
            for (ULONG32 k = 0; k < keysPerIteration; k++)
            {
                currentKey++;

                // Add the key
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, currentKey, CreateString(currentKey), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());

                    AddExpectedAddNotification(*changeHandlersArray, *txn, currentKey, CreateString(currentKey));
                }

                // Update the key
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, currentKey, CreateString(currentKey + 1), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());

                    AddExpectedUpdateNotification(*changeHandlersArray, *txn, currentKey, CreateString(currentKey + 1));
                }

                // Remove the key
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, currentKey, DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());

                    AddExpectedRemoveNotification(*changeHandlersArray, *txn, currentKey);
                }

                // Add the key
                {
                    auto txn = CreateWriteTransaction();
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, currentKey, CreateString(currentKey), DefaultTimeout, CancellationToken::None));
                    SyncAwait(txn->CommitAsync());

                    AddExpectedAddNotification(*changeHandlersArray, *txn, currentKey, CreateString(currentKey));
                }
            }
        }

        // Update all the keys, backwards
        for (LONG64 key = currentKey; key >= 0; key--)
        {
            {
                auto txn = CreateWriteTransaction();
                SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key + 1), DefaultTimeout, CancellationToken::None));
                SyncAwait(txn->CommitAsync());
                AddExpectedUpdateNotification(*changeHandlersArray, *txn, key, CreateString(key + 1));
                rebuildHandler->AddToExpectedRebuild(key, CreateString(key + 1), txn->TransactionSPtr->CommitSequenceNumber);
            }
        }

        ValidateDictionaryChangeHandlers(*changeHandlersArray);

        // Checkpoint, Close, Re-Open, and Recover all the stores 
        Checkpoint();
        RecoverWithNotifications(*rebuildHandler);
        CODING_ERROR_ASSERT(rebuildHandler->RebuildCallCount() == Stores->Count());
        rebuildHandler->ResetRebuildCallCount();
        
        // Create a new secondary, and copy the existing state to it
        auto secondaryStore = CreateSecondary();
        CopyWithNotifications(*secondaryStore, *rebuildHandler);
        CODING_ERROR_ASSERT(rebuildHandler->RebuildCallCount() == 1);
    }

    BOOST_AUTO_TEST_CASE(CRUD_FiresNotifications_ShouldSucceed)
    {
        const LONG64 keyCount = 100;

        auto changeHandlersArray = TrackNotificationsOnAllStores();

        // Add some keys first, so we can operate on them after tracking notifications
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(txn->CommitAsync());

            for (LONG64 key = 0; key < keyCount; key++)
            {
                AddExpectedAddNotification(*changeHandlersArray, *txn, key, CreateString(key));
            }
        }

        // Modify the keys
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                    break;
                case 1:
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                default:
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, -key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                }
            }

            SyncAwait(txn->CommitAsync());

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    AddExpectedRemoveNotification(*changeHandlersArray, *txn, key);
                    break;
                case 1:
                    AddExpectedUpdateNotification(*changeHandlersArray, *txn, key, CreateString(key));
                    break;
                default:
                    AddExpectedAddNotification(*changeHandlersArray, *txn, -key, CreateString(key));
                    break;
                }
            }
        }

        ValidateDictionaryChangeHandlers(*changeHandlersArray);
    }

    BOOST_AUTO_TEST_CASE(CRUD_FiresNotifications_Recovery_WithoutCheckpoint_ShouldSucceed)
    {
        const LONG64 keyCount = 100;

        auto changeHandlersArray = TrackNotificationsOnAllStores();
        auto rebuildHandler = CreateTestChangeHandler();

        // Add some keys first, so we can operate on them after tracking notifications
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(txn->CommitAsync());

            for (LONG64 key = 0; key < keyCount; key++)
            {
                AddExpectedAddNotification(*changeHandlersArray, *txn, key, CreateString(key));
            }
        }

        // Modify the keys
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                    break;
                case 1:
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                default:
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, -key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                }
            }

            SyncAwait(txn->CommitAsync());

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    AddExpectedRemoveNotification(*changeHandlersArray, *txn, key);
                    break;
                case 1:
                    AddExpectedUpdateNotification(*changeHandlersArray, *txn, key, CreateString(key));
                    break;
                default:
                    AddExpectedAddNotification(*changeHandlersArray, *txn, -key, CreateString(key));
                    break;
                }
            }
        }

        ValidateDictionaryChangeHandlers(*changeHandlersArray);
        
        // Since there is no checkpoint, the recovered state will empty
        RecoverWithNotifications(*rebuildHandler);
    }

    BOOST_AUTO_TEST_CASE(CRUD_FiresNotifications_Recovery_WithCheckpoint_ShouldSucceed)
    {
        const LONG64 keyCount = 100;

        auto changeHandlersArray = TrackNotificationsOnAllStores();
        auto rebuildHandler = CreateTestChangeHandler();

        LONG64 initialSequenceNumber = 0;

        // Add some keys first, so we can operate on them after tracking notifications
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(txn->CommitAsync());

            initialSequenceNumber = txn->TransactionSPtr->CommitSequenceNumber;

            for (LONG64 key = 0; key < keyCount; key++)
            {
                AddExpectedAddNotification(*changeHandlersArray, *txn, key, CreateString(key));
            }
        }

        // Modify the keys
        {
            auto txn = CreateWriteTransaction();

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    SyncAwait(Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None));
                    break;
                case 1:
                    SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                default:
                    SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, -key, CreateString(key), DefaultTimeout, CancellationToken::None));
                    break;
                }
            }

            SyncAwait(txn->CommitAsync());

            for (LONG64 key = 0; key < keyCount; key++)
            {
                switch (key % 3)
                {
                case 0:
                    AddExpectedRemoveNotification(*changeHandlersArray, *txn, key);
                    break;
                case 1:
                    AddExpectedUpdateNotification(*changeHandlersArray, *txn, key, CreateString(key));
                    rebuildHandler->AddToExpectedRebuild(key, CreateString(key), txn->TransactionSPtr->CommitSequenceNumber);
                    break;
                default:
                    AddExpectedAddNotification(*changeHandlersArray, *txn, -key, CreateString(key));
                    rebuildHandler->AddToExpectedRebuild(key, CreateString(key), initialSequenceNumber);
                    rebuildHandler->AddToExpectedRebuild(-key, CreateString(key), txn->TransactionSPtr->CommitSequenceNumber);
                    break;
                }
            }
        }

        ValidateDictionaryChangeHandlers(*changeHandlersArray);
        
        Checkpoint();
        RecoverWithNotifications(*rebuildHandler);
    }

    BOOST_AUTO_TEST_CASE(NotifyAll_UpdateMultiple_NotificationsFireInLockOrder_ShouldSucceed)
    {
        // Add some keys first, so we can operate on them after tracking notifications
        {
            auto txn = CreateWriteTransaction();
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, 1, CreateString(1), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, 2, CreateString(2), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->AddAsync(*txn->StoreTransactionSPtr, 3, CreateString(3), DefaultTimeout, CancellationToken::None));
            SyncAwait(txn->CommitAsync());
        }

        auto handlers = TrackNotificationsOnAllStores();

        {
            auto txn = CreateWriteTransaction();

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, 1, CreateString(11), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, 2, CreateString(22), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, 3, CreateString(33), DefaultTimeout, CancellationToken::None));

            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, 2, CreateString(222), DefaultTimeout, CancellationToken::None));
            SyncAwait(Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, 1, CreateString(111), DefaultTimeout, CancellationToken::None));

            SyncAwait(txn->CommitAsync());

            AddExpectedUpdateNotification(*handlers, *txn, 1, CreateString(11));
            AddExpectedUpdateNotification(*handlers, *txn, 2, CreateString(22));
            AddExpectedUpdateNotification(*handlers, *txn, 3, CreateString(33));
            AddExpectedUpdateNotification(*handlers, *txn, 2, CreateString(222));
            AddExpectedUpdateNotification(*handlers, *txn, 1, CreateString(111));
        }

        ValidateDictionaryChangeHandlers(*handlers);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
