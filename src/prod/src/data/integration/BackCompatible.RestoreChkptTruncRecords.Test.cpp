// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Data
{
    namespace Integration
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace TxnReplicator;
        using namespace TxnReplicator::TestCommon;

        StringLiteral const TraceComponent("RestoreCheckpointTruncateHeadRecordsTest");
        std::wstring const TestLogFileName(L"RestoreCheckpointTruncateHeadRecordsTest.log");

        class RestoreCheckpointTruncateHeadRecordsTest
        {
        protected:
            enum TestCase
            {
                Backup_Tx_Only = 0,
                Backup_Tx_Chkpt = 1,
                Backup_Tx_ChkptTruncateHead = 2
            };

        public:
            Awaitable<void> Test_Restore(
                __in RestoreCheckpointTruncateHeadRecordsTest::TestCase testCase,
                __in wstring const & testFolder,
                __in wstring const & backupFolder,
                __in int dictNum,
                __in Data::Log::LogManager & logManager,
                __in FABRIC_RESTORE_POLICY policy,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId)
            {
                NTSTATUS status = STATUS_UNSUCCESSFUL;
                KAllocator & allocator = underlyingSystem_->PagedAllocator();

                UpgradeStateProviderFactory::SPtr stateProviderFactory = UpgradeStateProviderFactory::Create(underlyingSystem_->PagedAllocator(), true);
                KString::SPtr backupFolderPath = KPath::CreatePath(backupFolder.c_str(), allocator);

                {
                    TestDataLossHandler::SPtr dataLossHandler = TestDataLossHandler::Create(
                        allocator,
                        backupFolderPath.RawPtr(),
                        policy);

                    Replica::SPtr replica = Replica::Create(
                        partitionId,
                        replicaId,
                        testFolder,
                        logManager,
                        allocator,
                        stateProviderFactory.RawPtr(),
                        nullptr,
                        dataLossHandler.RawPtr()); // Default FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION is managed (0).

                    status = dataLossHandler->Initialize(*replica->TxnReplicator);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    co_await replica->OpenAsync();

                    // Become Primary after open (For OnDataLossAsync):  
                    replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                    replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                    FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                    co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                    // Invoke OnDataLossAsync and become fully Primary
                    BOOLEAN isStateChanged = false;
                    status = co_await replica->OnDataLossAsync(isStateChanged);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_TRUE(isStateChanged == TRUE);

                    replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                    replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                    replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                    replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                    FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                    co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_NONE);
                    co_await replica->CloseAsync();
                }

                co_return;
            }

            void DeleteWorkingDir(__in wstring & dir)
            {
                // Post-clean up
                if (Directory::Exists(dir))
                {
                    Directory::Delete_WithRetry(dir, true, true);
                }
            }

        protected:
            wstring CreateFileName(
                __in wstring const & folderName)
            {
                wstring testFolderPath = Directory::GetCurrentDirectoryW();
                Path::CombineInPlace(testFolderPath, folderName);
                return testFolderPath;
            }

            void InitializeKtlConfig(
                __in std::wstring workDir,
                __in std::wstring fileName,
                __in KAllocator & allocator,
                __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings)
            {
                auto settings = std::make_unique<KtlLogManager::SharedLogContainerSettings>();

                KString::SPtr sharedLogFileName = KPath::CreatePath(workDir.c_str(), allocator);
                KPath::CombineInPlace(*sharedLogFileName, fileName.c_str());

                if (!Common::Directory::Exists(workDir))
                {
                    Common::Directory::Create(workDir);
                }

                KInvariant(sharedLogFileName->LengthInBytes() + sizeof(WCHAR) < 512 * sizeof(WCHAR)); // check to make sure there is space for the null terminator
                KMemCpySafe(&settings->Path[0], 512 * sizeof(WCHAR), sharedLogFileName->operator PVOID(), sharedLogFileName->LengthInBytes());
                settings->Path[sharedLogFileName->LengthInBytes() / sizeof(WCHAR)] = L'\0'; // set the null terminator
                settings->LogContainerId.GetReference().CreateNew();
                settings->LogSize = 1024 * 1024 * 512; // 512 MB.
                settings->MaximumNumberStreams = 0;
                settings->MaximumRecordSize = 0;
                settings->Flags = static_cast<ULONG>(Data::Log::LogCreationFlags::UseSparseFile);
                sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>(std::move(settings));
            }

            void EndTest()
            {
            }

            wstring SetupBackupFolder(__in TestCase testCase)
            {
                {
                    wstring backupFolder = Common::Directory::GetCurrentDirectoryW();
                    Path::CombineInPlace(backupFolder, L"ChkptTruncRecordsBackupContainer");
                    Directory::Delete_WithRetry(backupFolder, true, true);

                    wstring restorePath = Common::Directory::GetCurrentDirectoryW();
#ifdef PLATFORM_UNIX
                    Path::CombineInPlace(restorePath, L"data_upgrade");
#else
                    Path::CombineInPlace(restorePath, L"UpgradeTests");
#endif
                    Path::CombineInPlace(restorePath, L"5.1");
                    Path::CombineInPlace(restorePath, L"ManagedStateManager");
                    Path::CombineInPlace(restorePath, L"BackCompatChkptTruncHeadRecordTests");

                    switch (testCase)
                    {
                    case TestCase::Backup_Tx_Only:
                        Path::CombineInPlace(restorePath, L"Backup_TxOnly");
                        break;
                    case TestCase::Backup_Tx_Chkpt:
                        Path::CombineInPlace(restorePath, L"Backup_Tx_Chkpt");
                        break;
                    case TestCase::Backup_Tx_ChkptTruncateHead:
                        Path::CombineInPlace(restorePath, L"Backup_Tx_ChkptAndTruncatehead");
                        break;
                    default:
                        ASSERT_IF(true, "RestoreCheckpointTruncateHeadRecordsTest test case is invalid.")
                    }

                    ErrorCode error = Common::Directory::Copy(restorePath.c_str(), backupFolder.c_str(), true);
                    VERIFY_IS_TRUE(error.IsSuccess());

                    return backupFolder;
                }
            }

        protected:
            CommonConfig config; // load the configuration object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;
        };

        BOOST_FIXTURE_TEST_SUITE(RestoreCheckpointTruncateHeadRecordsTestSuite, RestoreCheckpointTruncateHeadRecordsTest);

        // Restore to the Full and Incremental Backup with four items from managed.
        BOOST_AUTO_TEST_CASE(Restore_Backup_TxOnly)
        {
            // Setup
            wstring testName(L"Restore_Backup_TxOnly");
            wstring testFolderPath = CreateFileName(testName);
            auto testCase = TestCase::Backup_Tx_Only;

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(testCase);

            KGuid partitionId;
            partitionId.CreateNew();
            FABRIC_REPLICA_ID const replicaId = 17;

            TEST_TRACE_BEGIN(testName)
            {
                // Test Setup
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                // Test call
                SyncAwait(
                    Test_Restore(
                    testCase,
                    workFolder,
                    backupFolder,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_SAFE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            DeleteWorkingDir(testFolderPath);
        }

        BOOST_AUTO_TEST_CASE(Restore_Backup_Tx_Chkpt)
        {
            // Setup
            wstring testName(L"Restore_Backup_Tx_Chkpt");
            wstring testFolderPath = CreateFileName(testName);
            auto testCase = TestCase::Backup_Tx_Chkpt;

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(testCase);

            KGuid partitionId;
            partitionId.CreateNew();
            FABRIC_REPLICA_ID const replicaId = 17;

            TEST_TRACE_BEGIN(testName)
            {
                // Test Setup
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                // Test call
                SyncAwait(Test_Restore(
                    testCase,
                    workFolder,
                    backupFolder,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_SAFE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            DeleteWorkingDir(testFolderPath);
        }

        BOOST_AUTO_TEST_CASE(Restore_Backup_Tx_ChkptAndTruncatehead)
        {
            // Setup
            wstring testName(L"Restore_Backup_Tx_ChkptAndTruncatehead");
            wstring testFolderPath = CreateFileName(testName);
            auto testCase = TestCase::Backup_Tx_ChkptTruncateHead;

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(testCase);

            KGuid partitionId;
            partitionId.CreateNew();
            FABRIC_REPLICA_ID const replicaId = 17;

            TEST_TRACE_BEGIN(testName)
            {
                // Test Setup
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                // Test call
                SyncAwait(
                    Test_Restore(
                    testCase,
                    workFolder,
                    backupFolder,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_SAFE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            DeleteWorkingDir(testFolderPath);
        }

        BOOST_AUTO_TEST_SUITE_END();
    }
}
