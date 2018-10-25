// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'tsTP'

namespace TStoreTests
{
    using namespace ktl;

    class StoreTest1Replica : public TStoreTestBase<int, int, IntComparer, TestStateSerializer<int>, TestStateSerializer<int>>
    {
    public:
        StoreTest1Replica()
        {
            Setup();
        }

        ~StoreTest1Replica()
        {
            Cleanup();
        }

        Awaitable<void> Add_SingleTransaction_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, Common::TimeSpan::MaxValue, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value);
                co_await tx->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
        }

        Awaitable<void> AddUpdate_SingleTransaction_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;
            int updatedValue = 7;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, updatedValue);
                co_await tx->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);
        }

        Awaitable<void> AddDelete_SingleTransaction_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> AddDeleteAdd_SingleTransaction_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value);
                co_await tx->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
        }

        Awaitable<void> NoAdd_Delete_ShouldFail_Test()
        {
            int key = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                co_await tx->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> AddDeleteUpdate_SingleTransaction_ShouldFail_Test()
        {
            int key = 5;
            int value = 6;
            int updateValue = 7;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                bool result_remove = co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                VERIFY_ARE_EQUAL(result_remove, true);
                co_await VerifyKeyDoesNotExistAsync(*Store, *tx->StoreTransactionSPtr, key);
                bool result_update = co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                VERIFY_ARE_EQUAL(result_update, false);
                co_await tx->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> AddAdd_SingleTransaction_ShouldFail_Test()
        {
            bool secondAddFailed = false;

            int key = 5;
            int value = 6;

            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);

            try
            {
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
            }
            catch (ktl::Exception &)
            {
                // todo once status codes are fixed, fix this as well
                secondAddFailed = true;
            }

            CODING_ERROR_ASSERT(secondAddFailed == true);
            co_await tx->AbortAsync();

            KBufferSerializer::SPtr serializer1 = nullptr;
            KBufferSerializer::Create(GetAllocator(), serializer1);

            StringStateSerializer::SPtr serializer2 = nullptr;
            StringStateSerializer::Create(GetAllocator(), serializer2);
        }

        Awaitable<void> MultipleAdds_SingleTransaction_ShouldSucceed_Test()
        {
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();

                for (int i = 0; i < 10; i++)
                {
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, i, i, DefaultTimeout, CancellationToken::None);
                }

                co_await tx->CommitAsync();
            }

            for (int i = 0; i < 10; i++)
            {
                int expectedValue = i;
                co_await VerifyKeyExistsAsync(*Store, i, -1, expectedValue);
            }
        }

        Awaitable<void> MultipleUpdates_SingleTransaction_ShouldSucceed_Test()
        {
            int key = 5;
            int value = -1;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();

                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);

                for (int i = 0; i < 10; i++)
                {
                    co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, i, DefaultTimeout, CancellationToken::None);
                }

                co_await tx->CommitAsync();
            }

            int expectedValue = 9;
            co_await VerifyKeyExistsAsync(*Store, key, -1, expectedValue);
        }

        Awaitable<void> AddUpdate_DifferentTransactions_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 5;
            int updateValue = 6;

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, updateValue);
        }

        Awaitable<void> AddDelete_DifferentTransactions_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                bool result = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result == true);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> AddUpdateRead_DifferentTransactions_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 5;
            int updateValue = 7;

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                bool res = co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(res == true);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, updateValue);
        }

        Awaitable<void> AddDeleteUpdate_DifferentTransactions_ShouldFail_Test()
        {
            int key = 5;
            int value = 5;
            int updateValue = 7;

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await VerifyKeyExistsAsync(*Store, *tx1->StoreTransactionSPtr, key, -1, value);
                co_await tx1->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                bool result = co_await Store->ConditionalRemoveAsync(*tx2->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(result == true);
                co_await tx2->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
                bool res = co_await Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, key, updateValue, DefaultTimeout, CancellationToken::None);
                CODING_ERROR_ASSERT(res == false);
                co_await tx3->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> MultipleAdds_MultipleTransactions_ShouldSucceed_Test()
        {
            for (int i = 0; i < 10; i++)
            {
                int key = i;
                int value = i;

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await tx->CommitAsync();
                }
            }

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 10; i++)
                {
                    co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, i, -1, i);
                }

                co_await tx->AbortAsync();
            }
        }

        Awaitable<void> MultipleAddsUpdates_MultipleTransactions_ShouldSucceed_Test()
        {
            for (int i = 0; i < 10; i++)
            {
                int key = i;
                int value = i;

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await tx->CommitAsync();
                }
            }

            for (int i = 0; i < 10; i++)
            {
                int key = i;
                int value = i + 10;

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                    co_await tx->CommitAsync();
                }
            }

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                for (int i = 0; i < 10; i++)
                {
                    // Verify updated value
                    co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, i, -1, i + 10);
                }

                co_await tx->AbortAsync();
            }
        }

        Awaitable<void> Add_Abort_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                KeyValuePair<LONG64, int> kvpair(-1, -1);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, 5, DefaultTimeout, kvpair, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair.Value == value);
                CODING_ERROR_ASSERT(kvpair.Key == 0);
                co_await tx->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> AddAdd_SameKeyOnConcurrentTransaction_ShouldTimeout_Test()
        {
            bool hasFailed = false;

            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);

                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                try
                {
                    co_await Store->AddAsync(*tx2->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                }
                catch (ktl::Exception & e)
                {
                    hasFailed = true;
                    CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                }

                CODING_ERROR_ASSERT(hasFailed);

                co_await tx2->AbortAsync();
                co_await tx1->AbortAsync();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> UpdateRead_SameKeyOnConcurrentTransaction_ShouldTimeout_Test()
        {
            bool hasFailed = false;

            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, key, 8, DefaultTimeout, CancellationToken::None);

                {
                    WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                    tx2->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
                    try
                    {
                        co_await VerifyKeyExistsAsync(*Store, *tx2->StoreTransactionSPtr, key, -1, 8);
                    }
                    catch (ktl::Exception & e)
                    {
                        hasFailed = true;
                        CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                    }

                    CODING_ERROR_ASSERT(hasFailed);

                    co_await tx2->AbortAsync();
                    co_await tx1->AbortAsync();
                }
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
        }

        Awaitable<void> WriteStore_NotPrimary_ShouldFail_Test()
        {
            Replicator->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
            bool hasFailed = false;

            int key = 5;
            int value = 6;

            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            try
            {
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
            }
            catch (ktl::Exception & e)
            {
                hasFailed = true;
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_PRIMARY);
            }

            CODING_ERROR_ASSERT(hasFailed);
            co_await tx->AbortAsync();

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> ReadStore_NotReadable_ShouldFail_Test()
        {
            bool hasFailed = false;

            int key = 5;
            int value = 6;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            Replicator->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            try
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, key, -1, value);
            }
            catch (ktl::Exception & e)
            {
                hasFailed = true;
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_NOT_READABLE);
            }
            co_await tx->AbortAsync();
            CODING_ERROR_ASSERT(hasFailed);
        }

        Awaitable<void> Add_ConditionalGet_ShouldSucceed_Test()
        {
            int key = 5;
            int key2 = 6;
            int value = 6;
            int updatedValue = 8;
            LONG64 lsn = -1;

            // Add
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                lsn = co_await tx->CommitAsync();
            }

            // Get
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                KeyValuePair<LONG64, int> kvpair(-1, -1);
                KeyValuePair<LONG64, int> kvpair2(-1, -1);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair.Key == lsn);
                CODING_ERROR_ASSERT(kvpair.Value == value);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key2, DefaultTimeout, kvpair2, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair2.Key == lsn);
                CODING_ERROR_ASSERT(kvpair2.Value == value);
                tx->StoreTransactionSPtr->Unlock();
            }

            // Update
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                bool update = co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None, lsn);
                CODING_ERROR_ASSERT(update);
                lsn = co_await tx->CommitAsync();
            }

            // Get
            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                KeyValuePair<LONG64, int> kvpair(-1, -1);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair.Key == lsn);
                CODING_ERROR_ASSERT(kvpair.Value == updatedValue);
                tx->StoreTransactionSPtr->Unlock();
            }
        }

        Awaitable<void> Add_ConditionalUpdate_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;
            LONG64 lsn = -1;
            int updatedValue = 8;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                lsn = co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                bool update = co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None, 1);
                CODING_ERROR_ASSERT(update == false);
                co_await tx->AbortAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                bool update = co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, updatedValue, DefaultTimeout, CancellationToken::None, lsn);
                CODING_ERROR_ASSERT(update);
                co_await tx->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, updatedValue);
        }

        Awaitable<void> Add_ConditionalDelete_ShouldSucceed_Test()
        {
            int key = 5;
            int value = 6;
            LONG64 lsn = -1;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                lsn = co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                bool remove = co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None, 1);
                CODING_ERROR_ASSERT(remove == false);
                co_await tx->AbortAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);

        {
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            bool remove = co_await Store->ConditionalRemoveAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, CancellationToken::None, lsn);
            CODING_ERROR_ASSERT(remove);
            co_await tx->CommitAsync();
        }

        co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> Read_WithUpdateLock_ShouldSucceed_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                tx->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                KeyValuePair<LONG64, int> kvpair(-1, -1);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair.Value == value);
            }
        }

        Awaitable<void> Read_WithUpdateLock_WithExistingReader_ShouldSucceed_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr existingReader = CreateWriteTransaction();
                KeyValuePair<LONG64, int> kvpair1(-1, -1);
                co_await Store->ConditionalGetAsync(*existingReader->StoreTransactionSPtr, key, DefaultTimeout, kvpair1, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair1.Value == value);

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    tx->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                    KeyValuePair<LONG64, int> kvpair2(-1, -1);
                    co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair2, CancellationToken::None);
                    CODING_ERROR_ASSERT(kvpair2.Value == value);
                }
            }
        }

        Awaitable<void> Read_WithExistingUpdater_ShouldTimeout_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr existingUpdater = CreateWriteTransaction();
                existingUpdater->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                KeyValuePair<LONG64, int> kvpair1(-1, -1);
                co_await Store->ConditionalGetAsync(*existingUpdater->StoreTransactionSPtr, key, DefaultTimeout, kvpair1, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair1.Value == value);

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();

                    try
                    {
                        KeyValuePair<LONG64, int> kvpair2(-1, -1);
                        co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair2, CancellationToken::None);
                        CODING_ERROR_ASSERT(false); // Should not get here
                    }
                    catch (ktl::Exception const & e)
                    {
                        CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                    }
                }
            }
        }

        Awaitable<void> Read_WithUpdateLock_WithExistingUpdater_ShouldTimeout_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr existingUpdater = CreateWriteTransaction();
                existingUpdater->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                KeyValuePair<LONG64, int> kvpair1(-1, -1);
                co_await Store->ConditionalGetAsync(*existingUpdater->StoreTransactionSPtr, key, DefaultTimeout, kvpair1, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair1.Value == value);

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                    tx->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                    try
                    {
                        KeyValuePair<LONG64, int> kvpair2(-1, -1);
                        co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair2, CancellationToken::None);
                        CODING_ERROR_ASSERT(false); // Should not get here
                    }
                    catch (ktl::Exception const & e)
                    {
                        CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                    }
                }
            }
        }

        Awaitable<void> Write_WithExistingUpdater_ShouldTimeout_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            {
                WriteTransaction<int, int>::SPtr existingUpdater = CreateWriteTransaction();
                existingUpdater->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                KeyValuePair<LONG64, int> kvpair1(-1, -1);
                co_await Store->ConditionalGetAsync(*existingUpdater->StoreTransactionSPtr, key, DefaultTimeout, kvpair1, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair1.Value == value);

                {
                    WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();

                    try
                    {
                        co_await Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value + 1, DefaultTimeout, CancellationToken::None);
                        CODING_ERROR_ASSERT(false); // Should not get here
                    }
                    catch (ktl::Exception const & e)
                    {
                        CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                    }
                }
            }
        }

        Awaitable<void> Write_AsUpdater_WithExistingReaders_ShouldWait_Test()
        {
            int key = 17;
            int value = 5;

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                co_await Store->AddAsync(*tx->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx->CommitAsync();
            }

            WriteTransaction<int, int>::SPtr existingReader = CreateWriteTransaction();

            KeyValuePair<LONG64, int> kvpair1(-1, -1);
            co_await Store->ConditionalGetAsync(*existingReader->StoreTransactionSPtr, key, DefaultTimeout, kvpair1, CancellationToken::None);
            CODING_ERROR_ASSERT(kvpair1.Value == value);

            {
                WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
                tx->StoreTransactionSPtr->LockingHints = LockingHints::Enum::UpdateLock;

                KeyValuePair<LONG64, int> kvpair2(-1, -1);
                co_await Store->ConditionalGetAsync(*tx->StoreTransactionSPtr, key, DefaultTimeout, kvpair2, CancellationToken::None);
                CODING_ERROR_ASSERT(kvpair2.Value == value);

                auto timedOutWriteTask = Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value + 1, DefaultTimeout, CancellationToken::None);
                try
                {
                    co_await timedOutWriteTask;
                    CODING_ERROR_ASSERT(false);
                }
                catch (ktl::Exception const & e)
                {
                    CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TIMEOUT);
                }

                auto writeTask = Store->ConditionalUpdateAsync(*tx->StoreTransactionSPtr, key, value + 1, DefaultTimeout, CancellationToken::None);

                existingReader = nullptr; // Dispose reader allowing writer to go through

                co_await writeTask;
                CODING_ERROR_ASSERT(writeTask.IsComplete() == true);
            }
        }

        Awaitable<void> Add_WithCancelledToken_ShouldNotAdd_Test()
        {
            int key = 17;
            int value = 5;

            CancellationTokenSource::SPtr tokenSource = nullptr;
            NTSTATUS status = CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSource);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            tokenSource->Cancel();

            try
            {
                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, tokenSource->Token);
                    CODING_ERROR_ASSERT(false); // Should not reach here
                }
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == STATUS_CANCELLED);
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
        }

        Awaitable<void> Update_WithCancelledToken_ShouldNotUpdate_Test()
        {
            int key = 17;
            int value = 5;
            int updateValue = 7;

            CancellationTokenSource::SPtr tokenSource = nullptr;
            NTSTATUS status = CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSource);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, tokenSource->Token);
                co_await txn->CommitAsync();
            }

            try
            {
                tokenSource->Cancel();

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*txn->StoreTransactionSPtr, key, updateValue, DefaultTimeout, tokenSource->Token);
                    CODING_ERROR_ASSERT(false); // Should not reach here
                }
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == STATUS_CANCELLED);
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
        }

        Awaitable<void> Remove_WithCancelledToken_ShouldNotRemove_Test()
        {
            int key = 17;
            int value = 5;

            CancellationTokenSource::SPtr tokenSource = nullptr;
            NTSTATUS status = CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSource);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, tokenSource->Token);
                co_await txn->CommitAsync();
            }

            try
            {
                tokenSource->Cancel();

                {
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalRemoveAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, tokenSource->Token);
                    CODING_ERROR_ASSERT(false); // Should not reach here
                }
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == STATUS_CANCELLED);
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, value);
        }

        Awaitable<void> Read_WithCancelledToken_ShouldNotSucceed_Test()
        {
            int key = 17;
            int value = 5;

            CancellationTokenSource::SPtr tokenSource = nullptr;
            NTSTATUS status = CancellationTokenSource::Create(GetAllocator(), ALLOC_TAG, tokenSource);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key, value, DefaultTimeout, tokenSource->Token);
                co_await txn->CommitAsync();
            }

            try
            {
                tokenSource->Cancel();

                {
                    KeyValuePair<LONG64, int> kvPair(-1, -1);
                    auto txn = CreateWriteTransaction();
                    co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key, DefaultTimeout, kvPair, tokenSource->Token);
                    CODING_ERROR_ASSERT(false); // Should not reach here
                }
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == STATUS_CANCELLED);
            }
        }

        Awaitable<void> SnapshotRead_FromSnapshotContainer_MovedFromDifferentialState_Test()
        {
            int key = 5;
            int value = 6;

            // Add
            {
                WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                co_await Store->AddAsync(*tx1->StoreTransactionSPtr, key, value, DefaultTimeout, CancellationToken::None);
                co_await tx1->CommitAsync();
            }

            // Start the snapshot transactiion here
            WriteTransaction<int, int>::SPtr tx3 = CreateWriteTransaction();
            tx3->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            co_await VerifyKeyExistsAsync(*Store, *tx3->StoreTransactionSPtr, key, -1, value);

            // Update causes entries to previous version.
            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, 7, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            // Update again to move entries to snapshot container
            {
                WriteTransaction<int, int>::SPtr tx2 = CreateWriteTransaction();
                co_await Store->ConditionalUpdateAsync(*tx2->StoreTransactionSPtr, key, 8, DefaultTimeout, CancellationToken::None);
                co_await tx2->CommitAsync();
            }

            co_await VerifyKeyExistsAsync(*Store, key, -1, 8);

            co_await VerifyKeyExistsAsync(*Store, *tx3->StoreTransactionSPtr, key, -1, value);
            co_await tx3->AbortAsync();
        }

        Awaitable<void> SnapshotRead_FromConsolidatedState_Test()
        {
            ULONG32 count = 4;

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            // Start the snapshot transactiion here
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            co_await CheckpointAsync();

            // Update after checkpoint.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 10, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, idx, -1, idx + 10);
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            co_await tx->AbortAsync();
        }

        Awaitable<void> SnapshotRead_FromSnapshotContainer_MovedFromDifferential_DuringConsolidation_Test()
        {
            ULONG32 count = 4;
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            // Start the snapshot transactiion here
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Read to start snapping visibility lsn here.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 10, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            // Read updated value to validate.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, idx, -1, idx + 10);
            }

            co_await CheckpointAsync();

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 20, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            // Read updated value to validate.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, idx, -1, idx + 20);
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            co_await tx->AbortAsync();
        }

        Awaitable<void> SnapshotRead_FromSnahotContainer_MovedFromConsolidatedState_Test()
        {
            ULONG32 count = 4;
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->AddAsync(*tx1->StoreTransactionSPtr, idx, idx, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            // Start the snapshot transactiion here
            WriteTransaction<int, int>::SPtr tx = CreateWriteTransaction();
            tx->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::Snapshot;

            // Read to start snapping visibility lsn here.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            co_await CheckpointAsync();

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                {
                    WriteTransaction<int, int>::SPtr tx1 = CreateWriteTransaction();
                    co_await Store->ConditionalUpdateAsync(*tx1->StoreTransactionSPtr, idx, idx + 10, DefaultTimeout, CancellationToken::None);
                    co_await tx1->CommitAsync();
                }
            }

            co_await CheckpointAsync();

            // Read updated value to validate.
            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, idx, -1, idx + 10);
            }

            for (ULONG32 idx = 0; idx < count; idx++)
            {
                co_await VerifyKeyExistsAsync(*Store, *tx->StoreTransactionSPtr, idx, -1, idx);
            }

            co_await tx->AbortAsync();
        }

        Awaitable<void> IStateProviderInfo_SetLang_GetLang_Test()
        {
            NTSTATUS status;
            IStateProviderInfo::SPtr stateProviderInfo = dynamic_cast<IStateProviderInfo*>(Store.RawPtr());
            VERIFY_IS_TRUE(stateProviderInfo != nullptr);

            // Without setting type information value should be null
            auto langTypeInfo = stateProviderInfo->GetLangTypeInfo(L"CSHARP");
            VERIFY_IS_TRUE(langTypeInfo == nullptr);

            // set and successfully retrieve type information for csharp
            status = stateProviderInfo->SetLangTypeInfo(L"CSHARP", L"System.String");
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            langTypeInfo = stateProviderInfo->GetLangTypeInfo(L"CSHARP");
            VERIFY_IS_TRUE(langTypeInfo->Compare(L"System.String") == 0);

            // type information for java should still be null
            langTypeInfo = stateProviderInfo->GetLangTypeInfo(L"JAVA");
            VERIFY_IS_TRUE(langTypeInfo == nullptr);

            VERIFY_IS_TRUE(stateProviderInfo->GetKind() == StateProviderKind::Store);

            co_return;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(StoreTest1ReplicaSuite, StoreTest1Replica)

    BOOST_AUTO_TEST_CASE(Add_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(Add_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdate_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddUpdate_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDelete_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddDelete_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDeleteAdd_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(AddDeleteAdd_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(NoAdd_Delete_ShouldFail)
    {
        SyncAwait(NoAdd_Delete_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDeleteUpdate_SingleTransaction_ShouldFail)
    {
        SyncAwait(AddDeleteUpdate_SingleTransaction_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(AddAdd_SingleTransaction_ShouldFail)
    {
        SyncAwait(AddAdd_SingleTransaction_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAdds_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(MultipleAdds_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleUpdates_SingleTransaction_ShouldSucceed)
    {
        SyncAwait(MultipleUpdates_SingleTransaction_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdate_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddUpdate_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDelete_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddDelete_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddUpdateRead_DifferentTransactions_ShouldSucceed)
    {
        SyncAwait(AddUpdateRead_DifferentTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddDeleteUpdate_DifferentTransactions_ShouldFail)
    {
        SyncAwait(AddDeleteUpdate_DifferentTransactions_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAdds_MultipleTransactions_ShouldSucceed)
    {
        SyncAwait(MultipleAdds_MultipleTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(MultipleAddsUpdates_MultipleTransactions_ShouldSucceed)
    {
        SyncAwait(MultipleAddsUpdates_MultipleTransactions_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Abort_ShouldSucceed)
    {
        SyncAwait(Add_Abort_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddAdd_SameKeyOnConcurrentTransaction_ShouldTimeout)
    {
        SyncAwait(AddAdd_SameKeyOnConcurrentTransaction_ShouldTimeout_Test());
    }

    BOOST_AUTO_TEST_CASE(UpdateRead_SameKeyOnConcurrentTransaction_ShouldTimeout)
    {
        SyncAwait(UpdateRead_SameKeyOnConcurrentTransaction_ShouldTimeout_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteStore_NotPrimary_ShouldFail)
    {
        SyncAwait(WriteStore_NotPrimary_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadStore_NotReadable_ShouldFail)
    {
        SyncAwait(ReadStore_NotReadable_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_ConditionalGet_ShouldSucceed)
    {
        SyncAwait(Add_ConditionalGet_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_ConditionalUpdate_ShouldSucceed)
    {
        SyncAwait(Add_ConditionalUpdate_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_ConditionalDelete_ShouldSucceed)
    {
        SyncAwait(Add_ConditionalDelete_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Read_WithUpdateLock_ShouldSucceed)
    {
        SyncAwait(Read_WithUpdateLock_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Read_WithUpdateLock_WithExistingReader_ShouldSucceed)
    {
        SyncAwait(Read_WithUpdateLock_WithExistingReader_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(Read_WithExistingUpdater_ShouldTimeout)
    {
        SyncAwait(Read_WithExistingUpdater_ShouldTimeout_Test());
    }

    BOOST_AUTO_TEST_CASE(Read_WithUpdateLock_WithExistingUpdater_ShouldTimeout)
    {
        SyncAwait(Read_WithUpdateLock_WithExistingUpdater_ShouldTimeout_Test());
    }

    BOOST_AUTO_TEST_CASE(Write_WithExistingUpdater_ShouldTimeout)
    {
        SyncAwait(Write_WithExistingUpdater_ShouldTimeout_Test());
    }

    BOOST_AUTO_TEST_CASE(Write_AsUpdater_WithExistingReaders_ShouldWait)
    {
        SyncAwait(Write_AsUpdater_WithExistingReaders_ShouldWait_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_WithCancelledToken_ShouldNotAdd)
    {
        SyncAwait(Add_WithCancelledToken_ShouldNotAdd_Test());
    }

    BOOST_AUTO_TEST_CASE(Update_WithCancelledToken_ShouldNotUpdate)
    {
        SyncAwait(Update_WithCancelledToken_ShouldNotUpdate_Test());
    }

    BOOST_AUTO_TEST_CASE(Remove_WithCancelledToken_ShouldNotRemove)
    {
        SyncAwait(Remove_WithCancelledToken_ShouldNotRemove_Test());
    }

    BOOST_AUTO_TEST_CASE(Read_WithCancelledToken_ShouldNotSucceed)
    {
        SyncAwait(Read_WithCancelledToken_ShouldNotSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnapshotContainer_MovedFromDifferentialState)
    {
        SyncAwait(SnapshotRead_FromSnapshotContainer_MovedFromDifferentialState_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_FromConsolidatedState)
    {
        SyncAwait(SnapshotRead_FromConsolidatedState_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnapshotContainer_MovedFromDifferential_DuringConsolidation)
    {
        SyncAwait(SnapshotRead_FromSnapshotContainer_MovedFromDifferential_DuringConsolidation_Test());
    }

    BOOST_AUTO_TEST_CASE(SnapshotRead_FromSnahotContainer_MovedFromConsolidatedState)
    {
        SyncAwait(SnapshotRead_FromSnahotContainer_MovedFromConsolidatedState_Test());
    }

    BOOST_AUTO_TEST_CASE(IStateProviderInfo_SetLang_GetLang)
    {
        SyncAwait(IStateProviderInfo_SetLang_GetLang_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
