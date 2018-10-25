// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "TStoreTestBase.h"

#define ALLOC_TAG 'lsTP'

namespace TStoreTests
{
    using namespace ktl;
    using namespace Data::Utilities;

    class UpgradeTest : public TStoreTestBase<LONG64, KString::SPtr, LongComparer, TestStateSerializer<LONG64>, NullableStringStateSerializer>
    {
    public:
        UpgradeTest()
        {
            SetupKtl();
            // Tests should call Setup explicitly since each test likely wants to behave differently
        }

        ~UpgradeTest()
        {
            Cleanup(ShouldRemoveStateOnCleanup);
        }

        __declspec(property(get = get_ShouldRemoveState, put = set_ShouldRemoveState)) bool ShouldRemoveStateOnCleanup;
        bool get_ShouldRemoveState()
        {
            return shouldRemoveState_;
        }

        void set_ShouldRemoveState(bool removeState)
        {
            shouldRemoveState_ = removeState;
        }

        KString::SPtr GenerateStringValue(__in const WCHAR* str)
        {
            KString::SPtr value;
            auto status = KString::Create(value, GetAllocator(), str);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        KString::SPtr GenerateStringValue(__in LONG64 seed)
        {
            KString::SPtr value;
            wstring str = wstring(L"value ") + to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        void ForceCreateDirectory(__in wstring relativePath)
        {
            wstring currentDirectory = Common::Directory::GetCurrentDirectoryW();
#ifdef PLATFORM_UNIX
            // #10358503: Temporary fix for running StateManager tests on Linux with RunTests
            wstring linuxFileModifier(L"/data_upgrade/5.1/NativeStore");
            if (Common::Directory::Exists(currentDirectory + linuxFileModifier))
            {
                currentDirectory += linuxFileModifier;
            }
            else
            {
                currentDirectory = currentDirectory + L"/.." + linuxFileModifier;
            }
#else
            wstring windowsFileModifier(L"\\UpgradeTests\\NativeStore");
            currentDirectory += windowsFileModifier;
#endif
            wstring absolutePath = Common::Path::Combine(currentDirectory, relativePath);

            if (Common::Directory::Exists(absolutePath))
            {
                Common::Directory::Delete(absolutePath, true);
            }

            Common::Directory::Create(absolutePath);
        }

        KString::SPtr NumberToString(__in LONG64 seed)
        {
            KString::SPtr value;
            wstring str = to_wstring(seed);
            auto status = KString::Create(value, GetAllocator(), str.c_str());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            return value;
        }

        ktl::Awaitable<void> OpenFileAsync(
            __in KStringView const & name,
            __in KBlockFile::CreateDisposition mode,
            __in ktl::io::KFileStream & filestream,
            __out KBlockFile::SPtr & fileSPtr)
        {
            KString::CSPtr fileName = CreateUpgradeFilepath(name);
            KWString filePath(GetAllocator(), *fileName);

            KBlockFile::CreateOptions createOptions = static_cast<KBlockFile::CreateOptions>(
                KBlockFile::CreateOptions::eInheritFileSecurity |
                KBlockFile::CreateOptions::eShareRead |
                KBlockFile::CreateOptions::eShareWrite);

            NTSTATUS status = co_await KBlockFile::CreateSparseFileAsync(
                filePath,
                TRUE,
                mode,
                createOptions,
                fileSPtr,
                nullptr,
                GetAllocator());

            Diagnostics::Validate(status);

            status = co_await filestream.OpenAsync(*fileSPtr);
            Diagnostics::Validate(status);
        }

        ktl::Awaitable<KBuffer::SPtr> GetFileBytesAsync(__in ktl::io::KFileStream & filestream)
        {
            LONG64 fileSize = filestream.GetLength();

            KBuffer::SPtr buffer = nullptr;
            KBuffer::Create(static_cast<ULONG>(fileSize), buffer, GetAllocator());

            ULONG bytesRead = 0;
            NTSTATUS status = co_await filestream.ReadAsync(*buffer, bytesRead);
            Diagnostics::Validate(status);
            CODING_ERROR_ASSERT(bytesRead == fileSize);

            co_return buffer;
        }

        ktl::Awaitable<KBuffer::SPtr> GetFileBytesAsync(__in KStringView const & name)
        {
            KBlockFile::SPtr fileSPtr = nullptr;
            ktl::io::KFileStream::SPtr streamSPtr = nullptr;
            ktl::io::KFileStream::Create(streamSPtr, GetAllocator());
            co_await OpenFileAsync(name, KBlockFile::CreateDisposition::eOpenExisting, *streamSPtr, fileSPtr);

            KBuffer::SPtr fileBytes = co_await GetFileBytesAsync(*streamSPtr);

            co_await streamSPtr->CloseAsync();
            fileSPtr->Close();

            co_return fileBytes;
        }

        ktl::Awaitable<void> WriteFileBytesAsync(
            __in ktl::io::KFileStream & filestream,
            __in KBuffer & bytes)
        {
            NTSTATUS status = co_await filestream.WriteAsync(bytes);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        ktl::Awaitable<void> WriteFileBytesAsync(
            __in KStringView const & name,
            __in KBuffer & bytes)
        {
            KBlockFile::SPtr fileSPtr = nullptr;
            ktl::io::KFileStream::SPtr streamSPtr = nullptr;
            ktl::io::KFileStream::Create(streamSPtr, GetAllocator());
            co_await OpenFileAsync(name, KBlockFile::CreateDisposition::eCreateAlways, *streamSPtr, fileSPtr);

            co_await WriteFileBytesAsync(*streamSPtr, bytes);

            co_await streamSPtr->CloseAsync();
            fileSPtr->Close();
        }

        ktl::Awaitable<void> SerializeAddOperationAsync(LONG64 txnId, wstring directory, LONG64 key, KString::SPtr value)
        {
            auto keyBytes = Store->GetKeyBytes(key);
            auto valueBytes = Store->GetValueBytes(value);

            MetadataOperationDataKV<LONG64, KString::SPtr>::CSPtr metadataSPtr = nullptr;
            NTSTATUS status = MetadataOperationDataKV<LONG64, KString::SPtr>::Create(key, value, Constants::SerializedVersion, StoreModificationType::Add, txnId, keyBytes, GetAllocator(), metadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                wstring metadataFilename = Common::Path::Combine(directory, L"metadata.metadata");
                KString::SPtr metadataFilenameSPtr = nullptr;
                KString::Create(metadataFilenameSPtr, GetAllocator(), metadataFilename.c_str());
                KBuffer::CSPtr meta = (*metadataSPtr)[0];
                co_await WriteFileBytesAsync(*metadataFilenameSPtr, const_cast<KBuffer &>(*meta));

                wstring dataFilename = Common::Path::Combine(directory, L"metadata.data");
                KString::SPtr dataFilenameSPtr = nullptr;
                KString::Create(dataFilenameSPtr, GetAllocator(), dataFilename.c_str());
                KBuffer::CSPtr data = (*metadataSPtr)[1];
                co_await WriteFileBytesAsync(*dataFilenameSPtr, const_cast<KBuffer &>(*data));
            }

            RedoUndoOperationData::SPtr redoUndoDataSPtr = nullptr;
            status = RedoUndoOperationData::Create(GetAllocator(), valueBytes, nullptr, redoUndoDataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                wstring metadataFilename = Common::Path::Combine(directory, L"data.metadata");
                KBuffer::CSPtr meta = (*redoUndoDataSPtr)[0];
                co_await WriteFileBytesAsync(metadataFilename.c_str(), const_cast<KBuffer &>(*meta));

                wstring dataFilename = Common::Path::Combine(directory, L"data.data");
                KBuffer::CSPtr data = (*redoUndoDataSPtr)[1];
                co_await WriteFileBytesAsync(dataFilename.c_str(), const_cast<KBuffer &>(*data));
            }
        }

        ktl::Awaitable<void> SerializeUndoAddOperationAsync(LONG64 txnId, wstring directory, LONG64 key)
        {
            auto keyBytes = Store->GetKeyBytes(key);

            MetadataOperationDataK<LONG64>::CSPtr metadataSPtr = nullptr;
            NTSTATUS status = MetadataOperationDataK<LONG64>::Create(key, Constants::SerializedVersion, StoreModificationType::Add, txnId, keyBytes, GetAllocator(), metadataSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            {
                wstring metadataFilename = Common::Path::Combine(directory, L"metadata.metadata");
                KString::SPtr metadataFilenameSPtr = nullptr;
                KString::Create(metadataFilenameSPtr, GetAllocator(), metadataFilename.c_str());
                KBuffer::CSPtr meta = (*metadataSPtr)[0];
                co_await WriteFileBytesAsync(*metadataFilenameSPtr, const_cast<KBuffer &>(*meta));

                wstring dataFilename = Common::Path::Combine(directory, L"metadata.data");
                KString::SPtr dataFilenameSPtr = nullptr;
                KString::Create(dataFilenameSPtr, GetAllocator(), dataFilename.c_str());
                KBuffer::CSPtr data = (*metadataSPtr)[1];
                co_await WriteFileBytesAsync(*dataFilenameSPtr, const_cast<KBuffer &>(*data));
            }
        }

        ktl::Awaitable<OperationData::SPtr> OpenMetadata(__in KStringView const & directoryPath)
        {
            // Add phase
            // Create metadata operation data
            OperationData::SPtr metadata = OperationData::Create(GetAllocator());

            KString::SPtr metadataPath = KPath::Combine(directoryPath, L"metadata.metadata", GetAllocator());
            KString::SPtr dataPath = KPath::Combine(directoryPath, L"metadata.data", GetAllocator());
            
            auto metadataBytes = co_await GetFileBytesAsync(*metadataPath);
            auto dataBytes = co_await GetFileBytesAsync(*dataPath);

            metadata->Append(*metadataBytes);
            metadata->Append(*dataBytes);
            
            co_return metadata.RawPtr();
        }

        ktl::Awaitable<OperationData::SPtr> OpenData(__in KStringView const & directoryPath)
        {
            // Add phase
            // Create metadata operation data
            OperationData::SPtr data = OperationData::Create(GetAllocator());

            KString::SPtr metadataPath = KPath::Combine(directoryPath, L"data.metadata", GetAllocator());
            KString::SPtr dataPath = KPath::Combine(directoryPath, L"data.data", GetAllocator());
            
            auto metadataBytes = co_await GetFileBytesAsync(*metadataPath);
            auto dataBytes = co_await GetFileBytesAsync(*dataPath);

            data->Append(*metadataBytes);
            data->Append(*dataBytes);
            
            co_return data.RawPtr();
        }


        KString::CSPtr CreateUpgradeFilepath(__in KStringView const & name)
        {
            wstring currentDirectory = Common::Directory::GetCurrentDirectoryW();
#ifdef PLATFORM_UNIX
            // #10358503: Temporary fix for running StateManager tests on Linux with RunTests
            wstring linuxFileModifier(L"/data_upgrade/5.1/NativeStore");
            if (Common::Directory::Exists(currentDirectory + linuxFileModifier))
            {
                currentDirectory += linuxFileModifier;
            }
            else
            {
                currentDirectory = currentDirectory + L"/.." + linuxFileModifier;
            }
#else
            wstring windowsFileModifier(L"\\UpgradeTests\\NativeStore");
            currentDirectory += windowsFileModifier;
#endif

            KString::SPtr path = KPath::CreatePath(currentDirectory.c_str(), GetAllocator());
            KPath::CombineInPlace(*path, name);
            
            return path.RawPtr();
        }



        static bool EqualityFunction(KString::SPtr & one, KString::SPtr & two)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            return one->Compare(*two) == 0;
        }

    private:
        bool shouldRemoveState_ = true;

        Common::CommonConfig config; // load the config object as it's needed for the tracing to work

#pragma region test functions
    public:
        ktl::Awaitable<void> ManagedCheckpoint_SingleAdd_ShouldSucceed_Test()
        {
            KString::SPtr directoryPath = KPath::Combine(L"managed_checkpoint", L"add", GetAllocator());
            KString::CSPtr workDirectory = CreateUpgradeFilepath(*directoryPath);
            Setup(1, DefaultHash, workDirectory);

            CODING_ERROR_ASSERT(Store->Count == 1);

            LONG64 key = 17;
            KString::SPtr expectedValue = GenerateStringValue(L"value");

            co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, UpgradeTest::EqualityFunction);

            // Don't delete the working directory
            this->ShouldRemoveStateOnCleanup = false;
            co_return;
        }

        ktl::Awaitable<void> ManagedCheckpoint_1000Adds_10Checkpoints_ShouldSucceed_Test()
        {
            KString::CSPtr workDirectory = CreateUpgradeFilepath(L"managed_multi_checkpoint");
            Setup(1, DefaultHash, workDirectory);

            CODING_ERROR_ASSERT(Store->Count == 1000);

            for (LONG64 key = 0; key < 1000; key++)
            {
                KString::SPtr expectedValue = GenerateStringValue(key);
                co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, UpgradeTest::EqualityFunction);
            }

            this->ShouldRemoveStateOnCleanup = false;
            co_return;
        }

        ktl::Awaitable<void> ManagedCheckpoint_1000AddsThen1000Updates_2Checkpoints_ShouldSucceed_Test()
        {
            KString::SPtr directoryPath = KPath::Combine(L"managed_checkpoint", L"addUpdate", GetAllocator());
            KString::CSPtr workDirectory = CreateUpgradeFilepath(*directoryPath);
            Setup(1, DefaultHash, workDirectory);

            CODING_ERROR_ASSERT(Store->Count == 1000);

            for (LONG64 key = 0; key < 1000; key++)
            {
                KString::SPtr expectedValue = NumberToString(key + 1);
                co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, UpgradeTest::EqualityFunction);
            }

            this->ShouldRemoveStateOnCleanup = false;
            co_return;
        }

        ktl::Awaitable<void> ManagedCheckpoint_1000AddsThen1000UpdatesThen1000Removes_3Checkpoints_ShouldSucceed_Test()
        {
            KString::SPtr directoryPath = KPath::Combine(L"managed_checkpoint", L"addUpdateRemove", GetAllocator());
            KString::CSPtr workDirectory = CreateUpgradeFilepath(*directoryPath);
            Setup(1, DefaultHash, workDirectory);

            CODING_ERROR_ASSERT(Store->Count == 0);
            this->ShouldRemoveStateOnCleanup = false;
            co_return;
        }

        ktl::Awaitable<void> ManagedAddThenUpdateThenRemoveOperation_ShouldSucceed_Test()
        {
            Setup(1);

            KString::SPtr addDirectoryPath = KPath::Combine(L"managed_replicated_ops", L"add", GetAllocator());
            KString::SPtr updateDirectoryPath = KPath::Combine(L"managed_replicated_ops", L"update", GetAllocator());
            KString::SPtr removeDirectoryPath = KPath::Combine(L"managed_replicated_ops", L"remove", GetAllocator());

            // Add phase
            OperationData::SPtr addMetadata = co_await OpenMetadata(*addDirectoryPath);
            OperationData::SPtr addData = co_await OpenData(*addDirectoryPath);

            {
                auto replicatorTransaction1 = CreateReplicatorTransaction();
                replicatorTransaction1->CommitSequenceNumber = 1;

                co_await Store->ApplyAsync(1, *replicatorTransaction1, TxnReplicator::ApplyContext::SecondaryRedo, addMetadata.RawPtr(), addData.RawPtr());
                replicatorTransaction1->Dispose();
            }

            auto key = 17;
            auto expectedValue1 = GenerateStringValue(L"add");
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue1, EqualityFunction);

            // Update phase
            OperationData::SPtr updateMetadata = co_await OpenMetadata(*updateDirectoryPath);
            OperationData::SPtr updateData = co_await OpenData(*updateDirectoryPath);

            {
                auto replicatorTransaction2 = CreateReplicatorTransaction();
                replicatorTransaction2->CommitSequenceNumber = 2;

                co_await Store->ApplyAsync(2, *replicatorTransaction2, TxnReplicator::ApplyContext::SecondaryRedo, updateMetadata.RawPtr(), updateData.RawPtr());
                replicatorTransaction2->Dispose();
            }

            auto expectedValue2 = GenerateStringValue(L"update");
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue2, EqualityFunction);

            // Remove Phase
            OperationData::SPtr removeMetadata = co_await OpenMetadata(*removeDirectoryPath);
            RedoUndoOperationData::SPtr removeData = nullptr;
            RedoUndoOperationData::Create(GetAllocator(), nullptr, nullptr, removeData);

            {
                auto replicatorTransaction3 = CreateReplicatorTransaction();
                replicatorTransaction3->CommitSequenceNumber = 3;

                co_await Store->ApplyAsync(3, *replicatorTransaction3, TxnReplicator::ApplyContext::SecondaryRedo, removeMetadata.RawPtr(), removeData.RawPtr());
                replicatorTransaction3->Dispose();
            }

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> ManagedAdd_FalseProgress_ShouldSucceed_Test()
        {
            Setup(1);

            KString::SPtr addDirectoryPath = KPath::Combine(L"managed_replicated_ops", L"add", GetAllocator());
            KString::SPtr undoDirectoryPath = KPath::Combine(L"managed_replicated_ops", L"undo_add", GetAllocator());

            // Add phase
            OperationData::SPtr addMetadata = co_await OpenMetadata(*addDirectoryPath);
            OperationData::SPtr addData = co_await OpenData(*addDirectoryPath);

            // Add phase
            {
                auto replicatorTransaction1 = CreateReplicatorTransaction();
                replicatorTransaction1->CommitSequenceNumber = 1;

                co_await Store->ApplyAsync(1, *replicatorTransaction1, TxnReplicator::ApplyContext::SecondaryRedo, addMetadata.RawPtr(), addData.RawPtr());
                replicatorTransaction1->Dispose();
            }

            auto key = 17;
            auto expectedValue1 = GenerateStringValue(L"add");
            co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue1, EqualityFunction);

            // Undo phase
            // Create metadata operation data
            OperationData::SPtr undoMetadata = co_await OpenMetadata(*undoDirectoryPath);

            auto replicatorTransaction2 = CreateReplicatorTransaction();
            replicatorTransaction2->CommitSequenceNumber = 1;

            co_await Store->ApplyAsync(1, *replicatorTransaction2, TxnReplicator::ApplyContext::SecondaryFalseProgress, undoMetadata.RawPtr(), nullptr);
            replicatorTransaction2->Dispose();

            co_await VerifyKeyDoesNotExistAsync(*Store, key);
            co_return;
        }

        ktl::Awaitable<void> ManagedCopy_100Adds_ShouldSucceed_Test()
        {
            Setup(1);

            wstring copyDataDirectory = Common::Path::Combine(L"managed_copy", L"add"); 

            co_await Store->BeginSettingCurrentStateAsync();

            const LONG64 numCopyOperations = 7;

            for (LONG64 stateRecordNumber = 0; stateRecordNumber < numCopyOperations; stateRecordNumber++)
            {
                KString::SPtr filename;
                wstring str = Common::Path::Combine(copyDataDirectory, to_wstring(stateRecordNumber));
                KString::Create(filename, GetAllocator(), str.c_str());

                KBuffer::SPtr fileDataBuffer = co_await GetFileBytesAsync(*filename);

                OperationData::SPtr operationDataSPtr = OperationData::Create(GetAllocator());
                operationDataSPtr->Append(*fileDataBuffer);

                co_await Store->SetCurrentStateAsync(stateRecordNumber, *operationDataSPtr, ktl::CancellationToken::None);
            }

            co_await Store->EndSettingCurrentStateAsync(ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(Store->Count == 100);

            for (LONG64 key = 0; key < 100; key++)
            {
                auto expectedValue = NumberToString(key);
                co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> ManagedCopy_100AddsThen100Updates_ShouldSucceed_Test()
        {
            Setup(1);

            wstring copyDataDirectory = Common::Path::Combine(L"managed_copy", L"addUpdate");

            co_await Store->BeginSettingCurrentStateAsync();

            const LONG64 numCopyOperations = 11;

            for (LONG64 stateRecordNumber = 0; stateRecordNumber < numCopyOperations; stateRecordNumber++)
            {
                KString::SPtr filename;
                wstring str = Common::Path::Combine(copyDataDirectory, to_wstring(stateRecordNumber));
                KString::Create(filename, GetAllocator(), str.c_str());

                KBuffer::SPtr fileDataBuffer = co_await GetFileBytesAsync(*filename);

                OperationData::SPtr operationDataSPtr = OperationData::Create(GetAllocator());
                operationDataSPtr->Append(*fileDataBuffer);

                co_await Store->SetCurrentStateAsync(stateRecordNumber, *operationDataSPtr, ktl::CancellationToken::None);
            }

            co_await Store->EndSettingCurrentStateAsync(ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(Store->Count == 100);

            for (LONG64 key = 0; key < 100; key++)
            {
                auto expectedValue = NumberToString(key + 1);
                co_await VerifyKeyExistsAsync(*Store, key, nullptr, expectedValue, EqualityFunction);
            }
            co_return;
        }

        ktl::Awaitable<void> ManagedCopy_100AddsThen100UpdatesThen100Removes_ShouldSucceed_Test()
        {
            Setup(1);

            wstring copyDataDirectory = Common::Path::Combine(L"managed_copy", L"addUpdateRemove");

            co_await Store->BeginSettingCurrentStateAsync();

            const LONG64 numCopyOperations = 15;

            for (LONG64 stateRecordNumber = 0; stateRecordNumber < numCopyOperations; stateRecordNumber++)
            {
                KString::SPtr filename;
                wstring str = Common::Path::Combine(copyDataDirectory, to_wstring(stateRecordNumber));
                KString::Create(filename, GetAllocator(), str.c_str());

                KBuffer::SPtr fileDataBuffer = co_await GetFileBytesAsync(*filename);

                OperationData::SPtr operationDataSPtr = OperationData::Create(GetAllocator());
                operationDataSPtr->Append(*fileDataBuffer);

                co_await Store->SetCurrentStateAsync(stateRecordNumber, *operationDataSPtr, ktl::CancellationToken::None);
            }

            co_await Store->EndSettingCurrentStateAsync(ktl::CancellationToken::None);
            CODING_ERROR_ASSERT(Store->Count == 0);
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(UpgradeTestSuite, UpgradeTest)

    // Steps to run test:
    // 1. Build  %SRCROOT%\external\Microsoft.ServiceFabric.Upgrade.Test.Internal
    // 2.     a. Run these tests from FabricUnitTests folder
    //    or, b. In the VS debugger settings, set the working directory to the FabricUnitTests folder

    // Uncomment to generate test data
    //BOOST_AUTO_TEST_CASE(SerializeOperations)
    //{
    //    Setup(1);

    //    ForceCreateDirectory(L"native_replicated");
    //    ForceCreateDirectory(L"native_replicated\\add");
    //    ForceCreateDirectory(L"native_replicated\\undo_add");

    //    {
    //        auto txn = CreateWriteTransaction();

    //        LONG64 key = 17;
    //        KString::SPtr value = GenerateStringValue(L"add");

    //        SyncAwait(SerializeAddOperationAsync(txn->TransactionSPtr->TransactionId, L"native_replicated\\add", key, value));
    //        SyncAwait(SerializeUndoAddOperationAsync(txn->TransactionSPtr->TransactionId, L"native_replicated\\undo_add", key));
    //    }
    //}

    BOOST_AUTO_TEST_CASE(ManagedCheckpoint_SingleAdd_ShouldSucceed)
    {
        SyncAwait(ManagedCheckpoint_SingleAdd_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedCheckpoint_1000Adds_10Checkpoints_ShouldSucceed)
    {
        SyncAwait(ManagedCheckpoint_1000Adds_10Checkpoints_ShouldSucceed_Test());
    }


    BOOST_AUTO_TEST_CASE(ManagedCheckpoint_1000AddsThen1000Updates_2Checkpoints_ShouldSucceed)
    {
        SyncAwait(ManagedCheckpoint_1000AddsThen1000Updates_2Checkpoints_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedCheckpoint_1000AddsThen1000UpdatesThen1000Removes_3Checkpoints_ShouldSucceed)
    {
        SyncAwait(ManagedCheckpoint_1000AddsThen1000UpdatesThen1000Removes_3Checkpoints_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedAddThenUpdateThenRemoveOperation_ShouldSucceed)
    {
        SyncAwait(ManagedAddThenUpdateThenRemoveOperation_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedAdd_FalseProgress_ShouldSucceed)
    {
        SyncAwait(ManagedAdd_FalseProgress_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedCopy_100Adds_ShouldSucceed)
    {
        SyncAwait(ManagedCopy_100Adds_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedCopy_100AddsThen100Updates_ShouldSucceed)
    {
        SyncAwait(ManagedCopy_100AddsThen100Updates_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ManagedCopy_100AddsThen100UpdatesThen100Removes_ShouldSucceed)
    {
        SyncAwait(ManagedCopy_100AddsThen100UpdatesThen100Removes_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_SUITE_END();
}
