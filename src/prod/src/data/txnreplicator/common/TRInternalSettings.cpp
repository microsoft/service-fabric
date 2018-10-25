// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TxnReplicator;
using Common::AcquireReadLock;
using Common::AcquireExclusiveLock;
using Common::EventArgs;

Common::StringLiteral const TraceComponent("TRInternalSettings");

template<class T>
inline void TRInternalSettings::TraceConfigUpdate(
    __in std::wstring configName,
    __in T previousConfig,
    __in T newConfig)
{
    Trace.WriteInfo(
        TraceComponent,
        "Updated {0} - Previous Value = {1}, New Value = {2}",
        configName,
        previousConfig,
        newConfig);
}

TRInternalSettings::TRInternalSettings(
    TransactionalReplicatorSettingsUPtr && userSettings,
    std::shared_ptr<TransactionalReplicatorConfig> const & globalConfig)
    : lock_()
    , globalConfig_(globalConfig)
    , userSettings_(move(userSettings))
{
    LoadSettings();
}

TRInternalSettingsSPtr TRInternalSettings::Create(
    TransactionalReplicatorSettingsUPtr && userSettings,
    std::shared_ptr<TransactionalReplicatorConfig> const & globalConfig)
{
    TRInternalSettingsSPtr obj = TRInternalSettingsSPtr(new TRInternalSettings(move(userSettings), globalConfig));
    return obj;
}

void TRInternalSettings::LoadSettings()
{
    AcquireExclusiveLock grab(lock_);
    
    // First, load all global settings that cannot be overridden
    int globalSettingsCount = LoadGlobalSettingsCallerHoldsLock();

    // Next, load default values from ClusterManifest that can be overridden
    int dynamicOverridableSettingsCount = LoadDefaultsForDynamicOverridableSettingsCallerHoldsLock();

    int staticOverridableSettingsCount = LoadDefaultsForStaticOverridableSettingsCallerHoldsLock();

    // Load all the settings overridden by the user
    if (userSettings_ != nullptr)
    {
        LoadUserSettingsCallerHoldsLock();
    }

    LoadDefaultsFromCodeIfInvalid();

    TESTASSERT_IF(globalSettingsCount != TR_GLOBAL_SETTINGS_COUNT);
    TESTASSERT_IF(dynamicOverridableSettingsCount != TR_OVERRIDABLE_DYNAMIC_SETTINGS_COUNT);
    TESTASSERT_IF(staticOverridableSettingsCount != TR_OVERRIDABLE_STATIC_SETTINGS_COUNT);
}

void TRInternalSettings::LoadDefaultsFromCodeIfInvalid()
{ 
    if (this->minLogSizeInMB_ == 0)
    {
        std::wstring error;
        Common::StringWriter writer(error);
        Common::ErrorCode ec = TransactionalReplicatorSettings::GetMinLogSizeInMB(
            writer,
            this->checkpointThresholdInMB_,
            this->minLogSizeInMB_);

        TESTASSERT_IFNOT(ec.IsSuccess(), "Failed to GetMinLogSizeInMB with EC: {0}", ec);
    }

    if (this->optimizeLogForLowerDiskUsage_ == true)
    {
        this->maxStreamSizeInMB_ = TransactionalReplicatorSettings::SpareMaxStreamSizeInMB;
    }
}

void TRInternalSettings::LoadUserSettingsCallerHoldsLock()
{
    DWORD64 flags = userSettings_->Flags;

    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB) != 0) 
    {
        this->checkpointThresholdInMB_ = userSettings_->CheckpointThresholdInMB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS) != 0)
    {
        this->slowApiMonitoringDuration_ = userSettings_->SlowApiMonitoringDuration;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB) != 0)
    {
        this->maxAccumulatedBackupLogSizeInMB_ = userSettings_->MaxAccumulatedBackupLogSizeInMB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB) != 0)
    {
        this->minLogSizeInMB_ = userSettings_->MinLogSizeInMB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR) != 0)
    {
        this->truncationThresholdFactor_ = userSettings_->TruncationThresholdFactor;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR) != 0)
    {
        this->throttlingThresholdFactor_ = userSettings_->ThrottlingThresholdFactor;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB) != 0)
    {
        this->maxRecordSizeInKB_ = userSettings_->MaxRecordSizeInKB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB) != 0)
    {
        this->maxStreamSizeInMB_ = userSettings_->MaxStreamSizeInMB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB) != 0)
    {
        this->maxWriteQueueDepthInKB_ = userSettings_->MaxWriteQueueDepthInKB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB) != 0)
    {
        this->maxMetadataSizeInKB_ = userSettings_->MaxMetadataSizeInKB;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD) != 0)
    {
        this->optimizeForLocalSSD_ = userSettings_->OptimizeForLocalSSD;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT) != 0)
    {
        this->enableSecondaryCommitApplyAcknowledgement_ = userSettings_->EnableSecondaryCommitApplyAcknowledgement;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE) != 0)
    {
        this->optimizeLogForLowerDiskUsage_ = userSettings_->OptimizeLogForLowerDiskUsage;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS) != 0)
    {
        this->slowLogIODuration_ = userSettings_->SlowLogIODuration;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD) != 0)
    {
        this->slowLogIOCountThreshold_ = userSettings_->SlowLogIOCountThreshold;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS) != 0)
    {
        this->slowLogIOTimeThreshold_ = userSettings_->SlowLogIOTimeThreshold;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS) != 0)
    {
        this->slowLogIOHealthReportTTL_ = userSettings_->SlowLogIOHealthReportTTL;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION) != 0)
    {
        this->serializationVersion_ = userSettings_->SerializationVersion;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS) != 0)
    {
        this->truncationInterval_ = userSettings_->TruncationInterval;
    }
    if ((flags & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS) != 0)
    {
        this->enableIncrementalBackupsAcrossReplicas_ = userSettings_->EnableIncrementalBackupsAcrossReplicas;
    }
}

int TRInternalSettings::LoadGlobalSettingsCallerHoldsLock()
{
    int i = 0;

    this->readAheadCacheSizeInKb_ = globalConfig_->ReadAheadCacheSizeInKb;
    globalConfig_->ReadAheadCacheSizeInKbEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<int64>(L"ReadAheadCacheSizeInKb", this->readAheadCacheSizeInKb_, globalConfig_->ReadAheadCacheSizeInKb);

        this->readAheadCacheSizeInKb_ = globalConfig_->ReadAheadCacheSizeInKb;
    });

    i += 1;

    this->copyBatchSizeInKb_ = globalConfig_->CopyBatchSizeInKb;
    globalConfig_->CopyBatchSizeInKbEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<int64>(L"CopyBatchSizeInKb", this->copyBatchSizeInKb_, globalConfig_->CopyBatchSizeInKb);

        this->copyBatchSizeInKb_ = globalConfig_->CopyBatchSizeInKb;
    });

    i += 1;
    
    this->progressVectorMaxEntries_ = globalConfig_->ProgressVectorMaxEntries;
    globalConfig_->ProgressVectorMaxEntriesEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<int64>(L"ProgressVectorMaxEntries", this->progressVectorMaxEntries_, globalConfig_->ProgressVectorMaxEntries);

        this->progressVectorMaxEntries_ = globalConfig_->ProgressVectorMaxEntries;
    });

    i += 1;

    this->test_LoggingEngine_ = globalConfig_->Test_LoggingEngine;
    globalConfig_->Test_LoggingEngineEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate(
            L"Test_LoggingEngine", 
            this->test_LoggingEngine_, 
            globalConfig_->Test_LoggingEngine);

        this->test_LoggingEngine_ = globalConfig_->Test_LoggingEngine;
    });

    i += 1;

    this->dispatchingMode_ = globalConfig_->DispatchingMode;
    globalConfig_->DispatchingModeEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate(
            L"DispatchingMode", 
            this->dispatchingMode_, 
            globalConfig_->DispatchingMode);

        this->dispatchingMode_ = globalConfig_->DispatchingMode;
    });

    i += 1;

    this->test_LogMinDelayIntervalMilliseconds_ = globalConfig_->Test_LogMinDelayIntervalMilliseconds;
    globalConfig_->Test_LogMinDelayIntervalMillisecondsEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<int64>(
            L"Test_LogMinDelayIntervalMilliseconds",
            this->test_LogMinDelayIntervalMilliseconds_,
            globalConfig_->Test_LogMinDelayIntervalMilliseconds);

        this->test_LogMinDelayIntervalMilliseconds_ = globalConfig_->Test_LogMinDelayIntervalMilliseconds;
    });

    i += 1;

    this->test_LogMaxDelayIntervalMilliseconds_ = globalConfig_->Test_LogMaxDelayIntervalMilliseconds;
    globalConfig_->Test_LogMaxDelayIntervalMillisecondsEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<int64>(
            L"Test_LogMaxDelayIntervalMilliseconds",
            this->test_LogMaxDelayIntervalMilliseconds_,
            globalConfig_->Test_LogMaxDelayIntervalMilliseconds);

        this->test_LogMaxDelayIntervalMilliseconds_ = globalConfig_->Test_LogMaxDelayIntervalMilliseconds;
    });

    i += 1;

    this->test_LogDelayRatio_ = globalConfig_->Test_LogDelayRatio;
    globalConfig_->Test_LogDelayRatioEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<double>(
            L"Test_LogDelayRatio",
            this->test_LogDelayRatio_,
            globalConfig_->Test_LogDelayRatio);

        this->test_LogDelayRatio_ = globalConfig_->Test_LogDelayRatio;
    });

    i += 1;

    this->test_LogDelayProcessExitRatio_ = globalConfig_->Test_LogDelayProcessExitRatio;
    globalConfig_->Test_LogDelayProcessExitRatioEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        TraceConfigUpdate<double>(
            L"Test_LogDelayProcessExitRatio",
            this->test_LogDelayProcessExitRatio_,
            globalConfig_->Test_LogDelayProcessExitRatio);

        this->test_LogDelayProcessExitRatio_ = globalConfig_->Test_LogDelayProcessExitRatio;
    });

    i += 1;

    this->flushedRecordsTraceVectorSize_ = globalConfig_->FlushedRecordsTraceVectorSize;
    i += 1;

    return i;
}

int TRInternalSettings::LoadDefaultsForDynamicOverridableSettingsCallerHoldsLock()
{
    // Register update handlers for overridable settings
    // User settings take precedence over global configs
    // If the latest user setting object indicatest that a config has been set, the handler will not update its value
    int i = 0;

    this->checkpointThresholdInMB_ = globalConfig_->CheckpointThresholdInMB;
    globalConfig_->CheckpointThresholdInMBEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateCheckpointThresholdInMB = 
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB) == 0;

        if(updateCheckpointThresholdInMB)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"CheckpointThresholdInMB",
                this->checkpointThresholdInMB_,
                globalConfig_->CheckpointThresholdInMB);

            this->checkpointThresholdInMB_ = globalConfig_->CheckpointThresholdInMB;
        }
    });
    i += 1;
    
    this->slowApiMonitoringDuration_ = globalConfig_->SlowApiMonitoringDuration;
    globalConfig_->SlowApiMonitoringDurationEntry.AddHandler(
        [&](EventArgs const &) {
        
        // Only update if no user value is provided
        bool updateSlowApiMonitoringDuration =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS) == 0;

        if (updateSlowApiMonitoringDuration)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"SlowApiMonitoringDuration",
                this->slowApiMonitoringDuration_.TotalSeconds(),
                globalConfig_->SlowApiMonitoringDuration.TotalSeconds());

            this->slowApiMonitoringDuration_ = globalConfig_->SlowApiMonitoringDuration;
        }
    });
    i += 1;

    this->maxAccumulatedBackupLogSizeInMB_ = globalConfig_->MaxAccumulatedBackupLogSizeInMB;
    globalConfig_->MaxAccumulatedBackupLogSizeInMBEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateMaxAccumulatedBackupLogSizeInMB =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB) == 0;

        if (updateMaxAccumulatedBackupLogSizeInMB)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"MaxAccumulatedBackupLogSizeInMB",
                this->maxAccumulatedBackupLogSizeInMB_,
                globalConfig_->MaxAccumulatedBackupLogSizeInMB);

            this->maxAccumulatedBackupLogSizeInMB_ = globalConfig_->MaxAccumulatedBackupLogSizeInMB;
        }
    });
    i += 1;

    this->minLogSizeInMB_ = globalConfig_->MinLogSizeInMB;
    globalConfig_->MinLogSizeInMBEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateMinLogSizeInMB =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB) == 0;

        if (updateMinLogSizeInMB)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"MinLogSizeInMB",
                this->minLogSizeInMB_,
                globalConfig_->MinLogSizeInMB);

            this->minLogSizeInMB_ = globalConfig_->MinLogSizeInMB;
            
            // If user sets this to 0, ensure we set it to the right value
            this->LoadDefaultsFromCodeIfInvalid();
        }
    });
    i += 1;
    
    this->truncationThresholdFactor_ = globalConfig_->TruncationThresholdFactor;
    globalConfig_->TruncationThresholdFactorEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateTruncationThresholdFactor =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR) == 0;

        if (updateTruncationThresholdFactor)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"TruncationThresholdFactor",
                this->truncationThresholdFactor_,
                globalConfig_->TruncationThresholdFactor);

            this->truncationThresholdFactor_ = globalConfig_->TruncationThresholdFactor;
        }
    });
    i += 1;

    this->throttlingThresholdFactor_ = globalConfig_->ThrottlingThresholdFactor;
    globalConfig_->ThrottlingThresholdFactorEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateThrottlingThresholdFactor =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR) == 0;

        if (updateThrottlingThresholdFactor)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"ThrottlingThresholdFactor",
                this->throttlingThresholdFactor_,
                globalConfig_->ThrottlingThresholdFactor);

            this->throttlingThresholdFactor_ = globalConfig_->ThrottlingThresholdFactor;
        }
    });
    i += 1;

    this->slowLogIODuration_ = globalConfig_->SlowLogIODuration;
    globalConfig_->SlowLogIODurationEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateSlowLogIODuration =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS) == 0;

        if (updateSlowLogIODuration)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"SlowLogIODuration",
                this->slowLogIODuration_.TotalSeconds(),
                globalConfig_->SlowLogIODuration.TotalSeconds());

            this->slowLogIODuration_ = globalConfig_->SlowLogIODuration;
        }
    });
    i += 1;

    this->slowLogIOCountThreshold_ = globalConfig_->SlowLogIOCountThreshold;
    globalConfig_->SlowLogIOCountThresholdEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateSlowLogIOCountThreshold =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD) == 0;

        if (updateSlowLogIOCountThreshold)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"SlowLogIOCountThreshold",
                this->slowLogIOCountThreshold_,
                globalConfig_->SlowLogIOCountThreshold);

            this->slowLogIOCountThreshold_ = globalConfig_->SlowLogIOCountThreshold;
        }
    });
    i += 1;

    this->slowLogIOTimeThreshold_ = globalConfig_->SlowLogIOTimeThreshold;
    globalConfig_->SlowLogIOTimeThresholdEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateSlowLogIOTimeThreshold =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS) == 0;

        if (updateSlowLogIOTimeThreshold)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"SlowLogIOTimeThreshold",
                this->slowLogIOTimeThreshold_.TotalSeconds(),
                globalConfig_->SlowLogIOTimeThreshold.TotalSeconds());

            this->slowLogIOTimeThreshold_ = globalConfig_->SlowLogIOTimeThreshold;
        }
    });
    i += 1;

    this->slowLogIOHealthReportTTL_ = globalConfig_->SlowLogIOHealthReportTTL;
    globalConfig_->SlowLogIOHealthReportTTLEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updateSlowLogIOHealthReportDuration =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS) == 0;

        if (updateSlowLogIOHealthReportDuration)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"SlowLogIOHealthReportTTL",
                this->slowLogIOHealthReportTTL_.TotalSeconds(),
                globalConfig_->SlowLogIOHealthReportTTL.TotalSeconds());

            this->slowLogIOHealthReportTTL_ = globalConfig_->SlowLogIOHealthReportTTL;
        }
    });
    i += 1;

    this->truncationInterval_ = globalConfig_->TruncationInterval;
    globalConfig_->TruncationIntervalEntry.AddHandler(
        [&](EventArgs const &) {

        // Only update if no user value is provided
        bool updatePeriodicCheckpointTruncationInterval =
            (userSettings_ == nullptr) ||
            (userSettings_->Flags & FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS) == 0;

        if (updatePeriodicCheckpointTruncationInterval)
        {
            AcquireExclusiveLock grab(lock_);

            TraceConfigUpdate<int64>(
                L"PeriodicCheckpointTruncationInterval",
                this->TruncationInterval.TotalSeconds(),
                globalConfig_->TruncationInterval.TotalSeconds());

            this->truncationInterval_ = globalConfig_->TruncationInterval;
        }
    });
    i += 1;

    return i;
}

int TRInternalSettings::LoadDefaultsForStaticOverridableSettingsCallerHoldsLock()
{
    int i = 0;

    // Handler not required for the following static configs:
    this->maxStreamSizeInMB_ = globalConfig_->MaxStreamSizeInMB;
    i += 1;

    this->maxRecordSizeInKB_ = globalConfig_->MaxRecordSizeInKB;
    i += 1;

    this->maxWriteQueueDepthInKB_ = globalConfig_->MaxWriteQueueDepthInKB;
    i += 1;

    this->maxMetadataSizeInKB_ = globalConfig_->MaxMetadataSizeInKB;
    i += 1;

    this->optimizeForLocalSSD_ = globalConfig_->OptimizeForLocalSSD;
    i += 1;

    this->optimizeLogForLowerDiskUsage_ = globalConfig_->OptimizeLogForLowerDiskUsage;
    i += 1;

    this->enableSecondaryCommitApplyAcknowledgement_ = globalConfig_->EnableSecondaryCommitApplyAcknowledgement;
    i += 1;

    this->serializationVersion_ = globalConfig_->SerializationVersion;
    i += 1;

    this->enableIncrementalBackupsAcrossReplicas_ = globalConfig_->EnableIncrementalBackupsAcrossReplicas;
    i += 1;

    return i;
}

int64 TRInternalSettings::get_CheckpointThresholdInMB() const
{
    AcquireReadLock grab(lock_);
    return checkpointThresholdInMB_;
}

Common::TimeSpan TRInternalSettings::get_SlowApiMonitoringDuration() const
{
    AcquireReadLock grab(lock_);
    return slowApiMonitoringDuration_;
}

int64 TRInternalSettings::get_MaxAccumulatedBackupLogSizeInMB() const
{
    AcquireReadLock grab(lock_);
    return maxAccumulatedBackupLogSizeInMB_;
}

int64 TRInternalSettings::get_MinLogSizeInMB() const
{
    AcquireReadLock grab(lock_);
    return minLogSizeInMB_;
}

int64 TRInternalSettings::get_TruncationThresholdFactor() const
{
    AcquireReadLock grab(lock_);
    return truncationThresholdFactor_;
}

int64 TRInternalSettings::get_ThrottlingThresholdFactor() const
{
    AcquireReadLock grab(lock_);
    return throttlingThresholdFactor_;
}

int64 TRInternalSettings::get_MaxRecordSizeInKB() const
{
    AcquireReadLock grab(lock_);
    return maxRecordSizeInKB_;
}

int64 TRInternalSettings::get_MaxStreamSizeInMB() const
{
    AcquireReadLock grab(lock_);
    return maxStreamSizeInMB_;
}

int64 TRInternalSettings::get_CopyBatchSizeInKb() const
{
    AcquireReadLock grab(lock_);
    return copyBatchSizeInKb_;
}

int64 TRInternalSettings::get_ProgressVectorMaxEntries() const
{
    AcquireReadLock grab(lock_);
    return progressVectorMaxEntries_;
}

int64 TRInternalSettings::get_ReadAheadCacheSizeInKb() const
{
    AcquireReadLock grab(lock_);
    return readAheadCacheSizeInKb_;
}

std::wstring TRInternalSettings::get_TestLoggingEngine() const
{
    AcquireReadLock grab(lock_);
    return test_LoggingEngine_;
}

std::wstring TRInternalSettings::get_DispatchingMode() const
{
    AcquireReadLock grab(lock_);
    return dispatchingMode_;
}

int64 TRInternalSettings::get_TestLogMinDelayIntervalInMilliseconds() const
{
    AcquireReadLock grab(lock_);
    return test_LogMinDelayIntervalMilliseconds_;
}

int64 TRInternalSettings::get_TestLogMaxDelayIntervalInMilliseconds() const
{
    AcquireReadLock grab(lock_);
    return test_LogMaxDelayIntervalMilliseconds_;
}

int64 TRInternalSettings::get_SerializationVersion() const
{
    AcquireReadLock grab(lock_);
    return serializationVersion_;
}

double TRInternalSettings::get_TestLogDelayRatio() const
{
    AcquireReadLock grab(lock_);
    return test_LogDelayRatio_;
}

double TRInternalSettings::get_TestLogDelayProcessExitRatio() const
{
    AcquireReadLock grab(lock_);
    return test_LogDelayProcessExitRatio_;
}

int64 TRInternalSettings::get_MaxWriteQueueDepthInKB() const
{
    AcquireReadLock grab(lock_);
    return maxWriteQueueDepthInKB_;
}

int64 TRInternalSettings::get_MaxMetadataSizeInKB() const
{
    AcquireReadLock grab(lock_);
    return maxMetadataSizeInKB_;
}

bool TRInternalSettings::get_OptimizeForLocalSSD() const
{
    AcquireReadLock grab(lock_);
    return optimizeForLocalSSD_;
}

bool TRInternalSettings::get_OptimizeLogForLowerDiskUsage() const
{
    AcquireReadLock grab(lock_);
    return optimizeLogForLowerDiskUsage_;
}

bool TRInternalSettings::get_EnableSecondaryCommitApplyAcknowledgement() const
{
    AcquireReadLock grab(lock_);
    return enableSecondaryCommitApplyAcknowledgement_;
}

bool TRInternalSettings::get_EnableIncrementalBackupsAcrossReplicas() const
{
    AcquireReadLock grab(lock_);
    return enableIncrementalBackupsAcrossReplicas_;
}

Common::TimeSpan TRInternalSettings::get_SlowLogIODuration() const
{
    AcquireReadLock grab(lock_);
    return slowLogIODuration_;
}

int64 TRInternalSettings::get_SlowLogIOCountThreshold() const
{
    AcquireReadLock grab(lock_);
    return slowLogIOCountThreshold_;
}

Common::TimeSpan TRInternalSettings::get_SlowLogIOTimeThreshold() const
{
    AcquireReadLock grab(lock_);
    return slowLogIOTimeThreshold_;
}

Common::TimeSpan TRInternalSettings::get_SlowLogIOHealthReportTTL() const
{
    AcquireReadLock grab(lock_);
    return slowLogIOHealthReportTTL_;
}

int64 TRInternalSettings::get_FlushedRecordsTraceVectorSize() const
{
    AcquireReadLock grab(lock_);
    return flushedRecordsTraceVectorSize_;
}

Common::TimeSpan TRInternalSettings::get_TruncationInterval() const
{
    AcquireReadLock grab(lock_);
    return truncationInterval_;
}

std::wstring TRInternalSettings::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);
    int settingsCount = WriteTo(writer, Common::FormatOptions(0, false, ""));

    TESTASSERT_IFNOT(
        settingsCount == TR_SETTINGS_COUNT, 
        "A new setting was added without including it in the trace output");

    return content;
}

int TRInternalSettings::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    int i = 0;

    w.WriteLine("CheckpointThresholdInMB = {0}, ", this->CheckpointThresholdInMB);
    i += 1;

    w.WriteLine("SlowApiMonitoringDuration = {0}, ", this->SlowApiMonitoringDuration);
    i += 1;

    w.WriteLine("MaxAccumulatedBackupLogSizeInMB = {0}, ", this->MaxAccumulatedBackupLogSizeInMB);
    i += 1;

    w.WriteLine("MinLogSizeInMB = {0}, ", this->MinLogSizeInMB);
    i += 1;

    w.WriteLine("TruncationThresholdFactor = {0}, ", this->TruncationThresholdFactor);
    i += 1;

    w.WriteLine("ThrottlingThresholdFactor = {0}, ", this->ThrottlingThresholdFactor);
    i += 1;

    w.WriteLine("MaxRecordSizeInKB = {0}, ", this->MaxRecordSizeInKB);
    i += 1;

    w.WriteLine("MaxStreamSizeInMB = {0}, ", this->MaxStreamSizeInMB);
    i += 1;

    w.WriteLine("MaxWriteQueueDepthInKB = {0}, ", this->MaxWriteQueueDepthInKB);
    i += 1;

    w.WriteLine("MaxMetadataSizeInKB = {0}, ", this->MaxMetadataSizeInKB);
    i += 1;

    w.WriteLine("SerializationVersion = {0}, ", this->SerializationVersion);
    i += 1;

    w.WriteLine("OptimizeForLocalSSD = {0}, ", this->OptimizeForLocalSSD);
    i += 1;

    w.WriteLine("OptimizeLogForLowerDiskUsage = {0}, ", this->OptimizeLogForLowerDiskUsage);
    i += 1;

    w.WriteLine("EnableSecondaryCommitApplyAcknowledgement = {0}, ", this->EnableSecondaryCommitApplyAcknowledgement);
    i += 1;

    w.WriteLine("EnableIncrementalBackupsAcrossReplicas = {0}, ", this->EnableIncrementalBackupsAcrossReplicas);
    i += 1;

    w.WriteLine("SlowLogIODuration = {0}, ", this->SlowLogIODuration);
    i += 1;

    w.WriteLine("SlowLogIOCountThreshold = {0}, ", this->SlowLogIOCountThreshold);
    i += 1;

    w.WriteLine("SlowLogIOTimeThreshold = {0}, ", this->SlowLogIOTimeThreshold);
    i += 1;

    w.WriteLine("SlowLogIOHealthReportTTL = {0}, ", this->SlowLogIOHealthReportTTL);
    i += 1;

    w.WriteLine("ReadAheadCacheSizeInKb = {0}, ", this->ReadAheadCacheSizeInKb);
    i += 1;

    w.WriteLine("CopyBatchSizeInKb = {0}, ", this->CopyBatchSizeInKb);
    i += 1;

    w.WriteLine("ProgressVectorMaxEntries = {0}, ", this->ProgressVectorMaxEntries);
    i += 1;

    w.WriteLine("Test_LoggingEngine = {0}, ", this->Test_LoggingEngine);
    i += 1;

    w.WriteLine("Test_LogMinDelayIntervalMilliseconds = {0}, ", this->Test_LogMinDelayIntervalMilliseconds);
    i += 1;

    w.WriteLine("Test_LogMaxDelayIntervalMilliseconds = {0}, ", this->Test_LogMaxDelayIntervalMilliseconds);
    i += 1;

    w.WriteLine("Test_LogDelayRatio = {0}, ", this->Test_LogDelayRatio);
    i += 1;

    w.WriteLine("Test_LogDelayProcessExitRatio = {0}, ", this->Test_LogDelayProcessExitRatio);
    i += 1;

    w.WriteLine("FlushedRecordsTraceVectorSize = {0}, ", this->FlushedRecordsTraceVectorSize);
    i += 1;

    w.WriteLine("TruncationInterval = {0}, ", this->TruncationInterval);
    i += 1;

    w.WriteLine("DispatchingMode = {0}, ", this->DispatchingMode);
    i += 1;

    return i;
}
