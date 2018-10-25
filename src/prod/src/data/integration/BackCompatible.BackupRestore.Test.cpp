// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

/// BackCompatible test for BackupRestore
/// Note: For now, the test only covers <int, int> type of store.
///       RDBug 11728221:BackCompat BackupAndRestore tests: Add cases 1. component states race, 2. backup cross replicas, 3. restore token verify
///       RDBug 11728220:BackCompat BackupAndRestore tests: Move helper functions to replica lib and share helper functions.
/// Test dimension:
///+---------------+--------------+------------------+---------------------+------------------------+----------------+----------------+
///| Backup Option |    Status    |    SM states     | StateProvider Count |      Restore From      | Restore Policy | Target Replica |
///+---------------+--------------+------------------+---------------------+------------------------+----------------+----------------+
///| Full          | LR state     | Empty            |          0          | Log + Empty Checkpoint | Safe           | Clean          |
///+---------------+--------------+------------------+---------------------+------------------------+----------------+----------------+
///| Incremental   | SM state     | Active           |          1          | Log + Checkpoint       | Force          | Dirty          |
///+---------------+--------------+------------------+---------------------+------------------------+----------------+----------------+
///|               | TStore state | Active + Deleted |          4          |                        |                |                |
///+---------------+--------------+------------------+---------------------+------------------------+----------------+----------------+
namespace Data
{
    namespace Integration
    {
        using namespace Common;
        using namespace ktl;
        using namespace Data::Utilities;
        using namespace TxnReplicator;
        using namespace TxnReplicator::TestCommon;

        StringLiteral const TraceComponent("BackCompatibleBackupRestoreTests");
        std::wstring const TestLogFileName(L"BackCompatibleBackupRestoreTests.log");

        class BackCompatibleBackupRestoreTests
        {
        public:
            Awaitable<void> Test_Restore_EmptyBackup_Success(
                __in wstring const & testFolder,
                __in wstring const & backupFolder,
                __in bool isReplicaClean,
                __in int numberOfActiveStateProviders,
                __in int numberOfDeleteStateProviders,
                __in Data::Log::LogManager & logManager,
                __in FABRIC_RESTORE_POLICY policy,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId);

            Awaitable<void> Test_Restore_Backup_Success(
                __in wstring const & testFolder,
                __in wstring const & backupFolder,
                __in int dictNum,
                __in Data::Log::LogManager & logManager,
                __in FABRIC_RESTORE_POLICY policy,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId);

            Awaitable<void> Test_Backup_WithoutPrepareCheckpointLSNProperty(
                __in wstring const & testFolder,
                __in wstring const & backupFolder,
                __in Data::Log::LogManager & logManager,
                __in FABRIC_RESTORE_POLICY policy,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId);

        protected:
            enum TestCase
            {
                Empty = 0,
                OneDict_Full = 1,
                FourDict_Full = 2,
                FourDict_Full_Checkpointed = 3,
                FourDict_FullAndIncremental = 4,
                FoutDict_FullAndIncremental_Checkpointed = 5,
                WithoutCheckpointLSN = 6,
            };

        protected:
            Awaitable<void> VerifyDictionaryAsync(
                __in wstring dictName,
                __in int dictIndex,
                __in Replica const & replica);

            wstring CreateFileName(
                __in wstring const & folderName);

            KUri::CSPtr GetStateProviderName(
                __in int stateProviderIndex);

            void VerifyIfStateProviderExists(
                __in ITransactionalReplicator & replicator,
                __in KUri const & stateProviderName,
                __in BOOLEAN expectResult);

            void InitializeKtlConfig(
                __in std::wstring workDir,
                __in std::wstring fileName,
                __in KAllocator & allocator,
                __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

            void EndTest();

            wstring SetupBackupFolder(__in TestCase testCase);

        protected:
            CommonConfig config; // load the configuration object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;
        };

        Awaitable<void> BackCompatibleBackupRestoreTests::Test_Restore_EmptyBackup_Success(
            __in wstring const & testFolder,
            __in wstring const & backupFolder,
            __in bool isReplicaClean,
            __in int numberOfActiveStateProviders,
            __in int numberOfDeleteStateProviders,
            __in Data::Log::LogManager & logManager,
            __in FABRIC_RESTORE_POLICY policy,
            __in KGuid const & partitionId,
            __in FABRIC_REPLICA_ID replicaId) 
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            KAllocator & allocator = underlyingSystem_->PagedAllocator();

            UpgradeStateProviderFactory::SPtr stateProviderFactory = UpgradeStateProviderFactory::Create(underlyingSystem_->PagedAllocator(), true);

            KString::SPtr backupFolderPath = KPath::CreatePath(backupFolder.c_str(), allocator);

            Data::Utilities::OperationData::SPtr initParams = Data::Utilities::OperationData::Create(allocator);
            Data::Utilities::BinaryWriter bw(allocator);
            bw.Write(L"Initial Value", Data::Utilities::UTF16);
            initParams->Append(*bw.GetBuffer(0));

            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());

            int lastInsertedStateProviderIndex = -1;
            {
                Replica::SPtr replica = Replica::Create(
                    partitionId,
                    replicaId,
                    testFolder,
                    logManager,
                    allocator,
                    stateProviderFactory.RawPtr());

                co_await replica->OpenAsync();

                // Become Primary after open:  
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                // 4. Update Status: Granted.
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                if (numberOfActiveStateProviders > 0)
                {
                    for (int i = 0; i < numberOfActiveStateProviders; ++i)
                    {
                        Transaction::SPtr txn;
                        replica->TxnReplicator->CreateTransaction(txn);
                        KFinally([&] {txn->Dispose(); });

                        KUri::CSPtr stateProviderName = GetStateProviderName(i);
                        status = co_await replica->TxnReplicator->AddAsync(
                            *txn,
                            *stateProviderName,
                            typeName,
                            initParams.RawPtr());
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        co_await txn->CommitAsync();

                        ++lastInsertedStateProviderIndex;
                    }

                    for (int i = 0; i < numberOfDeleteStateProviders; ++i)
                    {
                        Transaction::SPtr txn;
                        replica->TxnReplicator->CreateTransaction(txn);
                        KFinally([&] {txn->Dispose(); });

                        KUri::CSPtr stateProviderName = GetStateProviderName(i);
                        status = co_await replica->TxnReplicator->RemoveAsync(*txn, *stateProviderName);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        co_await txn->CommitAsync();
                    }

                    // Verify current states are expected
                    for (int i = 0; i <= lastInsertedStateProviderIndex; i++)
                    {
                        this->VerifyIfStateProviderExists(*replica->TxnReplicator, *GetStateProviderName(i), i < numberOfDeleteStateProviders ? FALSE : TRUE);
                    }
                }

                // Close the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                if (isReplicaClean)
                {
                    FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                    co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_NONE);
                }

                co_await replica->CloseAsync();
            }

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
                    dataLossHandler.RawPtr());

                status = dataLossHandler->Initialize(*replica->TxnReplicator);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                // Open the replica.
                co_await replica->OpenAsync();

                // Become Primary after open (For OnDataLossAsync):  
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 2; epoch3.ConfigurationNumber = 2; epoch3.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_PRIMARY);

                // Get OnDataLossAsync and become fully Primary
                // 1. OnDataLossAsync
                // 2. Update Status: Granted
                // TODO: UpdateCatchupConfig & WaitForQuorumCatchup for 1 replica?
                BOOLEAN isStateChanged = false;
                status = co_await replica->OnDataLossAsync(isStateChanged);
                VERIFY_ARE_EQUAL(status, policy == FABRIC_RESTORE_POLICY_FORCE ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER);
                VERIFY_ARE_EQUAL(isStateChanged, policy == FABRIC_RESTORE_POLICY_FORCE ? TRUE : FALSE);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                if (policy == FABRIC_RESTORE_POLICY_FORCE)
                {
                    // Verify restored state
                    for (int i = 0; i <= lastInsertedStateProviderIndex; i++)
                    {
                        this->VerifyIfStateProviderExists(*replica->TxnReplicator, *GetStateProviderName(i), FALSE);
                    }
                }

                // Drop the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Change Role to None
                // 3. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                FABRIC_EPOCH epoch4; epoch4.DataLossNumber = 2; epoch4.ConfigurationNumber = 3; epoch4.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch4, FABRIC_REPLICA_ROLE_NONE);

                co_await replica->CloseAsync();
            }

            co_return;
        }

        Awaitable<void> BackCompatibleBackupRestoreTests::Test_Restore_Backup_Success(
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
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                // Get OnDataLossAsync and become fully Primary
                // 1. OnDataLossAsync
                // 2. Update Status: Granted
                // TODO: UpdateCatchupConfig & WaitForQuorumCatchup for 1 replica?
                BOOLEAN isStateChanged = false;
                status = co_await replica->OnDataLossAsync(isStateChanged);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(isStateChanged == TRUE);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                wstring dictName = L"fabric:/test/dictionary";

                // Verify restored states
                if (dictNum == 1)
                {
                    co_await this->VerifyDictionaryAsync(dictName, -1, *replica);
                }
                else
                {
                    for(int i = 0; i < dictNum; ++i)
                    {
                        co_await this->VerifyDictionaryAsync(dictName, i, *replica);
                    }
                }

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_NONE);
                co_await replica->CloseAsync();
            }

            co_return;
        }

        Awaitable<void> BackCompatibleBackupRestoreTests::Test_Backup_WithoutPrepareCheckpointLSNProperty(
            __in wstring const & testFolder,
            __in wstring const & backupFolder,
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
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                // Get OnDataLossAsync and become fully Primary
                // 1. OnDataLossAsync
                // 2. Update Status: Granted
                // TODO: UpdateCatchupConfig & WaitForQuorumCatchup for 1 replica?
                BOOLEAN isStateChanged = false;
                status = co_await replica->OnDataLossAsync(isStateChanged);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(isStateChanged == TRUE);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(
                    *backupFolderPath,
                    allocator);

                BackupInfo result;
                status = co_await replica->TxnReplicator->BackupAsync(*backupCallbackHandler, result);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_NONE);
                co_await replica->CloseAsync();
            }

            co_return;
        }

        Awaitable<void> BackCompatibleBackupRestoreTests::VerifyDictionaryAsync(
            __in wstring dictName, 
            __in int dictIndex, 
            __in Replica const & replica)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;
            if (dictIndex > -1)
            {
                dictName.append(std::to_wstring(dictIndex));
            }

            {
                KUri::CSPtr dictionaryName;
                status = KUri::Create(dictName.c_str(), underlyingSystem_->PagedAllocator(), dictionaryName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                IStateProvider2::SPtr stateProvider2;
                status = replica.TxnReplicator->Get(*dictionaryName, stateProvider2);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider2);
            }

            {
                KUri::CSPtr dataStoreName;
                wstring dataStoreString = dictName + L"/dataStore";
                status = KUri::Create(dataStoreString.c_str(), underlyingSystem_->PagedAllocator(), dataStoreName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                IStateProvider2::SPtr stateProvider2;
                status = replica.TxnReplicator->Get(*dataStoreName, stateProvider2);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(stateProvider2);
                TStore::IStore <int, int> & store = dynamic_cast<Data::TStore::IStore<int, int> &>(*stateProvider2);
                VERIFY_ARE_EQUAL(2, store.Count);
                {
                    Transaction::SPtr txn = nullptr;
                    status = replica.TxnReplicator->CreateTransaction(txn);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    KFinally([&] { txn->Dispose(); });

                    Data::TStore::IStoreTransaction<int, int>::SPtr storeTxn;
                    store.CreateOrFindTransaction(*txn, storeTxn);

                    int key1 = 0;
                    int key2 = 1;

                    KeyValuePair<FABRIC_SEQUENCE_NUMBER, int> value1;
                    bool isKey1Found = co_await store.ConditionalGetAsync(*storeTxn, key1, TimeSpan::MaxValue, value1, CancellationToken::None);
                    VERIFY_IS_TRUE(isKey1Found);
                    VERIFY_ARE_EQUAL(key1, value1.Value)

                        KeyValuePair<FABRIC_SEQUENCE_NUMBER, int> value2;
                    bool isKey2Found = co_await store.ConditionalGetAsync(*storeTxn, key2, TimeSpan::MaxValue, value2, CancellationToken::None);
                    VERIFY_IS_TRUE(isKey2Found);
                    VERIFY_ARE_EQUAL(key2, value2.Value)

                        status = txn->Abort();
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }
            }

            co_return;
        }

        void BackCompatibleBackupRestoreTests::VerifyIfStateProviderExists(
            __in ITransactionalReplicator & replicator,
            __in KUri const & stateProviderName,
            __in BOOLEAN expectResult)
        {
            IStateProvider2::SPtr stateProvider;
            NTSTATUS status = replicator.Get(stateProviderName, stateProvider);
            VERIFY_ARE_EQUAL(status, expectResult ? STATUS_SUCCESS : SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_ARE_EQUAL(expectResult, stateProvider != nullptr);
        }

        wstring BackCompatibleBackupRestoreTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);
            return testFolderPath;
        }

        void BackCompatibleBackupRestoreTests::InitializeKtlConfig(
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

        KUri::CSPtr BackCompatibleBackupRestoreTests::GetStateProviderName(
            __in int stateProviderIndex)
        {
            wstring stateProviderName = wformatString(L"fabric:/store/{0}", stateProviderIndex);

            KUri::CSPtr spName;
            NTSTATUS status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            return spName;
        }

        void BackCompatibleBackupRestoreTests::EndTest()
        {
        }

        wstring BackCompatibleBackupRestoreTests::SetupBackupFolder(__in BackCompatibleBackupRestoreTests::TestCase testCase)
        {
            wstring backupFolder = Common::Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(backupFolder, L"BackupContainer");
            Directory::Delete_WithRetry(backupFolder, true, true);

            wstring restorePath = Common::Directory::GetCurrentDirectoryW();
#ifdef PLATFORM_UNIX
            Path::CombineInPlace(restorePath, L"data_upgrade");
#else
            Path::CombineInPlace(restorePath, L"UpgradeTests");
#endif
            Path::CombineInPlace(restorePath, L"5.1");
            Path::CombineInPlace(restorePath, L"NativeStateManager");
            Path::CombineInPlace(restorePath, L"ManagedBackupFolder");

            switch (testCase)
            {
            case TestCase::Empty:
                Path::CombineInPlace(restorePath, L"Empty");
                break;
            case OneDict_Full:
                Path::CombineInPlace(restorePath, L"OneDict_Full");
                break;
            case FourDict_Full:
                Path::CombineInPlace(restorePath, L"FourDict_Full");
                break;
            case FourDict_Full_Checkpointed:
                Path::CombineInPlace(restorePath, L"FourDict_Full_Checkpointed");
                break;
            case FourDict_FullAndIncremental:
                Path::CombineInPlace(restorePath, L"FourDict_FullAndIncremental");
                break;
            case FoutDict_FullAndIncremental_Checkpointed:
                Path::CombineInPlace(restorePath, L"FoutDict_FullAndIncremental_Checkpointed");
                break;
            case WithoutCheckpointLSN:
                Path::CombineInPlace(restorePath, L"BackupWithoutPrepareCheckpointLSN");
                break;
            default:
                ASSERT_IF(true, "BackCompatibleBackupRestoreTests test case is invalid.")
            }

            ErrorCode error = Common::Directory::Copy(restorePath.c_str(), backupFolder.c_str(), true);
            VERIFY_IS_TRUE(error.IsSuccess());

            return backupFolder;
        }

        BOOST_FIXTURE_TEST_SUITE(BackCompatibleBackupRestoreTestSuite, BackCompatibleBackupRestoreTests);

        // Restore to the empty backup from managed, and current state manager has no states.
        BOOST_AUTO_TEST_CASE(Restore_EmptyBackup_NoState_Success)
        {
            // Setup
            wstring testName(L"Restore_EmptyBackup_NoState_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::Empty);

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
                SyncAwait(Test_Restore_EmptyBackup_Success(
                    workFolder,
                    backupFolder,
                    false,
                    0, 
                    0, 
                    *logManager,
                    FABRIC_RESTORE_POLICY_FORCE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the empty backup from managed, and current state manager has active states.
        BOOST_AUTO_TEST_CASE(Restore_EmptyBackup_ActiveStates_Success)
        {
            // Setup
            wstring testName(L"Restore_EmptyBackup_ActiveStates_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::Empty);

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
                SyncAwait(Test_Restore_EmptyBackup_Success(
                    workFolder,
                    backupFolder,
                    false,
                    8,
                    0,
                    *logManager,
                    FABRIC_RESTORE_POLICY_FORCE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the empty backup from managed, and current state manager has active and deleted states.
        BOOST_AUTO_TEST_CASE(Restore_EmptyBackup_ActiveAndDeletedStates_Success)
        {
            // Setup
            wstring testName(L"Restore_EmptyBackup_ActiveAndDeletedStates_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::Empty);

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
                SyncAwait(Test_Restore_EmptyBackup_Success(
                    workFolder,
                    backupFolder,
                    false,
                    8,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_FORCE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the empty backup from managed, and current state manager has clean state.
        BOOST_AUTO_TEST_CASE(Restore_EmptyBackup_CleanStates_Success)
        {
            // Setup
            wstring testName(L"Restore_EmptyBackup_CleanStates_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::Empty);

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
                SyncAwait(Test_Restore_EmptyBackup_Success(
                    workFolder,
                    backupFolder,
                    true,
                    8,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_FORCE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the empty backup from managed, and current state manager has dirty states. Since the restore policy is set to safe,
        // restore should fail.
        BOOST_AUTO_TEST_CASE(RestoreSafe_EmptyBackup_DirtyStates_Success)
        {
            // Setup
            wstring testName(L"RestoreSafe_EmptyBackup_DirtyStates_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::Empty);

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
                SyncAwait(Test_Restore_EmptyBackup_Success(
                    workFolder,
                    backupFolder,
                    false,
                    8,
                    4,
                    *logManager,
                    FABRIC_RESTORE_POLICY_SAFE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the Full Backup with one item from managed.
        BOOST_AUTO_TEST_CASE(Restore_OneItem_WithoutCheckpoint_Success)
        {
            // Setup
            wstring testName(L"Restore_OneItem_WithoutCheckpoint_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::OneDict_Full);

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
                SyncAwait(Test_Restore_Backup_Success(
                    workFolder,
                    backupFolder,
                    1,
                    *logManager,
                    FABRIC_RESTORE_POLICY_SAFE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the Full Backup with four items from managed.
        BOOST_AUTO_TEST_CASE(Restore_FourItems_WithoutCheckpoint_Success)
        {
            // Setup
            wstring testName(L"Restore_FourItems_WithoutCheckpoint_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::FourDict_Full);

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
                SyncAwait(Test_Restore_Backup_Success(
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

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the Full Backup with four checkpointed items from managed.
        BOOST_AUTO_TEST_CASE(Restore_Backup_Four_Checkpointed_Success)
        {
            // Setup
            wstring testName(L"Restore_Backup_Four_Checkpointed_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::FourDict_Full_Checkpointed);

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
                SyncAwait(Test_Restore_Backup_Success(
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

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the Full and Incremental Backup with four items from managed.
        BOOST_AUTO_TEST_CASE(Restore_Backup_FullAndIncremental_Success)
        {
            // Setup
            wstring testName(L"Restore_Backup_FullAndIncremental_Success");
            wstring testFolderPath = CreateFileName(testName);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::FourDict_FullAndIncremental);

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
                SyncAwait(Test_Restore_Backup_Success(
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

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        // Restore to the Full and Incremental Backup with four checkpointed items from managed.
        BOOST_AUTO_TEST_CASE(Restore_Backup_FullAndIncremental_Checkpointed_Success)
        {
            // Setup
            wstring testName(L"Restore_Backup_FullAndIncremental_Checkpointed_Success");
            wstring testFolderPath = CreateFileName(testName);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::FoutDict_FullAndIncremental_Checkpointed);

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
                SyncAwait(Test_Restore_Backup_Success(
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

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Backup_WithoutPrepareCheckpointLSNProperty_Success)
        {
            // Setup
            wstring testName(L"Backup_WithoutPrepareCheckpointLSNProperty_Success");
            wstring testFolderPath = CreateFileName(testName);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = SetupBackupFolder(TestCase::WithoutCheckpointLSN);

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
                SyncAwait(Test_Backup_WithoutPrepareCheckpointLSNProperty(
                    workFolder,
                    backupFolder,
                    *logManager,
                    FABRIC_RESTORE_POLICY_FORCE,
                    partitionId,
                    replicaId));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_SUITE_END();
    }
}
