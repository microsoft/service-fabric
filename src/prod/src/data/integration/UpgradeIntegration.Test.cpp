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

        StringLiteral const TraceComponent("UpgradeIntegrationTests");
        std::wstring const TestLogFileName(L"ComTransactionalReplicatorTest.log");
        std::wstring const windowsDirectoryPath = L"UpgradeTests";
        std::wstring const linuxDirectoryPath = Common::Path::Combine(L"data_upgrade", L"5.1");

        class UpgradeIntegrationTests
        {
        public:
            Awaitable<void> Upgrade_From_Managed_LR(
                __in wstring const & workFolder,
                __in Data::Log::LogManager & logManager,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId);

            void CreateLogRecordUpgradeTest_WorkingDirectory(
                __in std::wstring & upgradeVersionPath,
                __in KAllocator & allocator,
                __out KString::SPtr & workDir)
            {
                Trace.WriteInfo(TraceComponent, "Creating log record upgrade test working dir");

                std::wstring workingDir = wformatString("{0}_tmp_upgrade_integration", upgradeVersionPath);
                std::wstring dir2 = L"";

#if !defined(PLATFORM_UNIX)        
                dir2 = windowsDirectoryPath;
#else
                dir2 = linuxDirectoryPath;
#endif

                ASSERT_IFNOT(dir2 != L"", "Directory path must be set");

                Trace.WriteInfo(TraceComponent, "Dir 2 : {0}", dir2);

                // Current directory
                std::wstring prevLogFileDirectory = Common::Directory::GetCurrentDirectoryW();
                std::wstring workingDirectory = Common::Directory::GetCurrentDirectoryW();

                prevLogFileDirectory = Common::Path::Combine(prevLogFileDirectory, dir2);
                workingDirectory = Common::Path::Combine(workingDirectory, dir2);

                prevLogFileDirectory = Common::Path::Combine(prevLogFileDirectory, upgradeVersionPath);
                workingDirectory = Common::Path::Combine(workingDirectory, workingDir);

                // Verify the target upgrade path must exist
                // Ex. WindowsFabric\out\debug-amd64\bin\FabricUnitTests\UpgradeTests\version_60
                bool directoryExists = Common::Directory::Exists(prevLogFileDirectory);
                ASSERT_IFNOT(directoryExists, "Existing log path: {0} not found", prevLogFileDirectory);

                ErrorCode er = Common::Directory::Copy(prevLogFileDirectory, workingDirectory, true);
                ASSERT_IFNOT(er.IsSuccess(), "Failed to create tmp working directory. Error : {0}", er.ErrorCodeValueToString());

                workDir = KString::Create(workingDirectory.c_str(), allocator);
            }

        protected:
            wstring CreateFileName(
                __in wstring const & folderName);

            KUri::CSPtr GetStateProviderName(
                __in int stateProviderIndex);

            void EndTest();
            
        protected:
            CommonConfig config; // load the config object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;
        };

        Awaitable<void> UpgradeIntegrationTests::Upgrade_From_Managed_LR(
            __in wstring const & testFolder,
            __in Data::Log::LogManager & logManager,
            __in KGuid const & partitionId,
            __in FABRIC_REPLICA_ID replicaId)
        {
            UpgradeStateProviderFactory::SPtr stateProviderFactory = UpgradeStateProviderFactory::Create(underlyingSystem_->PagedAllocator());

            {
                Replica::SPtr replica = Replica::Create(
                    partitionId,
                    replicaId,
                    testFolder,
                    logManager,
                    underlyingSystem_->PagedAllocator(),
                    stateProviderFactory.RawPtr(),
                    nullptr); // Default FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION is managed (0).

                co_await replica->OpenAsync();

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                FABRIC_EPOCH epoch2; epoch2.DataLossNumber = 1; epoch2.ConfigurationNumber = 2; epoch2.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch2, FABRIC_REPLICA_ROLE_PRIMARY);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                {
                    KUri::CSPtr dictionaryName;
                    KUri::Create(KUriView(L"fabric:/test/dictionary"), underlyingSystem_->PagedAllocator(), dictionaryName);

                    IStateProvider2::SPtr stateProvider2;
                    NTSTATUS status = replica->TxnReplicator->Get(*dictionaryName, stateProvider2);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider2);
                }

                {
                    KUri::CSPtr dataStoreName;
                    KUri::Create(KUriView(L"fabric:/test/dictionary/dataStore"), underlyingSystem_->PagedAllocator(), dataStoreName);

                    IStateProvider2::SPtr stateProvider2;
                    NTSTATUS status = replica->TxnReplicator->Get(*dataStoreName, stateProvider2);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    VERIFY_IS_NOT_NULL(stateProvider2);
                    TStore::IStore < KString::SPtr, KBuffer::SPtr> & store = dynamic_cast<Data::TStore::IStore<KString::SPtr, KBuffer::SPtr> &>(*stateProvider2);
                    VERIFY_ARE_EQUAL(2, store.Count);
                    {
                        Transaction::SPtr txn = nullptr;
                        status = replica->TxnReplicator->CreateTransaction(txn);
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        KFinally([&] { txn->Dispose(); });

                        Data::TStore::IStoreTransaction<KString::SPtr, KBuffer::SPtr>::SPtr storeTxn;
                        store.CreateOrFindTransaction(*txn, storeTxn);

                        KString::SPtr key1;
                        status = KString::Create(key1, underlyingSystem_->PagedAllocator(), L"1");
                        VERIFY_IS_TRUE(NT_SUCCESS(status));

                        KString::SPtr key2;
                        status = KString::Create(key2, underlyingSystem_->PagedAllocator(), L"2");
                        VERIFY_IS_TRUE(NT_SUCCESS(status));

                        KeyValuePair<FABRIC_SEQUENCE_NUMBER, KBuffer::SPtr> value1;
                        bool isKey1Found = co_await store.ConditionalGetAsync(*storeTxn, key1, TimeSpan::MaxValue, value1, CancellationToken::None);
                        VERIFY_IS_TRUE(isKey1Found);

                        KeyValuePair<FABRIC_SEQUENCE_NUMBER, KBuffer::SPtr> value2;
                        bool isKey2Found = co_await store.ConditionalGetAsync(*storeTxn, key2, TimeSpan::MaxValue, value2, CancellationToken::None);
                        VERIFY_IS_TRUE(isKey2Found);

                        status = txn->Abort();
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                    
                }

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                FABRIC_EPOCH epoch3; epoch3.DataLossNumber = 1; epoch3.ConfigurationNumber = 3; epoch3.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch3, FABRIC_REPLICA_ROLE_NONE);
                co_await replica->CloseAsync();
            }

            co_return;
        }

        wstring UpgradeIntegrationTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void UpgradeIntegrationTests::EndTest()
        {
        }

        KUri::CSPtr UpgradeIntegrationTests::GetStateProviderName(
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

    BOOST_FIXTURE_TEST_SUITE(UpgradeIntegrationTestSuite, UpgradeIntegrationTests);

        BOOST_AUTO_TEST_CASE(Upgrade_From_60_StrKey_ByteVal)
        {
            // Setup
            wstring testName(L"Upgrade_From_60_StrKey_ByteVal");
            TEST_TRACE_BEGIN(testName)
            {
                Common::Guid const existingGuid(L"0935b970-d011-4531-bc3a-7c07ccf8c9d1");
                KGuid const partitionId(existingGuid.AsGUID());
                FABRIC_REPLICA_ID const replicaId = 1;

                // Create working directory from UpgradeTests/version_60
                KString::SPtr workDir;
                std::wstring targetUpgradeVersion = L"version_60_strkey";

                CreateLogRecordUpgradeTest_WorkingDirectory(targetUpgradeVersion, underlyingSystem_->PagedAllocator(), workDir);
                ASSERT_IFNOT(workDir != nullptr, "Working directory cannot be null");

                KtlLogger::SharedLogSettingsSPtr sharedLogSettings = make_shared<KtlLogger::SharedLogSettings>();

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                // Run test
                SyncAwait(Upgrade_From_Managed_LR(
                    static_cast<LPCWSTR>(*workDir), 
                    *logManager,
                    partitionId,
                    replicaId));

                // Close log manager
                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;

                // Clear working directory
                ErrorCode er = Common::Directory::Delete_WithRetry(workDir->operator LPCWSTR(), true, true);
                ASSERT_IFNOT(er.IsSuccess(), "Failed to delete working directory");
            }
        }

    BOOST_AUTO_TEST_SUITE_END();
    }
}
