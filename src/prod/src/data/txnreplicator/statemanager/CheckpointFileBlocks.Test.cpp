// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace StateManagerTests
{
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data::StateManager;
    using namespace Data::Utilities;

    class CheckpointFileBlocksTests
    {
        Common::CommonConfig config; // load the config object as its needed for the tracing to work
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointFileBlocksTestsSuite, CheckpointFileBlocksTests)
        
    BOOST_AUTO_TEST_CASE(CreateAndGetter_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            CheckpointFileBlocks::SPtr checkpointFileBlocksSPtr = nullptr;
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            KSharedArray<ULONG32>::SPtr expectedArraySPtr = TestHelper::CreateRecordBlockSizesArray(allocator);

            for (ULONG32 i = 0; i < 10; i++)
            {
                status = expectedArraySPtr->Append(i);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            // test create with recordBlockSizes
            status = CheckpointFileBlocks::Create(*expectedArraySPtr, allocator, checkpointFileBlocksSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // verify the RecordBlockSizes value is right
            for (ULONG32 i = 0; i < 10; i++)
            {
                CODING_ERROR_ASSERT(i == (*checkpointFileBlocksSPtr->RecordBlockSizes)[i]);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Write_EmptyArray_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // test write with empty array
            KSharedArray<ULONG32>::SPtr expectedArraySPtr = TestHelper::CreateRecordBlockSizesArray(allocator);
            CheckpointFileBlocks::SPtr checkpointFileBlocksSPtr = nullptr;

            NTSTATUS status = CheckpointFileBlocks::Create(*expectedArraySPtr, allocator, checkpointFileBlocksSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryWriter writer(allocator);
            checkpointFileBlocksSPtr->Write(writer);

            // verify the readvalue
            BinaryReader reader(*writer.GetBuffer(0, writer.Position), allocator);
            LONG32 readValue;
            reader.Read(readValue);
            CODING_ERROR_ASSERT(0 == readValue);
        }
    }

    BOOST_AUTO_TEST_CASE(Write_Array_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            NTSTATUS status;

            // test write with array
            CheckpointFileBlocks::SPtr checkpointFileBlocksSPtr = nullptr;
            KSharedArray<ULONG32>::SPtr expectedArraySPtr = TestHelper::CreateRecordBlockSizesArray(allocator);
            for (ULONG32 i = 0; i < 10; i++)
            {
                status = expectedArraySPtr->Append(i);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            status = CheckpointFileBlocks::Create(*expectedArraySPtr, allocator, checkpointFileBlocksSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryWriter writer(allocator);
            checkpointFileBlocksSPtr->Write(writer);

            // verify: 1. read lenght 2. read items
            BinaryReader reader(*writer.GetBuffer(0, writer.Position), allocator);
            LONG32 readLength;
            reader.Read(readLength);
            CODING_ERROR_ASSERT(expectedArraySPtr->Count() == static_cast<ULONG>(readLength));

            for (ULONG32 i = 0; i < 10; i++)
            {
                ULONG32 readValue;
                reader.Read(readValue);
                CODING_ERROR_ASSERT(i == readValue);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(Read_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();

            // test set up
            BinaryWriter writer(allocator);
            ULONG32 expectedLength = 10;
            writer.Write(static_cast<LONG32>(expectedLength));
            for (ULONG32 i = 0; i < expectedLength; i++)
            {
                writer.Write(i);
            }
            BinaryReader reader(*writer.GetBuffer(0, writer.Position), allocator);
            BlockHandle::SPtr blockHandle = nullptr;
            NTSTATUS status = BlockHandle::Create(0, writer.Position, allocator, blockHandle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CheckpointFileBlocks::SPtr checkpointFileBlocksSPtr = CheckpointFileBlocks::Read(reader, *blockHandle, allocator);

            // verify the array values
            for (ULONG32 i = 0; i < expectedLength; i++)
            {
                auto temp = (*checkpointFileBlocksSPtr->RecordBlockSizes)[i];
                CODING_ERROR_ASSERT(i == temp);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(ReadAndWrite_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            NTSTATUS status;
            BinaryWriter writer(allocator);
            ULONG32 expectedLength = 10;

            CheckpointFileBlocks::SPtr checkpointFileBlocksSPtr = nullptr;
            KSharedArray<ULONG32>::SPtr expectedArraySPtr = TestHelper::CreateRecordBlockSizesArray(allocator);
            for (ULONG32 i = 0; i < expectedLength; i++)
            {
                status = expectedArraySPtr->Append(i);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            status = CheckpointFileBlocks::Create(*expectedArraySPtr, allocator, checkpointFileBlocksSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // write to the stream
            checkpointFileBlocksSPtr->Write(writer);

            BinaryReader reader(*writer.GetBuffer(0, writer.Position), allocator);
            BlockHandle::SPtr blockHandle = nullptr;

            status = BlockHandle::Create(0, writer.Position, allocator, blockHandle);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // read from the stream
            CheckpointFileBlocks::SPtr checkpointFileBlocks = CheckpointFileBlocks::Read(reader, *blockHandle, allocator);

            // verify the values
            for (ULONG32 i = 0; i < expectedLength; i++)
            {
                auto temp = (*checkpointFileBlocks->RecordBlockSizes)[i];
                CODING_ERROR_ASSERT(i == temp);
            }
        }
    }
    
    BOOST_AUTO_TEST_SUITE_END()
}
