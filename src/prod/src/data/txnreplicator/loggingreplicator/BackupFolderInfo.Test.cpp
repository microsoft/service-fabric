// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;

    StringLiteral const TraceComponent("BackupFolderInfoTests");

    class BackupFolderInfoTests
    {
    private: 
        class Backup
        {
        public:
            Backup(
                __in wstring & workFolder,
                __in FABRIC_BACKUP_OPTION backupOption,
                __in KGuid const & pid,
                __in FABRIC_REPLICA_ID rid,
                __in KGuid const & backupId,
                __in KGuid const & parentBackupId,
                __in FABRIC_EPOCH startingEpoch,
                __in FABRIC_SEQUENCE_NUMBER startingLSN,
                __in FABRIC_EPOCH endEpoch,
                __in FABRIC_SEQUENCE_NUMBER endLSN)
                : workFolder_(workFolder)
                , backupOption_(backupOption)
                , pid_(pid)
                , rid_(rid)
                , backupId_(backupId)
                , parentBackupId_(parentBackupId)
                , startingEpoch_(startingEpoch)
                , startingLSN_(startingLSN)
                , endEpoch_(endEpoch)
                , endLSN_(endLSN)
            {
            }

            wstring workFolder_;
            FABRIC_BACKUP_OPTION const backupOption_;
            KGuid const pid_;
            FABRIC_REPLICA_ID const rid_;
            KGuid const backupId_;
            KGuid const parentBackupId_;
            FABRIC_EPOCH const startingEpoch_;
            FABRIC_SEQUENCE_NUMBER const startingLSN_;
            FABRIC_EPOCH const endEpoch_;
            FABRIC_SEQUENCE_NUMBER const endLSN_;
        };

    public:
        void BackupFolderGenerator(
            __in wstring path)
        {
            this->folderPath = path;
            this->backupChain.clear();
        }

        int AddFull(
            __in KGuid const & pid,
            __in FABRIC_REPLICA_ID rid,
            __in FABRIC_EPOCH startingEpoch,
            __in FABRIC_SEQUENCE_NUMBER startingLSN,
            __in FABRIC_EPOCH endEpoch,
            __in FABRIC_SEQUENCE_NUMBER endLSN)
        {
            int index = static_cast<int>(this->backupChain.size());
            wstring path = Path::Combine(this->folderPath, std::to_wstring(index));

            KGuid backupId;
            backupId.CreateNew();

            Backup backup(path, FABRIC_BACKUP_OPTION_FULL, pid, rid, backupId, backupId, startingEpoch, startingLSN, endEpoch, endLSN);

            this->backupChain.push_back(backup);
            return index;
        }

        int AddIncremental(
            __in int parentIndex,
            __in KGuid const & pid,
            __in FABRIC_REPLICA_ID rid,
            __in FABRIC_EPOCH endEpoch,
            __in FABRIC_SEQUENCE_NUMBER endLSN,
            __in bool startFromHigherLSN)
        {
            Backup parentBackup = this->backupChain[parentIndex];
            int index = static_cast<int>(this->backupChain.size());
            wstring path = Path::Combine(this->folderPath, std::to_wstring(index));

            FABRIC_SEQUENCE_NUMBER startingLSN = startFromHigherLSN == false ? parentBackup.endLSN_ : parentBackup.endLSN_ + 1;

            KGuid backupId;
            backupId.CreateNew();

            Backup backup(path, FABRIC_BACKUP_OPTION_INCREMENTAL, pid, rid, backupId, parentBackup.backupId_, parentBackup.endEpoch_, startingLSN, endEpoch, endLSN);

            this->backupChain.push_back(backup);
            return index;
        }

        int Add(
            __in FABRIC_BACKUP_OPTION backupOption,
            __in KGuid const & pid,
            __in FABRIC_REPLICA_ID rid,
            __in FABRIC_EPOCH startingEpoch,
            __in FABRIC_SEQUENCE_NUMBER startingLSN,
            __in FABRIC_EPOCH endEpoch,
            __in FABRIC_SEQUENCE_NUMBER endLSN)
        {
            int index = static_cast<int>(this->backupChain.size());
            wstring path = Path::Combine(this->folderPath, std::to_wstring(index));

            KGuid backupId;
            backupId.CreateNew();

            Backup backup(path, backupOption, pid, rid, backupId, backupId, startingEpoch, startingLSN, endEpoch, endLSN);

            this->backupChain.push_back(backup);
            return index;
        }

    public:
        Awaitable<void> CreateBackupFolderAsync(
            __in wstring & folderDir,
            __in FABRIC_BACKUP_OPTION backupOption,
            __in KGuid const & pid,
            __in FABRIC_REPLICA_ID rid,
            __in KGuid const & backupId,
            __in KGuid const & parentBackupId,
            __in FABRIC_EPOCH startingEpoch,
            __in FABRIC_SEQUENCE_NUMBER startingLSN,
            __in FABRIC_EPOCH endEpoch,
            __in FABRIC_SEQUENCE_NUMBER endLSN)
        {
            ASSERT_IF(Directory::Exists(folderDir), "Folder should not exist");
            Common::Directory::Create(folderDir);

            // Create back metadata file
            KStringView metadataFileName = backupOption == FABRIC_BACKUP_OPTION_FULL ?
                BackupManager::FullMetadataFileName :
                BackupManager::IncrementalMetadataFileName;

            KString::SPtr metadataFilePath = KPath::CreatePath(folderDir.c_str(), underlyingSystem_->PagedAllocator());
            KPath::CombineInPlace(*metadataFilePath, metadataFileName);

            BackupMetadataFile::SPtr backupMetadataSPtr = nullptr;
            NTSTATUS status = BackupMetadataFile::Create(
                *PartitionedReplicaId::Create(pid, rid, underlyingSystem_->PagedAllocator()),
                KWString(underlyingSystem_->PagedAllocator(), *metadataFilePath),
                underlyingSystem_->PagedAllocator(),
                backupMetadataSPtr);
            ASSERT_IFNOT(NT_SUCCESS(status), "BackupMetadataFile: Create failed.");

            status = co_await backupMetadataSPtr->WriteAsync(
                backupOption,
                parentBackupId,
                backupId,
                pid,
                rid,
                startingEpoch,
                startingLSN,
                endEpoch,
                endLSN,
                CancellationToken::None);
            ASSERT_IFNOT(NT_SUCCESS(status), "BackupMetadataFile: WriteAsync failed.");

            if (backupOption == FABRIC_BACKUP_OPTION_FULL)
            {
                // Create state manager folder
                KString::SPtr smFolderPath = KPath::CreatePath(folderDir.c_str(), underlyingSystem_->PagedAllocator());
                KPath::CombineInPlace(*smFolderPath, BackupManager::StateManagerBackupFolderName);

                Common::Directory::Create(wstring(*smFolderPath));
            }
        }

        Awaitable<void> GenerateAsync()
        {
            this->CreateFolder(this->folderPath);

            for (Backup backup : this->backupChain)
            {
                co_await this->CreateBackupFolderAsync(
                    backup.workFolder_,
                    backup.backupOption_,
                    backup.pid_,
                    backup.rid_,
                    backup.backupId_,
                    backup.parentBackupId_,
                    backup.startingEpoch_,
                    backup.startingLSN_,
                    backup.endEpoch_,
                    backup.endLSN_);
            }
        }

        void CreateFolder(
            __in wstring backupFolderPath,
            __in_opt bool deleteIfAlreadyExists = true)
        {
            if (deleteIfAlreadyExists && Directory::Exists(backupFolderPath))
            {
                Directory::Delete(backupFolderPath, true);
            }

            Common::Directory::Create(backupFolderPath);
        }

        void CreateLogFile(wstring folderDir)
        {
            wstring logFolderPath = Path::Combine(folderDir, wstring(BackupManager::ReplicatorBackupFolderName));
            Common::Directory::Create(logFolderPath);

            wstring logFilePath = Path::Combine(logFolderPath, wstring(BackupManager::ReplicatorBackupLogName));

            File logFile;
            logFile.Open(logFilePath, FABRIC_FILE_MODE::FABRIC_FILE_MODE_CREATE_NEW, FABRIC_FILE_ACCESS_READ_WRITE, FABRIC_FILE_SHARE::FABRIC_FILE_SHARE_READ_WRITE, nullptr);
            logFile.Close();
        }

        void CleanUp()
        {
            if (Directory::Exists(this->folderPath))
            {
                Directory::Delete(this->folderPath, true);
            }
        }

        void Dispose()
        {
            this->CleanUp();
        }

        void Validate(
            __in std::vector<int> expectedChain,
            __in BackupFolderInfo const & backupFolderInfo)
        {
            VERIFY_ARE_EQUAL(backupChain[0].workFolder_, wstring(*backupFolderInfo.FullBackupFolderPath));
            VERIFY_ARE_EQUAL(expectedChain.size(), static_cast<size_t>(backupFolderInfo.BackupMetadataArray.Count()));
            VERIFY_ARE_EQUAL(expectedChain.size(), static_cast<size_t>(backupFolderInfo.LogPathList.Count()));
           
            ULONG currentIndex = 0;
            for(int i : expectedChain)
            {
                Backup backup = backupChain[i];
                auto metadataFile = backupFolderInfo.BackupMetadataArray[currentIndex++];

                VERIFY_ARE_EQUAL(backup.backupId_, metadataFile->BackupId);
                VERIFY_ARE_EQUAL(backup.parentBackupId_, metadataFile->ParentBackupId);
            }
        }

        FABRIC_EPOCH CreateEpoch(
            __in long dataLossNumber,
            __in long configurationNumber)
        {
            FABRIC_EPOCH newEpoch;
            newEpoch.DataLossNumber = dataLossNumber;
            newEpoch.ConfigurationNumber = configurationNumber;

            return newEpoch;
        }

    protected:
        KString::SPtr CreateFileName(
            __in KStringView const & inputName);

        void EndTest();

    public:
        wstring folderPath;
        std::vector<Backup> backupChain;
        CommonConfig config; // load the config object as its needed for the tracing to work
        FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    KString::SPtr BackupFolderInfoTests::CreateFileName(
        __in KStringView const & inputName)
    {
        KString::SPtr fileName;
        KAllocator & allocator = underlyingSystem_->PagedAllocator();

        wstring currentDirectoryPathCharArray = Common::Directory::GetCurrentDirectoryW();

        NTSTATUS status = KString::Create(fileName, allocator, Common::Path::GetPathGlobalNamespaceWstr().c_str());
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        BOOLEAN concatSuccess = fileName->Concat(currentDirectoryPathCharArray.c_str());
        CODING_ERROR_ASSERT(concatSuccess == TRUE);

        fileName = KPath::Combine(*fileName, inputName, allocator);

        return fileName;
    }

    void BackupFolderInfoTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(BackupFolderInfoTestSuite, BackupFolderInfoTests);

    BOOST_AUTO_TEST_CASE(AnalyzeAsync_Empty_FabricMissingFullBackupException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_Empty_FabricMissingFullBackupException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_Empty_FabricMissingFullBackupException");
            BackupFolderGenerator(wstring(*backupFolderPath));
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath,  allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_MISSING_FULL_BACKUP);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    BOOST_AUTO_TEST_CASE(AnalyzeAsync_TwoFull_FabricMissingFullBackupException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_TwoFull_FabricMissingFullBackupException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_TwoFull_FabricMissingFullBackupException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, rId_, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddFull(pId_, rId_, CreateEpoch(0, 1), 5, CreateEpoch(0, 1), 6);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_INVALID_OPERATION);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    BOOST_AUTO_TEST_CASE(Trim_FullIsNotOldest_ArgumentException)
    {
        TEST_TRACE_BEGIN("Trim_FullIsNotOldest_ArgumentException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"Trim_FullIsNotOldest_ArgumentException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 3), 30, CreateEpoch(0, 3), 50);
            this->Add(FABRIC_BACKUP_OPTION::FABRIC_BACKUP_OPTION_INCREMENTAL, pId_, 1, CreateEpoch(0, 1), 1, CreateEpoch(0, 1), 2);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    BOOST_AUTO_TEST_CASE(AnalyzeAsync_DataLossNumberChange_ArgumentException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_DataLossNumberChange_ArgumentException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_DataLossNumberChange_ArgumentException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 2), 6, false);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1 -> 3
    ///   -> 2 -> 4
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_TwoForksWithMoreThanOneLink_0_ArgumentException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_TwoForksWithMoreThanOneLink_0_ArgumentException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_TwoForksWithMoreThanOneLink_0_ArgumentException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            // Fork one
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 2), 6, false);
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 3), 6, false);

            // Chains
            this->AddIncremental(1, pId_, 1, CreateEpoch(1, 4), 10, false);
            this->AddIncremental(2, pId_, 1, CreateEpoch(1, 5), 10, false);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1 -> 2
    ///   -> 3 -> 4
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_TwoForksWithMoreThanOneLink_1_ArgumentException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_TwoForksWithMoreThanOneLink_1_ArgumentException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_TwoForksWithMoreThanOneLink_1_ArgumentException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            // Fork one
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 2), 6, false);
            this->AddIncremental(1, pId_, 1, CreateEpoch(1, 3), 10, false);

            // Chains
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 4), 6, false);
            this->AddIncremental(3, pId_, 1, CreateEpoch(1, 5), 10, false);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_OneFull_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_OneFull_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_OneFull_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_1_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_1_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_1_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false);
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 1 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_1_HigherLSN_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_1_HigherLSN_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_1_HigherLSN_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, true);
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 1 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    /// </summary>
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_1_DifferentReplica_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_1_DifferentReplica_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_1_DifferentReplica_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddIncremental(0, pId_, 2, CreateEpoch(0, 1), 6, false);
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 1 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    ///   -> 2
    ///   -> 3
    /// </summary>
    BOOST_AUTO_TEST_CASE(Trim_1_D3_Success)
    {
        TEST_TRACE_BEGIN("Trim_1_D3_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"Trim_1_D3_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, true);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 2), 6, true);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 3), 6, true);
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 3 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    ///   -> 2 -> 4 -> 5
    ///   -> 3
    /// </summary>
    BOOST_AUTO_TEST_CASE(Trim_1_D3_2_Success)
    {
        TEST_TRACE_BEGIN("Trim_1_D3_2_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"Trim_1_D3_2_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            // Full backup
            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            // Divergence
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, true);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 2), 6, true);
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 3), 6, true);

            // Continue from 2
            this->AddIncremental(2, pId_, 1, CreateEpoch(0, 4), 7, false);
            this->AddIncremental(4, pId_, 1, CreateEpoch(0, 4), 19, true);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 2, 4, 5 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    ///   -> 2 -> 4 -> 5
    ///   -> 3
    /// </summary>
    BOOST_AUTO_TEST_CASE(Trim_1_D3_2_Seperate_Replica_Success)
    {
        TEST_TRACE_BEGIN("Trim_1_D3_2_Seperate_Replica_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"Trim_1_D3_2_Seperate_Replica_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            // Full backup
            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            // Divergence
            this->AddIncremental(0, pId_, 2, CreateEpoch(0, 1), 6, false);
            this->AddIncremental(0, pId_, 3, CreateEpoch(0, 2), 6, false);
            this->AddIncremental(0, pId_, 4, CreateEpoch(0, 3), 6, false);

            // Continue from 2
            this->AddIncremental(2, pId_, 3, CreateEpoch(0, 4), 7, true);
            this->AddIncremental(4, pId_, 3, CreateEpoch(0, 4), 19, true);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 2, 4, 5 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1 -> 2 -> 3 -> 4 -> 7 -> 8 -> 9        -> 15       -> 17
    ///                  -> 5           -> 10       -> 14 -> 16 -> 18
    ///                  -> 6           -> 11 -> 12 -> 13       -> 19
    /// </summary>
    BOOST_AUTO_TEST_CASE(Trim_MultipleDivergence_Success)
    {
        TEST_TRACE_BEGIN("Trim_MultipleDivergence_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"Trim_MultipleDivergence_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            // Full backup
            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            this->AddIncremental(0, pId_, 2, CreateEpoch(0, 2), 5, true);  // 1
            this->AddIncremental(1, pId_, 2, CreateEpoch(0, 2), 10, true); // 2
            this->AddIncremental(2, pId_, 2, CreateEpoch(0, 2), 15, true); // 3

            // Divergence 1
            this->AddIncremental(3, pId_, 2, CreateEpoch(0, 2), 20, false); // 4
            this->AddIncremental(3, pId_, 3, CreateEpoch(0, 2), 21, false); // 5
            this->AddIncremental(3, pId_, 4, CreateEpoch(0, 3), 21, false); // 6

            // Continue from 2
            this->AddIncremental(4, pId_, 3, CreateEpoch(0, 4), 25, true); // 7
            this->AddIncremental(7, pId_, 3, CreateEpoch(0, 4), 30, true); // 8

            // Divergence 2
            this->AddIncremental(8, pId_, 3, CreateEpoch(0, 4), 35, true); // 9
            this->AddIncremental(8, pId_, 2, CreateEpoch(0, 5), 36, false); // 10
            this->AddIncremental(8, pId_, 3, CreateEpoch(0, 6), 37, true); // 11

            // Continue from 11
            this->AddIncremental(11, pId_, 3, CreateEpoch(0, 6), 40, true); // 12

            // Divergence 3
            this->AddIncremental(12, pId_, 3, CreateEpoch(0, 7), 45, true); // 13
            this->AddIncremental(12, pId_, 2, CreateEpoch(0, 8), 45, false); // 14
            this->AddIncremental(12, pId_, 3, CreateEpoch(0, 9), 45, true); // 15

            // Continue from 14
            this->AddIncremental(14, pId_, 3, CreateEpoch(0, 10), 50, true); // 16

            // Divergence 4
            this->AddIncremental(16, pId_, 3, CreateEpoch(0, 11), 55, true); // 17
            this->AddIncremental(16, pId_, 2, CreateEpoch(0, 12), 56, false); //18
            this->AddIncremental(16, pId_, 3, CreateEpoch(0, 13), 57, true); // 19
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 1, 2, 3, 4, 7, 8, 11, 12, 14, 16, 19 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    // Following test section is used to test the case that transient exception thrown during replicating backup log record, 
    // in which case same backup will be uploaded if user re-try the BackupAsync call.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_SameFull_FabricMissingFullBackupException)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_SameFull_FabricMissingFullBackupException")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_SameFull_FabricMissingFullBackupException");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);
            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1);

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), SF_STATUS_INVALID_OPERATION);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    BOOST_AUTO_TEST_CASE(AnalyzeAsync_DataLossNumberChange_Negtive)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_DataLossNumberChange_Negtive")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_DataLossNumberChange_Negtive");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 1
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 2
            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 2), 6, false); // 3

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1(Dup) -> 3
    ///   -> 2(Dup) -> 4
    /// </summary>
    /// Note:DWSI: diverge with same incrementals, which means bunch of same incrementals uploaded.
    ///      Dup: duplicate backups, which means same backups with different BackupId.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_SameBackup_TwoChainsWithMoreThanOneLink_Negtive)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_SameBackup_TwoChainsWithMoreThanOneLink_Negtive")
        {
            // Setup
            bool exceptionThrown = false;
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_SameBackup_TwoChainsWithMoreThanOneLink_Negtive");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            // Two folks
            this->AddIncremental(0, pId_, 2, CreateEpoch(1, 1), 6, false); // 1
            this->AddIncremental(0, pId_, 2, CreateEpoch(1, 1), 6, false); // 2

            this->AddIncremental(0, pId_, 1, CreateEpoch(1, 2), 10, false); // 3
            this->AddIncremental(0, pId_, 2, CreateEpoch(1, 3), 10, false); // 4

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator);

            try
            {
                SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));
            }
            catch (Exception & exception)
            {
                exceptionThrown = true;
                VERIFY_ARE_EQUAL(exception.GetStatus(), STATUS_INVALID_PARAMETER);
            }

            VERIFY_IS_TRUE(exceptionThrown);
            this->CleanUp();
        }
    }
    
    /// <summary>
    /// 0 -> 1(Dup)
    ///   -> 2(Dup)
    ///   -> 3
    /// </summary>
    /// Note:DWSI: diverge with same incrementals, which means bunch of same incrementals uploaded.
    ///      Dup: duplicate backups, which means same backups with different BackupId.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_DWSI_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_DWSI_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_DWSI_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 1
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 2
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 2), 6, false); // 3

            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 3 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    ///   -> 2(Dup) -> 4 -> 5
    ///   -> 3(Dup)
    /// </summary>
    /// Note:DWSI: diverge with same incrementals, which means bunch of same incrementals uploaded.
    ///      Dup: duplicate backups, which means same backups with different BackupId.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_DWSI_2_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_DWSI_2_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_DWSI_2_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, true); // 1
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 2), 6, true); // 2
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 2), 6, true); // 3

            // Continue from 2
            this->AddIncremental(2, pId_, 1, CreateEpoch(0, 3), 10, false); // 4
            this->AddIncremental(4, pId_, 1, CreateEpoch(0, 3), 15, true); // 5
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 2, 4, 5 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1
    ///   -> 2(Dup) -> 4 -> 5
    ///   -> 3(Dup)
    /// </summary>
    /// Note:DWSI: diverge with same incrementals, which means bunch of same incrementals uploaded.
    ///      Dup: duplicate backups, which means same backups with different BackupId.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_1_DWSI_Seperate_Replica_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_1_DWSI_Seperate_Replica_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_1_DWSI_Seperate_Replica_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            this->AddIncremental(0, pId_, 2, CreateEpoch(0, 1), 6, false); // 1
            this->AddIncremental(0, pId_, 3, CreateEpoch(0, 2), 6, false); // 2
            this->AddIncremental(0, pId_, 3, CreateEpoch(0, 2), 6, false); // 3

            // Continue from 2
            this->AddIncremental(2, pId_, 3, CreateEpoch(0, 3), 10, false);// 4
            this->AddIncremental(4, pId_, 3, CreateEpoch(0, 3), 15, true); // 5
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 2, 4, 5 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    /// <summary>
    /// 0 -> 1(Dup)      -> 5           -> 9  ->12
    ///   -> 2(Dup) -> 4 -> 6(Dup)      -> 10
    ///   -> 3(Dup)      -> 7(Dup) -> 8 -> 11
    /// </summary>
    /// Note:DWSI: diverge with same incrementals, which means bunch of same incrementals uploaded.
    ///      Dup: duplicate backups, which means same backups with different BackupId.
    BOOST_AUTO_TEST_CASE(AnalyzeAsync_MultipleDivergence_SameIncrementals_Success)
    {
        TEST_TRACE_BEGIN("AnalyzeAsync_MultipleDivergence_SameIncrementals_Success")
        {
            // Setup
            KString::SPtr backupFolderPath = CreateFileName(L"AnalyzeAsync_MultipleDivergence_SameIncrementals_Success");
            BackupFolderGenerator(wstring(*backupFolderPath));

            this->AddFull(pId_, 1, CreateEpoch(0, 0), 0, CreateEpoch(0, 1), 1); // 0 

            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 1
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 2
            this->AddIncremental(0, pId_, 1, CreateEpoch(0, 1), 6, false); // 3

            // Continue from 2
            this->AddIncremental(2, pId_, 2, CreateEpoch(0, 2), 10, true);// 4

            // Divergence 2
            this->AddIncremental(4, pId_, 3, CreateEpoch(0, 3), 15, false);// 5
            this->AddIncremental(4, pId_, 2, CreateEpoch(0, 4), 15, false);// 6
            this->AddIncremental(4, pId_, 2, CreateEpoch(0, 4), 15, false);// 7

            // Continue from 11
            this->AddIncremental(7, pId_, 2, CreateEpoch(0, 4), 20, true);// 8


            // Divergence 3
            this->AddIncremental(8, pId_, 3, CreateEpoch(0, 5), 25, true); // 9
            this->AddIncremental(8, pId_, 2, CreateEpoch(0, 7), 25, false);// 10
            this->AddIncremental(8, pId_, 3, CreateEpoch(0, 9), 25, true); // 11

            // Continue from 11
            this->AddIncremental(9, pId_, 3, CreateEpoch(0, 10), 30, true);// 12
            SyncAwait(GenerateAsync());

            // Test
            KGuid backupId;
            backupId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfo = BackupFolderInfo::Create(*prId_, backupId, *backupFolderPath, allocator, true);

            SyncAwait(backupFolderInfo->AnalyzeAsync(CancellationToken::None));

            std::vector<int> expectedList = { 0, 2, 4, 7, 8, 9, 12 };
            Validate(expectedList, *backupFolderInfo);
            this->CleanUp();
        }
    }

    BOOST_AUTO_TEST_SUITE_END();
}
