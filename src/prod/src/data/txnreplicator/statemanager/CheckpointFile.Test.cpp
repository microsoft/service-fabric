// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace StateManagerTests
{
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data::StateManager;
    using namespace Data::Utilities;

    class CheckpointFileTests
    {
        CommonConfig config; // load the config object as its needed for the tracing to work

    public:
        CheckpointFileTests()
        {
            expectedPartitionId.CreateNew();
        }

        ~CheckpointFileTests()
        {
        }

    public:
        SerializableMetadata::CSPtr CreateSerializableMetadata(
            __in KAllocator & allocator,
            __in bool isInitParameterNull,
            __in ULONG32 bufferCount = 1,
            __in ULONG32 bufferSize = 16);

        SerializableMetadata::CSPtr CreateSerializableMetadataWithID(
            __in ULONG32 id, 
            __in KAllocator & allocator);

        void Test_GetCopyData_VerifyCopyItems(
            __in ULONG32 metadataCount, 
            __in KAllocator & allocator);

        void DataCorruptionTest(__in ULONG32 position);
        void DataCorruptionPartialFileTest();

        void TestOneMetadataCreateAsyncAndOpenAsync(
            __in bool isInitParameterNull,
            __in ULONG32 bufferCount = 1,
            __in ULONG32 bufferSize = 16);

        void TestOneMetadataWriteReadMetadata(
            __in bool isInitParameterNull,
            __in ULONG32 bufferCount = 1,
            __in ULONG32 bufferSize = 16);

        // ReOpen the file again and close it to make sure previous close has completed.
        // And then clean up.
        void OpenAndCloseFileThenDelete(
            __in KWString & fileName, 
            __in KAllocator& allocator);

        CheckpointFile::SPtr CreateCheckpointFileSPtr(
            __in KWString const & filePath,
            __in KAllocator & allocator);

        KUri::CSPtr CreateStateProviderName(
            __in LONG64 stateProviderId,
            __in KStringView const & string,
            __in KAllocator & allocator);

        void VerifyProperties(
            __in KSharedArray<SerializableMetadata::CSPtr> const & metadataArray,
            __in CheckpointFile & checkpointFile);
    
        const LONG64 expectedStateProviderId = 6;
        const LONG64 expectedParentId = 17;
        const LONG64 expectedCreateLSN = 19;
        const LONG64 expectedDeleteLSN = 87;
        const MetadataMode::Enum expectedMetadataMode = MetadataMode::Enum::Active;
        const FABRIC_REPLICA_ID expectedReplicaId = 8;
        const FABRIC_SEQUENCE_NUMBER expectedPrepareCheckpointLSN = 64;
        KUriView expectedNameView = L"fabric:/sps/sp";
        KGuid expectedPartitionId;
    };

    SerializableMetadata::CSPtr CheckpointFileTests::CreateSerializableMetadata(
        __in KAllocator & allocator,
        __in bool isInitParameterNull,
        __in ULONG32 bufferCount,
        __in ULONG32 bufferSize)
    {
        KUri::CSPtr expectedNameCSPtr;
        NTSTATUS status = KUri::Create(expectedNameView, allocator, expectedNameCSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KString::SPtr expectedTypeCSPtr;
        status = KString::Create(expectedTypeCSPtr, allocator, TestStateProvider::TypeName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        OperationData::SPtr initParameters = isInitParameterNull ? nullptr : TestHelper::CreateInitParameters(allocator, bufferCount, bufferSize);

        // Setup
        SerializableMetadata::CSPtr serializablemetadataSPtr = nullptr;
        status = SerializableMetadata::Create(
            *expectedNameCSPtr,
            *expectedTypeCSPtr,
            expectedStateProviderId,
            expectedParentId,
            expectedCreateLSN,
            expectedDeleteLSN,
            expectedMetadataMode,
            allocator,
            initParameters.RawPtr(),
            serializablemetadataSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return serializablemetadataSPtr;
    }

    SerializableMetadata::CSPtr CheckpointFileTests::CreateSerializableMetadataWithID(
        __in ULONG32 id, 
        __in KAllocator & allocator)
    {
        KUri::CSPtr expectedNameCSPtr;
        NTSTATUS status = KUri::Create(expectedNameView, allocator, expectedNameCSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KString::SPtr expectedTypeCSPtr;
        status = KString::Create(expectedTypeCSPtr, allocator, TestStateProvider::TypeName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        OperationData::SPtr initParameters = TestHelper::CreateInitParameters(allocator);

        // Setup
        SerializableMetadata::CSPtr serializablemetadataSPtr = nullptr;
        status = SerializableMetadata::Create(
            *expectedNameCSPtr,
            *expectedTypeCSPtr,
            expectedStateProviderId + id,
            expectedParentId + id,
            id,
            id,
            expectedMetadataMode,
            allocator,
            initParameters.RawPtr(),
            serializablemetadataSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return serializablemetadataSPtr;
    }

    void CheckpointFileTests::Test_GetCopyData_VerifyCopyItems(
        __in ULONG32 metadataCount, 
        __in KAllocator & allocator)
    {
        KArray<SerializableMetadata::CSPtr> expectedSerializableMetadataArray(allocator);
        KArray<SerializableMetadata::CSPtr> resultSerializableMetadataArray(allocator);

        PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

        for (ULONG32 i = 0; i < metadataCount; ++i)
        {
            SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
            NTSTATUS status = expectedSerializableMetadataArray.Append(SerializableMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Copy from Primary.
        OperationDataEnumerator::SPtr operationDataEnumerator = CheckpointFile::GetCopyData(
            *partitionedReplicaIdCSPtr, 
            expectedSerializableMetadataArray, 
            expectedReplicaId + 1,
            SerializationMode::Enum::Native,
            allocator);

        while (operationDataEnumerator->MoveNext())
        {
            // Read on Secondary.
            KSharedPtr<SerializableMetadataEnumerator> result = CheckpointFile::ReadCopyData(*partitionedReplicaIdCSPtr, operationDataEnumerator->Current().RawPtr(), allocator);

            if (result != nullptr)
            {
                while (result->MoveNext())
                {
                    NTSTATUS status = resultSerializableMetadataArray.Append(result->Current());
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
            }
        }

        VERIFY_ARE_EQUAL(expectedSerializableMetadataArray.Count(), metadataCount);
        VERIFY_ARE_EQUAL(resultSerializableMetadataArray.Count(), metadataCount);

        for (ULONG i = 0; i < expectedSerializableMetadataArray.Count(); ++i)
        {
            SerializableMetadata::CSPtr current = resultSerializableMetadataArray[i];
            SerializableMetadata::CSPtr expected = expectedSerializableMetadataArray[i];
            VERIFY_ARE_EQUAL(*current->Name, *expected->Name);
            VERIFY_ARE_EQUAL(*current->TypeString, *expected->TypeString);
            VERIFY_ARE_EQUAL(current->StateProviderId, expected->StateProviderId);
            VERIFY_ARE_EQUAL(current->ParentStateProviderId, expected->ParentStateProviderId);
            VERIFY_ARE_EQUAL(current->CreateLSN, expected->CreateLSN);
            VERIFY_ARE_EQUAL(current->DeleteLSN, expected->DeleteLSN);
            VERIFY_ARE_EQUAL(current->MetadataMode, expected->MetadataMode);
        }
    }

    void CheckpointFileTests::DataCorruptionTest(
        __in ULONG32 position)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"DataCorruptionTest.txt", allocator);
            bool isThrow = false;

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadata(allocator, false);

            NTSTATUS status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));

            KBlockFile::SPtr FileSPtr = nullptr;
            io::KFileStream::SPtr fileStreamSPtr = nullptr;

            status = SyncAwait(KBlockFile::CreateSparseFileAsync(
                fileName,
                FALSE,
                KBlockFile::eOpenExisting,
                FileSPtr,
                nullptr,
                allocator,
                TEST_TAG));

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = io::KFileStream::Create(fileStreamSPtr, allocator, TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(fileStreamSPtr->OpenAsync(*FileSPtr));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            BinaryWriter writer(allocator);
            writer.Write(static_cast<ULONG64>(99));

            fileStreamSPtr->SetPosition(position);
            status = SyncAwait(fileStreamSPtr->WriteAsync(*writer.GetBuffer(0)));

            if (fileStreamSPtr != nullptr)
            {
                status = SyncAwait(fileStreamSPtr->CloseAsync());
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                fileStreamSPtr = nullptr;
            }

            FileSPtr->Close();

            try
            {
                SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
                CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

                while (true)
                {
                    SerializableMetadata::CSPtr serializableMetadataCSPtr = nullptr;
                    status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, serializableMetadataCSPtr));
                    if (status == STATUS_NOT_FOUND || STATUS_INTERNAL_DB_CORRUPTION)
                    {
                        break;
                    }

                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                SyncAwait(enumerator->CloseAsync());
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INTERNAL_DB_CORRUPTION);
                isThrow = true;
            }
           
            VERIFY_IS_TRUE(isThrow || status == STATUS_INTERNAL_DB_CORRUPTION);
            OpenAndCloseFileThenDelete(fileName, allocator);
        }
    }

    void CheckpointFileTests::DataCorruptionPartialFileTest()
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            const FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN = 16;
            KWString fileName = TestHelper::CreateFileName(L"DataCorruptionTest.txt", allocator);
            bool isThrow = false;

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadata(allocator, false);

            NTSTATUS status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            CheckpointFile::SPtr temp = nullptr;
            CheckpointFile::Create(
                *partitionedReplicaIdCSPtr, 
                fileName, 
                allocator, 
                temp);
            SyncAwait(temp->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                prepareCheckpointLSN, 
                CancellationToken::None));

            KBlockFile::SPtr FileSPtr = nullptr;

            status = SyncAwait(KBlockFile::CreateSparseFileAsync(
                fileName,
                FALSE,
                KBlockFile::eOpenExisting,
                FileSPtr,
                nullptr,
                allocator,
                TEST_TAG));

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            SyncAwait(FileSPtr->SetEndOfFileAsync(100));

            FileSPtr->Close();

            try
            {
                CheckpointFile::SPtr checkpointFileReopenSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INTERNAL_DB_CORRUPTION);
                isThrow = true;
            }

            CODING_ERROR_ASSERT(isThrow);
            OpenAndCloseFileThenDelete(fileName, allocator);
        }
    }

    void CheckpointFileTests::TestOneMetadataCreateAsyncAndOpenAsync(
        __in bool isInitParameterNull,
        __in ULONG32 bufferCount,
        __in ULONG32 bufferSize)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_OneMetadata_Open_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr serializablemetadataSPtr = CreateSerializableMetadata(allocator, isInitParameterNull, bufferCount, bufferSize);

            NTSTATUS status = SerializableMetadataArray->Append(serializablemetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;     

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                if (isInitParameterNull)
                {
                    VERIFY_IS_NULL(metadata->InitializationParameters);
                }

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, expectedCreateLSN);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, expectedDeleteLSN);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 1);
            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_OneMetadata_Open_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    void CheckpointFileTests::TestOneMetadataWriteReadMetadata(
        __in bool isInitParameterNull,
        __in ULONG32 bufferCount,
        __in ULONG32 bufferSize)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // Setup
            SerializableMetadata::CSPtr serializablemetadataSPtr = CreateSerializableMetadata(allocator, isInitParameterNull, bufferCount, bufferSize);
            BinaryWriter writer(allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(writer.Status()));

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

            TestCheckpointFileReadWrite::WriteMetadata(*partitionedReplicaIdCSPtr, writer, *serializablemetadataSPtr, SerializationMode::Enum::Native);

            BinaryReader reader(*writer.GetBuffer(0, writer.Position), allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(reader.Status()));

            serializablemetadataSPtr = TestCheckpointFileReadWrite::ReadMetadata(*partitionedReplicaIdCSPtr, reader, allocator);

            VERIFY_ARE_EQUAL(expectedNameView.Get(KUriView::eRaw), serializablemetadataSPtr->Name->Get(KUriView::eRaw));
            VERIFY_ARE_EQUAL(TestStateProvider::TypeName.Compare(*serializablemetadataSPtr->TypeString), 0);

            if (isInitParameterNull)
            {
                VERIFY_IS_NULL(serializablemetadataSPtr->InitializationParameters);
            }
            else
            {
                TestHelper::VerifyInitParameters(*serializablemetadataSPtr->InitializationParameters, bufferCount, bufferSize);
            }

            VERIFY_ARE_EQUAL(expectedStateProviderId, serializablemetadataSPtr->StateProviderId);
            VERIFY_ARE_EQUAL(expectedCreateLSN, serializablemetadataSPtr->CreateLSN);
            VERIFY_ARE_EQUAL(expectedDeleteLSN, serializablemetadataSPtr->DeleteLSN);
            VERIFY_ARE_EQUAL(expectedMetadataMode, serializablemetadataSPtr->MetadataMode);
        }
    }

    // ReOpen the file again and close it to make sure previous close has completed.
    // And then clean up.
    void CheckpointFileTests::OpenAndCloseFileThenDelete(
        __in KWString & fileName, 
        __in KAllocator& allocator)
    {
        KBlockFile::SPtr FileSPtr = nullptr;
        io::KFileStream::SPtr fileStreamSPtr = nullptr;

        NTSTATUS status = SyncAwait(KBlockFile::CreateSparseFileAsync(
            fileName,
            FALSE,
            KBlockFile::eOpenExisting,
            FileSPtr,
            nullptr,
            allocator,
            TEST_TAG));

        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        KFinally([&] {FileSPtr->Close(); });

        status = io::KFileStream::Create(fileStreamSPtr, allocator, TEST_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(fileStreamSPtr->OpenAsync(*FileSPtr));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = SyncAwait(fileStreamSPtr->CloseAsync());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = KVolumeNamespace::DeleteFileOrDirectory(fileName, allocator, nullptr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
    }

    CheckpointFile::SPtr CheckpointFileTests::CreateCheckpointFileSPtr(
        __in KWString const & filePath,
        __in KAllocator & allocator)
    {
        PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

        CheckpointFile::SPtr checkpointFileSPtr = nullptr;
        NTSTATUS status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr, 
            filePath, 
            allocator, 
            checkpointFileSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return checkpointFileSPtr;
    }

    KUri::CSPtr CheckpointFileTests::CreateStateProviderName(
        __in LONG64 stateProviderId,
        __in KStringView const & string,
        __in KAllocator & allocator)
    {
        NTSTATUS status;
        BOOLEAN isSuccess = TRUE;

        KString::SPtr rootString = nullptr;;
        status = KString::Create(rootString, allocator, string);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KLocalString<20> nameSuffix;
        isSuccess = nameSuffix.FromLONGLONG(stateProviderId);
        CODING_ERROR_ASSERT(isSuccess == TRUE);

        isSuccess = rootString->Concat(nameSuffix);
        CODING_ERROR_ASSERT(isSuccess == TRUE);

        KUri::SPtr output = nullptr;
        status = KUri::Create(*rootString, allocator, output);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return output.RawPtr();
    }

    void CheckpointFileTests::VerifyProperties(
        __in KSharedArray<SerializableMetadata::CSPtr> const & metadataArray,
        __in CheckpointFile & checkpointFile)
    {
        const FABRIC_STATE_PROVIDER_ID EmptyStateProviderId = 0;
        LONG64 expectedRootSPCount = 0;
        for (SerializableMetadata::CSPtr metadata : metadataArray)
        {
            if (metadata->ParentStateProviderId == EmptyStateProviderId)
            {
                expectedRootSPCount++;
            }
        }

        VERIFY_ARE_EQUAL(checkpointFile.PropertiesSPtr->StateProviderCount, metadataArray.Count());
        VERIFY_ARE_EQUAL(checkpointFile.PropertiesSPtr->RootStateProviderCount, expectedRootSPCount);
        VERIFY_ARE_EQUAL(checkpointFile.PropertiesSPtr->PartitionId, expectedPartitionId);
        VERIFY_ARE_EQUAL(checkpointFile.PropertiesSPtr->ReplicaId, expectedReplicaId);
    }

    BOOST_FIXTURE_TEST_SUITE(CheckpointFileTestSuite, CheckpointFileTests)

    BOOST_AUTO_TEST_CASE(Create_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            
            // set up
            #if !defined(PLATFORM_UNIX)
            KWString filePath(allocator, L"\\??\\CheckpointFile.txt");
            #else
            KWString filePath(allocator, L"/tmp/CheckpointFile.txt");
            #endif
            CODING_ERROR_ASSERT(NT_SUCCESS(filePath.Status()));

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(filePath, allocator);
        }
    }

    // WriteReadMetadata test will call WriteMetadata with a metadata which has a initialization parameter with one buffer
    // the ReadMetadata will return the metadata with the same initialization parameter
    BOOST_AUTO_TEST_CASE(WriteReadMetadata_OneSmallMetadata_SUCCESS)
    {
        TestOneMetadataWriteReadMetadata(false);
    }

    // WriteReadMetadata test will call WriteMetadata with a metadata which has a initialization parameter with 512 buffer
    // the ReadMetadata will return the metadata with the same initialization parameter
    BOOST_AUTO_TEST_CASE(WriteReadMetadata_OneSmallMetadata_LargeInitParameter_SUCCESS)
    {
        ULONG32 const bufferCount = 512;
        ULONG32 const bufferSize = 16;
        TestOneMetadataWriteReadMetadata(false, bufferCount, bufferSize);
    }

    // WriteReadMetadata test will call WriteMetadata with a metadata which has null initialization parameter
    // the ReadMetadata will return the metadata with the same initialization parameter
    BOOST_AUTO_TEST_CASE(WriteReadMetadata_OneSmallMetadata_NULLInitParameter_SUCCESS)
    {
        TestOneMetadataWriteReadMetadata(true);
    }

    // CreateAsync with zero items should success
    BOOST_AUTO_TEST_CASE(CreateAsync_ZeroItems_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_ZeroItems_SUCCESS_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);
            CheckpointFile::SPtr checkpointFileSPtr = nullptr;

            try
            {
                checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->WriteAsync(
                    *SerializableMetadataArray, 
                    SerializationMode::Enum::Native, 
                    false, 
                    expectedPrepareCheckpointLSN, 
                    CancellationToken::None));
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
                CODING_ERROR_ASSERT(false);
            }

            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_ZeroItems_SUCCESS_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(CreateAsync_OneMetadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_OneMetadata_SUCCESS_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadata(allocator, false);

            NTSTATUS status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CheckpointFile::SPtr checkpointFileSPtr = nullptr;

            try
            {
                checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->WriteAsync(
                    *SerializableMetadataArray, 
                    SerializationMode::Enum::Native, 
                    false, 
                    expectedPrepareCheckpointLSN, 
                    CancellationToken::None));
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_UNSUCCESSFUL);
                CODING_ERROR_ASSERT(false);
            }

            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_OneMetadata_SUCCESS_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    // Call CreateAsync function with empty file path, it should throw exception.
    BOOST_AUTO_TEST_CASE(CreateAsync_EmptyFilePath_THROW)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName(allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadata(allocator, false);

            NTSTATUS status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            try
            {
                CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->WriteAsync(
                    *SerializableMetadataArray, 
                    SerializationMode::Enum::Native, 
                    false, 
                    expectedPrepareCheckpointLSN, 
                    CancellationToken::None));
            }
            catch (Exception exception)
            {
                return;
            }

            CODING_ERROR_ASSERT(false);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_NotExistFile_THROW)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_Open_NotExistFile_THROW_TEST.txt", allocator);

            try
            {
                CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            }
            catch (Exception exception)
            {
                return;
            }

            CODING_ERROR_ASSERT(false);
        }
    }

    // Open call on a invalid path, in this case, the path is empty, should throw
    BOOST_AUTO_TEST_CASE(Open_EmptyFilePath_THROW)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName(allocator);
            CODING_ERROR_ASSERT(NT_SUCCESS(fileName.Status()));

            try
            {
                CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            }
            catch (Exception exception)
            {
                return;
            }

            CODING_ERROR_ASSERT(false);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_ManagedCheckpointFile_NoItem_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // "Managed_NoItem.sm" is a checkpoint file written by managed code with no item.
            KWString fileName = TestHelper::CreateFileName(L"Managed_NoItem.sm", allocator, TestHelper::FileSource::Managed);
            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                NTSTATUS status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                // No item in the file. Shoule not reach here.
                CODING_ERROR_ASSERT(false);
            }

            SyncAwait(enumerator->CloseAsync());

            // partitionId = {399a345a-23c0-4883-a81b-942a17baf7b4}
            // replicaId = 17
            // stateManagerFile.StateProviderCount = 0
            // stateManagerFile.RootStateProviderCount = 0
            KGuid expectedManagedPId(0x399a345a, 0x23c0, 0x4883, 0xa8, 0x1b, 0x94, 0x2a, 0x17, 0xba, 0xf7, 0xb4);

            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PartitionId, expectedManagedPId);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->ReplicaId, 17);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->RootStateProviderCount, 0);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->StateProviderCount, 0);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_ManagedCheckpointFile_OneItem_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // "Managed_OneItem.sm" is a checkpoint file written by managed code.
            KWString fileName = TestHelper::CreateFileName(L"Managed_OneItem.sm", allocator, TestHelper::FileSource::Managed);
            KUriView expectedName = L"fabric:/StateManagerFileTests/1";
            KStringView expectedTypeString = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

            // Metadata properties:
            //      CreateLsn = 1
            //      DeleteLsn = 1
            //      InitializationContext = null
            //      MetadataMode = Active
            //      Name = {fabric:/StateManagerFileTests/1}
            //      Type = {Name = "TestStateProvider" FullName = "System.Fabric.Replication.Test.TestStateProvider"}
            //      ParentStateProviderId = 0
            //      StateProviderId = 1
            //      TypeString = null

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                NTSTATUS status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(*(metadata->Name), expectedName);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, 1);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, 1);
                VERIFY_ARE_EQUAL(metadata->MetadataMode, MetadataMode::Enum::Active);
                VERIFY_ARE_EQUAL(metadata->StateProviderId, 1);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, 0);
                VERIFY_IS_NULL(metadata->InitializationParameters);
                VERIFY_ARE_EQUAL(*(metadata->TypeString), expectedTypeString);
            }

            SyncAwait(enumerator->CloseAsync());

            // CheckpointFileSPtr properties:
            //      PartitionId = { 96072f6e-fbe2-4539-af79-dcad64463278 }            
            //      replicaId = 17
            //      stateManagerFile.StateProviderCount = 1
            //      stateManagerFile.RootStateProviderCount = 1
            KGuid expectedManagedPId(0x96072f6e, 0xfbe2, 0x4539, 0xaf, 0x79, 0xdc, 0xad, 0x64, 0x46, 0x32, 0x78);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PartitionId, expectedManagedPId);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->ReplicaId, 17);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->RootStateProviderCount, 1);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->StateProviderCount, 1);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_ManagedCheckpointFile_OneItemEmptyInitializationParameters_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // "Managed_OneItemWithInitializationParameters.sm" is a checkpoint file written by managed code.
            KWString fileName = TestHelper::CreateFileName(L"Managed_OneItemEmptyInitializationParameters.sm", allocator, TestHelper::FileSource::Managed);
            KUriView expectedName = L"fabric:/StateManagerFileTests/0";
            KStringView expectedTypeString = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

            // Metadata properties:
            //      CreateLsn = 0
            //      DeleteLsn = 1
            //      InitializationContext = byte[0]
            //      MetadataMode = Active
            //      Name = {fabric:/StateManagerFileTests/0}
            //      Type = { Name = "TestStateProvider" FullName = "System.Fabric.Replication.Test.TestStateProvider" }            
            //      ParentStateProviderId = 0
            //      StateProviderId = 709695431
            //      TypeString = null

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                NTSTATUS status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(*(metadata->Name), expectedName);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, 0);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, 1);
                VERIFY_ARE_EQUAL(metadata->MetadataMode, MetadataMode::Enum::Active);
                VERIFY_ARE_EQUAL(metadata->StateProviderId, 709695431);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, 0);

                VERIFY_IS_NOT_NULL(metadata->InitializationParameters);
                VERIFY_ARE_EQUAL(metadata->InitializationParameters->BufferCount, 0);
                VERIFY_ARE_EQUAL(*(metadata->TypeString), expectedTypeString);
            }

            SyncAwait(enumerator->CloseAsync());

            // CheckpointFileSPtr properties:
            //      partitionId = {db5dd5ce-811c-423c-bfc8-6b10a6b37377}         
            //      replicaId = 17
            //      stateManagerFile.StateProviderCount = 1
            //      stateManagerFile.RootStateProviderCount = 1
            KGuid expectedManagedPId(0xdb5dd5ce, 0x811c, 0x423c, 0xbf, 0xc8, 0x6b, 0x10, 0xa6, 0xb3, 0x73, 0x77);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PartitionId, expectedManagedPId);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->ReplicaId, 17);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->RootStateProviderCount, 1);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->StateProviderCount, 1);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_ManagedCheckpointFile_OneItemWithInitializationParameters_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // "Managed_OneItemWithInitializationParameters.sm" is a checkpoint file written by managed code.
            KWString fileName = TestHelper::CreateFileName(L"Managed_OneItemWithInitializationParameters.sm", allocator, TestHelper::FileSource::Managed);
            KUriView expectedName = L"fabric:/StateManagerFileTests/0";
            KStringView expectedTypeString = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();

            // Metadata properties:
            //      CreateLsn = -1
            //      DeleteLsn = 2
            //      InitializationContext = null
            //      MetadataMode = Active
            //      Name = {fabric:/StateManagerFileTests/0}
            //      Type = {Name = "TestStateProvider" FullName = "System.Fabric.Replication.Test.TestStateProvider"}
            //      ParentStateProviderId = 0
            //      StateProviderId = 1662888584
            //      TypeString = null
            const int count = 8;
            byte managedInitializationParameters[count] = {23,20,179,186,116,75,251,225};

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                NTSTATUS status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(*(metadata->Name), expectedName);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, -1);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, 2);
                VERIFY_ARE_EQUAL(metadata->MetadataMode, MetadataMode::Enum::Active);
                VERIFY_ARE_EQUAL(metadata->StateProviderId, 1662888584);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, 0);
                VERIFY_ARE_EQUAL(*(metadata->TypeString), expectedTypeString);

                KBuffer::SPtr buffer = const_cast<KBuffer *>((*(metadata->InitializationParameters))[0].RawPtr());

                byte * point = static_cast<byte*>(buffer->GetBuffer());

                for (ULONG32 j = 0; j < count; j++)
                {
                    VERIFY_ARE_EQUAL(point[j], managedInitializationParameters[j]);
                }
            }

            SyncAwait(enumerator->CloseAsync());

            // CheckpointFileSPtr properties:
            //      partitionId = {6bbfe379-f58d-4361-9abf-00230c83bafb}            
            //      replicaId = 17
            //      stateManagerFile.StateProviderCount = 1
            //      stateManagerFile.RootStateProviderCount = 1
            KGuid expectedManagedPId(0x6bbfe379, 0xf58d, 0x4361, 0x9a, 0xbf, 0x00, 0x23, 0x0c, 0x83, 0xba, 0xfb);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PartitionId, expectedManagedPId);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->ReplicaId, 17);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->RootStateProviderCount, 1);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->StateProviderCount, 1);
        }
    }

    BOOST_AUTO_TEST_CASE(Open_ManagedCheckpointFile_1024Items_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            // "Managed_1024Items.sm" is a checkpoint file written by managed code.
            KWString fileName = TestHelper::CreateFileName(L"Managed_1024Items.sm", allocator, TestHelper::FileSource::Managed);
            KStringView expectedName = L"fabric:/StateManagerFileTests/";
            KStringView expectedTypeString = L"System.Fabric.Replication.Test.TestStateProvider, System.Fabric.Replicator.Test, Version=6.0.0.0, Culture=neutral, PublicKeyToken=365143bb27e7ac8b";
            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            // Metadata properties:
            //      CreateLsn = 1
            //      DeleteLsn = 1
            //      InitializationContext = null
            //      MetadataMode = Active
            //      Name = {fabric:/StateManagerFileTests/1}
            //      Type = {Name = "TestStateProvider" FullName = "System.Fabric.Replication.Test.TestStateProvider"}
            //      ParentStateProviderId = 0
            //      StateProviderId = 1
            //      TypeString = null
            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            int i = 1;
            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                NTSTATUS status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(*(metadata->Name), *CreateStateProviderName(i, expectedName, allocator));
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);
                VERIFY_ARE_EQUAL(metadata->MetadataMode, MetadataMode::Enum::Active);
                VERIFY_ARE_EQUAL(metadata->StateProviderId, i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, 0);
                VERIFY_IS_NULL(metadata->InitializationParameters);
                VERIFY_ARE_EQUAL(*(metadata->TypeString), expectedTypeString);

                i++;
            }

            SyncAwait(enumerator->CloseAsync());

            // CheckpointFileSPtr properties:
            //      partitionId = {995f9dd6-5cd4-4ba6-8e59-079fbc626645}            
            //      replicaId = 17
            //      stateManagerFile.StateProviderCount = 1
            //      stateManagerFile.RootStateProviderCount = 1
            KGuid expectedManagedPId(0x995f9dd6, 0x5cd4, 0x4ba6, 0x8e, 0x59, 0x07, 0x9f, 0xbc, 0x62, 0x66, 0x45);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PartitionId, expectedManagedPId);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->ReplicaId, 17);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->RootStateProviderCount, 1024);
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->StateProviderCount, 1024);
        }
    }

    BOOST_AUTO_TEST_CASE(CreateAsync_OneMetadata_EmptyInitParameter_Open_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            const FABRIC_STATE_PROVIDER_ID EmptyStateProviderId = 0;

            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_OneMetadata_EmptyInitParameter_Open_SUCCESS.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            KUri::CSPtr expectedNameCSPtr;
            NTSTATUS status = KUri::Create(expectedNameView, allocator, expectedNameCSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KString::SPtr expectedTypeCSPtr;
            status = KString::Create(expectedTypeCSPtr, allocator, TestStateProvider::TypeName);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            OperationData::SPtr initParameters = OperationData::Create(allocator);

            // Setup
            SerializableMetadata::CSPtr serializablemetadataSPtr = nullptr;
            status = SerializableMetadata::Create(
                *expectedNameCSPtr, 
                *expectedTypeCSPtr, 
                expectedStateProviderId, 
                EmptyStateProviderId,
                expectedCreateLSN, 
                expectedDeleteLSN, 
                expectedMetadataMode, 
                allocator, 
                initParameters.RawPtr(), 
                serializablemetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SerializableMetadataArray->Append(serializablemetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            auto enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_NOT_NULL(metadata->InitializationParameters);
                VERIFY_ARE_EQUAL(metadata->InitializationParameters->BufferCount, 0);
                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, EmptyStateProviderId);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, expectedCreateLSN);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, expectedDeleteLSN);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 1);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_OneMetadata_EmptyInitParameter_Open_SUCCESS.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    // Create checkpoint file with one metadata has null initialization parameters,
    // Open the checkpoint file and the return metadata should have null initialization parameters.
    BOOST_AUTO_TEST_CASE(CreateAsync_OneMetadata_NullInitParameter_Open_SUCCESS)
    {
        TestOneMetadataCreateAsyncAndOpenAsync(true);
    }
    
    // Create checkpoint file with one metadata has 16 buffers for initialization parameters,
    // Open the checkpoint file and the return metadata should have the same initialization parameters.
    BOOST_AUTO_TEST_CASE(CreateAsync_OneMetadata_Open_SUCCESS)
    {
        ULONG32 const bufferCount = 16;
        ULONG32 const bufferSize = 16;
        TestOneMetadataCreateAsyncAndOpenAsync(false, bufferCount, bufferSize);
    }

    BOOST_AUTO_TEST_CASE(CreateAsync_OpenTwice_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_OneMetadata_OpenTwice_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr serializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            SerializableMetadata::CSPtr serializableMetadataSPtr = CheckpointFileTests::CreateSerializableMetadata(allocator, false);

            NTSTATUS status = serializableMetadataArray->Append(serializableMetadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *serializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            auto enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, expectedCreateLSN);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, expectedDeleteLSN);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 1);

            SyncAwait(enumerator->CloseAsync());
            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));

            enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, expectedCreateLSN);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, expectedDeleteLSN);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 1);
            this->VerifyProperties(*serializableMetadataArray, *checkpointFileSPtr);

            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_OneMetadata_OpenTwice_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(CreateAsync_MultiMetadata_Open_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            NTSTATUS status;
            KWString fileName = TestHelper::CreateFileName(L"SMF_CreateAsync_MultiMetadata_OpenTest.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            for (ULONG32 i = 0; i < 10; ++i)
            {
                SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            auto enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId + i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId + i);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);
                ++i;
            }

            VERIFY_ARE_EQUAL(i, 10);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_CreateAsync_MultiMetadata_OpenTest.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(Write_32KMetadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            NTSTATUS status;
            KWString fileName = TestHelper::CreateFileName(L"SMF_Write_32KMetadata_SUCCESS_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            for (ULONG32 i = 0; i < 211; ++i)
            {
                SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            auto enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId + i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId + i);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);
                ++i;
            }

            VERIFY_ARE_EQUAL(i, 211);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_Write_32KMetadata_SUCCESS_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(Write_64KMetadata_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            NTSTATUS status;
            KWString fileName = TestHelper::CreateFileName(L"SMF_Write_64KMetadata_SUCCESS_Test.txt", allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            for (ULONG32 i = 0; i < 450; ++i)
            {
                SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);

            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, expectedPrepareCheckpointLSN);

            auto enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId + i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId + i);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 450);
            this->VerifyProperties(*SerializableMetadataArray, *checkpointFileSPtr);

            SyncAwait(enumerator->CloseAsync());

            KString::CSPtr filePath = TestHelper::CreateFileString(allocator, L"SMF_Write_64KMetadata_SUCCESS_Test.txt");
            Common::File::Delete(filePath->operator LPCWSTR(), true);
        }
    }

    //
    // Checkpoint file data corruption test
    //         0           148          156             164          172            342          350        370           378 
    // Stream: |__Metadata__|__Checksum__|__BlockHandle__|__Checksum__|__Properties__|__Checksum__|__Footer__|__Checksum__|
    // 
    //
    
    BOOST_AUTO_TEST_CASE(MetaDataCorruption_Open_THROW)
    {
        DataCorruptionTest(0);
    }

    BOOST_AUTO_TEST_CASE(MetaDataChecksumCorruption_Open_THROW)
    {
        DataCorruptionTest(148);
    }

    BOOST_AUTO_TEST_CASE(BlockHandleCorruption_Open_THROW)
    {
        DataCorruptionTest(156);
    }

    BOOST_AUTO_TEST_CASE(BlockHandleChecksumCorruption_Open_THROW)
    {
        DataCorruptionTest(164);
    }

    BOOST_AUTO_TEST_CASE(PropertiesCorruption_Open_THROW)
    {
        DataCorruptionTest(172);
    }

    BOOST_AUTO_TEST_CASE(PropertiesChecksumCorruption_Open_THROW)
    {
        DataCorruptionTest(342);
    }

    BOOST_AUTO_TEST_CASE(FooterCorruption_Open_THROW)
    {
        DataCorruptionTest(350);
    }

    BOOST_AUTO_TEST_CASE(FooterChecksumCorruption_Open_THROW)
    {
        DataCorruptionTest(370);
    }

    // The checkpoint file had a data corruption, now the partial file exists, OpenAsync call should throw.
    BOOST_AUTO_TEST_CASE(OpenAsync_PartialFile_Throws)
    {
        DataCorruptionPartialFileTest();
    }

    // Current checkpoint file does not exist, but the next checkpoint file corrupts, 
    // when we replace temp file to current file, we open it, now if it corrupts, should throw
    BOOST_AUTO_TEST_CASE(SafeFileReplace_CurrentDoesNotExists_CorruptNext_Throws)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            NTSTATUS status;
            bool isThrow = false;

            // Constants.
            KWString currentFileName = TestHelper::CreateFileName(L"Current.txt", allocator);
            KWString tmpFileName = TestHelper::CreateFileName(L"Tmp.txt", allocator);
            KWString backupFileName = TestHelper::CreateFileName(L"Backup.txt", allocator);

            KString::CSPtr currentFilePath = TestHelper::CreateFileString(allocator, L"Current.txt");
            KString::CSPtr tmpFilePath = TestHelper::CreateFileString(allocator, L"Tmp.txt");
            KString::CSPtr backupFilePath = TestHelper::CreateFileString(allocator, L"Backup.txt");

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

            // Empty file (invalid temporary file). 
            KBlockFile::SPtr FileSPtr = nullptr;
            status = SyncAwait(KBlockFile::CreateSparseFileAsync(
                tmpFileName,
                TRUE,
                KBlockFile::eCreateAlways,
                KBlockFile::eInheritFileSecurity,
                FileSPtr,
                nullptr,
                allocator,
                TEST_TAG));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            FileSPtr->Close();

            // Test
            try
            {
                SyncAwait(CheckpointFile::SafeFileReplaceAsync(
                    *partitionedReplicaIdCSPtr,
                    *currentFilePath,
                    *tmpFilePath,
                    *backupFilePath,
                    CancellationToken::None, 
                    allocator));
            }
            catch (Exception exception)
            {
                isThrow = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INTERNAL_DB_CORRUPTION);
            }

            CODING_ERROR_ASSERT(isThrow);
            // Verification
            bool currentExists = File::Exists(static_cast<LPCWSTR>(*currentFilePath));
            CODING_ERROR_ASSERT(currentExists == false);
            bool backupExists = File::Exists(static_cast<LPCWSTR>(*backupFilePath));
            CODING_ERROR_ASSERT(backupExists == false);
            bool tmpExists = File::Exists(static_cast<LPCWSTR>(*tmpFilePath));
            CODING_ERROR_ASSERT(tmpExists == true);

            OpenAndCloseFileThenDelete(tmpFileName, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(SafeFileReplace_CurrentExists_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            NTSTATUS status;

            // Constants.
            KWString currentFileName = TestHelper::CreateFileName(L"Current.txt", allocator);
            KWString tmpFileName = TestHelper::CreateFileName(L"Tmp.txt", allocator);
            KWString backupFileName = TestHelper::CreateFileName(L"Backup.txt", allocator);

            KString::CSPtr currentFilePath = TestHelper::CreateFileString(allocator, L"Current.txt");
            KString::CSPtr tmpFilePath = TestHelper::CreateFileString(allocator, L"Tmp.txt");
            KString::CSPtr backupFilePath = TestHelper::CreateFileString(allocator, L"Backup.txt");

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            // Create the Current Checkpoint File.
            for (ULONG32 i = 0; i < 10; ++i)
            {
                SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(currentFileName, allocator);
            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));

            // Create the Next CheckpointFile
            for (ULONG32 i = 10; i < 30; ++i)
            {
                SerializableMetadata::CSPtr SerializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = SerializableMetadataArray->Append(SerializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr nextCheckpointFileSPtr = CreateCheckpointFileSPtr(tmpFileName, allocator);
            SyncAwait(nextCheckpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));

            // Test
            SyncAwait(CheckpointFile::SafeFileReplaceAsync(
                *partitionedReplicaIdCSPtr,
                *currentFilePath,
                *tmpFilePath,
                *backupFilePath,
                CancellationToken::None, 
                allocator));

            // Verification
            bool tmpExists = File::Exists(static_cast<LPCWSTR>(*tmpFilePath));
            CODING_ERROR_ASSERT(tmpExists == false);
            bool backupExists = File::Exists(static_cast<LPCWSTR>(*backupFilePath));
            CODING_ERROR_ASSERT(backupExists == false);

            SyncAwait(checkpointFileSPtr->ReadAsync(CancellationToken::None));
            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId + i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId + i);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 30);

            SyncAwait(enumerator->CloseAsync());
            Common::File::Delete(currentFilePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(SafeFileReplace_CurrentDoesNotExists_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            NTSTATUS status;

            // Constants.
            KWString currentFileName = TestHelper::CreateFileName(L"Current.txt", allocator);
            KWString tmpFileName = TestHelper::CreateFileName(L"Tmp.txt", allocator);
            KWString backupFileName = TestHelper::CreateFileName(L"Backup.txt", allocator);

            KString::CSPtr currentFilePath = TestHelper::CreateFileString(allocator, L"Current.txt");
            KString::CSPtr tmpFilePath = TestHelper::CreateFileString(allocator, L"Tmp.txt");
            KString::CSPtr backupFilePath = TestHelper::CreateFileString(allocator, L"Backup.txt");

            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(expectedPartitionId, expectedReplicaId, allocator);

            KSharedArray<SerializableMetadata::CSPtr>::SPtr serializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            // Create the Next CheckpointFile
            for (ULONG32 i = 0; i < 30; ++i)
            {
                SerializableMetadata::CSPtr serializableMetadataSPtr = CreateSerializableMetadataWithID(i, allocator);
                status = serializableMetadataArray->Append(serializableMetadataSPtr);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CheckpointFile::SPtr nextCheckpointFileSPtr = CreateCheckpointFileSPtr(tmpFileName, allocator);
            SyncAwait(nextCheckpointFileSPtr->WriteAsync(
                *serializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));

            // Test
            SyncAwait(CheckpointFile::SafeFileReplaceAsync(
                *partitionedReplicaIdCSPtr,
                *currentFilePath,
                *tmpFilePath,
                *backupFilePath,
                CancellationToken::None, 
                allocator));

            // Verification
            bool tmpExists = File::Exists(static_cast<LPCWSTR>(*tmpFilePath));
            CODING_ERROR_ASSERT(tmpExists == false);
            bool backupExists = File::Exists(static_cast<LPCWSTR>(*backupFilePath));
            CODING_ERROR_ASSERT(backupExists == false);

            CheckpointFile::SPtr checkpointFileReopenSPtr = CreateCheckpointFileSPtr(currentFileName, allocator);
            SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));

            auto enumerator = checkpointFileReopenSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr metadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, metadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_ARE_EQUAL(metadata->StateProviderId, expectedStateProviderId + i);
                VERIFY_ARE_EQUAL(metadata->ParentStateProviderId, expectedParentId + i);
                VERIFY_ARE_EQUAL(metadata->CreateLSN, i);
                VERIFY_ARE_EQUAL(metadata->DeleteLSN, i);

                ++i;
            }

            VERIFY_ARE_EQUAL(i, 30);

            SyncAwait(enumerator->CloseAsync());
            Common::File::Delete(currentFilePath->operator LPCWSTR(), true);
        }
    }

    BOOST_AUTO_TEST_CASE(GetCopyData_NoData_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            CheckpointFileTests::Test_GetCopyData_VerifyCopyItems(0, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(GetCopyData_OneItem_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            CheckpointFileTests::Test_GetCopyData_VerifyCopyItems(1, allocator);
        }
    }

    BOOST_AUTO_TEST_CASE(GetCopyData_MultipleItem_SUCCESS)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();
            CheckpointFileTests::Test_GetCopyData_VerifyCopyItems(128, allocator);
        }
    }

    // CreateAsync call with CancellationTokenSource is canceled, should throw.
    BOOST_AUTO_TEST_CASE(CreateAsync_WithCanceledToken_Throws)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"CreateAsync_WithCanceledToken_Throws_Test.txt", allocator);

            bool isThrow = false;
            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            CancellationTokenSource::SPtr cts;
            NTSTATUS status = CancellationTokenSource::Create(allocator, TEST_TAG, cts);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            cts->Cancel();

            try
            {
                CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
                SyncAwait(checkpointFileSPtr->WriteAsync(
                    *SerializableMetadataArray, 
                    SerializationMode::Enum::Native, 
                    false, 
                    expectedPrepareCheckpointLSN, 
                    cts->Token));
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_CANCELLED);
                isThrow = true;
            }

            CODING_ERROR_ASSERT(isThrow);
            OpenAndCloseFileThenDelete(fileName, allocator);
        }
    }

    // OpenAsync call with CancellationTokenSource is canceled, should throw.
    BOOST_AUTO_TEST_CASE(OpenAsync_WithCanceledToken_Throws)
    {
        KtlSystem* underlyingSystem = TestHelper::CreateKtlSystem();
        KFinally([&]() { KtlSystem::Shutdown(); });
        {
            KAllocator& allocator = underlyingSystem->PagedAllocator();

            KWString fileName = TestHelper::CreateFileName(L"OpenAsync_WithCanceledToken_Throws_Test.txt", allocator);

            bool isThrow = false;
            KSharedArray<SerializableMetadata::CSPtr>::SPtr SerializableMetadataArray = TestHelper::CreateSerializableMetadataArray(allocator);

            CheckpointFile::SPtr checkpointFileSPtr = CreateCheckpointFileSPtr(fileName, allocator);
            SyncAwait(checkpointFileSPtr->WriteAsync(
                *SerializableMetadataArray, 
                SerializationMode::Enum::Native, 
                false, 
                expectedPrepareCheckpointLSN, 
                CancellationToken::None));

            CancellationTokenSource::SPtr cts;
            NTSTATUS status = CancellationTokenSource::Create(allocator, TEST_TAG, cts);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            cts->Cancel();

            try
            {
                SyncAwait(checkpointFileSPtr->ReadAsync(cts->Token));
            }
            catch (Exception & exception)
            {
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_CANCELLED);
                isThrow = true;
            }

            CODING_ERROR_ASSERT(isThrow);
            OpenAndCloseFileThenDelete(fileName, allocator);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
