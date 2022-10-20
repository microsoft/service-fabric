// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

#define DEFAULT_TXN_REPLICATOR_SETTING -1

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

    StringLiteral const TraceComponent = "TestTransactionalReplicatorSettings";
    std::wstring const DefaultSharedLogContainerGuid(L"3CA2CCDA-DD0F-49c8-A741-62AAC0D4EB62");
    std::wstring const DefaultTestSubDirectoryName(L"TransactionalReplicatorSettings_Test");
    std::wstring const TestLogFileName(L"TransactionalReplicatorTest.log");

    class TestTransactionalReplicatorSettings
    {
        CommonConfig commonConfig; // load the config object as its needed for the tracing to work

    public:
        Awaitable<TransactionalReplicator::SPtr> CreateAndOpenTransactionalReplicator(
            __in_opt TRANSACTIONAL_REPLICATOR_SETTINGS * config,
            __in Data::Log::LogManager::SPtr & logManager,
            __in KAllocator & allocator)
        {
            NTSTATUS status;

            KGuid partitionId;
            partitionId.CreateNew();
            PartitionedReplicaId::SPtr partitionedReplicaIdCSPtr = PartitionedReplicaId::Create(partitionId, 17, allocator);

            auto runtimeFolders = Data::TestCommon::TestRuntimeFolders::Create(GetWorkingDirectory().c_str(), allocator);
            auto partitionSPtr = Data::TestCommon::TestStatefulServicePartition::Create(allocator);

            TransactionalReplicator::SPtr txnReplicatorSPtr = nullptr;
            ComProxyStateReplicator::SPtr comProxyReplicator;
            ComPointer<IFabricStateReplicator> comV1Replicator;
            status = ComProxyStateReplicator::Create(*partitionedReplicaIdCSPtr, comV1Replicator, allocator, comProxyReplicator);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            TxnReplicator::TestCommon::TestStateProviderFactory::SPtr factory = TxnReplicator::TestCommon::TestStateProviderFactory::Create(allocator);
            
            TxnReplicator::TRInternalSettingsSPtr txReplicatorConfig;
            TransactionalReplicatorSettingsUPtr tmp;

            if(config != nullptr)
            {
                ErrorCode e = TransactionalReplicatorSettings::FromPublicApi(*config, tmp);
                CODING_ERROR_ASSERT(e.IsSuccess());
            }

            txReplicatorConfig = TRInternalSettings::Create(move(tmp), make_shared<TransactionalReplicatorConfig>());

            KTLLOGGER_SHARED_LOG_SETTINGS slPublicSettings = { L"", L"", 0, 0, 0 };
            KtlLoggerSharedLogSettingsUPtr sltmp;
            KtlLoggerSharedLogSettings::FromPublicApi(slPublicSettings, sltmp);

            TxnReplicator::SLInternalSettingsSPtr ktlLoggerSharedLogConfig = SLInternalSettings::Create(move(sltmp));
            TxnReplicator::TestCommon::TestDataLossHandler::SPtr dataLossHandler = TxnReplicator::TestCommon::TestDataLossHandler::Create(allocator);
            TestHealthClientSPtr healthClient = TestHealthClient::Create(true);
            
            txnReplicatorSPtr = TransactionalReplicator::Create(
                *partitionedReplicaIdCSPtr,
                *runtimeFolders,
                *partitionSPtr,
                *comProxyReplicator,
                static_cast<StateManager::IStateProvider2Factory &>(*factory),
                move(txReplicatorConfig),
                move(ktlLoggerSharedLogConfig),
                *logManager,
                *dataLossHandler,
                move(healthClient),
                true,
                allocator);

            CODING_ERROR_ASSERT(txnReplicatorSPtr != nullptr);
            
            status = co_await txnReplicatorSPtr->OpenAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = co_await txnReplicatorSPtr->CloseAsync();
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_return txnReplicatorSPtr;
        };

        void GetPublicApiSettings(
            __out TRANSACTIONAL_REPLICATOR_SETTINGS * config,
            __in int64 checkpointThresholdInMB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 slowApiMonitoringDuration = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 slowLogIODuration = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 slowLogIOCountThreshold = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 slowLogIOTimeThreshold = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 slowLogIOHealthReportDuration = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 maxAccumulatedBackupLogSizeInMB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 minLogSizeInMB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 truncationThresholdFactor = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 throttlingThresholdFactor = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 maxRecordSizeInKB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 maxStreamSizeInMB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 maxWriteQueueDepthInKB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 maxMetadataSizeInKB = DEFAULT_TXN_REPLICATOR_SETTING,
            __in int64 serializationVersion = DEFAULT_TXN_REPLICATOR_SETTING,
            __in bool enableSecondaryCommitApplyAcknowledgement = false,
            __in bool optimizeLogForLowerDiskUsage = true,
            __in bool enableIncrementalBackupsAcrossReplicas = false)
        {
            // Set all values to 0 initially
            *config = { 0 };

            if (checkpointThresholdInMB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->CheckpointThresholdInMB = static_cast<DWORD>(checkpointThresholdInMB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
            }
            if (slowApiMonitoringDuration > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SlowApiMonitoringDurationSeconds = static_cast<DWORD>(slowApiMonitoringDuration);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;
            }
            if (maxAccumulatedBackupLogSizeInMB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MaxAccumulatedBackupLogSizeInMB = static_cast<DWORD>(maxAccumulatedBackupLogSizeInMB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB;
            }
            if (minLogSizeInMB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MinLogSizeInMB = static_cast<DWORD>(minLogSizeInMB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
            }
            if (truncationThresholdFactor > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->TruncationThresholdFactor = static_cast<DWORD>(truncationThresholdFactor);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
            }
            if (throttlingThresholdFactor > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->ThrottlingThresholdFactor = static_cast<DWORD>(throttlingThresholdFactor);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;
            }
            if (maxRecordSizeInKB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MaxRecordSizeInKB = static_cast<DWORD>(maxRecordSizeInKB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB;
            }
            if (maxStreamSizeInMB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MaxStreamSizeInMB = static_cast<DWORD>(maxStreamSizeInMB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;
            }
            if (maxWriteQueueDepthInKB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MaxWriteQueueDepthInKB = static_cast<DWORD>(maxWriteQueueDepthInKB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB;
            }
            if (maxMetadataSizeInKB > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->MaxMetadataSizeInKB = static_cast<DWORD>(maxMetadataSizeInKB);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB;
            }
            if (enableSecondaryCommitApplyAcknowledgement != false)
            {
                config->EnableSecondaryCommitApplyAcknowledgement = enableSecondaryCommitApplyAcknowledgement;
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT;
            }
            if (optimizeLogForLowerDiskUsage != true)
            {
                config->OptimizeLogForLowerDiskUsage = optimizeLogForLowerDiskUsage;
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE;
            }
            if (slowLogIODuration > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SlowLogIODurationSeconds = static_cast<DWORD>(slowLogIODuration);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS;
            }
            if (slowLogIOCountThreshold > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SlowLogIOCountThreshold = static_cast<DWORD>(slowLogIOCountThreshold);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD;
            }
            if (slowLogIOTimeThreshold > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SlowLogIOTimeThresholdSeconds = static_cast<DWORD>(slowLogIOTimeThreshold);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS;
            }
            if (slowLogIOHealthReportDuration > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SlowLogIOHealthReportTTLSeconds = static_cast<DWORD>(slowLogIOHealthReportDuration);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS;
            }
            if (serializationVersion > DEFAULT_TXN_REPLICATOR_SETTING)
            {
                config->SerializationVersion = static_cast<DWORD>(serializationVersion);
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION;
            }
            if (enableIncrementalBackupsAcrossReplicas != false)
            {
                config->EnableIncrementalBackupsAcrossReplicas = enableIncrementalBackupsAcrossReplicas;
                config->Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS;
            }
        }

        void GetPublicApiSettingsFromReplicator(
            __in TransactionalReplicator & replicator,
            __out TRANSACTIONAL_REPLICATOR_SETTINGS * output)
        {
            TRInternalSettingsSPtr internalSettings = replicator.Config;
            TransactionalReplicatorSettingsUPtr obj;

            ErrorCode er = TransactionalReplicatorSettings::FromConfig(*internalSettings.get(), obj);

            CODING_ERROR_ASSERT(er.IsSuccess());
            obj->ToPublicApi(*output);
        };

        void GetDefaultApiSettings(__out TRANSACTIONAL_REPLICATOR_SETTINGS * output)
        {
            *output = { 0 };

            std::shared_ptr<TransactionalReplicatorConfig> defaultConfig = make_shared<TransactionalReplicatorConfig>();
            TransactionalReplicatorSettingsUPtr obj;
            ErrorCode er = TransactionalReplicatorSettings::FromConfig(*defaultConfig.get(), obj);

            CODING_ERROR_ASSERT(er.IsSuccess());
            obj->ToPublicApi(*output);
        };

        void VerifyExpectedAndActualPublicSettings(
            __in TRANSACTIONAL_REPLICATOR_SETTINGS * initial,
            __in TRANSACTIONAL_REPLICATOR_SETTINGS * output)
        {
            // Verify all user settings from TRANSACTIONAL_REPLICATOR_SETTINGS match 
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB)
            {
                VERIFY_ARE_EQUAL(initial->CheckpointThresholdInMB, output->CheckpointThresholdInMB);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS)
            {
                VERIFY_ARE_EQUAL(initial->SlowApiMonitoringDurationSeconds, output->SlowApiMonitoringDurationSeconds);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB)
            {
                VERIFY_ARE_EQUAL(initial->MaxAccumulatedBackupLogSizeInMB, output->MaxAccumulatedBackupLogSizeInMB);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB)
            {
                if (initial->MinLogSizeInMB == 0)
                {
                    // If MinLogSize is 0, default is checkpointthreshold/2
                    VERIFY_ARE_EQUAL(output->CheckpointThresholdInMB/2, output->MinLogSizeInMB);
                }
                else
                {
                    VERIFY_ARE_EQUAL(initial->MinLogSizeInMB, output->MinLogSizeInMB);
                }
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR)
            {
                VERIFY_ARE_EQUAL(initial->TruncationThresholdFactor, output->TruncationThresholdFactor);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR)
            {
                VERIFY_ARE_EQUAL(initial->ThrottlingThresholdFactor, output->ThrottlingThresholdFactor);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB)
            {
                VERIFY_ARE_EQUAL(initial->MaxRecordSizeInKB, output->MaxRecordSizeInKB);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB)
            {
                if (output->OptimizeLogForLowerDiskUsage)
                {
                    // If sparse file is set, log size is 200GB
                    VERIFY_ARE_EQUAL(200 * 1024, output->MaxStreamSizeInMB);
                }
                else
                {
                    VERIFY_ARE_EQUAL(initial->MaxStreamSizeInMB, output->MaxStreamSizeInMB);
                }
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB)
            {
                VERIFY_ARE_EQUAL(initial->MaxWriteQueueDepthInKB, output->MaxWriteQueueDepthInKB);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB)
            {
                VERIFY_ARE_EQUAL(initial->MaxMetadataSizeInKB, output->MaxMetadataSizeInKB);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT)
            {
                VERIFY_ARE_EQUAL(initial->EnableSecondaryCommitApplyAcknowledgement, output->EnableSecondaryCommitApplyAcknowledgement);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS)
            {
                VERIFY_ARE_EQUAL(initial->SlowLogIODurationSeconds, output->SlowLogIODurationSeconds);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD)
            {
                VERIFY_ARE_EQUAL(initial->SlowLogIOCountThreshold, output->SlowLogIOCountThreshold);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS)
            {
                VERIFY_ARE_EQUAL(initial->SlowLogIOTimeThresholdSeconds, output->SlowLogIOTimeThresholdSeconds);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS)
            {
                VERIFY_ARE_EQUAL(initial->SlowLogIOHealthReportTTLSeconds, output->SlowLogIOHealthReportTTLSeconds);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION)
            {
                VERIFY_ARE_EQUAL(initial->SerializationVersion, output->SerializationVersion);
            }
            if (initial->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS)
            {
                VERIFY_ARE_EQUAL(initial->EnableIncrementalBackupsAcrossReplicas, output->EnableIncrementalBackupsAcrossReplicas);
            }
        };

        void UpdateConfig(wstring name, wstring newvalue, ConfigSettings & settings, shared_ptr<ConfigSettingsConfigStore> & store)
        {
            ConfigSection section;
            section.Name = L"TransactionalReplicator2";

            ConfigParameter d1;
            d1.Name = name;
            d1.Value = newvalue;

            section.Parameters[d1.Name] = d1;
            settings.Sections[section.Name] = section;

            store->Update(settings);
        }

        void VerifyPublicApiValidationStatus(
            __in TRANSACTIONAL_REPLICATOR_SETTINGS & initial,
            __in TransactionalReplicatorSettingsUPtr & output,
            __in bool expectedStatus)
        {
            ErrorCode e = TransactionalReplicatorSettings::FromPublicApi(initial, output);
            VERIFY_ARE_EQUAL(e.IsSuccess(), expectedStatus);     // Confirm validation status
        }

        DWORD DWord(__in int64 value)
        {
            return static_cast<DWORD>(value);
        }

        void ResetLogDirectory()
        {
            Directory::Delete_WithRetry(DefaultTestSubDirectoryName, true, true);
        }

        std::wstring GetWorkingDirectory()
        {
            std::wstring workDir = Directory::GetCurrentDirectoryW();
            workDir = Path::Combine(workDir, DefaultTestSubDirectoryName);
            workDir = Path::Combine(workDir, DefaultSharedLogContainerGuid);
            return workDir;
        }

        KtlSystem * underlyingSystem_;

    private:
        KtlSystem * CreateKtlSystem()
        {
            KtlSystem* underlyingSystem;
            NTSTATUS status = KtlSystem::Initialize(FALSE, &underlyingSystem);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            underlyingSystem->SetStrictAllocationChecks(TRUE);

            return underlyingSystem;
        }
    };

    BOOST_FIXTURE_TEST_SUITE(TransactionalReplicatorSettingsTestSuite, TestTransactionalReplicatorSettings)

    BOOST_AUTO_TEST_CASE(Verify_DefaultSettings)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_DefaultSettings")
        {
            Common::ScopedHeap heap;

            TRANSACTIONAL_REPLICATOR_SETTINGS config;
            GetPublicApiSettings(&config);              // Do not load any settings

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                TestLogFileName,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::SPtr logManager = nullptr;
            status = Data::Log::LogManager::Create(
                allocator,
                logManager);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Create and open TransactionalReplicator
            TransactionalReplicator::SPtr createdReplicator = SyncAwait(this->CreateAndOpenTransactionalReplicator(&config, logManager, allocator));

            // Retrieve the public API object corresponding to TRInternalSettings object
            TRANSACTIONAL_REPLICATOR_SETTINGS createdConfig;
            GetPublicApiSettingsFromReplicator(*createdReplicator, &createdConfig);

            // Load the default API settings
            GetDefaultApiSettings(&config);

            // Verify the replicator contains the default settings when none are specified
            VerifyExpectedAndActualPublicSettings(&config, &createdConfig);

            // Reset the txnReplicator
            createdReplicator.Reset();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator from KSharedPtr::Reset()
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
            ResetLogDirectory();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_NullSettings)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_NullSettings")
        {
            Common::ScopedHeap heap;

            TRANSACTIONAL_REPLICATOR_SETTINGS config;
            TRANSACTIONAL_REPLICATOR_SETTINGS createdConfig;

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                TestLogFileName,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::SPtr logManager = nullptr;
            status = Data::Log::LogManager::Create(
                allocator,
                logManager);

            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Create and open TransactionalReplicator with TRANSACTIONAL_REPLICATOR_SETTINGS = nullptr
            TransactionalReplicator::SPtr createdReplicator = SyncAwait(this->CreateAndOpenTransactionalReplicator(nullptr, logManager, allocator));

            // Retrieve the public API object corresponding to TRInternalSettings object
            GetPublicApiSettingsFromReplicator(*createdReplicator, &createdConfig);

            // Load the default API settings
            GetDefaultApiSettings(&config);

            // Verify the replicator contains the default settings when none are specified
            VerifyExpectedAndActualPublicSettings(&config, &createdConfig);

            // Reset the txnReplicator
            createdReplicator.Reset();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator from KSharedPtr::Reset()
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
            ResetLogDirectory();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_UserSettings)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_UserSettings")
        {
            Common::ScopedHeap heap;

            TRANSACTIONAL_REPLICATOR_SETTINGS config;
            
            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                TestLogFileName,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::SPtr logManager = nullptr;
            status = Data::Log::LogManager::Create(
                allocator,
                logManager);

            status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Set a value for every exposed config in the public API
            GetPublicApiSettings(
                &config,
                100,    // CheckpointThresholdInMB
                310,    // SlowApiMonitoringDuration
                2,    // SlowLogIODuration
                60,     // SlowLogIOCountThreshold
                300,    // SlowLogIOTimeThreshold
                31,     // SlowLogIOHealthReportDuration
                810,    // MaxAccumulatedBackupLogSizeInMB
                100,    // MinLogSizeInMB
                2,      // TruncationThresholdFacto
                10,     // ThrottlingThresholdFactor
                2048,   // MaxRecordSizeInKB
                1024,   // MaxStreamSizeinMB
                2048,   // MaxWriteQueueDepthinKB
                2048,   // MaxMetadataSizeinKB,
                0,      // SerializationVersion
                true,   // EnableSecondaryCommitApplyAcknowledgement
                false,  // OptimizeLogForLowerDiskUsage
                false); // EnableIncrementalBackupsAcrossReplicas

            // Create and open TransactionalReplicator
            TransactionalReplicator::SPtr createdReplicator = SyncAwait(this->CreateAndOpenTransactionalReplicator(&config, logManager, allocator));

            // Retrieve the public API object corresponding to TRInternalSettings object
            TRANSACTIONAL_REPLICATOR_SETTINGS createdConfig;
            GetPublicApiSettingsFromReplicator(*createdReplicator, &createdConfig);

            // Verify the replicator contains the initially specified settings
            VerifyExpectedAndActualPublicSettings(&config, &createdConfig);

            // Reset the txnReplicator
            createdReplicator.Reset();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator from KSharedPtr::Reset()
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
            ResetLogDirectory();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_DynamicSetting_Update)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_DynamicSetting_Update")
        {
            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                TestLogFileName,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::SPtr logManager = nullptr;

            status = Data::Log::LogManager::Create(
                allocator,
                logManager);
            
            status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ConfigSettings settings;
            std::shared_ptr<ConfigSettingsConfigStore> configStore = make_shared<ConfigSettingsConfigStore>(settings);
            Config::SetConfigStore(configStore);

            // Create and open TransactionalReplicator with null TRANSACTIONAL_REPLICATOR_SETTINGS
            TransactionalReplicator::SPtr createdReplicator = SyncAwait(this->CreateAndOpenTransactionalReplicator(nullptr, logManager, allocator));
            TRInternalSettingsSPtr configObject = createdReplicator->Config;

            // Verify global settings are updated dynamically

            UpdateConfig(L"ReadAheadCacheSizeInKb", L"10", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->ReadAheadCacheSizeInKb, 10);

            UpdateConfig(L"CopyBatchSizeInKb", L"10", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->CopyBatchSizeInKb, 10);

            UpdateConfig(L"ProgressVectorMaxEntries", L"10", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->ProgressVectorMaxEntries, 10);

            UpdateConfig(L"Test_LogMinDelayIntervalMilliseconds", L"120", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->Test_LogMinDelayIntervalMilliseconds, 120);

            UpdateConfig(L"Test_LogMaxDelayIntervalMilliseconds", L"121", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->Test_LogMaxDelayIntervalMilliseconds, 121);

            UpdateConfig(L"Test_LogDelayRatio", L"0.23", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->Test_LogDelayRatio, 0.23);
            
            UpdateConfig(L"Test_LogDelayProcessExitRatio", L"0.33", settings, configStore);
            Sleep(500);
            VERIFY_IS_TRUE(std::abs(0.33 - configObject->Test_LogDelayProcessExitRatio) < 0.001);

            // Verify overrideable settings are updated dynamically
            // Since the initial TRANSACTIONAL_REPLICATOR_SETTINGS object did not define any non-default configs (nullptr),
            // the expected behavior is for those to be updated as well
            UpdateConfig(L"MinLogSizeInMB", L"290", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->MinLogSizeInMB, 290);

            UpdateConfig(L"CheckpointThresholdInMB", L"1200", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->CheckpointThresholdInMB, 1200);

            UpdateConfig(L"MaxAccumulatedBackupLogSizeInMB", L"300", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->MaxAccumulatedBackupLogSizeInMB, 300);

            UpdateConfig(L"TruncationThresholdFactor", L"4", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->TruncationThresholdFactor, 4);

            UpdateConfig(L"ThrottlingThresholdFactor", L"10", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->ThrottlingThresholdFactor, 10);

            UpdateConfig(L"SlowLogIODuration", L"120", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIODuration, Common::TimeSpan::FromSeconds(120));

            UpdateConfig(L"SlowLogIOCountThreshold", L"19", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOCountThreshold, 19);

            UpdateConfig(L"SlowLogIOTimeThreshold", L"400", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOTimeThreshold, Common::TimeSpan::FromSeconds(400));

            UpdateConfig(L"SlowLogIOHealthReportTTL", L"900", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOHealthReportTTL, Common::TimeSpan::FromSeconds(900));

            UpdateConfig(L"SlowApiMonitoringDuration", L"140", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowApiMonitoringDuration, Common::TimeSpan::FromSeconds(140));

            // Reset the txnReplicator
            createdReplicator.Reset();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator from KSharedPtr::Reset()
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
            ResetLogDirectory();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_DynamicSetting_UserValue_Update)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_DynamicSetting_UserValue_Update")
        {
            Common::ScopedHeap heap;

            KtlLogger::SharedLogSettingsSPtr sharedLogSettings;
            TR_Test_InitializeKtlConfig(
                allocator,
                GetWorkingDirectory(),
                TestLogFileName,
                sharedLogSettings);

            CODING_ERROR_ASSERT(sharedLogSettings != nullptr);

            Data::Log::LogManager::SPtr logManager = nullptr;
            status = Data::Log::LogManager::Create(
                allocator,
                logManager);

            status = SyncAwait(logManager->OpenAsync(CancellationToken::None, sharedLogSettings));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            TRANSACTIONAL_REPLICATOR_SETTINGS config;

            // Set values for CheckpointThresholdInMb, SlowApiMonitoringDuration, MaxAccumulatedBackupLogSizeInMB 
            //                SlowLogIODuration, SlowLogIOCountThreshold, SlowLogIOTimeThreshold, SlowLogIOHealthReportDuration

            int64 initialCheckpointThreshold = 120;
            int64 initialSlowApiMonitoringDuration = 340;
            int64 initialMaxAccumulatedBackupLogSizeInMb = 440;
            int64 initialSlowLogIODuration = 200;
            int64 initialSlowLogIOCountThreshold = 15;
            int64 initialSlowLogIOTimeThreshold = 300;
            int64 initialSlowLogIOHealthReportTTL = 150;

            GetPublicApiSettings(
                &config,
                initialCheckpointThreshold,
                initialSlowApiMonitoringDuration,
                initialSlowLogIODuration,
                initialSlowLogIOCountThreshold,
                initialSlowLogIOTimeThreshold,
                initialSlowLogIOHealthReportTTL,
                initialMaxAccumulatedBackupLogSizeInMb);

            ConfigSettings settings;
            auto configStore = make_shared<ConfigSettingsConfigStore>(settings);
            Config::SetConfigStore(configStore);

            // Create and open TransactionalReplicator
            TransactionalReplicator::SPtr createdReplicator = SyncAwait(this->CreateAndOpenTransactionalReplicator(&config, logManager, allocator));
            TRInternalSettingsSPtr configObject = createdReplicator->Config;

            // Retrieve the public API object corresponding to TRInternalSettings object
            TRANSACTIONAL_REPLICATOR_SETTINGS createdConfig;
            GetPublicApiSettingsFromReplicator(*createdReplicator, &createdConfig);

            // Verify the replicator contains the initially specified settings
            VerifyExpectedAndActualPublicSettings(&config, &createdConfig);

            // Update CheckpointThresholdInMb and confirm the value set through the Public API is maintained
            UpdateConfig(L"CheckpointThresholdInMb", L"10", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->CheckpointThresholdInMB, initialCheckpointThreshold);

            // Update SlowApiMonitoringDuration and confirm the value set through the Public API is maintained
            UpdateConfig(L"SlowApiMonitoringDuration", L"1000", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowApiMonitoringDuration, TimeSpan::FromSeconds(initialSlowApiMonitoringDuration * 1.00));

            // Update MaxAccumulatedBackupLogSizeInMB and confirm the value set through the Public API is maintained
            UpdateConfig(L"MaxAccumulatedBackupLogSizeInMB", L"550", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->MaxAccumulatedBackupLogSizeInMB, initialMaxAccumulatedBackupLogSizeInMb);

            // Update SlowLogIO related settings and confirm the value set through the Public API is maintained
            UpdateConfig(L"SlowLogIODuration", L"1200", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIODuration, TimeSpan::FromSeconds(initialSlowLogIODuration * 1.00));

            UpdateConfig(L"SlowLogIOCountThreshold", L"550", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOCountThreshold, initialSlowLogIOCountThreshold);

            UpdateConfig(L"SlowLogIOTimeThreshold", L"400", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOTimeThreshold, Common::TimeSpan::FromSeconds(initialSlowLogIOTimeThreshold * 1.00));

            UpdateConfig(L"SlowLogIOHealthReportTTL", L"900", settings, configStore);
            Sleep(500);
            VERIFY_ARE_EQUAL(configObject->SlowLogIOHealthReportTTL, Common::TimeSpan::FromSeconds(initialSlowLogIOHealthReportTTL * 1.00));

            // Reset the txnReplicator
            createdReplicator.Reset();

            // Delay 1 second to allow the background close task started by LogManager::Dispose to complete
            // LogManager::Dispose is called as part of ~TransactionalReplicator from KSharedPtr::Reset()
            Sleep(1000);

            SyncAwait(logManager->CloseAsync(CancellationToken::None));
            ResetLogDirectory();
        }
    }

    BOOST_AUTO_TEST_CASE(Verify_FromPublicApi_SettingValidation)
    {
        TR_TEST_TRACE_BEGIN(L"Verify_FromPublicApi_SettingValidation")
        {
            UNREFERENCED_PARAMETER(allocator);
            Common::ScopedHeap heap;

            TRANSACTIONAL_REPLICATOR_SETTINGS config;                       // Public settings object
            TransactionalReplicatorSettingsUPtr output;                     // ServiceModel TransactionalReplicatorSettings object unique ptr

            GetDefaultApiSettings(&config);                           // Populate default values
            config.CheckpointThresholdInMB = DWord(0);                      // CheckpointThresholdInMB = 0
            VerifyPublicApiValidationStatus(config, output, false);         // CheckpointThresholdInMB should be greater than 1

            GetDefaultApiSettings(&config);
            config.MaxAccumulatedBackupLogSizeInMB = DWord(0);              // MaxAccumulatedBackupLogSize = 0
            VerifyPublicApiValidationStatus(config, output, false);         // MaxAccumulatedBackupLogSize should be greater than 0

            GetDefaultApiSettings(&config);
            config.MaxAccumulatedBackupLogSizeInMB = DWord(10);
            config.MaxStreamSizeInMB = DWord(8);                            // MaxAccumulatedBackupLogSize > MaxStreamSizeInMB
            VerifyPublicApiValidationStatus(config, output, false);         // MaxAccumulatedBackupLogSizeInMB must be less than MaxStreamSizeInMB

            GetDefaultApiSettings(&config);
            config.MinLogSizeInMB = DWord(10);                              // MinLogSizeInMB = 10
            config.MaxStreamSizeInMB = DWord(5);                            // MaxStreamSizeInMB = 5
            VerifyPublicApiValidationStatus(config, output, false);         // MinLogSizeInMB must be smaller than MaxStreamSizeInMB

            GetDefaultApiSettings(&config);
            config.TruncationThresholdFactor = DWord(1);
            VerifyPublicApiValidationStatus(config, output, false);         // TruncationThresholdFactor must be greater than 1

            GetDefaultApiSettings(&config);
            config.MinLogSizeInMB = DWord(300);
            config.TruncationThresholdFactor = DWord(2);
            config.MaxStreamSizeInMB = DWord(310);
            VerifyPublicApiValidationStatus(config, output, false);         // MinLogSizeInMB * TruncationThresholdFactor must be smaller than MaxStreamSizeInMB

            GetDefaultApiSettings(&config);
            config.ThrottlingThresholdFactor = DWord(1);
            VerifyPublicApiValidationStatus(config, output, false);         // ThrottlingThresholdFactor must be greater than 1

            GetDefaultApiSettings(&config);
            config.TruncationThresholdFactor = DWord(7);
            config.ThrottlingThresholdFactor = DWord(6);
            VerifyPublicApiValidationStatus(config, output, false);         // ThrottlingThresholdFactor must be greater than 1

            GetDefaultApiSettings(&config);
            config.MaxRecordSizeInKB = DWord(3);
            VerifyPublicApiValidationStatus(config, output, false);         // MaxRecordSizeInKB must be a multiple of 4

            GetDefaultApiSettings(&config);
            config.MaxRecordSizeInKB = DWord(64);
            VerifyPublicApiValidationStatus(config, output, false);         // MaxRecordSizeInKB must be >= 128

            GetDefaultApiSettings(&config);
            config.MaxWriteQueueDepthInKB = DWord(6);
            VerifyPublicApiValidationStatus(config, output, false);         // MaxWriteQueueDepthInKB must be a multiple of 4

            GetDefaultApiSettings(&config);
            config.MaxMetadataSizeInKB = DWord(13);
            VerifyPublicApiValidationStatus(config, output, false);         // MaxMetadataSizeInKB must be a multiple of 4

            GetDefaultApiSettings(&config);
            config.EnableSecondaryCommitApplyAcknowledgement = TRUE;
            VerifyPublicApiValidationStatus(config, output, true);          // EnableSecondaryCommitApplyAcknowledgement has no verification

            GetDefaultApiSettings(&config);
            config.OptimizeLogForLowerDiskUsage = FALSE;
            config.MaxRecordSizeInKB = DWord(262148);
            VerifyPublicApiValidationStatus(config, output, false);          // MaxStreamSizeInMB * 1024 must be larger or equal to MaxRecordSizeInKB * 16

            GetDefaultApiSettings(&config);
            config.OptimizeLogForLowerDiskUsage = FALSE;
            config.MaxStreamSizeInMB = DWord(2048);
            config.CheckpointThresholdInMB = DWord(4096);
            VerifyPublicApiValidationStatus(config, output, false);          // MaxStreamSizeInMB must be greater than CheckpointThresholdInMB when the log is not optimized for low disk usage.

            GetDefaultApiSettings(&config);
            config.SlowLogIODurationSeconds = 200;
            config.SlowLogIOTimeThresholdSeconds = 190;
            VerifyPublicApiValidationStatus(config, output, true);  // Validation is expected to pass by overwriting the incorrect values provided (slow io duration > slow io time threshold)
            std::shared_ptr<TRConfigValues> defaultValues = std::make_shared<TRConfigValues>();
            ASSERT_IFNOT(output->SlowLogIODuration == defaultValues->SlowLogIODuration, "Expected SlowLogIODuration.Seconds value to be overwritten");
            ASSERT_IFNOT(output->SlowLogIOTimeThreshold == defaultValues->SlowLogIOTimeThreshold, "Expected SlowLogIOTimeThreshold.Seconds value to be overwritten");

            config.SerializationVersion = DWord(0);
            VerifyPublicApiValidationStatus(config, output, true);          // SerializationVersion should be 0 or 1 

            GetDefaultApiSettings(&config);
            config.SerializationVersion = DWord(1);
            VerifyPublicApiValidationStatus(config, output, true);          // SerializationVersion should be 0 or 1 

            GetDefaultApiSettings(&config);
            config.SerializationVersion = DWord(2);
            VerifyPublicApiValidationStatus(config, output, false);          // SerializationVersion should be 0 or 1 

            GetDefaultApiSettings(&config);
            config.SerializationVersion = DWord(-1);
            VerifyPublicApiValidationStatus(config, output, false);          // SerializationVersion should be 0 or 1 
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
