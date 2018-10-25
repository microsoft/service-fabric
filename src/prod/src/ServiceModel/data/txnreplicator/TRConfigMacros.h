// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TxnReplicator
{

#define TR_GLOBAL_SETTINGS_COUNT 10
#define TR_OVERRIDABLE_STATIC_SETTINGS_COUNT 9
#define TR_OVERRIDABLE_DYNAMIC_SETTINGS_COUNT 11
#define TR_OVERRIDABLE_SETTINGS_COUNT (TR_OVERRIDABLE_STATIC_SETTINGS_COUNT + TR_OVERRIDABLE_DYNAMIC_SETTINGS_COUNT)

#define TR_SETTINGS_COUNT (TR_GLOBAL_SETTINGS_COUNT + TR_OVERRIDABLE_SETTINGS_COUNT)

#define DEFINE_TR_OVERRIDEABLE_CONFIG_PROPERTIES() \
            __declspec(property(get=get_CheckpointThresholdInMB)) int64 CheckpointThresholdInMB ; \
            int64 get_CheckpointThresholdInMB() const; \
            __declspec(property(get=get_SlowApiMonitoringDuration)) Common::TimeSpan SlowApiMonitoringDuration ; \
            Common::TimeSpan get_SlowApiMonitoringDuration() const ;\
            __declspec(property(get=get_MaxAccumulatedBackupLogSizeInMB)) int64 MaxAccumulatedBackupLogSizeInMB ; \
            int64 get_MaxAccumulatedBackupLogSizeInMB() const; \
            __declspec(property(get=get_MinLogSizeInMB)) int64 MinLogSizeInMB; \
            int64 get_MinLogSizeInMB() const; \
            __declspec(property(get=get_TruncationThresholdFactor)) int64 TruncationThresholdFactor; \
            int64 get_TruncationThresholdFactor() const; \
            __declspec(property(get=get_ThrottlingThresholdFactor)) int64 ThrottlingThresholdFactor; \
            int64 get_ThrottlingThresholdFactor() const; \
            __declspec(property(get=get_MaxRecordSizeInKB)) int64 MaxRecordSizeInKB; \
            int64 get_MaxRecordSizeInKB() const; \
            __declspec(property(get=get_MaxStreamSizeInMB)) int64 MaxStreamSizeInMB; \
            int64 get_MaxStreamSizeInMB() const; \
            __declspec(property(get=get_MaxWriteQueueDepthInKB)) int64 MaxWriteQueueDepthInKB; \
            int64 get_MaxWriteQueueDepthInKB() const; \
            __declspec(property(get=get_MaxMetadataSizeInKB)) int64 MaxMetadataSizeInKB; \
            int64 get_MaxMetadataSizeInKB() const; \
            __declspec(property(get=get_SerializationVersion)) int64 SerializationVersion; \
            int64 get_SerializationVersion() const; \
            __declspec(property(get=get_EnableIncrementalBackupsAcrossReplicas)) bool EnableIncrementalBackupsAcrossReplicas; \
            bool get_EnableIncrementalBackupsAcrossReplicas() const; \
            __declspec(property(get=get_OptimizeForLocalSSD)) bool OptimizeForLocalSSD; \
            bool get_OptimizeForLocalSSD() const; \
            __declspec(property(get=get_OptimizeLogForLowerDiskUsage)) bool OptimizeLogForLowerDiskUsage; \
            bool get_OptimizeLogForLowerDiskUsage() const; \
            __declspec(property(get=get_EnableSecondaryCommitApplyAcknowledgement)) bool EnableSecondaryCommitApplyAcknowledgement; \
            bool get_EnableSecondaryCommitApplyAcknowledgement() const; \
            __declspec(property(get=get_SlowLogIODuration)) Common::TimeSpan SlowLogIODuration ; \
            Common::TimeSpan get_SlowLogIODuration() const ;\
            __declspec(property(get=get_SlowLogIOCountThreshold)) int64 SlowLogIOCountThreshold; \
            int64 get_SlowLogIOCountThreshold() const; \
            __declspec(property(get=get_SlowLogIOTimeThreshold)) Common::TimeSpan SlowLogIOTimeThreshold ; \
            Common::TimeSpan get_SlowLogIOTimeThreshold() const ;\
            __declspec(property(get=get_SlowLogIOHealthReportTTL)) Common::TimeSpan SlowLogIOHealthReportTTL ; \
            Common::TimeSpan get_SlowLogIOHealthReportTTL() const ;\
            __declspec(property(get=get_TruncationInterval)) Common::TimeSpan TruncationInterval ; \
            Common::TimeSpan get_TruncationInterval() const ;\

#define DEFINE_TR_GLOBAL_CONFIG_PROPERTIES() \
            __declspec(property(get=get_ReadAheadCacheSizeInKb)) int64 ReadAheadCacheSizeInKb ; \
            int64 get_ReadAheadCacheSizeInKb() const; \
            __declspec(property(get=get_CopyBatchSizeInKb)) int64 CopyBatchSizeInKb ; \
            int64 get_CopyBatchSizeInKb() const; \
            __declspec(property(get=get_ProgressVectorMaxEntries)) int64 ProgressVectorMaxEntries ; \
            int64 get_ProgressVectorMaxEntries() const; \
            __declspec(property(get=get_TestLoggingEngine)) std::wstring Test_LoggingEngine ; \
            std::wstring get_TestLoggingEngine() const; \
            __declspec(property(get=get_TestLogMinDelayIntervalInMilliseconds)) int64 Test_LogMinDelayIntervalMilliseconds; \
            int64 get_TestLogMinDelayIntervalInMilliseconds() const; \
            __declspec(property(get=get_TestLogMaxDelayIntervalInMilliseconds)) int64 Test_LogMaxDelayIntervalMilliseconds; \
            int64 get_TestLogMaxDelayIntervalInMilliseconds() const; \
            __declspec(property(get=get_TestLogDelayRatio)) double Test_LogDelayRatio ; \
            double get_TestLogDelayRatio() const; \
            __declspec(property(get=get_TestLogDelayProcessExitRatio)) double Test_LogDelayProcessExitRatio ; \
            double get_TestLogDelayProcessExitRatio() const; \
            __declspec(property(get=get_FlushedRecordsTraceVectorSize)) int64 FlushedRecordsTraceVectorSize ; \
            int64 get_FlushedRecordsTraceVectorSize() const; \
            std::wstring get_DispatchingMode() const; \
            __declspec(property(get=get_DispatchingMode)) std::wstring DispatchingMode; \

#define DEFINE_GET_TR_CONFIG_METHOD() \
            void GetTransactionalReplicatorSettingsStructValues(TxnReplicator::TRConfigValues & config) const \
                    { \
                config.CheckpointThresholdInMB = static_cast<DWORD>(this->CheckpointThresholdInMB); \
                config.SlowApiMonitoringDuration = this->SlowApiMonitoringDuration; \
                config.MaxAccumulatedBackupLogSizeInMB = static_cast<DWORD>(this->MaxAccumulatedBackupLogSizeInMB); \
                config.MinLogSizeInMB = static_cast<DWORD>(this->MinLogSizeInMB); \
                config.TruncationThresholdFactor = static_cast<DWORD>(this->TruncationThresholdFactor); \
                config.ThrottlingThresholdFactor = static_cast<DWORD>(this->ThrottlingThresholdFactor); \
                config.MaxRecordSizeInKB = static_cast<DWORD>(this->MaxRecordSizeInKB); \
                config.MaxStreamSizeInMB = static_cast<DWORD>(this->MaxStreamSizeInMB); \
                config.MaxWriteQueueDepthInKB = static_cast<DWORD>(this->MaxWriteQueueDepthInKB); \
                config.MaxMetadataSizeInKB = static_cast<DWORD>(this->MaxMetadataSizeInKB); \
                config.SerializationVersion = static_cast<DWORD>(this->SerializationVersion); \
                config.EnableIncrementalBackupsAcrossReplicas = this->EnableIncrementalBackupsAcrossReplicas; \
                config.OptimizeForLocalSSD = this->OptimizeForLocalSSD; \
                config.OptimizeLogForLowerDiskUsage = this->OptimizeLogForLowerDiskUsage; \
                config.EnableSecondaryCommitApplyAcknowledgement = this->EnableSecondaryCommitApplyAcknowledgement; \
                config.SlowLogIODuration = this->SlowLogIODuration; \
                config.SlowLogIOCountThreshold = static_cast<DWORD>(this->SlowLogIOCountThreshold); \
                config.SlowLogIOTimeThreshold = this->SlowLogIOTimeThreshold; \
                config.SlowLogIOHealthReportTTL = this->SlowLogIOHealthReportTTL; \
                config.TruncationInterval = this->TruncationInterval; \
            }; \

#define DEFINE_TR_PRIVATE_CONFIG_MEMBERS() \
            Common::TimeSpan slowApiMonitoringDuration_; \
            int64 checkpointThresholdInMB_; \
            int64 maxAccumulatedBackupLogSizeInMB_; \
            int64 minLogSizeInMB_; \
            int64 truncationThresholdFactor_; \
            int64 throttlingThresholdFactor_; \
            int64 maxRecordSizeInKB_; \
            int64 maxStreamSizeInMB_; \
            int64 maxWriteQueueDepthInKB_;  \
            int64 maxMetadataSizeInKB_;  \
            int64 slowLogIOCountThreshold_;  \
            int64 serializationVersion_; \
            bool enableIncrementalBackupsAcrossReplicas_; \
            bool optimizeForLocalSSD_;  \
            bool optimizeLogForLowerDiskUsage_;  \
            bool enableSecondaryCommitApplyAcknowledgement_;  \
            Common::TimeSpan slowLogIODuration_; \
            Common::TimeSpan slowLogIOTimeThreshold_; \
            Common::TimeSpan slowLogIOHealthReportTTL_; \
            Common::TimeSpan truncationInterval_;

#define DEFINE_TR_PRIVATE_GLOBAL_CONFIG_MEMBERS() \
            int64 readAheadCacheSizeInKb_; \
            int64 copyBatchSizeInKb_; \
            int64 progressVectorMaxEntries_; \
            int64 flushedRecordsTraceVectorSize_; \
            std::wstring test_LoggingEngine_; \
            int64 test_LogMinDelayIntervalMilliseconds_; \
            int64 test_LogMaxDelayIntervalMilliseconds_; \
            double test_LogDelayRatio_; \
            double test_LogDelayProcessExitRatio_; \
            std::wstring dispatchingMode_; \

/*ProgressVectorMaxEntires is set to the maximum number of records that can be traced*/
#define TR_CONFIG_PROPERTIES(section_name)\
            PUBLIC_CONFIG_ENTRY(uint, section_name, CheckpointThresholdInMB, 50, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowApiMonitoringDuration, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxAccumulatedBackupLogSizeInMB, 800, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MinLogSizeInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, TruncationThresholdFactor, 2, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, ThrottlingThresholdFactor, 4, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxRecordSizeInKB, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxWriteQueueDepthInKB, 0, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxMetadataSizeInKB, 4, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(bool, section_name, OptimizeForLocalSSD, false, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(bool, section_name, OptimizeLogForLowerDiskUsage, true, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, ReadAheadCacheSizeInKb, 1024, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, CopyBatchSizeInKb, 64, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIODuration, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, SlowLogIOCountThreshold, 60, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIOTimeThreshold, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIOHealthReportTTL, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, TruncationInterval, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSecondaryCommitApplyAcknowledgement, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxStreamSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, ProgressVectorMaxEntries, 800, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, FlushedRecordsTraceVectorSize, 32, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SerializationVersion, 0, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableIncrementalBackupsAcrossReplicas, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(std::wstring, section_name, DispatchingMode, L"", Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(std::wstring, section_name, Test_LoggingEngine, L"ktl", Common::ConfigEntryUpgradePolicy::NotAllowed); \
            TEST_CONFIG_ENTRY(uint, section_name, Test_LogMinDelayIntervalMilliseconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(uint, section_name, Test_LogMaxDelayIntervalMilliseconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(double, section_name, Test_LogDelayRatio, 0, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0)); \
            TEST_CONFIG_ENTRY(double, section_name, Test_LogDelayProcessExitRatio, 0, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0)); \

/*ProgressVectorMaxEntires is set to the maximum number of records that can be traced*/
#define TR_INTERNAL_CONFIG_PROPERTIES(section_name)\
            INTERNAL_CONFIG_ENTRY(uint, section_name, CheckpointThresholdInMB, 50, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowApiMonitoringDuration, Common::TimeSpan::FromSeconds(300), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxAccumulatedBackupLogSizeInMB, 800, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MinLogSizeInMB, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, TruncationThresholdFactor, 2, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, ThrottlingThresholdFactor, 4, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxRecordSizeInKB, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxWriteQueueDepthInKB, 0, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxMetadataSizeInKB, 4, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, OptimizeForLocalSSD, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, OptimizeLogForLowerDiskUsage, true, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSecondaryCommitApplyAcknowledgement, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxStreamSizeInMB, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, ReadAheadCacheSizeInKb, 1024, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, CopyBatchSizeInKb, 64, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SerializationVersion, 1, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableIncrementalBackupsAcrossReplicas, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIODuration, Common::TimeSpan::FromSeconds(1), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SlowLogIOCountThreshold, 60, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIOTimeThreshold, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowLogIOHealthReportTTL, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, ProgressVectorMaxEntries, 800, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, FlushedRecordsTraceVectorSize, 32, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, TruncationInterval, Common::TimeSpan::FromSeconds(0), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(std::wstring, section_name, DispatchingMode, L"", Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(std::wstring, section_name, Test_LoggingEngine, L"ktl", Common::ConfigEntryUpgradePolicy::NotAllowed); \
            TEST_CONFIG_ENTRY(uint, section_name, Test_LogMinDelayIntervalMilliseconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(uint, section_name, Test_LogMaxDelayIntervalMilliseconds, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            TEST_CONFIG_ENTRY(double, section_name, Test_LogDelayRatio, 0, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0)); \
            TEST_CONFIG_ENTRY(double, section_name, Test_LogDelayProcessExitRatio, 0, Common::ConfigEntryUpgradePolicy::Dynamic, Common::InRange<double>(0.0, 1.0)); \
            \
            DEFINE_GET_TR_CONFIG_METHOD()

}


