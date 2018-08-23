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

    class CheckpointFilePropertiesTests
    {
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    public:
        CheckpointFilePropertiesTests()
        {
        }

        ~CheckpointFilePropertiesTests()
        {
        }

    public:
        static CheckpointFileProperties::SPtr Read(
            __in BinaryReader & reader,
            __in BlockHandle const & handle,
            __in KAllocator & allocator);

        void CheckpointFilePropertiesReadAndWriteTest(
            __in bool codeIncludeCheckpointLSN,
            __in bool fileIncludeCheckpointLSN);
    };

    CheckpointFileProperties::SPtr CheckpointFilePropertiesTests::Read(
        __in BinaryReader & reader,
        __in BlockHandle const & handle,
        __in KAllocator & allocator)
    {
        CheckpointFileProperties::SPtr properties;
        NTSTATUS status = CheckpointFileProperties::Create(allocator, properties);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        properties->Test_Ignore = true;

        reader.Position = static_cast<ULONG>(handle.Offset);

        while (reader.get_Position() < static_cast<ULONG>(handle.EndOffset()))
        {
            KString::SPtr readString;
            reader.Read(readString);

            // Read the size in bytes of the property's value.
            ULONG valueSize = VarInt::ReadInt32(reader);

            // Read the property's value.
            KStringView propertyName = static_cast<KStringView>(*readString);
            properties->ReadProperty(reader, propertyName, valueSize);
        }

        return properties;
    }

    void CheckpointFilePropertiesTests::CheckpointFilePropertiesReadAndWriteTest(
        __in bool codeIncludeCheckpointLSN,
        __in bool fileIncludeCheckpointLSN)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // Expected
            BlockHandle::SPtr expectedBlocksHandle = nullptr;
            BlockHandle::SPtr expectedMetadataHandle = nullptr;
            NTSTATUS status = BlockHandle::Create(0, 10, allocator, expectedBlocksHandle);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = BlockHandle::Create(5, 15, allocator, expectedMetadataHandle);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            KGuid expectedPartitionId;
            expectedPartitionId.CreateNew();
            FABRIC_REPLICA_ID expectedReplicaId = 34;
            ULONG32 expectedRootStateProviderCount = 16;
            ULONG32 expectedStateProviderCount = 8;
            FABRIC_SEQUENCE_NUMBER expectedPrepareCheckpointLSN = 64;

            CheckpointFileProperties::SPtr checkpointFilePropertiesSPtr = nullptr;
            status = CheckpointFileProperties::Create(
                expectedBlocksHandle.RawPtr(),
                expectedMetadataHandle.RawPtr(),
                expectedPartitionId,
                expectedReplicaId,
                expectedRootStateProviderCount,
                expectedStateProviderCount,
                false,
                expectedPrepareCheckpointLSN,
                allocator,
                checkpointFilePropertiesSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            if (fileIncludeCheckpointLSN == false)
            {
                checkpointFilePropertiesSPtr->Test_Ignore = true;
            }

            KBlockFile::SPtr fakeFileSPtr = nullptr;
            ktl::io::KFileStream::SPtr fakeStream = nullptr;

            KWString fileName = TestHelper::CreateFileName(L"SMFP_WriteRead_SUCCESS.txt", allocator);

            status = SyncAwait(Data::TestCommon::FileFormatTestHelper::CreateBlockFile(fileName, allocator, CancellationToken::None, fakeFileSPtr, fakeStream));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            status = SyncAwait(fakeStream->OpenAsync(*fakeFileSPtr));
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            BlockHandle::SPtr writeHandle = nullptr;

            FileBlock<CheckpointFileProperties::SPtr>::SerializerFunc serializerFunction(checkpointFilePropertiesSPtr.RawPtr(), &CheckpointFileProperties::Write);
            status = SyncAwait(FileBlock<CheckpointFileProperties::SPtr>::WriteBlockAsync(*fakeStream, serializerFunction, allocator, CancellationToken::None, writeHandle));

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(writeHandle->Offset, 0);

            // If the file does not include the CheckpointLSN, the size is 120, otherwise the size is 143.
            VERIFY_ARE_EQUAL(writeHandle->Size, fileIncludeCheckpointLSN ? 143 : 120);

            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->MetadataHandle->Offset, 5);
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->MetadataHandle->Size, 15);

            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->BlocksHandle->Offset, 0);
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->BlocksHandle->Size, 10);

            CheckpointFileProperties::SPtr readHandle = nullptr;

            if (codeIncludeCheckpointLSN)
            {
                FileBlock<CheckpointFileProperties::SPtr>::DeserializerFunc deserializerFuncFunction(&CheckpointFileProperties::Read);
                readHandle = SyncAwait(FileBlock<CheckpointFileProperties::SPtr>::ReadBlockAsync(*fakeStream, *writeHandle, deserializerFuncFunction, allocator, CancellationToken::None));

            }
            else
            {
                FileBlock<CheckpointFileProperties::SPtr>::DeserializerFunc deserializerFuncFunction(&CheckpointFilePropertiesTests::Read);
                readHandle = SyncAwait(FileBlock<CheckpointFileProperties::SPtr>::ReadBlockAsync(*fakeStream, *writeHandle, deserializerFuncFunction, allocator, CancellationToken::None));
            }

            VERIFY_ARE_EQUAL(readHandle->MetadataHandle->Offset, 5);
            VERIFY_ARE_EQUAL(readHandle->MetadataHandle->Size, 15);

            VERIFY_ARE_EQUAL(readHandle->BlocksHandle->Offset, 0);
            VERIFY_ARE_EQUAL(readHandle->BlocksHandle->Size, 10);

            VERIFY_ARE_EQUAL(readHandle->ReplicaId, expectedReplicaId);
            VERIFY_ARE_EQUAL(readHandle->RootStateProviderCount, expectedRootStateProviderCount);
            VERIFY_ARE_EQUAL(readHandle->StateProviderCount, expectedStateProviderCount);
            VERIFY_ARE_EQUAL(readHandle->PartitionId, expectedPartitionId);
            VERIFY_ARE_EQUAL(
                readHandle->PrepareCheckpointLSN, 
                codeIncludeCheckpointLSN && fileIncludeCheckpointLSN ? expectedPrepareCheckpointLSN : FABRIC_INVALID_SEQUENCE_NUMBER);

            status = SyncAwait(fakeStream->CloseAsync());
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            fakeFileSPtr->Close();

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMFP_WriteRead_SUCCESS.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    BOOST_FIXTURE_TEST_SUITE(CheckpointFilePropertiesTestSuite, CheckpointFilePropertiesTests)

    BOOST_AUTO_TEST_CASE(Create_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            CheckpointFileProperties::SPtr checkpointFilePropertiesSPtr = nullptr;

            NTSTATUS status = CheckpointFileProperties::Create(allocator, checkpointFilePropertiesSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->PrepareCheckpointLSN, FABRIC_INVALID_SEQUENCE_NUMBER);
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->PartitionId, KGuid(TRUE));
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->ReplicaId, 0);
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->RootStateProviderCount, 0);
            VERIFY_ARE_EQUAL(checkpointFilePropertiesSPtr->StateProviderCount, 0);
        }
    }

    BOOST_AUTO_TEST_CASE(WriteRead_SUCCESS)
    {
        this->CheckpointFilePropertiesReadAndWriteTest(true, true);
    }

    // Verify adding the PrepareCheckpointLSN does not effect the file reading or writing.
    // The following case is the checkpoint file has the PrepareCheckpointLSN property, and the 
    // Code version does not include PrepareCheckpointLSN property, in which case the reading will
    // just ignore PrepareCheckpointLSN property.
    BOOST_AUTO_TEST_CASE(PrepareCheckpointLSN_CodeDoesNotInclude_FileInclude)
    {
        this->CheckpointFilePropertiesReadAndWriteTest(false, true);
    }

    // Verify adding the PrepareCheckpointLSN does not effect the file reading or writing.
    // The following case is the checkpoint file does not have the PrepareCheckpointLSN property, and the 
    // Code version includes PrepareCheckpointLSN property, in which case the  PrepareCheckpointLSN is FABRIC_INVALID_SEQUENCE_NUMBER
    BOOST_AUTO_TEST_CASE(PrepareCheckpointLSN_CodeInclude_FileDoesNotInclude)
    {
        this->CheckpointFilePropertiesReadAndWriteTest(true, false);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
