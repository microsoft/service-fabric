// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class CheckpointManagerTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:

        enum Enum
        {
            AllActive = 0,

            AllDeleted = 1,

            Both = 2,
        };

        KWString CreateFileName(__in KStringView const & name, __in KAllocator & allocator)
        {
            KWString fileStreamName(allocator, name);

            WCHAR currentDirectoryPathCharArray[MAX_PATH];
            GetCurrentDirectory(MAX_PATH, currentDirectoryPathCharArray);

            BOOL result = PathAppendW(currentDirectoryPathCharArray, fileStreamName);
            VERIFY_IS_TRUE(result == 1);

            KWString fileName(allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
            VERIFY_IS_TRUE(NT_SUCCESS(fileName.Status()));

            KWString currentFilePath(allocator, currentDirectoryPathCharArray);
            VERIFY_IS_TRUE(NT_SUCCESS(currentFilePath.Status()));

            fileName += currentFilePath;

            return fileName;
        }

        MetadataManager::SPtr CreateMetadataManagerAndPopulateMetadatas(__in Enum testMode, __in ULONG metadataCount, __out KSharedArray<Metadata::CSPtr>::SPtr & outArray)
        {         
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
            outArray = TestHelper::CreateMetadataArray(GetAllocator());

            for (LONG64 i = 0; i < metadataCount; i++)
            {
                KUri::CSPtr expectedName;
                wchar_t number = static_cast<wchar_t>(i);

                auto status = KUri::Create(KUriView(&number), GetAllocator(), expectedName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                auto metadata = TestHelper::CreateMetadata(*expectedName, false, i+1, GetAllocator());

                bool addActive = (Enum::AllActive == testMode);

                if (testMode == Enum::Both)
                {
                    addActive = (i % 2 == 1);
                }

                if (addActive)
                {
                    bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
                    VERIFY_IS_TRUE(isAdded == true);
                }
                else
                {
                    metadata->MetadataMode = MetadataMode::DelayDelete;
                    bool isAdded = metadataManagerSPtr->AddDeleted(i+1, *metadata);
                    VERIFY_IS_TRUE(isAdded == true);
                }

                outArray->Append(metadata.RawPtr());
            }

            return metadataManagerSPtr;
        }
        
        void Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(
            __in ULONG metadataCount, 
            __in Enum testMode, 
            __in FABRIC_STATE_PROVIDER_ID checkpointLSN,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_, 
                *stateProviderFactorySPtr_, 
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode, 
                GetAllocator(), 
                checkpointManagerSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManager = CreateMetadataManagerAndPopulateMetadatas(testMode, metadataCount, metadataArray);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManager, checkpointLSN);

            // Test
            ULONG expectedActiveMetadataCount = 0;
            ULONG expectedDeletedMetadataCount = 0;

            if (testMode == Enum::AllActive)
            {
                expectedActiveMetadataCount = metadataCount;
            }
            else if (testMode == Enum::AllDeleted)
            {
                expectedDeletedMetadataCount = metadataCount;
            }
            else
            {
                expectedActiveMetadataCount = metadataCount / 2;
                expectedDeletedMetadataCount = metadataCount - expectedActiveMetadataCount;
            }

            KArray<Metadata::CSPtr> activeResult = checkpointManagerSPtr->GetPreparedActiveOrDeletedList(MetadataMode::Active);
            VERIFY_ARE_EQUAL(activeResult.Count(), expectedActiveMetadataCount);
            for (ULONG i = 0; i < expectedActiveMetadataCount; i++)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>(activeResult[i]->StateProviderId - 1);
                TestHelper::VerifyMetadata(activeResult[i].RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
            }

            KArray<Metadata::CSPtr> deletedResult = checkpointManagerSPtr->GetPreparedActiveOrDeletedList(MetadataMode::DelayDelete);
            VERIFY_ARE_EQUAL(deletedResult.Count(), expectedDeletedMetadataCount);
            for (ULONG i = 0; i < expectedDeletedMetadataCount; i++)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>(deletedResult[i]->StateProviderId - 1);
                TestHelper::VerifyMetadata(deletedResult[i].RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
            }

            VERIFY_ARE_EQUAL(checkpointManagerSPtr->PrepareCheckpointLSN, checkpointLSN);

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        void Test_PerformCheckpointAsync_OneMetadata_SUCCESS(
            __in LONG64 checkpointLSN,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_,
                *stateProviderFactorySPtr_,
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode, 
                GetAllocator(), 
                checkpointManagerSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
            KUri::CSPtr expectedName;
            status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto metadata = TestHelper::CreateMetadata(*expectedName, false, GetAllocator());
            bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
            VERIFY_IS_TRUE(isAdded);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

            // Test
            auto tempCheckpointFile = KPath::Combine(*replicaFolderPath, TempCheckpointPrefix, GetAllocator());

            CheckpointFile::SPtr checkpointFileReopenSPtr = nullptr;
            status = CheckpointFile::Create(
                *partitionedReplicaIdCSPtr_, 
                KWString(GetAllocator(), *tempCheckpointFile), 
                GetAllocator(),
                checkpointFileReopenSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));

            CheckpointFileAsyncEnumerator::SPtr enumerator = checkpointFileReopenSPtr->GetAsyncEnumerator();
            ULONG32 i = 0;

            while (true)
            {
                SerializableMetadata::CSPtr serializableMetadata = nullptr;
                status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, serializableMetadata));
                if (status == STATUS_NOT_FOUND)
                {
                    break;
                }

                VERIFY_IS_TRUE(NT_SUCCESS(status));

                VERIFY_IS_TRUE(serializableMetadata->StateProviderId == metadata->StateProviderId);
                VERIFY_IS_TRUE(serializableMetadata->ParentStateProviderId == metadata->ParentId);
                VERIFY_IS_TRUE(serializableMetadata->CreateLSN == metadata->CreateLSN);
                VERIFY_IS_TRUE(serializableMetadata->DeleteLSN == metadata->DeleteLSN);

                ++i;
            }

            VERIFY_IS_TRUE(i == 1);

            SyncAwait(enumerator->CloseAsync());

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        void Test_RecoverCheckpointAsync_MultipleMetadata_SUCCESS(
            __in ULONG metadataCount, 
            __in LONG64 checkpointLSN,
            __in Enum testMode,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_,
                *stateProviderFactorySPtr_,
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(), 
                checkpointManagerSPtr); 
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManager = CreateMetadataManagerAndPopulateMetadatas(testMode, metadataCount, metadataArray);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManager, checkpointLSN);
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManager, CancellationToken::None));
            SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManager, CancellationToken::None));

            CheckpointManager::SPtr recoverCheckpointManagerSPtr = nullptr;
            status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(), 
                recoverCheckpointManagerSPtr);
            MetadataManager::SPtr recoverMetadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
            auto result = SyncAwait(recoverCheckpointManagerSPtr->RecoverCheckpointAsync(*recoverMetadataManagerSPtr, CancellationToken::None));

            // Tests
            VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *recoverCheckpointManagerSPtr);

            // verify the state of the result of RecoverCheckpointAsync
            VERIFY_ARE_EQUAL(metadataArray->Count(), metadataCount);
            VERIFY_ARE_EQUAL(result->Count(), metadataCount);
            for (ULONG i = 0; i < metadataCount; i++)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>((*result)[i]->StateProviderId - 1);
                TestHelper::VerifyMetadata((*result)[i].RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
            }

            // verify the active metadata of metadatamanager
            auto metadataEnumerator = recoverMetadataManagerSPtr->GetMetadataCollection();
            ULONG count = 0;
            while (metadataEnumerator->MoveNext())
            {
                Metadata::SPtr current = (metadataEnumerator->Current()).Value;
                ULONG expectedMetadataIndex = static_cast<ULONG>(current->StateProviderId - 1);
                TestHelper::VerifyMetadata(current.RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
                count++;
            }

            VERIFY_ARE_EQUAL(count, metadataCount / 2);

            KSharedArray<Metadata::CSPtr>::SPtr softDeletedMetadataArray = nullptr;
            status = recoverMetadataManagerSPtr->GetDeletedConstMetadataArray(softDeletedMetadataArray);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            count = 0;
            for (Metadata::CSPtr current : *softDeletedMetadataArray)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>(current->StateProviderId - 1);
                TestHelper::VerifyMetadata(current.RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
                count++;
            }

            VERIFY_ARE_EQUAL(count, metadataCount / 2);

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        //
        // CheckpointFile Refactory regression test, to cover the case that in the RecoverCheckpointAsync
        // call, the file stream closed before exception thrown during the metadata enumeration. So there
        // is no file leak
        // Algorithm:
        //     1. Create the metadataManager
        //     2. Populate the metadatas and take checkpoint 
        //     3. Inject CreateStateProvider failure
        //     4. Create a new metadataManager and call Recover
        //     5. Verify the exception thrown 
        // Note: It will assert in CheckpointFileAsyncEnumerator destructor if CloseAsync is not called before exception throw.
        //
        void Test_RecoverCheckpointAsync_Throw_NoFileStreamLeak(
            __in ULONG metadataCount, 
            __in LONG64 checkpointLSN, 
            __in Enum testMode,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
            bool isThrow = false;

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_,
                *stateProviderFactorySPtr_,
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_,
                *apiDispatcher,
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(),
                checkpointManagerSPtr); 
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManager = CreateMetadataManagerAndPopulateMetadatas(testMode, metadataCount, metadataArray);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManager, checkpointLSN);
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManager, CancellationToken::None));
            SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManager, CancellationToken::None));

            CheckpointManager::SPtr recoverCheckpointManagerSPtr = nullptr;
            status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(), 
                recoverCheckpointManagerSPtr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            MetadataManager::SPtr recoverMetadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());

            // Inject create state provider failure, so the RecoverCheckpointAsync call will throw exception
            stateProviderFactorySPtr_->FaultAPI = FaultStateProviderType::Constructor;

            try
            {
                auto result = SyncAwait(recoverCheckpointManagerSPtr->RecoverCheckpointAsync(*recoverMetadataManagerSPtr, CancellationToken::None));
            }
            catch (Exception & exception)
            {
                // Verify the exception thrown and it's the one we expected 
                isThrow = true;
                VERIFY_IS_TRUE(exception.GetStatus() == SF_STATUS_COMMUNICATION_ERROR);
            }

            // Verify the exception catched.
            VERIFY_IS_TRUE(isThrow);

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        void Test_Clean_CurrentAndTmpFiles_SUCCESS(
            __in ULONG metadataCount, 
            __in LONG64 checkpointLSN,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_,
                *stateProviderFactorySPtr_,
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_,
                *apiDispatcher,
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(),
                checkpointManagerSPtr); 
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManager = CreateMetadataManagerAndPopulateMetadatas(Enum::Both, metadataCount, metadataArray);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManager, checkpointLSN);
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManager, CancellationToken::None));
            SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManager, CancellationToken::None));
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManager, CancellationToken::None));

            // Tests
            VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, true, false, *checkpointManagerSPtr);

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        void Test_Recover_ShutdownBeforeOrAfterComplete_SUCCESS(
            __in ULONG metadataCount, 
            __in LONG64 checkpointLSN, 
            __in Enum testMode, 
            __in bool shutdownBeforeComplete,
            __in SerializationMode::Enum serializationMode)
        {
            KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
            KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
            Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

            ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
                *partitionedReplicaIdCSPtr_,
                *stateProviderFactorySPtr_,
                GetAllocator());

            CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
            NTSTATUS status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_,
                *apiDispatcher,
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(),
                checkpointManagerSPtr); 
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

            // Setup 
            MetadataManager::SPtr metadataManager = CreateMetadataManagerAndPopulateMetadatas(testMode, metadataCount, metadataArray);
            checkpointManagerSPtr->PrepareCheckpoint(*metadataManager, checkpointLSN);
            SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManager, CancellationToken::None));
            
            if(!shutdownBeforeComplete)
            {
                SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManager, CancellationToken::None));
            }

            // Mock the situation shutdown here, and recreate matadatamanager to call CompleteCheckpoint and recover from checkpoint file
            MetadataManager::SPtr newMetadataManager = TestHelper::CreateMetadataManager(GetAllocator());
            SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*newMetadataManager, CancellationToken::None));

            CheckpointManager::SPtr recoverCheckpointManagerSPtr = nullptr;
            status = CheckpointManager::Create(
                *partitionedReplicaIdCSPtr_, 
                *apiDispatcher, 
                *workFolderPath,
                *replicaFolderPath,
                serializationMode,
                GetAllocator(), 
                recoverCheckpointManagerSPtr);

            MetadataManager::SPtr recoverMetadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
            auto result = SyncAwait(recoverCheckpointManagerSPtr->RecoverCheckpointAsync(*recoverMetadataManagerSPtr, CancellationToken::None));

            // Tests
            VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *recoverCheckpointManagerSPtr);

            // Verify the state of the result of RecoverCheckpointAsync
            VERIFY_ARE_EQUAL(metadataArray->Count(), metadataCount);
            VERIFY_ARE_EQUAL(result->Count(), metadataCount);
            for (ULONG i = 0; i < metadataCount; i++)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>((*result)[i]->StateProviderId - 1);
                TestHelper::VerifyMetadata((*result)[i].RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
            }

            // verify the active metadata of metadatamanager
            auto metadataEnumerator = recoverMetadataManagerSPtr->GetMetadataCollection();
            ULONG count = 0;
            while (metadataEnumerator->MoveNext())
            {
                Metadata::SPtr current = (metadataEnumerator->Current()).Value;
                ULONG expectedMetadataIndex = static_cast<ULONG>(current->StateProviderId - 1);
                TestHelper::VerifyMetadata(current.RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
                count++;
            }

            VERIFY_ARE_EQUAL(count, metadataCount / 2);

            KSharedArray<Metadata::CSPtr>::SPtr softDeletedMetadataArray = nullptr;
            status = recoverMetadataManagerSPtr->GetDeletedConstMetadataArray(softDeletedMetadataArray);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            count = 0;
            for (Metadata::CSPtr current : *softDeletedMetadataArray)
            {
                ULONG expectedMetadataIndex = static_cast<ULONG>(current->StateProviderId - 1);
                TestHelper::VerifyMetadata(current.RawPtr(), (*metadataArray)[expectedMetadataIndex].RawPtr());
                count++;
            }

            VERIFY_ARE_EQUAL(count, metadataCount / 2);

            Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
        }

        void VerifyFileExistAndCleanUpFile(
            __in KStringView const & replicaPath,
            __in bool expectedCPTFileExist, 
            __in bool expectedTMPFileExist, 
            __in bool expectedBKPFileExist, 
            __in CheckpointManager & checkpointManager)
        {
            KString::SPtr currentCheckpointFile = KPath::Combine(replicaPath, CurrentCheckpointPrefix, GetAllocator());
            KString::SPtr tempCheckpointFile = KPath::Combine(replicaPath, TempCheckpointPrefix, GetAllocator());
            KString::SPtr backupCheckpointFile = KPath::Combine(replicaPath, BackupCheckpointPrefix, GetAllocator());

            VERIFY_ARE_EQUAL(Common::File::Exists(static_cast<LPCWSTR>(*currentCheckpointFile)), expectedCPTFileExist);
            VERIFY_ARE_EQUAL(Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFile)), expectedTMPFileExist);
            VERIFY_ARE_EQUAL(Common::File::Exists(static_cast<LPCWSTR>(*backupCheckpointFile)), expectedBKPFileExist);

            checkpointManager.Clean();
            VERIFY_IS_FALSE(Common::File::Exists(static_cast<LPCWSTR>(*currentCheckpointFile)));
            VERIFY_IS_FALSE(Common::File::Exists(static_cast<LPCWSTR>(*tempCheckpointFile)));

            if(Common::File::Exists(static_cast<LPCWSTR>(*backupCheckpointFile)))
            {
                KString::CSPtr filePath = TestHelper::CreateFileString(GetAllocator(), BackupCheckpointPrefix);
                Common::File::Delete(static_cast<LPCWSTR>(*filePath), true);
            }
        }

        KStringView const CurrentCheckpointPrefix = L"StateManager.cpt";
        KStringView const TempCheckpointPrefix = L"StateManager.tmp";
        KStringView const BackupCheckpointPrefix = L"StateManager.bkp";
    };

    BOOST_FIXTURE_TEST_SUITE(CheckpointManagerTestSuite, CheckpointManagerTest)

    // Create checkpointManagerSPtr test
    BOOST_AUTO_TEST_CASE(Create_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr); 
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Test
        VERIFY_ARE_EQUAL(checkpointManagerSPtr->PrepareCheckpointLSN, FABRIC_INVALID_SEQUENCE_NUMBER);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // PrepareCheckpoint function test with no Metadata
    // both GetPreparedActiveList and GetPreparedDeletedList should return 0
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_EmptyMetadata_SUCCESS)
    {
        ULONG const metadataCount = 0;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::Both, checkpointLSN, SerializationMode::Native);
    }

    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_EmptyMetadata_Managed_SUCCESS)
    {
        ULONG const metadataCount = 0;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::Both, checkpointLSN, SerializationMode::Managed);
    }

    // PrepareCheckpoint function test, with all active metadatas populated, 
    // After calling PrepareCheckpoint, GetPreparedActiveList should return all active metadatas
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_OneActiveMetadatas_InGetPreparedActiveList)
    {
        ULONG const metadataCount = 1;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::AllActive, checkpointLSN, SerializationMode::Native);
    }

    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_MultipleActiveMetadatas_AllInGetPreparedActiveList)
    {
        ULONG const metadataCount = 1024;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::AllActive, checkpointLSN, SerializationMode::Native);
    }

    // PrepareCheckpoint function test, with all SoftDeleted metadatas populated, 
    // After calling PrepareCheckpoint, GetPreparedActiveList should return 0 active metadatas, because all metadata are SoftDeleted
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_OneSoftDeletedMetadatas_InGetPreparedDeletedList)
    {
        ULONG const metadataCount = 1;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::AllDeleted, checkpointLSN, SerializationMode::Native);
    }

    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_MultipleSoftDeletedMetadatas_AllInGetPreparedDeletedList)
    {
        ULONG const metadataCount = 1024;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::AllDeleted, checkpointLSN, SerializationMode::Native);
    }

    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_MultipleSoftDeletedMetadatas_Managed_AllInGetPreparedDeletedList)
    {
        ULONG const metadataCount = 1024;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::AllDeleted, checkpointLSN, SerializationMode::Managed);
    }

    // PrepareCheckpoint function test, with both active metadatas and SoftDeleted metadatas populated, 
    // when we populate Metadatas, we insert half active metadatas and half SoftDeleted metadatas
    // After calling PrepareCheckpoint, GetPreparedActiveList should get all active metadatas, and GetPreparedDeletedList return all SoftDeleted metadatas
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_ActiveAndSoftDeleted_SUCCESS)
    {
        ULONG const metadataCount = 1024;
        LONG64 const checkpointLSN = 1;

        Test_PrepareCheckpoint_MultipleMetadata_SUCCESS(metadataCount, Enum::Both, checkpointLSN, SerializationMode::Native);
    }

    // PerformCheckpointAsync function test, with one metadata
    // after calling PerformCheckpointAsync, open the checkpoint file and verify the metadata is correct
    BOOST_AUTO_TEST_CASE(PerformCheckpointAsync_OneMetadata_SUCCESS)
    {        
        LONG64 const checkpointLSN = 1;

        Test_PerformCheckpointAsync_OneMetadata_SUCCESS(checkpointLSN, SerializationMode::Native);
    }

    BOOST_AUTO_TEST_CASE(PerformCheckpointAsync_OneMetadata_Managed_SUCCESS)
    {
        LONG64 const checkpointLSN = 1;

        Test_PerformCheckpointAsync_OneMetadata_SUCCESS(checkpointLSN, SerializationMode::Managed);
    }

    // CompleteCheckpointAsync with only temp checkpoint file exists, which will change the temp file to current checkpoint file
    BOOST_AUTO_TEST_CASE(CompleteCheckpointAsync_TmpFileExist_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
        LONG64 const checkpointLSN = 1;

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr); 
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

        // Setup 
        MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        KUri::CSPtr expectedName;
        status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        auto metadata = TestHelper::CreateMetadata(*expectedName, false, GetAllocator());
        bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
        VERIFY_IS_TRUE(isAdded);
        checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        // Test
        KString::SPtr currentCheckpointFile = KPath::Combine(*replicaFolderPath, CurrentCheckpointPrefix, GetAllocator());
        auto tmpFileName = KWString(GetAllocator(), *currentCheckpointFile);

        CheckpointFile::SPtr checkpointFileReopenSPtr = nullptr;
        status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr_, 
            tmpFileName, 
            GetAllocator(),
            checkpointFileReopenSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));

        auto enumerator = checkpointFileReopenSPtr->GetAsyncEnumerator();
        ULONG32 i = 0;

        while (true)
        {
            SerializableMetadata::CSPtr serializableMetadata = nullptr;
            status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, serializableMetadata));
            if (status == STATUS_NOT_FOUND)
            {
                break;
            }

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(serializableMetadata->StateProviderId == metadata->StateProviderId);
            VERIFY_IS_TRUE(serializableMetadata->ParentStateProviderId == metadata->ParentId);
            VERIFY_IS_TRUE(serializableMetadata->CreateLSN == metadata->CreateLSN);
            VERIFY_IS_TRUE(serializableMetadata->DeleteLSN == metadata->DeleteLSN);

            ++i;
        }

        VERIFY_IS_TRUE(i == 1);
        SyncAwait(enumerator->CloseAsync());

        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // CompleteCheckpointAsync with only backup file exists, in this case, current checkpoint file must exists
    BOOST_AUTO_TEST_CASE(CompleteCheckpointAsync_BkpFileExist_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
        LONG64 const checkpointLSN = 1;

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr); 
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

        // Setup 
        MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        KUri::CSPtr expectedName;
        status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        auto metadata = TestHelper::CreateMetadata(*expectedName, false, GetAllocator());
        bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
        VERIFY_IS_TRUE(isAdded);
        checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        // Test
        KString::SPtr tempCheckpointFileString = KPath::Combine(*replicaFolderPath, TempCheckpointPrefix, GetAllocator());
        KString::SPtr backupCheckpointFileString = KPath::Combine(*replicaFolderPath, BackupCheckpointPrefix, GetAllocator());

        auto errorCode = Common::File::Move(
            static_cast<LPCWSTR>(*tempCheckpointFileString),
            static_cast<LPCWSTR>(*backupCheckpointFileString));
        VERIFY_IS_TRUE(errorCode.IsSuccess());

        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // CompleteCheckpointAsync with only current checkpoint file exists, which means the complete already been called
    BOOST_AUTO_TEST_CASE(CompleteCheckpointAsync_CptFileExist_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
        LONG64 const checkpointLSN = 1;

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

        // Setup 
        MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        KUri::CSPtr expectedName;
        status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        auto metadata = TestHelper::CreateMetadata(*expectedName, false, GetAllocator());
        bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
        VERIFY_IS_TRUE(isAdded);
        checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        // Test
        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // Mock the situation that some replicas called CompleteCheckpointAsync but some not.
    // In this case, the CompleteCheckpointAsync will be called twice
    BOOST_AUTO_TEST_CASE(CompleteCheckpointAsync_DoubleCall_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
        LONG64 const checkpointLSN = 1;

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

        // Setup 
        MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        KUri::CSPtr expectedName;
        status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        auto metadata = TestHelper::CreateMetadata(*expectedName, false, GetAllocator());
        bool isAdded = metadataManagerSPtr->TryAdd(*expectedName, *metadata);
        VERIFY_IS_TRUE(isAdded);
        checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        // Test
        KString::SPtr currentCheckpointFile = KPath::Combine(*replicaFolderPath, CurrentCheckpointPrefix, GetAllocator());
        KWString fileName(GetAllocator(), *currentCheckpointFile);

        CheckpointFile::SPtr checkpointFileReopenSPtr = nullptr;
        status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr_, 
            fileName, 
            GetAllocator(),
            checkpointFileReopenSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));

        auto enumerator = checkpointFileReopenSPtr->GetAsyncEnumerator();
        ULONG32 i = 0;

        while (true)
        {
            SerializableMetadata::CSPtr serializableMetadata = nullptr;
            status = SyncAwait(enumerator->GetNextAsync(CancellationToken::None, serializableMetadata));
            if (status == STATUS_NOT_FOUND)
            {
                break;
            }

            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_TRUE(serializableMetadata->StateProviderId == metadata->StateProviderId);
            VERIFY_IS_TRUE(serializableMetadata->ParentStateProviderId == metadata->ParentId);
            VERIFY_IS_TRUE(serializableMetadata->CreateLSN == metadata->CreateLSN);
            VERIFY_IS_TRUE(serializableMetadata->DeleteLSN == metadata->DeleteLSN);

            ++i;
        }

        VERIFY_IS_TRUE(i == 1);
        SyncAwait(enumerator->CloseAsync());

        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // RecoverCheckpointAsync test should recover from checkpoint file which has both active and deleted Metadata
    BOOST_AUTO_TEST_CASE(RecoverCheckpointAsync_WithActiveAndDeletedMetadata_SUCCESS)
    {
        LONG64 const checkpointLSN = 1;
        ULONG const metadataCount = 10;

        Test_RecoverCheckpointAsync_MultipleMetadata_SUCCESS(metadataCount, checkpointLSN, Enum::Both, SerializationMode::Native);
    }

    // Mock the situation that shutdown happened after PerformCheckpointAsync, so the temp checkpoint file is saved,
    // In this case, we recreate the matadatamanager to call CompleteCheckpointAsync and RecoverCheckpointAsync should recover the origin states
    BOOST_AUTO_TEST_CASE(Recover_ShutdownBeforeComplete_SUCCESS)
    {
        LONG64 const checkpointLSN = 1;
        ULONG const metadataCount = 10;

        Test_Recover_ShutdownBeforeOrAfterComplete_SUCCESS(metadataCount, checkpointLSN, Enum::Both, true, SerializationMode::Native);
    }

    // Mock the situation that shutdown happened after CompleteCheckpointAsync, so the current checkpoint file is saved,
    // In this case, we recreate the matadatamanager to call CompleteCheckpointAsync and RecoverCheckpointAsync should recover the origin states
    BOOST_AUTO_TEST_CASE(Recover_ShutdownAfterComplete_SUCCESS)
    {
        LONG64 const checkpointLSN = 1;
        ULONG const metadataCount = 10;

        Test_Recover_ShutdownBeforeOrAfterComplete_SUCCESS(metadataCount, checkpointLSN, Enum::Both, false, SerializationMode::Native);
    }

    // Call recover with no checkpoint file exists, in this case, it will call PerformCheckpointAsync and CompleteCheckpointAsync
    // the checkpoint file will be generated but RecoverCheckpointAsync will return nullptr.
    BOOST_AUTO_TEST_CASE(RecoverCheckpointAsync_NoCheckpointFileExist_SUCCESS)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));

        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr); 
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        MetadataManager::SPtr recoverMetadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        auto result = SyncAwait(checkpointManagerSPtr->RecoverCheckpointAsync(*recoverMetadataManagerSPtr, CancellationToken::None));

        // Tests
        VERIFY_IS_TRUE(result == nullptr);
        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);
        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    // The clean function will remove current checkpoint file and temp checkpoint file
    BOOST_AUTO_TEST_CASE(Clean_CurrentAndTempFile_SUCCESS)
    {
        LONG64 const checkpointLSN = 1;
        ULONG const metadataCount = 10;

        Test_Clean_CurrentAndTmpFiles_SUCCESS(metadataCount, checkpointLSN, SerializationMode::Native);
    }

    // RDBug 9809142:RecoverCheckpointAsync_Throw_NoFileStreamLeak test failed on linux retail build
#if !defined(PLATFORM_UNIX)
    //
    // Scenario:        CheckpointFile Refactory regression test, to cover the case that in the RecoverCheckpointAsync
    //                  call, the file stream closed before exception thrown during the metadata enumeration.
    // Note:            It will assert in CheckpointFileAsyncEnumerator destructor if CloseAsync is not called before 
    //                  exception throw.
    // Expected Result: Exception should throw and no file leak.
    // 
    BOOST_AUTO_TEST_CASE(RecoverCheckpointAsync_Throw_NoFileStreamLeak)
    {
        LONG64 const checkpointLSN = 1;
        ULONG const metadataCount = 10;

        Test_RecoverCheckpointAsync_Throw_NoFileStreamLeak(metadataCount, checkpointLSN, Enum::Both, SerializationMode::Native);
    }
#endif

    // Goal: Test used to verify the PrepareCheckpointLSN is correctly set when we do the checkpoint, 
    // and PrepareCheckpointLSN is correctly read when we recover from the checkpoint file.
    BOOST_AUTO_TEST_CASE(PrepareCheckpointLSN_SetAndGet)
    {
        KString::CSPtr workFolderPath = TestHelper::CreateFileString(GetAllocator(), L"Work");
        KString::SPtr replicaFolderPath = KPath::Combine(*workFolderPath, L"PID_RID", GetAllocator());
        Common::Directory::CreateDirectoryW(static_cast<LPCWSTR>(*replicaFolderPath));
        LONG64 const checkpointLSN = 16;

        // Setup
        ApiDispatcher::SPtr apiDispatcher = ApiDispatcher::Create(
            *partitionedReplicaIdCSPtr_,
            *stateProviderFactorySPtr_,
            GetAllocator());

        CheckpointManager::SPtr checkpointManagerSPtr = nullptr;
        NTSTATUS status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            checkpointManagerSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        KSharedArray<Metadata::CSPtr>::SPtr metadataArray = nullptr;

        MetadataManager::SPtr metadataManagerSPtr = TestHelper::CreateMetadataManager(GetAllocator());
        KUri::CSPtr expectedName;
        status = KUri::Create(KUriView(L"fabric:/sps/sp"), GetAllocator(), expectedName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        checkpointManagerSPtr->PrepareCheckpoint(*metadataManagerSPtr, checkpointLSN);
        SyncAwait(checkpointManagerSPtr->PerformCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        SyncAwait(checkpointManagerSPtr->CompleteCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));

        // Test
        // After the PrepareCheckpoint function call, the PrepareCheckpointLSN in checkpointManager should be set to the correct number.
        VERIFY_ARE_EQUAL(checkpointManagerSPtr->PrepareCheckpointLSN, checkpointLSN);

        KString::SPtr currentCheckpointFile = KPath::Combine(*replicaFolderPath, CurrentCheckpointPrefix, GetAllocator());
        KWString fileName(GetAllocator(), *currentCheckpointFile);

        // Open the checkpoint file, the file should have the expected PrepareCheckpointLSN
        CheckpointFile::SPtr checkpointFileReopenSPtr = nullptr;
        status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr_,
            fileName,
            GetAllocator(),
            checkpointFileReopenSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        SyncAwait(checkpointFileReopenSPtr->ReadAsync(CancellationToken::None));

        // After reading from the checkpoint file, the PrepareCheckpointLSN should be as expected
        VERIFY_ARE_EQUAL(checkpointFileReopenSPtr->PropertiesSPtr->PrepareCheckpointLSN, checkpointLSN);

        // Create a new checkpoint manager and recover the checkpoint, the PrepareCheckpointLSN should as expected.
        CheckpointManager::SPtr newCheckpointManagerSPtr = nullptr;
        status = CheckpointManager::Create(
            *partitionedReplicaIdCSPtr_,
            *apiDispatcher,
            *workFolderPath,
            *replicaFolderPath,
            SerializationMode::Native,
            GetAllocator(),
            newCheckpointManagerSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        VERIFY_ARE_EQUAL(newCheckpointManagerSPtr->PrepareCheckpointLSN, FABRIC_INVALID_SEQUENCE_NUMBER);
        SyncAwait(newCheckpointManagerSPtr->RecoverCheckpointAsync(*metadataManagerSPtr, CancellationToken::None));
        VERIFY_ARE_EQUAL(newCheckpointManagerSPtr->PrepareCheckpointLSN, checkpointLSN);

        VerifyFileExistAndCleanUpFile(*replicaFolderPath, true, false, false, *checkpointManagerSPtr);

        Common::Directory::Delete(static_cast<LPCWSTR>(*workFolderPath), true);
    }

    BOOST_AUTO_TEST_SUITE_END()
}
