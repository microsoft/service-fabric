// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace FabricTest;
using namespace Common;
using namespace Reliability::ReplicationComponent;
using namespace TxnReplicator;

const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_RequireServiceAck = L"RE_RequireServiceAck";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_UseStreamFaults = L"RE_UseStreamFaults";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_SecondaryClearAcknowledgedOperations = L"RE_SecondaryClearAcknowledgedOperations";

const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_MaxReplicationMessageSize = L"RE_MaxReplicationMessageSize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_MaxPrimaryReplicationQueueMemorySize = L"RE_MaxPrimaryReplicationQueueMemorySize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_MaxPrimaryReplicationQueueSize = L"RE_MaxPrimaryReplicationQueueSize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_InitialPrimaryReplicationQueueSize = L"RE_InitialPrimaryReplicationQueueSize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_MaxSecondaryReplicationQueueMemorySize = L"RE_MaxSecondaryReplicationQueueMemorySize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_MaxSecondaryReplicationQueueSize = L"RE_MaxSecondaryReplicationQueueSize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_InitialSecondaryReplicationQueueSize = L"RE_InitialSecondaryReplicationQueueSize";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_PrimaryWaitForPendingQuorumsTimeoutMilliseconds = L"RE_PrimaryWaitForPendingQuorumsTimeoutMilliseconds";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_BatchAckIntervalMilliseconds = L"RE_BatchAckIntervalMilliseconds";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_ReplicatorAddress = L"RE_ReplicatorAddress";
const std::wstring ReplicatorSettingServiceInitDataParser::InitData_RE_ReplicatorPort = L"RE_ReplicatorPort";

const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_CheckpointThresholdInMB = L"TR_CheckpointThresholdInMB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MaxAccumulatedBackupLogSizeInMB = L"TR_MaxAccumulatedBackupLogSizeInMB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SlowApiMonitoringDurationSeconds = L"TR_SlowApiMonitoringDurationSeconds";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MinLogSizeInMB = L"TR_MinLogSizeInMB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_TruncationThresholdFactor = L"TR_TruncationThresholdFactor";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_ThrottlingThresholdFactor = L"TR_ThrottlingThresholdFactor";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MaxRecordSizeInKB = L"TR_MaxRecordSizeInKB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MaxStreamSizeInMB = L"TR_MaxStreamSizeInMB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_OptimizeForLocalSSD = L"TR_OptimizeForLocalSSD";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MaxWriteQueueDepthInKB = L"TR_MaxWriteQueueDepthInKB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_MaxMetadataSizeInKB = L"TR_MaxMetadataSizeInKB";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_EnableSecondaryCommitApplyAcknowledgement = L"TR_EnableSecondaryCommitApplyAcknowledgement";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_OptimizeLogForLowerDiskUsage = L"TR_OptimizeLogForLowerDiskUsage";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SlowLogIODurationSeconds = L"TR_SlowLogIODurationSeconds";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SlowLogIOCountThreshold = L"TR_SlowLogIOCountThreshold";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SlowLogIOTimeThresholdSeconds = L"TR_SlowLogIOTimeThresholdSeconds";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SlowLogIOHealthReportTTL = L"TR_SlowLogIOHealthReportTTL";
const std::wstring TransactionalReplicatorSettingServiceInitDataParser::InitData_TR_SerializationVersion = L"TR_SerializationVersion";

bool ServiceInitDataParser::GetValueForKey(
    __in std::wstring const & pairString,
    __in std::wstring const & keyName,
    __out std::wstring & value)
{
    std::vector<std::wstring> tokens;
    Common::StringUtility::Split<std::wstring>(pairString, tokens, L":");
            
    ASSERT_IFNOT(
        tokens.size() == 2,
        "ServiceInitDataParser - Invalid format for init data value. Key Value Pairs expected with each pair in {Key{0}Value} format.", L":");
            
    if (Common::StringUtility::AreEqualCaseInsensitive(keyName, tokens[0]))
    {
        value = std::move(tokens[1]);
        return true;
    }

    return false;
}

std::vector<std::wstring> ServiceInitDataParser::GetKeyValuePairList(__in std::wstring const & initDataString)
{
    std::vector<std::wstring> tokens;
    Common::StringUtility::Split<std::wstring>(initDataString, tokens, L";");

    return tokens;
}

ReplicatorSettingsUPtr ReplicatorSettingServiceInitDataParser::CreateReplicatorSettings(
    std::wstring const & initData,
    std::wstring const & partitionId)
{
    // If RandomReplicatorSettings is enabled, this service is running as part of unreliable random
    // For some services, the init data may be non-null to test specific scenarios even in unreliable random.
    if (FabricTestCommonConfig::GetConfig().UseRandomReplicatorSettings && initData.empty())
    {
        return CreateRandomReplicatorSettings(
            partitionId, 
            true, // Can use EOS
            0); 
    }
    else
    {
        return Parse();
    }
}

ReplicatorSettingsUPtr ReplicatorSettingServiceInitDataParser::CreateReplicatorSettingsForTransactionalReplicator(
    std::wstring const & initData,
    std::wstring const & partitionId)
{
    // If RandomReplicatorSettings is enabled, this service is running as part of unreliable random
    // For some services, the init data may be non-null to test specific scenarios even in unreliable random.
    if (FabricTestCommonConfig::GetConfig().UseRandomReplicatorSettings && initData.empty())
    {
        return CreateRandomReplicatorSettings(
            partitionId,
            // Do not use EOS ACK Feature
            false,
            // Do not create queue with less memory size for v2 replicator as the avg size of replication op is ~8KB (min is 1 and max is 16KB) during random runs
            FabricTestCommonConfig::GetConfig().MinReplicationQueueMemoryForV2ReplicatorInKB * 1024);
    }
    else
    {
        return Parse();
    }
}

ReplicatorSettingsUPtr ReplicatorSettingServiceInitDataParser::Parse()
{
    FABRIC_REPLICATOR_SETTINGS replicatorSettings = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX1 replicatorSettingsEx1 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX2 replicatorSettingsEx2 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX3 replicatorSettingsEx3 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX4 replicatorSettingsEx4 = { 0 };
    replicatorSettings.Flags = FABRIC_REPLICATOR_SETTINGS_NONE; // no memory limit, default number of items limit = 1024.
    replicatorSettings.Reserved = &replicatorSettingsEx1;
    replicatorSettingsEx1.Reserved = &replicatorSettingsEx2;
    replicatorSettingsEx2.Reserved = &replicatorSettingsEx3;
    replicatorSettingsEx3.Reserved = &replicatorSettingsEx4;
    replicatorSettingsEx4.Reserved = NULL;

    bool tempbool = false;
    uint tempuint = 0;

    if (parser_.GetValue<bool>(InitData_RE_RequireServiceAck, tempbool))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
        replicatorSettings.RequireServiceAck = tempbool ? true : false;
        tempbool = false;
    }

    if (parser_.GetValue<bool>(InitData_RE_UseStreamFaults, tempbool))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
        replicatorSettingsEx2.UseStreamFaultsAndEndOfStreamOperationAck = tempbool ? true : false;
        tempbool = false;
    }

    if (parser_.GetValue<bool>(InitData_RE_SecondaryClearAcknowledgedOperations, tempbool))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
        replicatorSettingsEx1.SecondaryClearAcknowledgedOperations = tempbool ? true : false;
        tempbool = false;
    }

    if (parser_.GetValue<uint>(InitData_RE_MaxReplicationMessageSize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
        replicatorSettingsEx1.MaxReplicationMessageSize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_MaxPrimaryReplicationQueueMemorySize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
        replicatorSettingsEx3.MaxPrimaryReplicationQueueMemorySize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_InitialPrimaryReplicationQueueSize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE;
        replicatorSettingsEx3.InitialPrimaryReplicationQueueSize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_MaxPrimaryReplicationQueueSize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
        replicatorSettingsEx3.MaxPrimaryReplicationQueueSize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_MaxSecondaryReplicationQueueMemorySize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
        replicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_MaxSecondaryReplicationQueueSize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
        replicatorSettingsEx3.MaxSecondaryReplicationQueueSize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_InitialSecondaryReplicationQueueSize, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE;
        replicatorSettingsEx3.InitialSecondaryReplicationQueueSize = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_PrimaryWaitForPendingQuorumsTimeoutMilliseconds, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;
        replicatorSettingsEx3.PrimaryWaitForPendingQuorumsTimeoutMilliseconds = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_RE_BatchAckIntervalMilliseconds, tempuint))
    {
        replicatorSettings.Flags |= FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL;
        replicatorSettings.BatchAcknowledgementIntervalMilliseconds = tempuint;
        tempuint = 0;
    }

    std::wstring tempReplicatorAddress = L"";
    if (parser_.GetValue<std::wstring>(InitData_RE_ReplicatorAddress, tempReplicatorAddress)) {
        if (parser_.GetValue<uint>(InitData_RE_ReplicatorPort, tempuint)) {
            tempReplicatorAddress = wformatString(L"{0}:{1}", tempReplicatorAddress, std::to_wstring(tempuint));

            replicatorSettings.Flags |= FABRIC_REPLICATOR_ADDRESS;
            replicatorSettings.ReplicatorAddress = tempReplicatorAddress.c_str();
            tempuint = 0;
        }
        else
        {
            ASSERT_IFNOT(
                false,
                "RE_ReplicatorPort must be specified as well when RE_ReplicatorAddress configuration is passed.");
        }
    }

    // Parse new settings if needed here
    ReplicatorSettingsUPtr rv;
    auto error = ReplicatorSettings::FromPublicApi(replicatorSettings, rv);
    ASSERT_IFNOT(error.IsSuccess(), "Invalid Replicator Settings");
    return rv;
}

ReplicatorSettingsUPtr ReplicatorSettingServiceInitDataParser::CreateRandomReplicatorSettings(
    std::wstring const & partitionId,
    bool canEnableEOSConfigForRandom,
    int minimumQueueMemorySize)
{
    const int PrimaryReplicationQueueSizeInRandomMode = 2048;
    const int SecondaryReplicationQueueSizeInRandomMode = 4096;

    FABRIC_REPLICATOR_SETTINGS replicatorSettings = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX1 replicatorSettingsEx1 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX2 replicatorSettingsEx2 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX3 replicatorSettingsEx3 = { 0 };
    FABRIC_REPLICATOR_SETTINGS_EX4 replicatorSettingsEx4 = { 0 };
    replicatorSettings.Flags = FABRIC_REPLICATOR_SETTINGS_NONE; // no memory limit, default number of items limit = 1024.
    replicatorSettings.Reserved = &replicatorSettingsEx1;
    replicatorSettingsEx1.Reserved = &replicatorSettingsEx2;
    replicatorSettingsEx2.Reserved = &replicatorSettingsEx3;
    replicatorSettingsEx3.Reserved = &replicatorSettingsEx4;
    replicatorSettingsEx4.Reserved = NULL;

    std::hash<wstring> hashFunc;
    int random = hashFunc(partitionId) % 4;
    if (random > 0)
    {
        // In case only memory limit is set, the number of items limit will be ignored
        replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
        replicatorSettingsEx3.MaxPrimaryReplicationQueueMemorySize = GetRandomMemoryQueueSize(minimumQueueMemorySize);

        if (canEnableEOSConfigForRandom)
        {
            replicatorSettings.Flags |= FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK;
            replicatorSettingsEx2.UseStreamFaultsAndEndOfStreamOperationAck = true;
            // When end of stream ack's are enabled, we must enable RequireServiceAck
            replicatorSettings.Flags |= FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK;
            replicatorSettings.RequireServiceAck = true;
        }

        if (random > 1)
        {
            // Limit on both memory size and number of items
            replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE;
            replicatorSettingsEx3.MaxPrimaryReplicationQueueSize = PrimaryReplicationQueueSizeInRandomMode;

            // Let Operations on the secondary get cleared on acknowledgement
            replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS;
            replicatorSettingsEx1.SecondaryClearAcknowledgedOperations = true;
        }

        if (random > 2)
        {
            // Limit on both memory size and number of items
            replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE;
            replicatorSettingsEx3.MaxSecondaryReplicationQueueSize = SecondaryReplicationQueueSizeInRandomMode;

            // Let Operations on the secondary get cleared on acknowledgement
            replicatorSettings.Flags |= FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE;
            replicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize = GetRandomMemoryQueueSize(minimumQueueMemorySize);

            // Let Primary wait for 1 second before closing, to receive any pending quorum acks
            replicatorSettings.Flags |= FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT;
            replicatorSettingsEx3.PrimaryWaitForPendingQuorumsTimeoutMilliseconds = 1000;
        }
    }

    replicatorSettings.Flags |= FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE;
    replicatorSettingsEx1.MaxReplicationMessageSize = replicatorSettingsEx3.MaxPrimaryReplicationQueueMemorySize;

    if (replicatorSettingsEx1.MaxReplicationMessageSize == 0 ||
        (replicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize != 0 &&
         replicatorSettingsEx1.MaxReplicationMessageSize > replicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize))
    {
        // Use smaller value
        replicatorSettingsEx1.MaxReplicationMessageSize = replicatorSettingsEx3.MaxSecondaryReplicationQueueMemorySize;
    }

    if (replicatorSettingsEx1.MaxReplicationMessageSize == 0)
    {
        replicatorSettingsEx1.MaxReplicationMessageSize = 50 * 1024 * 1024; // Use default
    }

    Reliability::ReplicationComponent::ReplicatorSettingsUPtr rv;
    auto error = Reliability::ReplicationComponent::ReplicatorSettings::FromPublicApi(replicatorSettings, rv);
    ASSERT_IFNOT(error.IsSuccess(), "Invalid Replicator Settings");

    return rv;
}

int ReplicatorSettingServiceInitDataParser::GetRandomMemoryQueueSize(int minimum) const
{
    int minimumQueueSize = 2 * 1024 * 1024;
    int QueueSizeMultiplier = 50 * 1024;
    if (minimum > minimumQueueSize)
    {
        minimumQueueSize = minimum;
    }

    return minimumQueueSize + (QueueSizeMultiplier * random_.Next(10, 20));
}

TransactionalReplicatorSettingsUPtr TransactionalReplicatorSettingServiceInitDataParser::CreateTransactionalReplicatorSettings(std::wstring const & initData)
{
    if(FabricTestCommonConfig::GetConfig().UseRandomReplicatorSettings && initData.empty())
    {
        return CreateRandomTransactionalReplicatorSettings();
    }
    else
    {
        return Parse();
    }
}

TransactionalReplicatorSettingsUPtr TransactionalReplicatorSettingServiceInitDataParser::Parse()
{
    TRANSACTIONAL_REPLICATOR_SETTINGS txReplicatorSettings = { 0 };

    // Load settings from initDataStr
    uint tempuint = 0;
    bool tempbool = false;

    if (parser_.GetValue<uint>(InitData_TR_CheckpointThresholdInMB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
        txReplicatorSettings.CheckpointThresholdInMB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_MaxAccumulatedBackupLogSizeInMB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB;
        txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_SlowApiMonitoringDurationSeconds, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;
        txReplicatorSettings.SlowApiMonitoringDurationSeconds = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_MinLogSizeInMB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
        txReplicatorSettings.MinLogSizeInMB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_TruncationThresholdFactor, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
        txReplicatorSettings.TruncationThresholdFactor = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_ThrottlingThresholdFactor, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;
        txReplicatorSettings.ThrottlingThresholdFactor = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_MaxRecordSizeInKB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB;
        txReplicatorSettings.MaxRecordSizeInKB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_MaxStreamSizeInMB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;
        txReplicatorSettings.MaxStreamSizeInMB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<bool>(InitData_TR_OptimizeForLocalSSD, tempbool))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_FOR_LOCAL_SSD;
        txReplicatorSettings.OptimizeForLocalSSD = tempbool;
        tempbool = false;
    }

    if (parser_.GetValue<uint>(InitData_TR_MaxWriteQueueDepthInKB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_WRITE_QUEUE_DEPTH_KB;
        txReplicatorSettings.MaxWriteQueueDepthInKB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_MaxMetadataSizeInKB, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_METADATA_SIZE_KB;
        txReplicatorSettings.MaxMetadataSizeInKB = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<bool>(InitData_TR_EnableSecondaryCommitApplyAcknowledgement, tempbool))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_ENABLE_SECONDARY_COMMIT_APPLY_ACKNOWLEDGEMENT;
        txReplicatorSettings.EnableSecondaryCommitApplyAcknowledgement = tempbool;
        tempbool = false;
    }

    if (parser_.GetValue<bool>(InitData_TR_OptimizeLogForLowerDiskUsage, tempbool))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE;
        txReplicatorSettings.OptimizeLogForLowerDiskUsage = tempbool;
        tempbool = false;
    }

    if (parser_.GetValue<uint>(InitData_TR_SlowLogIODurationSeconds, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;
        txReplicatorSettings.SlowLogIODurationSeconds = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_SlowLogIOCountThreshold, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_COUNT_THRESHOLD;
        txReplicatorSettings.SlowLogIOCountThreshold = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_SlowLogIOTimeThresholdSeconds, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_TIME_THRESHOLD_SECONDS;
        txReplicatorSettings.SlowLogIOTimeThresholdSeconds = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_SlowLogIOHealthReportTTL, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_LOG_IO_HEALTH_REPORT_TTL_SECONDS;
        txReplicatorSettings.SlowLogIOHealthReportTTLSeconds = tempuint;
        tempuint = 0;
    }

    if (parser_.GetValue<uint>(InitData_TR_SerializationVersion, tempuint))
    {
        txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SERIALIZATION_VERSION;
        txReplicatorSettings.SerializationVersion = tempuint;
        tempuint = 0;
    }

    TransactionalReplicatorSettingsUPtr trv;
    auto error = TransactionalReplicatorSettings::FromPublicApi(txReplicatorSettings, trv);
    ASSERT_IFNOT(error.IsSuccess(), "Invalid TransactionalReplicatorSettings");
    return trv;
}

TransactionalReplicatorSettingsUPtr TransactionalReplicatorSettingServiceInitDataParser::CreateRandomTransactionalReplicatorSettings()
{
    TRANSACTIONAL_REPLICATOR_SETTINGS txReplicatorSettings = { 0 };

    // Setting the record size to a random number leads to a bugcheck in the kernel
    // Until AlanWar gets to fix the bug # 9493822 use default settings rather than random settings

    // TODO: AlanWar - Please delete the following lines and uncomment the below block which is commented out now once the bug is fixed in the kernel
    uint truncationThresholdFactor = 2;
    uint throttlingThresholdFactor = 4;
    uint minLogSize = 2;
    uint maxStreamSizeInMB = 50;
    uint checkpointThresholdInMB = 1;
    uint maxAccumulatedBackupSize = 15;
    // Generate slow api duration between 60 and 180 seconds
    uint slowApiDuration = random_.Next(60, 180);
    // Generate maxRecordSizeInKB
    uint maxRecordSizeInKB = GetRandomMultipleOf4();
    maxRecordSizeInKB = 256;

    // Configure log truncation for 1 minute
    uint truncationIntervalSeconds = 60;

    /*
    // Generate checkpoint threshold between 1 and 10
    uint maxCheckpointThreshold = 10;
    uint minCheckpointThreshold = 1;

    uint truncationThresholdFactor = random_.Next(2, 5);
    uint throttlingThresholdFactor = random_.Next(truncationThresholdFactor + 1, truncationThresholdFactor + 10);

    // Generate min log size
    uint minLogSize = std::max(1, (int)maxCheckpointThreshold * 10);

    // Generate random stream size in MB
    uint maxStreamSizeInMB = random_.Next(minLogSize, maxCheckpointThreshold * 10);

    // Ensure that minLogSize * truncationThreshold is less than max stream size
    while (minLogSize * truncationThresholdFactor >= maxStreamSizeInMB)
    {
        maxStreamSizeInMB++;
    }

    uint checkpointThresholdInMB = random_.Next(minCheckpointThreshold, maxCheckpointThreshold);

    uint maxAccumulatedBackupSize = random_.Next(1, maxStreamSizeInMB - 1);

    // Generate slow api duration between 60 and 180 seconds
    uint slowApiDuration = random_.Next(60, 180);

    // Generate maxRecordSizeInKB
    uint maxRecordSizeInKB = GetRandomMultipleOf4();

    // maxrecordsizein * 16 must be < maxstreamsize
    // limit the record size between 512KB and 6MB
    while (
        (maxRecordSizeInKB * 16) >= (maxStreamSizeInMB * 1024)
        || maxRecordSizeInKB < 512
        || maxRecordSizeInKB >(6 * 1024))
    {
        maxRecordSizeInKB = GetRandomMultipleOf4();
    }
    */

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
    txReplicatorSettings.CheckpointThresholdInMB = checkpointThresholdInMB;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_ACCUMULATED_BACKUP_LOG_SIZE_MB;
    txReplicatorSettings.MaxAccumulatedBackupLogSizeInMB = maxAccumulatedBackupSize;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_SLOW_API_MONITORING_DURATION_SECONDS;
    txReplicatorSettings.SlowApiMonitoringDurationSeconds = slowApiDuration;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
    txReplicatorSettings.MinLogSizeInMB = minLogSize;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
    txReplicatorSettings.TruncationThresholdFactor = truncationThresholdFactor;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;
    txReplicatorSettings.ThrottlingThresholdFactor = throttlingThresholdFactor;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_RECORD_SIZE_KB;
    txReplicatorSettings.MaxRecordSizeInKB = maxRecordSizeInKB;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB;
    txReplicatorSettings.MaxStreamSizeInMB = maxStreamSizeInMB;

    txReplicatorSettings.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS;
    txReplicatorSettings.LogTruncationIntervalSeconds = truncationIntervalSeconds;

    TransactionalReplicatorSettingsUPtr trv;
    auto error = TransactionalReplicatorSettings::FromPublicApi(txReplicatorSettings, trv);
    ASSERT_IFNOT(error.IsSuccess(), "Invalid TransactionalReplicatorSettings");
    return trv;
}

uint TransactionalReplicatorSettingServiceInitDataParser::GetRandomMultipleOf4()
{
    uint t = random_.Next();
    
    while((t % 4) != 0)
    {
        t = random_.Next();
    }

    return t;
}
