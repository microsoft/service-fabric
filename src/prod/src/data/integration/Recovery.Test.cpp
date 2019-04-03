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

        StringLiteral const TraceComponent("RestoreTests");
        std::wstring const TestLogFileName(L"ComTransactionalReplicatorTest.log");

        class RecoveryTests
        {
        public:
            Awaitable<void> Test_Recovery(
                __in wstring const & workFolder,
                __in Data::Log::LogManager & logManager,
                __in ULONG32 iterationCount);

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

            CommonConfig config; // load the config object as its needed for the tracing to work
            KtlSystem * underlyingSystem_;

            KGuid pId_;
            FABRIC_REPLICA_ID rId_;
            PartitionedReplicaId::SPtr prId_;
        };

        Awaitable<void> RecoveryTests::Test_Recovery(
            __in wstring const & testFolder,
            __in Data::Log::LogManager & logManager,
            __in ULONG32 iterationCount)
        {
            // #10145496: Using TStore instead of TSP here.
            TestStateProviderFactory::SPtr stateProviderFactory = TestStateProviderFactory::Create(
                underlyingSystem_->PagedAllocator());

            {
                Replica::SPtr replica = Replica::Create(
                    pId_,
                    rId_,
                    testFolder,
                    logManager,
                    underlyingSystem_->PagedAllocator(),
                    stateProviderFactory.RawPtr());

                co_await replica->OpenAsync();

                FABRIC_EPOCH epoch1; epoch1.DataLossNumber = 1; epoch1.ConfigurationNumber = 1; epoch1.Reserved = nullptr;
                co_await replica->ChangeRoleAsync(epoch1, FABRIC_REPLICA_ROLE_PRIMARY);

                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED);

                {
                    Transaction::SPtr txn;
                    replica->TxnReplicator->CreateTransaction(txn);
                    KFinally([&] {txn->Dispose(); });

                    KUri::CSPtr stateProviderName = GetStateProviderName(0);
                    KStringView typeName(
                        (PWCHAR)TestStateProvider::TypeName.cbegin(),
                        (ULONG)TestStateProvider::TypeName.size() + 1,
                        (ULONG)TestStateProvider::TypeName.size());
                    NTSTATUS status = co_await replica->TxnReplicator->AddAsync(*txn, *stateProviderName, typeName);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    co_await txn->CommitAsync();
                }
                
                replica->SetReadStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);
                replica->SetWriteStatus(FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY);

                co_await replica->CloseAsync();
            }

            for (ULONG32 i = 0; i < iterationCount; i++)
            {
                Replica::SPtr replica = Replica::Create(
                    pId_,
                    rId_,
                    testFolder,
                    logManager,
                    underlyingSystem_->PagedAllocator(),
                    stateProviderFactory.RawPtr());

                // TODO: For now I am not verifying the state to see if we can increase the rate of repro of a bug.
                co_await replica->OpenAsync();
                co_await replica->CloseAsync();
            }

            co_return;
        }

        wstring RecoveryTests::CreateFileName(
            __in wstring const & folderName)
        {
            wstring testFolderPath = Directory::GetCurrentDirectoryW();
            Path::CombineInPlace(testFolderPath, folderName);

            return testFolderPath;
        }

        void RecoveryTests::EndTest()
        {
            prId_.Reset();
        }

        void RecoveryTests::InitializeKtlConfig(
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

        KUri::CSPtr RecoveryTests::GetStateProviderName(
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

        BOOST_FIXTURE_TEST_SUITE(RecoveryTestSuite, RecoveryTests);

        BOOST_AUTO_TEST_CASE(Recovery_Single_SUCCESS)
        {
            // Setup
            wstring testName(L"Recovery_Single_SUCCESS");
            wstring testFolderPath = CreateFileName(testName);

            // Pre-clean up
            Directory::Delete_WithRetry(testFolderPath, true, true);

            wstring workFolder = Path::Combine(testFolderPath, L"work");

            TEST_TRACE_BEGIN(testName)
            {
                KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
                InitializeKtlConfig(testFolderPath, TestLogFileName, underlyingSystem_->NonPagedAllocator(), sharedLogSettings);

                Data::Log::LogManager::SPtr logManager;
                status = Data::Log::LogManager::Create(underlyingSystem_->NonPagedAllocator(), logManager);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));

                SyncAwait(Test_Recovery(workFolder, *logManager, 4));

                status = SyncAwait(logManager->CloseAsync(CancellationToken::None));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                logManager = nullptr;
            }

            // Post-clean up
            Directory::Delete_WithRetry(testFolderPath, true, true);
        }

        BOOST_AUTO_TEST_SUITE_END();
    }
}
