// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'sbTP'


namespace TStoreTests
{
    using namespace ktl;

    class BufferBufferConsolidationTest : public TStoreTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
        BufferBufferConsolidationTest()
        {
            Setup(3);
            SyncAwait(Store->RecoverCheckpointAsync(CancellationToken::None));
        }

        ~BufferBufferConsolidationTest()
        {
            Cleanup();
        }

        KBuffer::SPtr ToBuffer(__in ULONG num)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(sizeof(ULONG), bufferSptr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            auto buffer = (ULONG *)bufferSptr->GetBuffer();
            buffer[0] = num;
            return bufferSptr;
        }

        KBuffer::SPtr MakeBuffer(ULONG32 numElements, ULONG32 multiplier = 1)
        {
            KBuffer::SPtr bufferSptr;
            auto status = KBuffer::Create(numElements * sizeof(ULONG32), bufferSptr, GetAllocator());
            Diagnostics::Validate(status);

            auto buffer = (ULONG32 *)bufferSptr->GetBuffer();
            for (ULONG32 i = 0; i < numElements; i++)
            {
                buffer[i] = multiplier * (i + 1); // so that buffer[0] = multiplier
            }

            return bufferSptr;
        }

        static bool BufferEquals(PVOID context, KBuffer::SPtr & one, KBuffer::SPtr & two)
        {
            ULONG32 numElements = *(ULONG32 *)context;
            auto oneBytes = (byte *)one->GetBuffer();
            auto twoBytes = (byte *)two->GetBuffer();

            for (ULONG32 i = 0; i < numElements; i++)
            {
                if (oneBytes[i] != twoBytes[i])
                {
                    return false;
                }
            }

            return true;
        }

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(BufferBufferConsolidationTestSuite, BufferBufferConsolidationTest)

    BOOST_AUTO_TEST_CASE(Add_ConslidateRead_ShouldSucceed)
    {
        KArray<KBuffer::SPtr> keys(GetAllocator(), 10);
        KArray<KBuffer::SPtr> values(GetAllocator(), 10);
        ULONG32 bufferSize = 100;

        ValueEqualityFunctionType valueComparer;
        valueComparer.Bind(&bufferSize, BufferEquals);
        
        // Create test data
        for (ULONG32 i = 0; i < 10; i++)
        {
            keys.Append(ToBuffer(i));
            values.Append(MakeBuffer(bufferSize, i));
        }
        
        // Add test data to store
        {
            WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

            for (ULONG32 i = 0; i < 10; i++)
            {
                SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, keys[i], values[i], DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(tx1->CommitAsync());
        }
        
        // Checkpoint
        Checkpoint();
        
        // Verify test data exists
        {
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
                for (int i = 0; i < 10; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, keys[i], nullptr, values[i], valueComparer));
                }

                SyncAwait(tx2->AbortAsync());
            }
        }
        
        // Recover
        CloseAndReOpenStore();

        // Check test data recovered
        for (ULONG32 i = 0; i < 10; i++)
        {
            SyncAwait(VerifyKeyExistsInStoresAsync(keys[i], nullptr, values[i], valueComparer));
        }

    }

    BOOST_AUTO_TEST_CASE(AddConsolidateRead_UpdateRead_ShouldSucceed)
    {
        KArray<KBuffer::SPtr> keys(GetAllocator(), 10);
        KArray<KBuffer::SPtr> values(GetAllocator(), 10);
        KArray<KBuffer::SPtr> updatedValues(GetAllocator(), 10);
        ULONG32 bufferSize = 100;

        ValueEqualityFunctionType valueComparer;
        valueComparer.Bind(&bufferSize, BufferEquals);

        // Generate test data
        for (ULONG32 i = 0; i < 10; i++)
        {
            keys.Append(ToBuffer(i));
            values.Append(MakeBuffer(bufferSize, i));
            updatedValues.Append(MakeBuffer(bufferSize, i + 10));
        }

        // Add test data
        {
            WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

            for (ULONG32 i = 0; i < 10; i++)
            {
                SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, keys[i], values[i], DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(tx1->CommitAsync());
        }

        // Checkpoint
        Checkpoint();

        // Verify store still has test data
        {
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
                for (int i = 0; i < 10; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, keys[i], nullptr, values[i], valueComparer));
                }

                SyncAwait(tx2->AbortAsync());
            }
        }

        // Recover
        CloseAndReOpenStore();

        // Update
        {
            WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx3 = CreateWriteTransaction();
            for (ULONG32 i = 0; i < 10; i++)
            {
                SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, keys[i], updatedValues[i], DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(tx3->CommitAsync());
        }

        {
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx4 = CreateWriteTransaction(*(*Stores)[m]);
                for (int i = 0; i < 10; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx4->StoreTransactionSPtr, keys[i], nullptr, updatedValues[i], valueComparer));
                }

                SyncAwait(tx4->AbortAsync());
            }
        }
    }

    BOOST_AUTO_TEST_CASE(AddConsolidateRead_UpdateConsolidateRead_ShouldSucceed)
    {
        KArray<KBuffer::SPtr> keys(GetAllocator(), 10);
        KArray<KBuffer::SPtr> values(GetAllocator(), 10);
        KArray<KBuffer::SPtr> updatedValues(GetAllocator(), 10);
        ULONG32 bufferSize = 100;

        ValueEqualityFunctionType valueComparer;
        valueComparer.Bind(&bufferSize, BufferEquals);

        // Generate test data
        for (ULONG32 i = 0; i < 10; i++)
        {
            keys.Append(ToBuffer(i));
            values.Append(MakeBuffer(bufferSize, i));
            updatedValues.Append(MakeBuffer(bufferSize, i + 10));
        }
        
        // Add test data to store
        {
            WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

            for (ULONG32 i = 0; i < 10; i++)
            {
                SyncAwait(Store->AddAsync(*tx1->StoreTransactionSPtr, keys[i], values[i], DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(tx1->CommitAsync());
        }

        // Checkpoint
        Checkpoint();
        
        // Verify test data still in store
        {
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx2 = CreateWriteTransaction(*(*Stores)[m]);
                for (int i = 0; i < 10; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx2->StoreTransactionSPtr, keys[i], nullptr, values[i], valueComparer));
                }

                SyncAwait(tx2->AbortAsync());
            }
        }
        
        // Update store values
        {
            WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx3 = CreateWriteTransaction();
            for (ULONG32 i = 0; i < 10; i++)
            {
                SyncAwait(Store->ConditionalUpdateAsync(*tx3->StoreTransactionSPtr, keys[i], updatedValues[i], DefaultTimeout, CancellationToken::None));
            }

            SyncAwait(tx3->CommitAsync());
        }
        
        // Checkpoint again
        Checkpoint();

        // Verify updated data still in store
        {
            for (ULONG m = 0; m < Stores->Count(); m++)
            {
                WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx4 = CreateWriteTransaction(*(*Stores)[m]);
                for (int i = 0; i < 10; i++)
                {
                    SyncAwait(VerifyKeyExistsAsync(*(*Stores)[m], *tx4->StoreTransactionSPtr, keys[i], nullptr, updatedValues[i], valueComparer));
                }

                SyncAwait(tx4->AbortAsync());
            }
        }

        // Recover and verify
        CloseAndReOpenStore();

        for (ULONG32 i = 0; i < 10; i++)
        {
            SyncAwait(VerifyKeyExistsInStoresAsync(keys[i], nullptr, updatedValues[i], valueComparer));
        }
    }
    
   
    BOOST_AUTO_TEST_SUITE_END()

}
