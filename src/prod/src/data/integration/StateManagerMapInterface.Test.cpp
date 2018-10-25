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

        StringLiteral const TraceComponent("StateManagerMapInterfaceTest");
        std::wstring const TestLogFileName(L"ComTransactionalReplicatorTest.log");

        class StateManagerMapInterfaceTest
        {
        public:
            enum ISM 
            {
                Get = 0,
                AddAsync =1,
                RemoveAsync = 2,
                CreateEnumerator = 3,
                GetOrAddAsync = 4
            };

        public:
            Awaitable<void> Run_StateManagerInterface_WithoutStatusGranted(
                __in ISM ism, 
                __in wstring const & testFolder,
                __in Data::Log::LogManager & logManager,
                __in KGuid const & sourcePartitionId,
                __in FABRIC_REPLICA_ID sourceReplicaID);

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

        protected:
            CommonConfig config; // load the configuration object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;
        };

        Awaitable<void> StateManagerMapInterfaceTest::Run_StateManagerInterface_WithoutStatusGranted(
            __in ISM ism,
            __in wstring const & testFolder,
            __in Data::Log::LogManager & logManager,
            __in KGuid const & sourcePartitionId,
            __in FABRIC_REPLICA_ID sourceReplicaID)
        {
            NTSTATUS status = STATUS_UNSUCCESSFUL;

            KAllocator & allocator = underlyingSystem_->PagedAllocator();

            // #10145496: Using TStore instead of TSP here.
            TxnReplicator::TestCommon::TestStateProviderFactory::SPtr stateProviderFactory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(allocator);
            KStringView typeName(
                (PWCHAR)TestStateProvider::TypeName.cbegin(),
                (ULONG)TestStateProvider::TypeName.size() + 1,
                (ULONG)TestStateProvider::TypeName.size());

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

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);

                KUri::CSPtr stateProviderName = GetStateProviderName(++lastInsertedStateProviderIndex);
                TxnReplicator::IStateProvider2::SPtr stateProvider = nullptr;
                Transaction::SPtr txn = nullptr;

                if (ism == StateManagerMapInterfaceTest::Get)
                {
                    // This is the read case, the SM will check read status and return SF_STATUS_NOT_READABLE if not granted.
                    status = replica->TxnReplicator->Get(*stateProviderName, stateProvider);
                    VERIFY_ARE_EQUAL(status, SF_STATUS_NOT_READABLE);
                    VERIFY_IS_NULL(stateProvider);
                }
                else if (ism == StateManagerMapInterfaceTest::AddAsync)
                {
                    // In the add case, if status Not_Primary, will return Not_Primary status.
                    // In the transient case, we let the write go through, here it will fail since txn
                    // AddOperation will fail with SF_STATUS_TRANSACTION_NOT_ACTIVE
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });
                    status = co_await replica->TxnReplicator->AddAsync(
                        *txn,
                        *stateProviderName,
                        typeName);
                    VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_NOT_ACTIVE);
                }
                else if (ism == StateManagerMapInterfaceTest::GetOrAddAsync)
                {
                    // GetOrAddAsync will check Get call first, so it will check read status first
                    // and return SF_STATUS_NOT_READABLE in this case.
                    bool exist = false;
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });
                    status = co_await replica->TxnReplicator->GetOrAddAsync(
                        *txn,
                        *stateProviderName,
                        typeName,
                        stateProvider,
                        exist);
                    VERIFY_ARE_EQUAL(status, SF_STATUS_NOT_READABLE);
                    VERIFY_IS_NULL(stateProvider);
                }
                else if (ism == StateManagerMapInterfaceTest::CreateEnumerator)
                {
                    // This is the read case, the SM will check read status and return SF_STATUS_NOT_READABLE if not granted.
                    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr enumerator = nullptr;;
                    status = replica->TxnReplicator->CreateEnumerator(
                        true,
                        enumerator);
                    VERIFY_ARE_EQUAL(status, SF_STATUS_NOT_READABLE);
                    VERIFY_IS_NULL(enumerator);
                }
                else if (ism == StateManagerMapInterfaceTest::RemoveAsync)
                {
                    // We let the write go through in the transient case, but here SM will check whether 
                    // the item is in MetadataManager, so failed with SF_STATUS_NAME_DOES_NOT_EXIST.
                    Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, TxnReplicator::IStateProvider2::SPtr>>::SPtr enumerator = nullptr;
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });
                    status = co_await replica->TxnReplicator->RemoveAsync(
                        *txn,
                        *stateProviderName);
                    VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
                }
                else
                {
                    // Code should not reach here.
                    VERIFY_IS_TRUE(false);
                }

                co_await replica->CloseAsync();
            }

            co_return;
        }

        wstring StateManagerMapInterfaceTest::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        KUri::CSPtr StateManagerMapInterfaceTest::GetStateProviderName(
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

        void StateManagerMapInterfaceTest::EndTest()
        {
        }

        void StateManagerMapInterfaceTest::InitializeKtlConfig(
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

        BOOST_FIXTURE_TEST_SUITE(StateManagerMapInterfaceTestSuite, StateManagerMapInterfaceTest);

        BOOST_AUTO_TEST_CASE(ISM_Get_StatusNotGranted_Return)
        {
            // Setup
            wstring testName(L"ISM_Get_StatusNotGranted_Return");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

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
                SyncAwait(Run_StateManagerInterface_WithoutStatusGranted(
                    ISM::Get,
                    workFolder,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId));

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

        BOOST_AUTO_TEST_CASE(ISM_AddAsync_StatusNotGranted_Return)
        {
            // Setup
            wstring testName(L"ISM_Get_StatusNotGranted_Return");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

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
                SyncAwait(Run_StateManagerInterface_WithoutStatusGranted(
                    ISM::AddAsync,
                    workFolder,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId));

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

        BOOST_AUTO_TEST_CASE(ISM_GetOrAddAsync_StatusNotGranted_Return)
        {
            // Setup
            wstring testName(L"ISM_Get_StatusNotGranted_Return");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

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
                SyncAwait(Run_StateManagerInterface_WithoutStatusGranted(
                    ISM::GetOrAddAsync,
                    workFolder,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId));

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

        BOOST_AUTO_TEST_CASE(ISM_CreateEnumerator_StatusNotGranted_Return)
        {
            // Setup
            wstring testName(L"ISM_Get_StatusNotGranted_Return");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

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
                SyncAwait(Run_StateManagerInterface_WithoutStatusGranted(
                    ISM::CreateEnumerator,
                    workFolder,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId));

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

        BOOST_AUTO_TEST_CASE(ISM_RemoveAsync_StatusNotGranted_Return)
        {
            // Setup
            wstring testName(L"ISM_Get_StatusNotGranted_Return");
            wstring testFolderPath = CreateFileName(testName);

            // Start up cleanup.
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

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
                SyncAwait(Run_StateManagerInterface_WithoutStatusGranted(
                    ISM::RemoveAsync,
                    workFolder,
                    *logManager,
                    sourcePartitionId,
                    sourceReplicaId));

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

        BOOST_AUTO_TEST_SUITE_END();
    }
}
