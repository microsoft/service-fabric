// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TxnReplicator;
using namespace ServiceModel;

int64 const TransactionalReplicatorSettings::DefaultMinLogSizeInMBDivider = 2;
int64 const TransactionalReplicatorSettings::SmallestMinLogSizeInMB = 1;
int64 const TransactionalReplicatorSettings::SpareMaxStreamSizeInMB = 200 * 1024;

TransactionalReplicatorSettings::TransactionalReplicatorSettings()
    : flags_(FABRIC_TRANSACTIONAL_REPLICATOR_SETTINGS_NONE)
    , slowApiMonitoringDuration_()
    , checkpointThresholdInMB_(0)
    , maxAccumulatedBackupLogSizeInMB_(0)
    , minLogSizeInMB_(0)
    , truncationThresholdFactor_(0)
    , throttlingThresholdFactor_(0)
    , maxRecordSizeInKB_(0)
    , enableSecondaryCommitApplyAcknowledgement_(false)
    , slowLogIODuration_()
    , slowLogIOCountThreshold_(0)
    , slowLogIOTimeThreshold_()
    , slowLogIOHealthReportTTL_()
    , truncationInterval_()
{
}

ErrorCode TransactionalReplicatorSettings::FromPublicApi(
    __in TRANSACTIONAL_REPLICATOR_SETTINGS const & transactionalReplicatorSettings2,
    __out TransactionalReplicatorSettingsUPtr & output)
{
    TransactionalReplicatorSettingsUPtr created = unique_ptr<TransactionalReplicatorSettings>(new TransactionalReplicatorSettings());

    // Populate all member variables
    created->flags_ = transactionalReplicatorSettings2.Flags;
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS)
    {
        created->slowApiMonitoringDuration_ = TimeSpan::FromSeconds(transactionalReplicatorSettings2.SlowApiMonitoringDurationSeconds);
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB)
    {
        created->checkpointThresholdInMB_ = transactionalReplicatorSettings2.CheckpointThresholdInMB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB)
    {
        created->maxAccumulatedBackupLogSizeInMB_ = transactionalReplicatorSettings2.MaxAccumulatedBackupLogSizeInMB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB)
    {
        created->minLogSizeInMB_ = transactionalReplicatorSettings2.MinLogSizeInMB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR)
    {
        created->truncationThresholdFactor_ = transactionalReplicatorSettings2.TruncationThresholdFactor;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR)
    {
        created->throttlingThresholdFactor_ = transactionalReplicatorSettings2.ThrottlingThresholdFactor;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB)
    {
        created->maxRecordSizeInKB_ = transactionalReplicatorSettings2.MaxRecordSizeInKB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB)
    {
        created->maxStreamSizeInMB_ = transactionalReplicatorSettings2.MaxStreamSizeInMB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB)
    {
        created->maxWriteQueueDepthInKB_ = transactionalReplicatorSettings2.MaxWriteQueueDepthInKB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB)
    {
        created->maxMetadataSizeInKB_ = transactionalReplicatorSettings2.MaxMetadataSizeInKB;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD)
    {
        created->optimizeForLocalSSD_ = transactionalReplicatorSettings2.OptimizeForLocalSSD;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT)
    {
        created->enableSecondaryCommitApplyAcknowledgement_ = transactionalReplicatorSettings2.EnableSecondaryCommitApplyAcknowledgement;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE)
    {
        created->optimizeLogForLowerDiskUsage_ = transactionalReplicatorSettings2.OptimizeLogForLowerDiskUsage;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS)
    {
        created->slowLogIODuration_ = TimeSpan::FromSeconds(transactionalReplicatorSettings2.SlowLogIODurationSeconds);
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD)
    {
        created->slowLogIOCountThreshold_ = transactionalReplicatorSettings2.SlowLogIOCountThreshold;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS)
    {
        created->slowLogIOTimeThreshold_ = TimeSpan::FromSeconds(transactionalReplicatorSettings2.SlowLogIOTimeThresholdSeconds);
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS)
    {
        created->slowLogIOHealthReportTTL_ = TimeSpan::FromSeconds(transactionalReplicatorSettings2.SlowLogIOHealthReportTTLSeconds);
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION)
    {
        created->serializationVersion_ = transactionalReplicatorSettings2.SerializationVersion;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS)
    {
        created->enableIncrementalBackupsAcrossReplicas_ = transactionalReplicatorSettings2.EnableIncrementalBackupsAcrossReplicas;
    }
    if (created->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS)
    {
        created->truncationInterval_ = TimeSpan::FromSeconds(transactionalReplicatorSettings2.LogTruncationIntervalSeconds);
    }

    return MoveIfValid(
        move(created),
        output);
}

ErrorCode TransactionalReplicatorSettings::FromConfig(
    __in TRConfigBase const & trConfig,
    __out TransactionalReplicatorSettingsUPtr & output)
{
    TransactionalReplicatorSettingsUPtr created = unique_ptr<TransactionalReplicatorSettings>(new TransactionalReplicatorSettings());

    TRConfigValues config;
    trConfig.GetTransactionalReplicatorSettingsStructValues(config);

    created->checkpointThresholdInMB_ = config.CheckpointThresholdInMB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;

    created->slowApiMonitoringDuration_ = config.SlowApiMonitoringDuration;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;

    created->maxAccumulatedBackupLogSizeInMB_ = config.MaxAccumulatedBackupLogSizeInMB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB;

    created->minLogSizeInMB_ = config.MinLogSizeInMB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;

    created->truncationThresholdFactor_ = config.TruncationThresholdFactor;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;

    created->throttlingThresholdFactor_ = config.ThrottlingThresholdFactor;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;

    created->maxRecordSizeInKB_ = config.MaxRecordSizeInKB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB;

    created->maxStreamSizeInMB_ = config.MaxStreamSizeInMB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;

    created->maxWriteQueueDepthInKB_ = config.MaxWriteQueueDepthInKB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB;

    created->maxMetadataSizeInKB_ = config.MaxMetadataSizeInKB;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB;

    created->optimizeForLocalSSD_ = config.OptimizeForLocalSSD;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD;

    created->enableSecondaryCommitApplyAcknowledgement_ = config.EnableSecondaryCommitApplyAcknowledgement;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT;

    created->optimizeLogForLowerDiskUsage_ = config.OptimizeLogForLowerDiskUsage;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE;

    created->slowLogIODuration_ = config.SlowLogIODuration;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS;

    created->slowLogIOCountThreshold_ = config.SlowLogIOCountThreshold;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD;

    created->slowLogIOTimeThreshold_ = config.SlowLogIOTimeThreshold;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS;

    created->slowLogIOHealthReportTTL_ = config.SlowLogIOHealthReportTTL;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS;

    created->serializationVersion_ = config.SerializationVersion;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION;

    created->truncationInterval_ = config.TruncationInterval;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS;
    
    created->enableIncrementalBackupsAcrossReplicas_ = config.EnableIncrementalBackupsAcrossReplicas;
    created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS;

    return MoveIfValid(
        move(created),
        output);
}

HRESULT TxnReplicator::TransactionalReplicatorSettings::FromConfig(
    __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in std::wstring const & configurationPackageName,
    __in std::wstring const & sectionName,
    __out IFabricTransactionalReplicatorSettingsResult ** replicatorSettings)
{
    // Validate code package activation context is not null
    if (codePackageActivationContextCPtr.GetRawPointer() == NULL)
    {
        ServiceModelEventSource::Trace->ReplicatorSettingsError(L"CodePackageActivationContext is NULL");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<IFabricConfigurationPackage> configPackageCPtr;
    HRESULT hr = codePackageActivationContextCPtr->GetConfigurationPackage(
        configurationPackageName.c_str(),
        configPackageCPtr.InitializationAddress());

    if (hr != S_OK ||
        configPackageCPtr.GetRawPointer() == NULL)
    {
        wstring errorMessage;
        StringWriter messageWriter(errorMessage);

        messageWriter.Write(
            "Failed to load TransactionalReplicatorSettings configuration package - {0}, HRESULT = {1}",
            configurationPackageName,
            hr);

        ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);  // TODO: Add TRSettingsError Trace 
        return ComUtility::OnPublicApiReturn(hr);
    }

    TransactionalReplicatorSettingsUPtr settings;
    hr = TransactionalReplicatorSettings::ReadFromConfigurationPackage(
        codePackageActivationContextCPtr,
        configPackageCPtr,
        sectionName,
        settings).ToHResult();

    if (hr == S_OK)
    {
        ASSERT_IFNOT(settings, "Transactional replicator settings cannot be null if validation succeeded");
        return ComTransactionalReplicatorSettingsResult::ReturnTransactionalReplicatorSettingsResult(
            move(settings),
            replicatorSettings);
    }
    else
    {
        return ComUtility::OnPublicApiReturn(hr);
    }
}

void TransactionalReplicatorSettings::ToPublicApi(__out TRANSACTIONAL_REPLICATOR_SETTINGS & transactionalReplicatorSettings)
{
    transactionalReplicatorSettings.Reserved = NULL;
    transactionalReplicatorSettings.Flags = flags_;

    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB) 
    {
        transactionalReplicatorSettings.CheckpointThresholdInMB = static_cast<DWORD>(checkpointThresholdInMB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS)
    {
        transactionalReplicatorSettings.SlowApiMonitoringDurationSeconds = static_cast<DWORD>(slowApiMonitoringDuration_.TotalSeconds());
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB)
    {
        transactionalReplicatorSettings.MaxAccumulatedBackupLogSizeInMB = static_cast<DWORD>(maxAccumulatedBackupLogSizeInMB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB)
    {
        transactionalReplicatorSettings.MinLogSizeInMB = static_cast<DWORD>(minLogSizeInMB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR)
    {
        transactionalReplicatorSettings.TruncationThresholdFactor = static_cast<DWORD>(truncationThresholdFactor_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR)
    {
        transactionalReplicatorSettings.ThrottlingThresholdFactor = static_cast<DWORD>(throttlingThresholdFactor_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB)
    {
        transactionalReplicatorSettings.MaxRecordSizeInKB = static_cast<DWORD>(maxRecordSizeInKB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB)
    {
        transactionalReplicatorSettings.MaxStreamSizeInMB = static_cast<DWORD>(maxStreamSizeInMB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB)
    {
        transactionalReplicatorSettings.MaxMetadataSizeInKB = static_cast<DWORD>(maxMetadataSizeInKB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB)
    {
        transactionalReplicatorSettings.MaxWriteQueueDepthInKB = static_cast<DWORD>(maxWriteQueueDepthInKB_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD) 
    {
        transactionalReplicatorSettings.OptimizeForLocalSSD = optimizeForLocalSSD_;
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT)
    {
        transactionalReplicatorSettings.EnableSecondaryCommitApplyAcknowledgement = enableSecondaryCommitApplyAcknowledgement_;
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE)
    {
        transactionalReplicatorSettings.OptimizeLogForLowerDiskUsage = optimizeLogForLowerDiskUsage_;
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS)
    {
        transactionalReplicatorSettings.SlowLogIODurationSeconds = static_cast<DWORD>(slowLogIODuration_.TotalSeconds());
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD)
    {
        transactionalReplicatorSettings.SlowLogIOCountThreshold = static_cast<DWORD>(slowLogIOCountThreshold_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS)
    {
        transactionalReplicatorSettings.SlowLogIOTimeThresholdSeconds = static_cast<DWORD>(slowLogIOTimeThreshold_.TotalSeconds());
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS)
    {
        transactionalReplicatorSettings.SlowLogIOHealthReportTTLSeconds = static_cast<DWORD>(slowLogIOHealthReportTTL_.TotalSeconds());
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION)
    {
        transactionalReplicatorSettings.SerializationVersion = static_cast<DWORD>(serializationVersion_);
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS)
    {
        transactionalReplicatorSettings.LogTruncationIntervalSeconds = static_cast<DWORD>(truncationInterval_.TotalSeconds());
    }
    if (flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS)
    {
        transactionalReplicatorSettings.EnableIncrementalBackupsAcrossReplicas = enableIncrementalBackupsAcrossReplicas_;
    }
}

Common::ErrorCode TxnReplicator::TransactionalReplicatorSettings::ReadFromConfigurationPackage(
    __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName,
    __out TransactionalReplicatorSettingsUPtr & output)
{
    ASSERT_IF(codePackageActivationContextCPtr.GetRawPointer() == NULL, "activationContext is null");
    ASSERT_IF(configPackageCPtr.GetRawPointer() == NULL, "configPackageCPtr is null");

    auto created = unique_ptr<TransactionalReplicatorSettings>(new TransactionalReplicatorSettings());
    bool hasValue = false;

    TRConfigValues configNames; // Dummy object used to retrieve key names of settings

    // CheckpointThresholdInMB
    ErrorCode error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.CheckpointThresholdInMBEntry.Key,
        created->checkpointThresholdInMB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB; }

    // SlowApiMonitoringDuration
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SlowApiMonitoringDurationEntry.Key,
        created->slowApiMonitoringDuration_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS; }

    // MaxAccumulatedBackupLogSizeInMB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaxAccumulatedBackupLogSizeInMBEntry.Key,
        created->maxAccumulatedBackupLogSizeInMB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB; }

    // MinLogSizeInMB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MinLogSizeInMBEntry.Key,
        created->minLogSizeInMB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB; }

    // TruncationThresholdFactor
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.TruncationThresholdFactorEntry.Key,
        created->truncationThresholdFactor_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR; }

    // ThrottlingThresholdFactor
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.ThrottlingThresholdFactorEntry.Key,
        created->throttlingThresholdFactor_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR; }

    // MaxRecordSizeInKB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaxRecordSizeInKBEntry.Key,
        created->maxRecordSizeInKB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB; }

    // MaxStreamSizeInMB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaxStreamSizeInMBEntry.Key,
        created->maxStreamSizeInMB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB; }

    // MaxWriteQueueDepthInKB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaxWriteQueueDepthInKBEntry.Key,
        created->maxWriteQueueDepthInKB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB; }

    // MaxMetadataSizeInKB
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaxMetadataSizeInKBEntry.Key,
        created->maxMetadataSizeInKB_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB; }

    // OptimizeForLocalSSD
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.OptimizeForLocalSSDEntry.Key,
        created->optimizeForLocalSSD_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD; }

    // OptimizeLogForLowerDiskUsage
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.OptimizeLogForLowerDiskUsageEntry.Key,
        created->optimizeLogForLowerDiskUsage_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE; }

    // EnableSecondaryCommitApplyAck
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.EnableSecondaryCommitApplyAcknowledgementEntry.Key,
        created->enableSecondaryCommitApplyAcknowledgement_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT; }

    // SlowLogIODuration
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SlowLogIODurationEntry.Key,
        created->slowLogIODuration_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS; }

    // SlowLogIOCountThreshold
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SlowLogIOCountThresholdEntry.Key,
        created->slowLogIOCountThreshold_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD; }

    // SlowLogIOTimeThreshold
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SlowLogIOTimeThresholdEntry.Key,
        created->slowLogIOTimeThreshold_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS; }

    // SlowLogIOHealthReportTTL
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SlowLogIOHealthReportTTLEntry.Key,
        created->slowLogIOHealthReportTTL_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS; }

    // SerializationVersion
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.SerializationVersionEntry.Key,
        created->serializationVersion_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION; }

    // PeriodicCheckpointTruncationInterval
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.TruncationIntervalEntry.Key,
        created->truncationInterval_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS; }
    // EnableIncrementalBackupsAcrossReplicas
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.EnableIncrementalBackupsAcrossReplicasEntry.Key,
        created->enableIncrementalBackupsAcrossReplicas_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->flags_ |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS; }

    return MoveIfValid(
        move(created),
        output);
}

int64 TransactionalReplicatorSettings::get_CheckpointThresholdInMB() const
{
    return checkpointThresholdInMB_;
}

Common::TimeSpan TransactionalReplicatorSettings::get_SlowApiMonitoringDuration() const
{
    return slowApiMonitoringDuration_;
}

int64 TransactionalReplicatorSettings::get_MaxAccumulatedBackupLogSizeInMB() const
{
    return maxAccumulatedBackupLogSizeInMB_;
}

int64 TransactionalReplicatorSettings::get_MinLogSizeInMB() const
{
    return minLogSizeInMB_;
}

int64 TransactionalReplicatorSettings::get_TruncationThresholdFactor() const
{
    return truncationThresholdFactor_;
}

int64 TransactionalReplicatorSettings::get_ThrottlingThresholdFactor() const
{
    return throttlingThresholdFactor_;
}

int64 TransactionalReplicatorSettings::get_MaxRecordSizeInKB() const
{
    return maxRecordSizeInKB_;
}

int64 TransactionalReplicatorSettings::get_MaxStreamSizeInMB() const
{
    return maxStreamSizeInMB_;
}

int64 TransactionalReplicatorSettings::get_MaxWriteQueueDepthInKB() const
{
    return maxWriteQueueDepthInKB_;
}

int64 TransactionalReplicatorSettings::get_SerializationVersion() const
{
    return serializationVersion_;
}

int64 TransactionalReplicatorSettings::get_MaxMetadataSizeInKB() const
{
    return maxMetadataSizeInKB_;
}

bool TransactionalReplicatorSettings::get_OptimizeForLocalSSD() const
{
    return optimizeForLocalSSD_;
}

bool TransactionalReplicatorSettings::get_OptimizeLogForLowerDiskUsage() const
{
    return optimizeLogForLowerDiskUsage_;
}

bool TransactionalReplicatorSettings::get_EnableSecondaryCommitApplyAcknowledgement() const
{
    return enableSecondaryCommitApplyAcknowledgement_;
}

bool TransactionalReplicatorSettings::get_EnableIncrementalBackupsAcrossReplicas() const
{
    return enableIncrementalBackupsAcrossReplicas_;
}

Common::TimeSpan TransactionalReplicatorSettings::get_SlowLogIODuration() const
{
    return slowLogIODuration_;
}

int64 TransactionalReplicatorSettings::get_SlowLogIOCountThreshold() const
{
    return slowLogIOCountThreshold_;
}

Common::TimeSpan TransactionalReplicatorSettings::get_SlowLogIOTimeThreshold() const
{
    return slowLogIOTimeThreshold_;
}

Common::TimeSpan TransactionalReplicatorSettings::get_SlowLogIOHealthReportTTL() const
{
    return slowLogIOHealthReportTTL_;
}

Common::TimeSpan TransactionalReplicatorSettings::get_TruncationInterval() const
{
    return truncationInterval_;
}

ErrorCode TransactionalReplicatorSettings::MoveIfValid(
    __in TransactionalReplicatorSettingsUPtr && toBeValidated,
    __in TransactionalReplicatorSettingsUPtr & output)
{
    wstring errorMessage;
    StringWriter messageWriter(errorMessage);
    std::shared_ptr<TRConfigValues> defaultValues = std::make_shared<TRConfigValues>();

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB)
    {
        if (toBeValidated->CheckpointThresholdInMB < 1)
        {
            messageWriter.WriteLine(
                "CheckpointThresholdInMB = {0}. It should be greater than 0 ",
                toBeValidated->CheckpointThresholdInMB);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB)
    {
        if (toBeValidated->MaxAccumulatedBackupLogSizeInMB < 1)
        {
            messageWriter.WriteLine(
                "MaxAccumulatedBackupLogSizeInMB = {0}.  It should be greater than 0",
                toBeValidated->MaxAccumulatedBackupLogSizeInMB);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage); // TODO: Create trace for TransactionalReplicatorSettings error
            return ErrorCodeValue::InvalidArgument;
        }

        if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB &&
            toBeValidated->MaxAccumulatedBackupLogSizeInMB >= toBeValidated->MaxStreamSizeInMB)
        {
            messageWriter.WriteLine(
                "MaxAccumulatedBackupLogSizeInMB = {0}.  MaxStreamSizeInMB = {1}. MaxAccumulatedBackupLogSizeInMB must be less than MaxStreamSizeInMB.",
                toBeValidated->MaxAccumulatedBackupLogSizeInMB,
                toBeValidated->MaxStreamSizeInMB);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS)
    {
        if (toBeValidated->SlowApiMonitoringDuration < Common::TimeSpan::Zero ||
            toBeValidated->SlowApiMonitoringDuration == Common::TimeSpan::MaxValue)
        {
            messageWriter.WriteLine(
                "SlowApiMonitoringDuration = {0}. SlowApiMonitoring must be greater than zero and less than TimeSpan.MaxValue, default = TimeSpan::FromSeconds(300). A value of zero disables api monitoring.",
                toBeValidated->SlowApiMonitoringDuration);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
            return ErrorCodeValue::InvalidArgument;
        }
    }

    ErrorCode minLogStatus = ValidateMinLogSize(messageWriter, toBeValidated, defaultValues);
    if(!minLogStatus.IsSuccess())
    {
        ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
        return minLogStatus;
    }

    ErrorCode loggerSettingStatus = ValidateLoggerSettings(messageWriter, toBeValidated);
    if(!loggerSettingStatus.IsSuccess())
    {
        ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
        return loggerSettingStatus;
    }

    ErrorCode logIOSettingStatus = ValidateLogIOHealthReportSettings(messageWriter, toBeValidated, defaultValues);
    if (!logIOSettingStatus.IsSuccess())
    {
        // Trace error message but pass validation
        // Incorrect configuration expected to be overridden with default values
        ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage);
    }

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION)
    {
        if (toBeValidated->SerializationVersion > 1 || toBeValidated->SerializationVersion < 0)
        {
            messageWriter.WriteLine(
                "SerializationVersion = {0}. Current SerializationVersion should be either 0 or 1.",
                toBeValidated->SerializationVersion);

            ServiceModelEventSource::Trace->ReplicatorSettingsError(errorMessage); 
            return ErrorCodeValue::InvalidArgument;
        }
    }

    output = move(toBeValidated);
    return ErrorCode();
}

int64 TransactionalReplicatorSettings::GetThrottleThresholdInMB(
    __in int64 throttleThresholdFactor,
    __in int64 checkpointThresholdInMB,
    __in int64 minLogSizeInMb)
{
    int64 throttleThresholdInMBFromCheckpointInMB = checkpointThresholdInMB * throttleThresholdFactor;
    int64 throttleThresholdInMBFromMinLogSizeInMB = minLogSizeInMb * throttleThresholdFactor;
    return std::max(throttleThresholdInMBFromCheckpointInMB, throttleThresholdInMBFromMinLogSizeInMB);
}

ErrorCode TransactionalReplicatorSettings::ValidateMinLogSize(
    __in Common::StringWriter & messageWriter,
    __in TransactionalReplicatorSettingsUPtr & toBeValidated,
    __in std::shared_ptr<TRConfigValues> & defaultValues)
{
    ErrorCode e;

    int64 checkpointThresholdInMB =
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB ?
        toBeValidated->CheckpointThresholdInMB :
        defaultValues->CheckpointThresholdInMB;

    int64 minLogSizeInMB =
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB ?
        toBeValidated->MinLogSizeInMB :
        defaultValues->MinLogSizeInMB;

    e = GetMinLogSizeInMB(messageWriter, checkpointThresholdInMB, minLogSizeInMB);
    if(!e.IsSuccess())
    {
        return e;
    }

    int64 truncationThresholdFactor =
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR ?
        toBeValidated->TruncationThresholdFactor :
        defaultValues->TruncationThresholdFactor;

    int64 throttlingThresholdFactor =
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR ?
        toBeValidated->ThrottlingThresholdFactor :
        defaultValues->ThrottlingThresholdFactor;

    int64 maxStreamSizeInMB =
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB ?
        toBeValidated->MaxStreamSizeInMB :
        defaultValues->MaxStreamSizeInMB;

    bool isSparseFileOff = 
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE &&
        toBeValidated->OptimizeLogForLowerDiskUsage == false;

    maxStreamSizeInMB = isSparseFileOff ? maxStreamSizeInMB : SpareMaxStreamSizeInMB;
    
    // MinLogSizeInMB checks.
    if (minLogSizeInMB < 1)
    {
        messageWriter.WriteLine(
            "MinLogSizeInMB {0} must be greater than 0",
            minLogSizeInMB);

        return ErrorCodeValue::InvalidArgument;
    }

    if (minLogSizeInMB >= maxStreamSizeInMB)
    {
        messageWriter.WriteLine(
            "MinLogSizeInMB = {0}. MaxStreamSizeInMB = {1}. MinLogSizeInMB {0} must be smaller than MaxStreamSizeInMB {1}.",
            minLogSizeInMB,
            maxStreamSizeInMB);

        return ErrorCodeValue::InvalidArgument;
    }

    // TruncationFactor checks
    if (truncationThresholdFactor < 2)
    {
        messageWriter.WriteLine(
            "TruncationThresholdFactor {0} must be greater than 1",
            truncationThresholdFactor);

        return ErrorCodeValue::InvalidArgument;
    }

    int64 truncationThresholdInMB = minLogSizeInMB * truncationThresholdFactor;
    if (truncationThresholdInMB >= maxStreamSizeInMB)
    {
        messageWriter.WriteLine(
            "MinLogSizeInMB {0} * TruncationThresholdFactor {1} must be smaller than MaxStreamSizeInMB {2}",
            minLogSizeInMB,
            truncationThresholdFactor,
            truncationThresholdInMB);

        return ErrorCodeValue::InvalidArgument;
    }

    // ThrottlingThresholdFactor checks.
    // Throttling threshold factor must be at least three since truncation must at least be 2
    if (throttlingThresholdFactor < 3)
    {
        messageWriter.WriteLine(
            "ThrottlingThresholdFactor {0} must be greater than 2",
            throttlingThresholdFactor);

        return ErrorCodeValue::InvalidArgument;
    }

    if (throttlingThresholdFactor <= truncationThresholdFactor)
    {
        messageWriter.WriteLine(
            "ThrottlingThresholdFactor {0} must be greater than TruncationThresholdFactor {1}.",
            throttlingThresholdFactor,
            truncationThresholdFactor);

        return ErrorCodeValue::InvalidArgument;
    }

    int64 throttlingThresholdInMB = GetThrottleThresholdInMB(
        throttlingThresholdFactor,
        checkpointThresholdInMB,
        minLogSizeInMB);

    if (throttlingThresholdInMB >= maxStreamSizeInMB)
    {
        messageWriter.WriteLine(
            "Max(MinLogSizeInMB {0} * ThrottlingThresholdFactor {1}, CheckpointThresholdInMB {2} * ThrottlingThresholdFactor {1}) must be smaller than MaxStreamSizeInMB {3}",
            minLogSizeInMB,
            throttlingThresholdFactor,
            checkpointThresholdInMB,
            maxStreamSizeInMB);

        return ErrorCodeValue::InvalidArgument;
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode TransactionalReplicatorSettings::ValidateLoggerSettings(
    __in Common::StringWriter & messageWriter,
    __in TransactionalReplicatorSettingsUPtr & toBeValidated)
{
    if(toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB)
    {
        if(toBeValidated->MaxWriteQueueDepthInKB < 0)
        {
            messageWriter.WriteLine(
                "MaxWriteQueueDepthInKB {0} must be greater than 0",
                toBeValidated->MaxWriteQueueDepthInKB);

            return ErrorCodeValue::InvalidArgument;
        }

        if (!(toBeValidated->MaxWriteQueueDepthInKB == 0 ||
            IsMultipleOf4(toBeValidated->MaxWriteQueueDepthInKB)))
        {
            messageWriter.WriteLine(
                "MaxWriteQueueDepthInKB must be zero or a multiple of 4. Provided {0}",
                toBeValidated->MaxWriteQueueDepthInKB);

            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB)
    {
        if (toBeValidated->MaxMetadataSizeInKB < 0)
        {
            messageWriter.WriteLine(
                "MaxMetadataSizeInKB {0} must be greater than 0",
                toBeValidated->MaxMetadataSizeInKB);

            return ErrorCodeValue::InvalidArgument;
        }

        if (!IsMultipleOf4(toBeValidated->MaxMetadataSizeInKB))
        {
            messageWriter.WriteLine(
                "MaxMetadataSizeInKB must be a multiple of 4. Provided {0}",
                toBeValidated->MaxMetadataSizeInKB);

            return ErrorCodeValue::InvalidArgument;
        }
    }

    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB)
    {
        if (!IsMultipleOf4(toBeValidated->MaxRecordSizeInKB))
        {
            messageWriter.WriteLine(
                "MaxRecordSizeInKB must be a multiple of 4. Provided {0}",
                toBeValidated->MaxRecordSizeInKB);

            return ErrorCodeValue::InvalidArgument;
        }

        if(toBeValidated->MaxRecordSizeInKB < 128)
        {
            messageWriter.WriteLine(
                "MaxRecordSizeInKB must be greater than or equal to 128. Provided {0}",
                toBeValidated->MaxRecordSizeInKB);

            return ErrorCodeValue::InvalidArgument;
        }
    }

    if((toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE &&
        toBeValidated->optimizeLogForLowerDiskUsage_ == false) &&
        (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB &&
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB))
    {
        auto logSizeInKB = toBeValidated->MaxStreamSizeInMB * 1024;
        if(logSizeInKB < 16 * toBeValidated->MaxRecordSizeInKB)
        {
            messageWriter.WriteLine(
                "MaxStreamSizeInMB * 1024 must be larger or equal to MaxRecordSizeInKB * 16. Provided MaxStreamSizeInMB={0}, MaxRecordSizeInKB={1}",
                toBeValidated->MaxRecordSizeInKB,
                toBeValidated->MaxRecordSizeInKB);

            return ErrorCodeValue::InvalidArgument;
        }
    }

    // If the log is not optimized for low disk, it means it is NOT a sparse log.
    // Hence validate that the checkpoint threshold falls within the log size limit
    if ((toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE &&
        toBeValidated->optimizeLogForLowerDiskUsage_ == false) &&
        (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB &&
            toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB))
    {
        if(toBeValidated->MaxStreamSizeInMB < toBeValidated->CheckpointThresholdInMB)
        {
            messageWriter.WriteLine(
                "MaxStreamSizeInMB must be greater than CheckpointThresholdInMB when the log is not optimized for low disk usage. Provided MaxStreamSizeInMB={0}, CheckpointThresholdInMB={1}",
                toBeValidated->MaxRecordSizeInKB,
                toBeValidated->CheckpointThresholdInMB);

            return ErrorCodeValue::InvalidArgument;
        }
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode TransactionalReplicatorSettings::ValidateLogIOHealthReportSettings(
    __in Common::StringWriter & messageWriter,
    __in TransactionalReplicatorSettingsUPtr & toBeValidated, 
    __in std::shared_ptr<TRConfigValues> & defaultValues)
{
    if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_DURATION_SECONDS &&
        toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS)
    {
        if (toBeValidated->flags_ & FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD)
        {
            // SlowLogIOCountThreshold of 0 disables monitoring and health reports
            // Monitoring is disabled, do not fail validation
            if (toBeValidated->SlowLogIOCountThreshold == 0)
            {
                return ErrorCodeValue::Success;
            }
        }

        if (toBeValidated->SlowLogIODuration >= toBeValidated->SlowLogIOTimeThreshold)
        {
            messageWriter.WriteLine(
                "SlowLogIOTimeThreshold must be less than SlowLogIODuration - Resetting to default values. Provided SlowLogIODuration={0} SlowLogIOTimeThreshold={1}. Default values: SlowLogIODuration={2}, SlowLogIOCountThreshold={4}, SlowLogIOTimeThreshold={3}",
                toBeValidated->SlowLogIODuration,
                toBeValidated->SlowLogIOTimeThreshold,
                defaultValues->SlowLogIODuration,
                defaultValues->SlowLogIOCountThreshold,
                defaultValues->SlowLogIOTimeThreshold);

            // Invalid user settings are overridden with default values
            // Ensures validation does not fail on log IO related configuration
            toBeValidated->slowLogIODuration_ = defaultValues->SlowLogIODuration;
            toBeValidated->slowLogIOCountThreshold_ = defaultValues->SlowLogIOCountThreshold;
            toBeValidated->slowLogIOTimeThreshold_ = defaultValues->SlowLogIOTimeThreshold;

            return ErrorCodeValue::InvalidArgument;
        }
    }

    return ErrorCodeValue::Success;
}

Common::ErrorCode TransactionalReplicatorSettings::GetMinLogSizeInMB(
    __in Common::StringWriter & messageWriter,
    __in int64 checkpointThresholdInMB,
    __out int64 & minLogSizeInMB)
{
    if(minLogSizeInMB < 0)
    {
        messageWriter.WriteLine("MinLogSizeInMB: {0} must be greater than or equal to 0", minLogSizeInMB);
        return ErrorCodeValue::InvalidArgument;
    }

    if(checkpointThresholdInMB < 0)
    {
        messageWriter.WriteLine("CheckpointThresholdInMB: {0} must be greater than 0", checkpointThresholdInMB);
        return ErrorCodeValue::InvalidArgument;
    }

    if(minLogSizeInMB == 0)
    {
        int64 defaultSize = checkpointThresholdInMB / DefaultMinLogSizeInMBDivider;

        if (defaultSize < SmallestMinLogSizeInMB)
        {
            minLogSizeInMB = SmallestMinLogSizeInMB;
            return ErrorCodeValue::Success;
        }

        minLogSizeInMB = defaultSize;
        return ErrorCodeValue::Success;
    }

    return ErrorCodeValue::Success;
}

bool TransactionalReplicatorSettings::IsMultipleOf4(int64 value)
{
    return ((value % 4) == 0);
}

bool TransactionalReplicatorSettings::IsTRSettingsEx1FlagsUsed(DWORD64 const & flags)
{
    return flags & FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_INCREMENTAL_BACKUPS_ACROSS_REPLICAS;
}

void TransactionalReplicatorSettings::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(
        "[checkpointThresholdInMB={0} maxAccumulatedBackupLogSizeInMB={1} minLogSizeInMB={2}\
        truncationThresholdFactor={3} throttlingThresholdFactor={4} maxRecordSizeInKB={5} maxStreamSizeInMB={6}\
        maxWriteQueueDepthInKB={7} maxMetadataSizeInKB={8} slowLogIOCountThreshold={9} serializationVersion={10}\
        optimizeForLocalSSD={11} optimizeLogForLowerDiskUsage={12} enableSecondaryCommitApplyAcknowledgement={13}\
        enableIncrementalBackupsAcrossReplicas={14}]",
        checkpointThresholdInMB_,
        maxAccumulatedBackupLogSizeInMB_,
        minLogSizeInMB_,
        truncationThresholdFactor_,
        throttlingThresholdFactor_,
        maxRecordSizeInKB_,
        maxStreamSizeInMB_,
        maxWriteQueueDepthInKB_,
        maxMetadataSizeInKB_,
        slowLogIOCountThreshold_,
        serializationVersion_,
        optimizeForLocalSSD_,
        optimizeLogForLowerDiskUsage_,
        enableSecondaryCommitApplyAcknowledgement_,
        enableIncrementalBackupsAcrossReplicas_);

    w.Write(
        "[slowApiMonitoringDuration={0} slowLogIODuration={1} slowLogIOTimeThreshold={2} slowLogIOHealthReportTTL={3}, , periodicCheckpointTruncationInterval={4}]",
        slowApiMonitoringDuration_,
        slowLogIODuration_,
        slowLogIOTimeThreshold_,
        slowLogIOHealthReportTTL_,
        truncationInterval_);
}
