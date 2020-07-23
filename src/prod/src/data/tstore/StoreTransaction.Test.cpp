// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'ssTP'

namespace TStoreTests
{
    class StoreTransactionTest : public TStoreTestBase<LONG64, KBuffer::SPtr, LongComparer, TestStateSerializer<LONG64>, KBufferSerializer>
    {
    public:
        StoreTransactionTest()
        {
            Setup(1);
            Store->EnableSweep = false;
        }

        ~StoreTransactionTest()
        {
            Cleanup();
        }

        KBuffer::SPtr CreateBuffer(__in LONG64 size, __in byte fillValue=0xb6)
        {
            CODING_ERROR_ASSERT(size >= 0);

            KBuffer::SPtr bufferSPtr = nullptr;
            NTSTATUS status = KBuffer::Create(static_cast<ULONG>(size), bufferSPtr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            byte* buffer = static_cast<byte *>(bufferSPtr->GetBuffer());
            memset(buffer, fillValue, static_cast<ULONG>(size));

            return bufferSPtr;
        }

        ktl::Awaitable<void> AbortTxnInBackground(WriteTransaction<LONG64, KBuffer::SPtr> & txn)
        {
            co_await CorHelper::ThreadPoolThread(GetAllocator().GetKtlSystem().DefaultSystemThreadPool());
            txn.StoreTransactionSPtr->Unlock();
        }

        ktl::Awaitable<void> VerifyKeyLockIsReleasedIfTxnIsAbortedBeforeLockAcquisition()
        {
            long key1 = 17;
            long key2 = 18;
            KBuffer::SPtr value = CreateBuffer(4);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto txn1 = CreateWriteTransaction();
            co_await Store->ConditionalUpdateAsync(*txn1->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
            
            auto abortTask = AbortTxnInBackground(*txn1);

            try
            {
                co_await Store->ConditionalUpdateAsync(*txn1->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TRANSACTION_ABORTED);
            }
            
            co_await abortTask;

            // Dispose txn
            txn1 = nullptr;

            // If the abort of txn above does not do a proper cleanup, get on lock key2 will see a timeout exception
            {
                KeyValuePair<LONG64, KBuffer::SPtr> kvp;
                auto txn = CreateWriteTransaction();
                txn->StoreTransactionSPtr->ReadIsolationLevel = StoreTransactionReadIsolationLevel::ReadRepeatable;
                co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key1, DefaultTimeout, kvp, CancellationToken::None);
                co_await Store->ConditionalGetAsync(*txn->StoreTransactionSPtr, key2, DefaultTimeout, kvp, CancellationToken::None);
                co_await txn->CommitAsync();
            }
        }

        ktl::Awaitable<void> VerifyPrimeLockIsReleasedIfTxnIsAbortedBeforeLockAcquisition()
        {
            long key1 = 17;
            long key2 = 18;
            KBuffer::SPtr value = CreateBuffer(4);

            {
                auto txn = CreateWriteTransaction();
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
                co_await Store->AddAsync(*txn->StoreTransactionSPtr, key2, value, DefaultTimeout, CancellationToken::None);
                co_await txn->CommitAsync();
            }

            auto txn1 = CreateWriteTransaction();

            auto updateTask = Store->ConditionalUpdateAsync(*txn1->StoreTransactionSPtr, key1, value, DefaultTimeout, CancellationToken::None);
            auto abortTask = AbortTxnInBackground(*txn1);

            try
            {
                await updateTask;
            }
            catch (ktl::Exception const & e)
            {
                CODING_ERROR_ASSERT(e.GetStatus() == SF_STATUS_TRANSACTION_ABORTED);
            }

            await abortTask;

            // Dispose txn
            txn1 = nullptr;

            // If the abort of txn above does not do a proper cleanup, prime lock will timeout
            co_await Store->LockManagerSPtr->AcquirePrimeLockAsync(LockMode::Exclusive, Common::TimeSpan::FromSeconds(1));
            Store->LockManagerSPtr->ReleasePrimeLock(LockMode::Exclusive);
        }

#pragma region test functions
    public:
        ktl::Awaitable<void> VerifyKeyLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion_Test()
        {
            co_await VerifyKeyLockIsReleasedIfTxnIsAbortedBeforeLockAcquisition();
            co_return;
        }

        ktl::Awaitable<void> VerifyPrimeLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion_Test()
        {
            co_await VerifyPrimeLockIsReleasedIfTxnIsAbortedBeforeLockAcquisition();
            co_return;
        }
    #pragma endregion
    };


    BOOST_FIXTURE_TEST_SUITE(StoreTransactionTestSuite, StoreTransactionTest)

    BOOST_AUTO_TEST_CASE(VerifyKeyLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion)
    {
        SyncAwait(VerifyKeyLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion_Test());
    }

    BOOST_AUTO_TEST_CASE(VerifyPrimeLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion)
    {
        SyncAwait(VerifyPrimeLockIsReleasedIfTxnIsAbortedBeforeLockAcquistion_Test());
    }

    BOOST_AUTO_TEST_SUITE_END()
}

