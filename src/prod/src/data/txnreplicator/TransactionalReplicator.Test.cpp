// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

extern void TR_Test_InitializeKtlConfig(
    __in KAllocator & allocator,
    __in std::wstring workDir,
    __in std::wstring fileName,
    __out KtlLogger::SharedLogSettingsSPtr & sharedLogSettings);

namespace TransactionalReplicatorTests
{
    using namespace std;
    using namespace Common;
    using namespace ktl;
    using namespace Data;
    using namespace TxnReplicator;
    using namespace Data::Utilities;

    std::wstring const DefaultSharedLogContainerGuid(L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62");
    std::wstring const DefaultTestSubDirectoryName(L"TransactionalReplicator_Test");

    class TransactionalReplicatorTest
    {
        CommonConfig config; // load the config object as its needed for the tracing to work

    public:
        Awaitable<void> Test_Life_CreateOpenClose_Success(
            __in KAllocator & allocator,
            __in Data::Log::LogManager & logManager)
        {
            NTSTATUS status;

            KGuid partitionId;
            partitionId.CreateNew();
            std::wstring workingDirectory = GetWorkingDirectory();
            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(partitionId, 17, allocator);

            auto runtimeFolders = Data::TestCommon::TestRuntimeFolders::Create(workingDirectory.c_str(), allocator);
            auto partitionSPtr = Data::TestCommon::TestStatefulServicePartition::Create(allocator);

            TransactionalReplicator::SPtr txnReplicatorSPtr = nullptr;
            ComProxyStateReplicator::SPtr comProxyReplicator;
            ComPointer<IFabricStateReplicator> comV1Replicator;
            status = ComProxyStateReplicator::Create(*partitionedReplicaIdCSPtr, comV1Replicator, allocator, comProxyReplicator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KTLLOGGER_SHARED_LOG_SETTINGS slPublicSettings = { L"", L"", 0, 0, 0 };
            KtlLoggerSharedLogSettingsUPtr sltmp;
            KtlLoggerSharedLogSettings::FromPublicApi(slPublicSettings, sltmp);

            TxnReplicator::SLInternalSettingsSPtr ktlLoggerSharedLogConfig = SLInternalSettings::Create(move(sltmp));
            TxnReplicator::TestCommon::TestStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrPublicSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrPublicSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr txReplicatorConfig = TRInternalSettings::Create(move(tmp), make_shared<TransactionalReplicatorConfig>());
            TestHealthClientSPtr healthClient = TestHealthClient::Create(true);
            
            TxnReplicator::TestCommon::TestDataLossHandler::SPtr dataLossHandler = TxnReplicator::TestCommon::TestDataLossHandler::Create(allocator);

            txnReplicatorSPtr = TransactionalReplicator::Create(
                *partitionedReplicaIdCSPtr,
                *runtimeFolders,
                *partitionSPtr,
                *comProxyReplicator,
                static_cast<StateManager::IStateProvider2Factory &>(*factory),
                move(txReplicatorConfig),
                move(ktlLoggerSharedLogConfig),
                logManager,
                *dataLossHandler,
                move(healthClient),
                true,
                allocator);

            CODING_ERROR_ASSERT(txnReplicatorSPtr);
            
            status = co_await txnReplicatorSPtr->OpenAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = co_await txnReplicatorSPtr->CloseAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (Directory::Exists(DefaultTestSubDirectoryName))
            {
                Directory::Delete_WithRetry(DefaultTestSubDirectoryName, true, true);
            }
        };

        Awaitable<void> InitializeAndOpenLogManager(
            __in KAllocator & allocator,
            __out Data::Log::LogManager::SPtr & logManager)
        {
            NTSTATUS status;
            status = Data::Log::LogManager::Create(allocator, logManager);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;

            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                L"TransactionalReplicatorTest.log",
                sharedLogSettings);

            status = co_await logManager->OpenAsync(CancellationToken::None, sharedLogSettings);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        std::wstring GetWorkingDirectory()
        {
            std::wstring workDir = Directory::GetCurrentDirectoryW();
            workDir = Path::Combine(workDir, DefaultTestSubDirectoryName);
            workDir = Path::Combine(workDir, DefaultSharedLogContainerGuid);
            return workDir;
        }

    private:
        KtlSystem* CreateKtlSystem()
        {
            KtlSystem* underlyingSystem;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            underlyingSystem->SetStrictAllocationChecks(TRUE);

            return underlyingSystem;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(TransactionalReplicatorTestSuite, TransactionalReplicatorTest)

    BOOST_AUTO_TEST_CASE(Life_CreateOpenAndClose_Success)
    {
        KtlSystem* underlyingSystem;
        NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);

        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        underlyingSystem->SetStrictAllocationChecks(TRUE);

        {
            KAllocator& allocator = underlyingSystem->NonPagedAllocator();
            
            Data::Log::LogManager::SPtr logManager;
            SyncAwait(InitializeAndOpenLogManager(allocator, logManager));
            SyncAwait(this->Test_Life_CreateOpenClose_Success(allocator, *logManager));

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator when Test_Life_CreateOpenClose_Success completes
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
        }
  
        KtlSystem::Shutdown();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
