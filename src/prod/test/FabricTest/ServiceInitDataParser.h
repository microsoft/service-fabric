// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ServiceInitDataParser
    {
        DENY_COPY(ServiceInitDataParser);

    private:    
        std::vector<std::wstring> tokens_;

    public:
        ServiceInitDataParser(__in std::wstring const & initDataStr)        
        {
            tokens_ = GetKeyValuePairList(initDataStr);
        }

        template <typename T>
        bool GetValue(
            __in std::wstring const & keyName,
            __out T & value)
        {
            std::wstring stringValue;

            for (auto it = tokens_.begin(); it != tokens_.end(); it++)
            {
                bool success = GetValueForKey(*it, keyName, stringValue);

                if (success)
                {
                    ASSERT_IFNOT(
                        Common::Config::TryParse<T>(value, stringValue),
                        "ServiceInitDataParser - Failed to parse string {0} for value",
                        *it);

                    return true;
                }
            }

            return false;
        }

    private:
        static bool GetValueForKey(
            __in std::wstring const & pairString,
            __in std::wstring const & keyName,
            __out std::wstring & value);

        static std::vector<std::wstring> GetKeyValuePairList(__in std::wstring const & initDataString);
    };

    class TransactionalReplicatorSettingServiceInitDataParser
    {
        DENY_COPY(TransactionalReplicatorSettingServiceInitDataParser);

    public:
        TransactionalReplicatorSettingServiceInitDataParser(__in std::wstring const & initDataStr)
            : parser_(initDataStr)
            , random_()
        {
        }

        TxnReplicator::TransactionalReplicatorSettingsUPtr CreateTransactionalReplicatorSettings(std::wstring const & initData);

        // Overridable public settings
        static const std::wstring InitData_TR_CheckpointThresholdInMB;
        static const std::wstring InitData_TR_MaxAccumulatedBackupLogSizeInMB;
        static const std::wstring InitData_TR_SlowApiMonitoringDurationSeconds;
        static const std::wstring InitData_TR_MinLogSizeInMB;
        static const std::wstring InitData_TR_TruncationThresholdFactor;
        static const std::wstring InitData_TR_ThrottlingThresholdFactor;
        static const std::wstring InitData_TR_MaxRecordSizeInKB;
        static const std::wstring InitData_TR_MaxStreamSizeInMB;
        static const std::wstring InitData_TR_OptimizeForLocalSSD;
        static const std::wstring InitData_TR_MaxWriteQueueDepthInKB;
        static const std::wstring InitData_TR_MaxMetadataSizeInKB;
        static const std::wstring InitData_TR_EnableSecondaryCommitApplyAcknowledgement;
        static const std::wstring InitData_TR_OptimizeLogForLowerDiskUsage;
        static const std::wstring InitData_TR_SlowLogIODurationSeconds;
        static const std::wstring InitData_TR_SlowLogIOCountThreshold;
        static const std::wstring InitData_TR_SlowLogIOTimeThresholdSeconds;
        static const std::wstring InitData_TR_SlowLogIOHealthReportTTL;
        static const std::wstring InitData_TR_SerializationVersion;

    private:
        TxnReplicator::TransactionalReplicatorSettingsUPtr Parse();

        TxnReplicator::TransactionalReplicatorSettingsUPtr CreateRandomTransactionalReplicatorSettings();

        uint GetRandomMultipleOf4();

        ServiceInitDataParser parser_;
        Common::Random random_;
    };

    class ReplicatorSettingServiceInitDataParser
    {
        DENY_COPY(ReplicatorSettingServiceInitDataParser);

    public:
        ReplicatorSettingServiceInitDataParser(__in std::wstring const & initDataStr)
            : parser_(initDataStr)
        {            
        }

        Reliability::ReplicationComponent::ReplicatorSettingsUPtr CreateReplicatorSettings(
            std::wstring const & initData,
            std::wstring const & partitionId);

        // V1 replicator settings specifically for TransactionalReplicator
        // Ensure Queue Sizes are not small
        // Ensure EOS ACk is not enabled
        // Ensure max message size is larger than copy batch size
        // more validations done here
        Reliability::ReplicationComponent::ReplicatorSettingsUPtr CreateReplicatorSettingsForTransactionalReplicator(
            std::wstring const & initData,
            std::wstring const & partitionId);

        static const std::wstring InitData_RE_RequireServiceAck;
        static const std::wstring InitData_RE_UseStreamFaults;
        static const std::wstring InitData_RE_SecondaryClearAcknowledgedOperations;

        static const std::wstring InitData_RE_MaxReplicationMessageSize;
        static const std::wstring InitData_RE_MaxPrimaryReplicationQueueMemorySize;
        static const std::wstring InitData_RE_MaxPrimaryReplicationQueueSize;
        static const std::wstring InitData_RE_InitialPrimaryReplicationQueueSize;
        static const std::wstring InitData_RE_MaxSecondaryReplicationQueueMemorySize;
        static const std::wstring InitData_RE_MaxSecondaryReplicationQueueSize;
        static const std::wstring InitData_RE_InitialSecondaryReplicationQueueSize;
        static const std::wstring InitData_RE_PrimaryWaitForPendingQuorumsTimeoutMilliseconds;
        static const std::wstring InitData_RE_BatchAckIntervalMilliseconds;
        static const std::wstring InitData_RE_ReplicatorAddress;
        static const std::wstring InitData_RE_ReplicatorPort;

    private:
        Reliability::ReplicationComponent::ReplicatorSettingsUPtr Parse();

        Reliability::ReplicationComponent::ReplicatorSettingsUPtr CreateRandomReplicatorSettings(
            std::wstring const & partitionId,
            bool canEnableEOSConfigForRandom,
            int minimumQueueMemorySize);

        int GetRandomMemoryQueueSize(int minimum) const;

        ServiceInitDataParser parser_;
        mutable Common::Random random_;
    };
};
