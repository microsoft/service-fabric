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

        StringLiteral const TraceComponent("BackupTests");
        std::wstring const TestLogFileName(L"ComTransactionalReplicatorTest.log");

        class BackupTests
        {
        public:
            Awaitable<void> Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                __in wstring const & workFolder,
                __in wstring const & backupFolder,
                __in wstring const & testFolderPath,
                __in int numberOfStateProviders,
                __in int numberOfTxnPerSP,
                __in int numberOfOperationPerTxn,
                __in bool immediatelyClose,
                __in FABRIC_REPLICA_ROLE replicaRole);

        public:
            Awaitable<void> Test_RestoreIncremental(
                __in wstring const & workFolder,
                __in wstring const & backupFolder,
                __in int numberOfStateProvidersInFullCheckpoint,
                __in int numberOfStateProvidersPerIteration,
                __in int numberOfIterations,
                __in Data::Log::LogManager & logManager,
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID,
                __in KGuid const & targetPartitionId,
                __in FABRIC_REPLICA_ID targetReplicaID,
                __in FABRIC_RESTORE_POLICY policy);

            Awaitable<void> Test_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                __in wstring const & workFolder,
                __in wstring const & backupFolder,
                __in Data::Log::LogManager & logManager,
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID,
                __in int numberOfStateProviders,
                __in int numberOfTxnPerSP,
                __in int numberOfOperationPerTxn,
                __in bool immediatelyClose,
                __in FABRIC_REPLICA_ROLE replicaRole);

        protected:
            static bool IsSameReplica(
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID,
                __in KGuid const & targetPartitionId,
                __in FABRIC_REPLICA_ID targetReplicaID);

            template<class TKey, class TValue>
            static NTSTATUS GetStore(
                __in ITransactionalReplicator & replicator,
                __in KUri const & stateProviderName,
                __out typename Data::TStore::IStore<TKey, TValue>::SPtr & store);

            static Awaitable<NTSTATUS> PopulateAsync(
                __in KtlSystem & system,
                __in ITransactionalReplicator & replicator,
                __in Data::TStore::IStore<int, int> & store,
                __in int startingKey,
                __in int numberOfTxns,
                __in int numberOfOperationsPerTxn) noexcept;

        protected:
            wstring CreateFileName(
                __in wstring const & folderName);

            void InitializeKtlConfig(
                __in std::wstring workDir,
                __in std::wstring fileName,
                __in KAllocator & allocator,
                __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

            KUri::CSPtr GetStateProviderName(
                __in int stateProviderIndex);

            void EndTest();

        private:
            void VerifyStateProviderExists(
                __in ITransactionalReplicator & replicator,
                __in KUri const & stateProviderName);

        protected:
            CommonConfig config; // load the configuration object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;
        };

        Awaitable<void> BackupTests::Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
            __in wstring const & workFolder,
            __in wstring const & backupFolder,
            __in wstring const & testFolderPath,
            __in int numberOfStateProviders,
            __in int numberOfTxnPerSP,
            __in int numberOfOperationPerTxn,
            __in bool immediatelyClose,
            __in FABRIC_REPLICA_ROLE replicaRole)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            // KTL test input
            KGuid sourcePartitionId;
            sourcePartitionId.CreateNew();
            FABRIC_REPLICA_ID sourceReplicaId = 777;

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

            Data::Log::LogManager::SPtr logManager;
            status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = co_await logManager->OpenAsync(CancellationToken::None, sharedLogSettings);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_await Test_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                workFolder,
                backupFolder,
                *logManager,
                sourcePartitionId,
                sourceReplicaId,
                numberOfStateProviders,
                numberOfTxnPerSP,
                numberOfOperationPerTxn,
                immediatelyClose,
                replicaRole);

            status = co_await logManager->CloseAsync(CancellationToken::None);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            logManager = nullptr;

            co_return;
        }

        Awaitable<void> BackupTests::Test_RestoreIncremental(
            __in wstring const & testFolder,
            __in wstring const & backupFolder,
            __in int numberOfStateProvidersInFullCheckpoint,
            __in int numberOfStateProvidersPerIteration,
            __in int numberOfIterations,
            __in Data::Log::LogManager & logManager,
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID,
            __in KGuid const & targetPartitionId,
            __in FABRIC_REPLICA_ID targetReplicaID,
            __in FABRIC_RESTORE_POLICY policy)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            KAllocator & allocator = underlyingSystem_->PagedAllocator();

            // #10145496: Using TStore instead of TSP here.
            TxnReplicator::TestCommon::TestStateProviderFactory::SPtr stateProviderFactory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(
                allocator);

            KString::SPtr backupFolderPath = KPath::CreatePath(backupFolder.c_str(), allocator);
            TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(
                *backupFolderPath,
                allocator);

            Data::Utilities::OperationData::SPtr initParams = Data::Utilities::OperationData::Create(allocator);
            Data::Utilities::BinaryWriter bw(allocator);
            bw.Write(L"Initial Value", Data::Utilities::UTF16);
            initParams->Append(*bw.GetBuffer(0));

            int lastInsertedStateProviderIndex = -1;
            {
                Replica::SPtr replica = Replica::Create(
                    sourcePartitionId,
                    sourceReplicaID,
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
                
                KStringView typeName(
                    (PWCHAR)TestStateProvider::TypeName.cbegin(),
                    (ULONG)TestStateProvider::TypeName.size() + 1,
                    (ULONG)TestStateProvider::TypeName.size());
                for (int i = 0; i < numberOfStateProvidersInFullCheckpoint; ++i)
                {
                    Transaction::SPtr txn;
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });

                    KUri::CSPtr stateProviderName = GetStateProviderName(++lastInsertedStateProviderIndex);
                    status = co_await replica->TxnReplicator->AddAsync(
                        *txn,
                        *stateProviderName,
                        typeName,
                        initParams.RawPtr());
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    co_await txn->CommitAsync();
                }

                status = replica->TxnReplicator->Test_RequestCheckpointAfterNextTransaction();
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                for (int iteration = 0; iteration < numberOfIterations; ++iteration)
                {
                    for (int i = 0; i < numberOfStateProvidersPerIteration; ++i)
                    {
                        Transaction::SPtr txn;
                        replica->TxnReplicator->CreateTransaction(txn);
                        KFinally([&] {txn->Dispose(); });

                        KUri::CSPtr stateProviderName = GetStateProviderName(++lastInsertedStateProviderIndex);
                        status = co_await replica->TxnReplicator->AddAsync(
                            *txn,
                            *stateProviderName,
                            typeName,
                            initParams.RawPtr());
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        co_await txn->CommitAsync();
                    }

                    if (iteration == 0)
                    {
                        BackupInfo result;
                        status = co_await replica->TxnReplicator->BackupAsync(*backupCallbackHandler, result);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                    else
                    {
                        BackupInfo result;
                        status = co_await replica->TxnReplicator->BackupAsync(*backupCallbackHandler, FABRIC_BACKUP_OPTION_INCREMENTAL, Common::TimeSpan::FromMinutes(4), CancellationToken::None, result);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                }

                // Close the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                bool isSameReplica = IsSameReplica(sourcePartitionId, sourceReplicaID, targetPartitionId, targetReplicaID);
                if (isSameReplica == false)
                {
                    FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 1; epoch3.ConfigurationNumber = 2; epoch3.Reserved = nullptr;
                    co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);
                }

                co_await replica->CloseAsync();
            }

            {
                TestDataLossHandler::SPtr dataLossHandler = TestDataLossHandler::Create(
                    allocator, 
                    backupFolderPath.RawPtr(), 
                    policy);

                Replica::SPtr replica = Replica::Create(
                    targetPartitionId,
                    targetReplicaID,
                    testFolder,
                    logManager,
                    allocator,
                    stateProviderFactory.RawPtr(),
                    nullptr,
                    dataLossHandler.RawPtr());

                dataLossHandler->Initialize(*replica->TxnReplicator);

                // Open the replica.
                co_await replica->OpenAsync();

                // Become Primary after open (For OnDataLossAsync):  
                // 1. Update Status: Pending
                // 2. New Epoch with new data loss number.
                // 3. Change Role to Primary
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 2; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_PRIMARY);

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

                // Verify that the replica is latest version number after recovery.
                {
                    FABRIC_EPOCH currentEpoch;
                    status = replica->TxnReplicator->GetCurrentEpoch(currentEpoch);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    VERIFY_ARE_EQUAL(epoch2.DataLossNumber, 2);
                    VERIFY_ARE_EQUAL(epoch2.ConfigurationNumber, 2);
                }

                // Verify restored state
                for (int i = 0; i <= lastInsertedStateProviderIndex; i++)
                {
                    VerifyStateProviderExists(*replica->TxnReplicator, *GetStateProviderName(i));
                }

                // Drop the replica:  
                // 1. Update Status: NOT PRIMARY
                // 2. Change Role to None
                // 3. Close
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 2; epoch3.ConfigurationNumber = 3; epoch3.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);

                co_await replica->CloseAsync();
            }

            co_return;
        }

        Awaitable<void> BackupTests::Test_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
            __in wstring const & workFolder,
            __in wstring const & backupFolder,
            __in Data::Log::LogManager & logManager,
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID,
            __in int numberOfStateProviders,
            __in int numberOfTxnPerSP,
            __in int numberOfOperationPerTxn,
            __in bool immediatelyClose,
            __in FABRIC_REPLICA_ROLE replicaRole)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            // Verify the inputs.
            if (immediatelyClose)
            {
                VERIFY_ARE_EQUAL(replicaRole, FABRIC_REPLICA_ROLE_UNKNOWN);
            }
            else
            {
                VERIFY_IS_TRUE(replicaRole == FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY || replicaRole == FABRIC_REPLICA_ROLE_NONE);
            }

            KAllocator & allocator = underlyingSystem_->PagedAllocator();

            KString::SPtr backupFolderPath = KPath::CreatePath(backupFolder.c_str(), allocator);
            TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(
                *backupFolderPath,
                allocator);

            Data::Utilities::OperationData::SPtr initParams = Data::Utilities::OperationData::Create(allocator);
            Data::Utilities::BinaryWriter bw(allocator);
            bw.Write(L"Initial Value", Data::Utilities::UTF16);
            initParams->Append(*bw.GetBuffer(0));

            {
                // Setting min log size in MB to 200 to avoid truncation of the log.
                TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = { 0 };
                txnReplicatorSettings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
                txnReplicatorSettings.CheckpointThresholdInMB = 200;

                Replica::SPtr replica = Replica::Create(
                    sourcePartitionId,
                    sourceReplicaID,
                    workFolder,
                    logManager,
                    allocator,
                    nullptr,
                    &txnReplicatorSettings);

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

                // Full backup
                {
                    BackupInfo fullBackupInfo;
                    status = co_await replica->TxnReplicator->BackupAsync(*backupCallbackHandler, fullBackupInfo);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                // Workload to increase the potential race gap. 
                // Note that due to the optimization in the incremental backup path, this became a hard race.
                for (int i = 0; i < numberOfStateProviders; ++i)
                {
                    Transaction::SPtr txn;
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });

                    KUri::CSPtr stateProviderName = GetStateProviderName(i);
                    status = co_await replica->TxnReplicator->AddAsync(
                        *txn,
                        *stateProviderName,
                        L"StoreType",
                        initParams.RawPtr());
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    co_await txn->CommitAsync();
                }

                KArray<Awaitable<NTSTATUS>> awaitableArray(allocator);
                for (int i = 0; i < numberOfStateProviders; ++i)
                {
                    KUri::CSPtr stateProviderName = GetStateProviderName(i);
                    Data::TStore::IStore<int, int>::SPtr store;
                    status = GetStore<int, int>(*replica->TxnReplicator, *stateProviderName, store);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));

                    Awaitable<NTSTATUS> awaitable = PopulateAsync(*underlyingSystem_, *replica->TxnReplicator, *store, 0, numberOfTxnPerSP, numberOfOperationPerTxn);
                    status = awaitableArray.Append(Ktl::Move(awaitable));
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                status = co_await Data::Utilities::TaskUtilities<NTSTATUS>::WhenAll_NoException(awaitableArray);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                // Incremental backup before the test incremental backup to increase the range.
                {
                    BackupInfo incrementalBackupInfo;
                    status = co_await replica->TxnReplicator->BackupAsync(
                        *backupCallbackHandler, 
                        FABRIC_BACKUP_OPTION_INCREMENTAL, 
                        Common::TimeSpan::FromMinutes(4), 
                        CancellationToken::None, 
                        incrementalBackupInfo);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                // Setup the backup handler so that we can block it.
                AwaitableCompletionSource<void>::SPtr acs;
                status = AwaitableCompletionSource<void>::Create(allocator, KTL_TAG_TEST, acs);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                backupCallbackHandler->SetBackupCallbackACS(*acs);

                // The test backup call.
                BackupInfo backupInfo;
                Awaitable<NTSTATUS> backupAwaitable = replica->TxnReplicator->BackupAsync(
                    *backupCallbackHandler,
                    FABRIC_BACKUP_OPTION_INCREMENTAL,
                    Common::TimeSpan::FromMinutes(4),
                    CancellationToken::None, 
                    backupInfo);
                VERIFY_IS_TRUE(NT_SUCCESS(status));

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                // Racing change role to none.
                Awaitable<void> racingAwaitable;
                if (immediatelyClose == false)
                {
                    FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                    racingAwaitable = replica->ChangeRoleAsync(epoch2, replicaRole);
                }
                else
                {
                    racingAwaitable = replica->CloseAsync();
                }
                
                VERIFY_IS_FALSE(racingAwaitable.IsComplete());
                VERIFY_IS_FALSE(backupAwaitable.IsComplete());

                acs->Set();

                co_await racingAwaitable;

                // Test that backup has been drained.
                status = co_await backupAwaitable;
                VERIFY_ARE_EQUAL(SF_STATUS_NOT_PRIMARY, status);

                if (immediatelyClose != true)
                {
                    if (replicaRole != FABRIC_REPLICA_ROLE_NONE)
                    {
                        FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 1; epoch3.ConfigurationNumber = 3; epoch3.Reserved = nullptr;
                        co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);
                    }

                    co_await replica->CloseAsync();
                }
            }

            if (immediatelyClose == true)
            {
                // Setting min log size in MB to 200 to avoid truncation of the log.
                TRANSACTIONAL_REPLICATOR_SETTINGS txnReplicatorSettings = { 0 };
                txnReplicatorSettings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
                txnReplicatorSettings.CheckpointThresholdInMB = 200;

                Replica::SPtr replica = Replica::Create(
                    sourcePartitionId,
                    sourceReplicaID,
                    workFolder,
                    logManager,
                    allocator,
                    nullptr,
                    &txnReplicatorSettings);

                co_await replica->OpenAsync();

                FABRIC_EPOCH epoch4; epoch4.DataLossNumber = 1; epoch4.ConfigurationNumber = 4; epoch4.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch4, FABRIC_REPLICA_ROLE_NONE);

                co_await replica->CloseAsync();
            }

            co_return;
        }

        bool BackupTests::IsSameReplica(
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID,
            __in KGuid const & targetPartitionId,
            __in FABRIC_REPLICA_ID targetReplicaID)
        {
            return sourcePartitionId == targetPartitionId && sourceReplicaID == targetReplicaID;
        }

        template<class TKey, class TValue>
        NTSTATUS BackupTests::GetStore(
            __in ITransactionalReplicator & replicator,
            __in KUri const & stateProviderName,
            __out typename Data::TStore::IStore<TKey, TValue>::SPtr & store)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            TxnReplicator::IStateProvider2::SPtr stateProvider;
            status = replicator.Get(stateProviderName, stateProvider);
            RETURN_ON_FAILURE(status);

            store = &dynamic_cast<Data::TStore::IStore<TKey, TValue> &>(*stateProvider);
            return status;
        }

        Awaitable<NTSTATUS> BackupTests::PopulateAsync(
            __in KtlSystem & system,
            __in ITransactionalReplicator & replicator,
            __in Data::TStore::IStore<int, int> & store,
            __in int startingKey,
            __in int numberOfTxns,
            __in int numberOfOperationsPerTxn) noexcept
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            co_await ktl::CorHelper::ThreadPoolThread(system.DefaultThreadPool());

            int currentKey = startingKey;

            for (int i = 0; i < numberOfTxns; ++i)
            {
                Transaction::SPtr txn;
                status = replicator.CreateTransaction(txn);
                CO_RETURN_ON_FAILURE(status);

                KFinally([txn] {txn->Dispose(); });

                Data::TStore::IStoreTransaction<int, int>::SPtr storeTxn;
                store.CreateOrFindTransaction(*txn, storeTxn);

                for (int opIndex = 0; opIndex < numberOfOperationsPerTxn; ++opIndex)
                {
                    co_await store.AddAsync(*storeTxn, currentKey, currentKey, TimeSpan::FromSeconds(4), CancellationToken::None);
                    currentKey++;
                }

                status = co_await txn->CommitAsync();
                CO_RETURN_ON_FAILURE(status);
            }

            co_return status;
        }

        wstring BackupTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void BackupTests::EndTest()
        {
        }

        void BackupTests::VerifyStateProviderExists(
            __in ITransactionalReplicator & replicator,
            __in KUri const & stateProviderName)
        {
            NTSTATUS status;

            IStateProvider2::SPtr stateProvider;
            status = replicator.Get(stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
        }

        void BackupTests::InitializeKtlConfig(
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
        };

        KUri::CSPtr BackupTests::GetStateProviderName(
            __in int stateProviderIndex)
        {
            wstring stateProviderName = wformatString(L"fabric:/store/{0}", stateProviderIndex);

            KUri::CSPtr spName;
            NTSTATUS status = KUri::Create(KStringView(stateProviderName.c_str()), underlyingSystem_->PagedAllocator(), spName);
            if (NT_SUCCESS(status) == false)
            {
                throw Exception(status);
            }

            return spName;
        }

        BOOST_FIXTURE_TEST_SUITE(BackupTestSuite, BackupTests);

        BOOST_AUTO_TEST_CASE(Restore_Full_SameReplica_Success)
        {
            // Setup
            wstring testName(L"Restore_Full_SameReplica_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                // KTL test input
                KGuid sourcePartitionId;
                sourcePartitionId.CreateNew();
                FABRIC_REPLICA_ID const sourceReplicaId = 777;

                // Test Setup
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                // Test call
                SyncAwait(Test_RestoreIncremental(
                    workFolder, 
                    backupFolder, 
                    16, 
                    32, 
                    1, 
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId,
                    sourcePartitionId,
                    sourceReplicaId,
                    FABRIC_RESTORE_POLICY_FORCE));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Restore_Full_DifferentTargetPartition_Success)
        {
            // Setup
            wstring testName(L"Restore_Full_DifferentTargetPartition_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                // KTL test input
                KGuid sourcePartitionId;
                sourcePartitionId.CreateNew();
                FABRIC_REPLICA_ID sourceReplicaId = 777;

                KGuid targetPartitionId;
                targetPartitionId.CreateNew();
                FABRIC_REPLICA_ID targetReplicaId = 555;

                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(Test_RestoreIncremental(
                    workFolder, 
                    backupFolder, 
                    16, 
                    32, 
                    1, 
                    *logManager, 
                    sourcePartitionId, 
                    sourceReplicaId, 
                    targetPartitionId, 
                    targetReplicaId,
                    FABRIC_RESTORE_POLICY_SAFE));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Restore_Incremental_SameReplica_Success)
        {
            // Setup
            wstring testName(L"Restore_Incremental_SameReplica_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                // KTL test input
                KGuid sourcePartitionId;
                sourcePartitionId.CreateNew();
                FABRIC_REPLICA_ID sourceReplicaId = 777;

                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(Test_RestoreIncremental(
                    workFolder,
                    backupFolder,
                    16,
                    16,
                    4,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId,
                    sourcePartitionId,
                    sourceReplicaId,
                    FABRIC_RESTORE_POLICY_FORCE));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Restore_Incremental_DifferentTargetPartition_Success)
        {
            // Setup
            wstring testName(L"Restore_Incremental_DifferentTargetPartition_Success");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                // KTL test input
                KGuid sourcePartitionId;
                sourcePartitionId.CreateNew();
                FABRIC_REPLICA_ID sourceReplicaId = 777;

                KGuid targetPartitionId;
                targetPartitionId.CreateNew();
                FABRIC_REPLICA_ID targetReplicaId = 555;

                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(Test_RestoreIncremental(
                    workFolder, 
                    backupFolder, 
                    16, 
                    16, 
                    4, 
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId,
                    targetPartitionId,
                    targetReplicaId,
                    FABRIC_RESTORE_POLICY_SAFE));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Backup_ConcurrentClose_WithoutOperations_DrainTheBackup)
        {
            int const StateProviderCount = 0;
            int const NumberOfTxnPerSP = 0;
            int const NumberOfOpPerTxn = 0;

            // Setup
            wstring testName(L"Backup_ConcurrentClose_WithoutOperations_DrainTheBackup");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                SyncAwait(Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                    workFolder,
                    backupFolder,
                    testFolderPath,
                    StateProviderCount,
                    NumberOfTxnPerSP,
                    NumberOfOpPerTxn,
                    true,
                    FABRIC_REPLICA_ROLE_UNKNOWN));
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Backup_ConcurrentClose_WithOperations_DrainTheBackup)
        {
            int const StateProviderCount = 8;
            int const NumberOfTxnPerSP = 8;
            int const NumberOfOpPerTxn = 8;

            // Setup
            wstring testName(L"Backup_ConcurrentClose_WithOperations_DrainTheBackup");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                SyncAwait(Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                    workFolder,
                    backupFolder,
                    testFolderPath,
                    StateProviderCount,
                    NumberOfTxnPerSP,
                    NumberOfOpPerTxn,
                    true,
                    FABRIC_REPLICA_ROLE_UNKNOWN));
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }


        BOOST_AUTO_TEST_CASE(Backup_Concurency_Backup_ChangeRoleToSecondary_DrainTheBackup)
        {
            int const StateProviderCount = 8;
            int const NumberOfTxnPerSP = 8;
            int const NumberOfOpPerTxn = 8;

            // Setup
            wstring testName(L"Backup_Concurency_Backup_ChangeRoleToSecondary_DrainTheBackup");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");


            TEST_TRACE_BEGIN(testName)
            {
                SyncAwait(Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                    workFolder,
                    backupFolder,
                    testFolderPath,
                    StateProviderCount,
                    NumberOfTxnPerSP,
                    NumberOfOpPerTxn,
                    false,
                    FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY));
            }

            // Post-clean up
            if (Directory::Exists(testFolderPath))
            {
                Directory::Delete_WithRetry(testFolderPath, true, true);
            }
        }

        BOOST_AUTO_TEST_CASE(Backup_Concurency_Backup_ChangeRoleToNone_DrainTheBackup)
        {
            int const StateProviderCount = 8;
            int const NumberOfTxnPerSP = 8;
            int const NumberOfOpPerTxn = 8;

            // Setup
            wstring testName(L"Backup_Concurency_Backup_ChangeRoleToNone_DrainTheBackup");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");
            wstring backupFolder = Path::Combine(testFolderPath, L"backup");

            TEST_TRACE_BEGIN(testName)
            {
                SyncAwait(Run_Backup_Concurency_Backup_ChangeRoleOrClose_DrainTheBackup(
                    workFolder,
                    backupFolder,
                    testFolderPath,
                    StateProviderCount,
                    NumberOfTxnPerSP,
                    NumberOfOpPerTxn,
                    false,
                    FABRIC_REPLICA_ROLE_NONE));
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
