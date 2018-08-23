//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'sbTP'


namespace TStoreTests
{
    using namespace ktl;

    class SweepManagerTest : public TStoreTestBase<KBuffer::SPtr, KBuffer::SPtr, KBufferComparer, KBufferSerializer, KBufferSerializer>
    {
    public:
       SweepManagerTest()
        {
            Setup(1);
            SyncAwait(Store->RecoverCheckpointAsync(CancellationToken::None));
        }

        ~SweepManagerTest()
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

        ktl::Awaitable<void> AddItemsAsync(__in ULONG32 startKey, __in ULONG32 endKey)
        {
           KArray<KBuffer::SPtr> keys(GetAllocator(), 10);
           KArray<KBuffer::SPtr> values(GetAllocator(), 10);
           ULONG32 bufferSize = 100;

           ValueEqualityFunctionType valueComparer;
           valueComparer.Bind(&bufferSize, BufferEquals);

           // Create test data
           for (ULONG32 i = startKey; i <= endKey; i++)
           {
              keys.Append(ToBuffer(i));
              values.Append(MakeBuffer(bufferSize, i));
           }

           ULONG32 count = (endKey - startKey) + 1;

           // Add data to differential state.
           {
              WriteTransaction<KBuffer::SPtr, KBuffer::SPtr>::SPtr tx1 = CreateWriteTransaction();

              for (ULONG32 i = 0; i < count; i++)
              {
                 co_await Store->AddAsync(*tx1->StoreTransactionSPtr, keys[i], values[i], DefaultTimeout, CancellationToken::None);
              }

              co_await tx1->CommitAsync();
           }
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

#pragma region test functions
    public:
        ktl::Awaitable<void> Add_Consolidate_SweepUntilMemoryIsBelowThreshold_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 consolidatedSize = Store->ConsolidationManagerSPtr->GetMemorySize(Store->GetEstimatedKeySize());
           LONG64 consolidatedKeyAndMetadataSize = 100 * Store->GetEstimatedKeySize() + 100 * Constants::ValueMetadataSizeBytes;
           LONG64 consolidatedValueSize = consolidatedSize - consolidatedKeyAndMetadataSize;

           co_await AddItemsAsync(100, 200);

           LONG64 startMemorySize = Store->GetMemorySize();
           LONG64 expectedSize = startMemorySize - consolidatedValueSize;

           Store->SweepManagerSPtr->SweepUntilMemoryIsBelowThreshold(10, CancellationToken::None, expectedSize + 1);
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 2);

           LONG64 endMemorySize = Store->GetMemorySize();

           LONG64 sweptSize = startMemorySize - endMemorySize;
           CODING_ERROR_ASSERT(NT_SUCCESS(sweptSize == consolidatedSize));
            co_return;
        }

        ktl::Awaitable<void> Add_Consolidate_Sweep_SizeLesserThan50PercentOfSweepThreshold_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 startMemorySize = Store->GetMemorySize();

           Store->SweepManagerSPtr->MemoryBufferSize = (startMemorySize * 3);
           Store->SweepManagerSPtr->Sweep();
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 1);

           LONG64 endMemorySize = Store->GetMemorySize();
           CODING_ERROR_ASSERT(NT_SUCCESS(startMemorySize == endMemorySize));
            co_return;
        }

        ktl::Awaitable<void> Add_Consolidate_Size_Between50And75PercentOfSweepThreshold_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 startMemorySize = Store->GetMemorySize();

           Store->SweepManagerSPtr->MemoryBufferSize = static_cast<LONG64>(startMemorySize * 2);

           // Consolidated state size increased
           Store->SweepManagerSPtr->Sweep();
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 2);

           LONG64 memorySizeBeforeSweep = Store->GetMemorySize();

           co_await AddItemsAsync(100, 200);

           // Consolidated state size has not increasd.
           Store->SweepManagerSPtr->Sweep();
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 1);

           LONG64 memorySizeAfterSweep = Store->GetMemorySize();

           CODING_ERROR_ASSERT(NT_SUCCESS(memorySizeBeforeSweep == memorySizeAfterSweep));
            co_return;
        }

        ktl::Awaitable<void> Add_Consolidate_Size_GreaterThan75PercentOfSweepThreshold_LesserThanThreshold_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 startMemorySize = Store->GetMemorySize();

           Store->SweepManagerSPtr->MemoryBufferSize = static_cast<LONG64>(startMemorySize * 1.25);

           // Consolidated state size increased
           Store->SweepManagerSPtr->Sweep();
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 2);

           LONG64 memorySizeAfterSweep = Store->GetMemorySize();

           CODING_ERROR_ASSERT(NT_SUCCESS(memorySizeAfterSweep == 0));
            co_return;
        }

        ktl::Awaitable<void> Add_Consolidate_Size_GreaterThanSweepThreshold_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 startMemorySize = Store->GetMemorySize();

           Store->SweepManagerSPtr->MemoryBufferSize = startMemorySize;

           // Consolidated state size increased
           Store->SweepManagerSPtr->Sweep();
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 2);

           LONG64 memorySizeAfterSweep = Store->GetMemorySize();

           CODING_ERROR_ASSERT(NT_SUCCESS(memorySizeAfterSweep == 0));
            co_return;
        }

        ktl::Awaitable<void> CancelSweep_ShouldSucceed_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 startMemorySize = Store->GetMemorySize();

           Store->SweepManagerSPtr->MemoryBufferSize = startMemorySize;

           ktl::Awaitable<void> sweepTask = Store->SweepManagerSPtr->SweepAsync();

           co_await Store->SweepManagerSPtr->CancelSweepTaskAsync();
           CODING_ERROR_ASSERT(sweepTask.IsComplete() == true);
           co_await sweepTask;
            co_return;
        }

        ktl::Awaitable<void> CloseStore_WhileSweeping_ShouldSucceed_Test()
        {
            co_await AddItemsAsync(0, 99);

            // Move to consolidated state.
            co_await CheckpointAsync();

            Store->SweepManagerSPtr->MemoryBufferSize = Store->GetMemorySize();
            Awaitable<void> sweepTask = Store->SweepManagerSPtr->SweepAsync();

            co_await CloseAndReOpenStoreAsync();

            CODING_ERROR_ASSERT(sweepTask.IsComplete() == true);
            co_await sweepTask;
            co_return;
        }
    
        ktl::Awaitable<void> AddConsolidateSweep_AfterAllItemsAreThrown_SweepShouldHaltEvenIfTargetSizeIsNotMet_Test()
        {
           co_await AddItemsAsync(0, 99);

           // Move to consolidated state.
           co_await CheckpointAsync();

           LONG64 consolidatedSize = Store->ConsolidationManagerSPtr->GetMemorySize(Store->GetEstimatedKeySize());
           LONG64 consolidatedKeyAndMetadataSize = 100 * Store->GetEstimatedKeySize() + 100 * Constants::ValueMetadataSizeBytes;
           LONG64 consolidatedValueSize = consolidatedSize - consolidatedKeyAndMetadataSize;

           co_await AddItemsAsync(100, 200);

           LONG64 startMemorySize = Store->GetMemorySize();
           LONG64 possibleTargetSize = startMemorySize - consolidatedValueSize;
           LONG64 desiredSize = possibleTargetSize - 1;

           Store->SweepManagerSPtr->SweepUntilMemoryIsBelowThreshold(10, CancellationToken::None, desiredSize);

           // Sweep should give up after it has reached 
           CODING_ERROR_ASSERT(Store->SweepManagerSPtr->NumberOfSweepCycles == 3);

           LONG64 endMemorySize = Store->GetMemorySize();

           LONG64 sweptSize = startMemorySize - endMemorySize;
           CODING_ERROR_ASSERT(NT_SUCCESS(sweptSize == consolidatedSize));
           CODING_ERROR_ASSERT(sweptSize < desiredSize);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(SweepManagerTestSuite, SweepManagerTest)

    BOOST_AUTO_TEST_CASE(Add_Consolidate_SweepUntilMemoryIsBelowThreshold)
    {
        SyncAwait(Add_Consolidate_SweepUntilMemoryIsBelowThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Consolidate_Sweep_SizeLesserThan50PercentOfSweepThreshold)
    {
        SyncAwait(Add_Consolidate_Sweep_SizeLesserThan50PercentOfSweepThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Consolidate_Size_Between50And75PercentOfSweepThreshold)
    {
        SyncAwait(Add_Consolidate_Size_Between50And75PercentOfSweepThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Consolidate_Size_GreaterThan75PercentOfSweepThreshold_LesserThanThreshold)
    {
        SyncAwait(Add_Consolidate_Size_GreaterThan75PercentOfSweepThreshold_LesserThanThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(Add_Consolidate_Size_GreaterThanSweepThreshold)
    {
        SyncAwait(Add_Consolidate_Size_GreaterThanSweepThreshold_Test());
    }

    BOOST_AUTO_TEST_CASE(CancelSweep_ShouldSucceed)
    {
        SyncAwait(CancelSweep_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(CloseStore_WhileSweeping_ShouldSucceed)
    {
        SyncAwait(CloseStore_WhileSweeping_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(AddConsolidateSweep_AfterAllItemsAreThrown_SweepShouldHaltEvenIfTargetSizeIsNotMet)
    {
        SyncAwait(AddConsolidateSweep_AfterAllItemsAreThrown_SweepShouldHaltEvenIfTargetSizeIsNotMet_Test());
    }
  
    BOOST_AUTO_TEST_SUITE_END()
}
