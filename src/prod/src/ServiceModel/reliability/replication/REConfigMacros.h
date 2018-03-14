// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
#define RE_GLOBAL_STATIC_SETTINGS_COUNT 0
#define RE_GLOBAL_DYNAMIC_SETTINGS_COUNT 20

#define RE_GLOBAL_SETTINGS_COUNT RE_GLOBAL_STATIC_SETTINGS_COUNT + RE_GLOBAL_DYNAMIC_SETTINGS_COUNT

#define RE_OVERRIDABLE_SETTINGS_COUNT 21

#define RE_SETTINGS_COUNT (RE_GLOBAL_SETTINGS_COUNT + RE_OVERRIDABLE_SETTINGS_COUNT)

// This macro defines all the settings in the replicator config that are GLOBAL
#define DECLARE_RE_GLOBAL_SETTINGS_PROPERTIES() \
            __declspec(property(get=get_MaxPendingAcknowledgements)) int64 MaxPendingAcknowledgements ; \
            int64 get_MaxPendingAcknowledgements() const; \
            __declspec(property(get=get_EnableReplicationOperationHeaderInBody)) bool EnableReplicationOperationHeaderInBody ; \
            bool get_EnableReplicationOperationHeaderInBody() const; \
            __declspec(property(get=get_TraceInterval)) Common::TimeSpan TraceInterval ; \
            Common::TimeSpan get_TraceInterval() const ;\
            __declspec(property(get=get_QueueFullTraceInterval)) Common::TimeSpan QueueFullTraceInterval ; \
            Common::TimeSpan get_QueueFullTraceInterval() const ;\
            __declspec(property(get=get_QueueHealthMonitoringInterval)) Common::TimeSpan QueueHealthMonitoringInterval ; \
            Common::TimeSpan get_QueueHealthMonitoringInterval() const ;\
            __declspec(property(get=get_SlowApiMonitoringInterval)) Common::TimeSpan SlowApiMonitoringInterval ; \
            Common::TimeSpan get_SlowApiMonitoringInterval() const ;\
            __declspec(property(get=get_QueueHealthWarningAtUsagePercent)) int64 QueueHealthWarningAtUsagePercent ; \
            int64 get_QueueHealthWarningAtUsagePercent() const; \
            __declspec(property(get=get_CompleteReplicateThreadCount)) int64 CompleteReplicateThreadCount ; \
            int64 get_CompleteReplicateThreadCount() const; \
            __declspec(property(get=get_AllowMultipleQuorumSet)) bool AllowMultipleQuorumSet; \
            bool get_AllowMultipleQuorumSet() const; \
            __declspec(property(get=get_EnableSlowIdleRestartForVolatile)) bool EnableSlowIdleRestartForVolatile; \
            bool get_EnableSlowIdleRestartForVolatile() const; \
            __declspec(property(get=get_EnableSlowIdleRestartForPersisted)) bool EnableSlowIdleRestartForPersisted; \
            bool get_EnableSlowIdleRestartForPersisted() const; \
            __declspec(property(get=get_SlowIdleRestartAtQueueUsagePercent)) int64 SlowIdleRestartAtQueueUsagePercent ; \
            int64 get_SlowIdleRestartAtQueueUsagePercent() const; \
            __declspec(property(get=get_EnableSlowActiveSecondaryRestartForVolatile)) bool EnableSlowActiveSecondaryRestartForVolatile; \
            bool get_EnableSlowActiveSecondaryRestartForVolatile() const; \
            __declspec(property(get=get_EnableSlowActiveSecondaryRestartForPersisted)) bool EnableSlowActiveSecondaryRestartForPersisted; \
            bool get_EnableSlowActiveSecondaryRestartForPersisted() const; \
            __declspec(property(get=get_SlowActiveSecondaryRestartAtQueueUsagePercent)) int64 SlowActiveSecondaryRestartAtQueueUsagePercent ; \
            int64 get_SlowActiveSecondaryRestartAtQueueUsagePercent() const; \
            __declspec(property(get=get_ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness)) int64 ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness ; \
            int64 get_ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness() const; \
            __declspec(property(get=get_SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation)) Common::TimeSpan SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation ; \
            Common::TimeSpan get_SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation() const ;\
            __declspec(property(get=get_SecondaryProgressRateDecayFactor)) double SecondaryProgressRateDecayFactor ; \
            double get_SecondaryProgressRateDecayFactor() const; \
            __declspec(property(get=get_IdleReplicaMaxLagDurationBeforePromotion)) Common::TimeSpan IdleReplicaMaxLagDurationBeforePromotion ; \
            Common::TimeSpan get_IdleReplicaMaxLagDurationBeforePromotion() const ;\
            __declspec(property(get=get_SecondaryReplicatorBatchTracingArraySize)) int64 SecondaryReplicatorBatchTracingArraySize ; \
            int64 get_SecondaryReplicatorBatchTracingArraySize() const; \

// This macro defines all the settings in the replicator config that are overridable by the user using the CreateReplicator() API
#define DECLARE_RE_OVERRIDABLE_SETTINGS_PROPERTIES() \
            __declspec(property(get=get_RequireServiceAck)) bool RequireServiceAck; \
            bool get_RequireServiceAck() const; \
            __declspec(property(get=get_SecondaryClearAcknowledgedOperations)) bool SecondaryClearAcknowledgedOperations; \
            bool get_SecondaryClearAcknowledgedOperations() const; \
            __declspec(property(get=get_UseStreamFaultsAndEndOfStreamOperationAck,put=set_UseStreamFaultsAndEndOfStreamOperationAck)) bool UseStreamFaultsAndEndOfStreamOperationAck; \
            bool get_UseStreamFaultsAndEndOfStreamOperationAck() const; \
            void set_UseStreamFaultsAndEndOfStreamOperationAck(bool value); \
            __declspec(property(get=get_RetryInterval)) Common::TimeSpan RetryInterval ; \
            Common::TimeSpan get_RetryInterval() const ;\
            __declspec(property(get=get_BatchAckInterval)) Common::TimeSpan BatchAcknowledgementInterval; \
            Common::TimeSpan get_BatchAckInterval() const; \
            __declspec(property(get=get_PrimaryWaitForPendingQuorumsTimeout)) Common::TimeSpan PrimaryWaitForPendingQuorumsTimeout; \
            Common::TimeSpan get_PrimaryWaitForPendingQuorumsTimeout() const; \
            __declspec(property(get=get_InitialReplicationQueueSize)) int64 InitialReplicationQueueSize ; \
            int64 get_InitialReplicationQueueSize() const; \
            __declspec(property(get=get_MaxReplicationQueueSize)) int64 MaxReplicationQueueSize ; \
            int64 get_MaxReplicationQueueSize() const; \
            __declspec(property(get=get_InitialCopyQueueSize)) int64 InitialCopyQueueSize ; \
            int64 get_InitialCopyQueueSize() const; \
            __declspec(property(get=get_MaxCopyQueueSize)) int64 MaxCopyQueueSize ; \
            int64 get_MaxCopyQueueSize() const; \
            __declspec(property(get=get_MaxReplicationQueueMemorySize)) int64 MaxReplicationQueueMemorySize ; \
            int64 get_MaxReplicationQueueMemorySize() const; \
            __declspec(property(get=get_MaxReplicationMessageSize)) int64 MaxReplicationMessageSize ; \
            int64 get_MaxReplicationMessageSize() const; \
            __declspec(property(get=get_InitialPrimaryReplicationQueueSize)) int64 InitialPrimaryReplicationQueueSize ; \
            int64 get_InitialPrimaryReplicationQueueSize() const; \
            __declspec(property(get=get_MaxPrimaryReplicationQueueSize)) int64 MaxPrimaryReplicationQueueSize ; \
            int64 get_MaxPrimaryReplicationQueueSize() const; \
            __declspec(property(get=get_MaxPrimaryReplicationQueueMemorySize)) int64 MaxPrimaryReplicationQueueMemorySize ; \
            int64 get_MaxPrimaryReplicationQueueMemorySize() const; \
            __declspec(property(get=get_InitialSecondaryReplicationQueueSize)) int64 InitialSecondaryReplicationQueueSize ; \
            int64 get_InitialSecondaryReplicationQueueSize() const; \
            __declspec(property(get=get_MaxSecondaryReplicationQueueSize)) int64 MaxSecondaryReplicationQueueSize ; \
            int64 get_MaxSecondaryReplicationQueueSize() const; \
            __declspec(property(get=get_MaxSecondaryReplicationQueueMemorySize)) int64 MaxSecondaryReplicationQueueMemorySize ; \
            int64 get_MaxSecondaryReplicationQueueMemorySize() const; \
            __declspec(property(get=get_ReplicatorAddress)) std::wstring ReplicatorAddress ; \
            std::wstring get_ReplicatorAddress() const; \
            __declspec(property(get=get_ReplicatorListenAddress)) std::wstring ReplicatorListenAddress ; \
            std::wstring get_ReplicatorListenAddress() const; \
            __declspec(property(get=get_ReplicatorPublishAddress)) std::wstring ReplicatorPublishAddress ; \
            std::wstring get_ReplicatorPublishAddress() const; \

#define DEFINE_GETCONFIG_METHOD() \
            void GetReplicatorSettingsStructValues(Reliability::ReplicationComponent::REConfigValues & config) const \
                    { \
                config.InitialReplicationQueueSize = static_cast<DWORD>(this->InitialReplicationQueueSize); \
                config.MaxReplicationQueueSize = static_cast<DWORD>(this->MaxReplicationQueueSize); \
                config.MaxReplicationQueueMemorySize = static_cast<DWORD>(this->MaxReplicationQueueMemorySize); \
                config.InitialPrimaryReplicationQueueSize = static_cast<DWORD>(this->InitialPrimaryReplicationQueueSize); \
                config.MaxPrimaryReplicationQueueSize = static_cast<DWORD>(this->MaxPrimaryReplicationQueueSize); \
                config.MaxPrimaryReplicationQueueMemorySize = static_cast<DWORD>(this->MaxPrimaryReplicationQueueMemorySize); \
                config.InitialSecondaryReplicationQueueSize = static_cast<DWORD>(this->InitialSecondaryReplicationQueueSize); \
                config.MaxSecondaryReplicationQueueSize = static_cast<DWORD>(this->MaxSecondaryReplicationQueueSize); \
                config.MaxSecondaryReplicationQueueMemorySize = static_cast<DWORD>(this->MaxSecondaryReplicationQueueMemorySize); \
                config.InitialCopyQueueSize = static_cast<DWORD>(this->InitialCopyQueueSize); \
                config.MaxCopyQueueSize = static_cast<DWORD>(this->MaxCopyQueueSize); \
                config.BatchAcknowledgementInterval = this->BatchAcknowledgementInterval; \
                config.MaxReplicationMessageSize = static_cast<DWORD>(this->MaxReplicationMessageSize); \
                config.RequireServiceAck = this->RequireServiceAck; \
                config.ReplicatorAddress = this->ReplicatorAddress; \
                config.ReplicatorListenAddress = this->ReplicatorListenAddress; \
                config.ReplicatorPublishAddress = this->ReplicatorPublishAddress; \
                config.SecondaryClearAcknowledgedOperations = this->SecondaryClearAcknowledgedOperations; \
                config.UseStreamFaultsAndEndOfStreamOperationAck = this->UseStreamFaultsAndEndOfStreamOperationAck; \
                config.RetryInterval = this->RetryInterval; \
                config.PrimaryWaitForPendingQuorumsTimeout = this->PrimaryWaitForPendingQuorumsTimeout; \
            } \

#define RE_CONFIG_PROPERTIES(section_name, batch_acknowledgement_interval, max_primary_replication_queue_size, max_primary_replication_queue_memory_size, max_secondary_replication_queue_size, max_secondary_replication_queue_memory_size) \
            PUBLIC_CONFIG_ENTRY(uint, section_name, InitialReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxReplicationQueueSize, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxReplicationQueueMemorySize, 0, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, InitialCopyQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxCopyQueueSize, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, BatchAcknowledgementInterval, Common::TimeSpan::FromMilliseconds(batch_acknowledgement_interval), Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxPendingAcknowledgements, 32, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableReplicationOperationHeaderInBody, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxReplicationMessageSize, 52428800, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(bool, section_name, RequireServiceAck, false, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(std::wstring, section_name, ReplicatorAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(std::wstring, section_name, ReplicatorListenAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(std::wstring, section_name, ReplicatorPublishAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(bool, section_name, SecondaryClearAcknowledgedOperations, false, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(bool, section_name, UseStreamFaultsAndEndOfStreamOperationAck, false, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, InitialPrimaryReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxPrimaryReplicationQueueSize, max_primary_replication_queue_size, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxPrimaryReplicationQueueMemorySize, max_primary_replication_queue_memory_size, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, InitialSecondaryReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxSecondaryReplicationQueueSize, max_secondary_replication_queue_size, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, MaxSecondaryReplicationQueueMemorySize, max_secondary_replication_queue_memory_size, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, PrimaryWaitForPendingQuorumsTimeout, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, QueueHealthMonitoringInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(uint, section_name, QueueHealthWarningAtUsagePercent, 80, Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowApiMonitoringInterval, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static); \
            PUBLIC_CONFIG_ENTRY(Common::TimeSpan, section_name, RetryInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static); \
            \
            INTERNAL_CONFIG_ENTRY(uint, section_name, CompleteReplicateThreadCount, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, AllowMultipleQuorumSet, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, TraceInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, QueueFullTraceInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSlowIdleRestartForVolatile, false, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSlowIdleRestartForPersisted, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SlowIdleRestartAtQueueUsagePercent, 85, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSlowActiveSecondaryRestartForVolatile, false, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableSlowActiveSecondaryRestartForPersisted, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SlowActiveSecondaryRestartAtQueueUsagePercent, 90, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(double, section_name, SecondaryProgressRateDecayFactor, 0.5, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, IdleReplicaMaxLagDurationBeforePromotion, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, SecondaryReplicatorBatchTracingArraySize, 32, Common::ConfigEntryUpgradePolicy::Dynamic); \

// -----------------------------------------------------------------------------------------
            // NOTE - Update the list of configs in ReplicatorSettings.cpp when new configs that 
            // have a corresponding entry in the public structure - FABRIC_REPLICATOR_SETTINGS are added 
            // -----------------------------------------------------------------------------------------

#define RE_INTERNAL_CONFIG_PROPERTIES(section_name, batch_acknowledgement_interval, max_primary_replication_queue_size, max_primary_replication_queue_memory_size, max_secondary_replication_queue_size, max_secondary_replication_queue_memory_size) \
            INTERNAL_CONFIG_ENTRY(uint, section_name, InitialReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxReplicationQueueSize, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxReplicationQueueMemorySize, max_primary_replication_queue_memory_size, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, InitialCopyQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxCopyQueueSize, 1024, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, BatchAcknowledgementInterval, Common::TimeSpan::FromMilliseconds(batch_acknowledgement_interval), Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, MaxPendingAcknowledgements, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, EnableReplicationOperationHeaderInBody, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxReplicationMessageSize, 52428800, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, RequireServiceAck, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(std::wstring, section_name, ReplicatorAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(std::wstring, section_name, ReplicatorListenAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(std::wstring, section_name, ReplicatorPublishAddress, L"localhost:0", Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, SecondaryClearAcknowledgedOperations, false, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(bool, section_name, UseStreamFaultsAndEndOfStreamOperationAck, true, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, InitialPrimaryReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxPrimaryReplicationQueueSize, max_primary_replication_queue_size, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxPrimaryReplicationQueueMemorySize, max_primary_replication_queue_memory_size, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, InitialSecondaryReplicationQueueSize, 64, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxSecondaryReplicationQueueSize, max_secondary_replication_queue_size, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(uint, section_name, MaxSecondaryReplicationQueueMemorySize, max_secondary_replication_queue_memory_size, Common::ConfigEntryUpgradePolicy::Static); \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, PrimaryWaitForPendingQuorumsTimeout, Common::TimeSpan::Zero, Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, QueueHealthMonitoringInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, QueueHealthWarningAtUsagePercent, 80, Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowApiMonitoringInterval, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Static); \
            \
            INTERNAL_CONFIG_ENTRY(Common::TimeSpan, section_name, RetryInterval, Common::TimeSpan::FromSeconds(5), Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, CompleteReplicateThreadCount, 0, Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(bool, section_name, AllowMultipleQuorumSet, true, Common::ConfigEntryUpgradePolicy::Static); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, TraceInterval, Common::TimeSpan::FromSeconds(30), Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, QueueFullTraceInterval, Common::TimeSpan::FromSeconds(10), Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(bool, section_name, EnableSlowIdleRestartForVolatile, false, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(bool, section_name, EnableSlowIdleRestartForPersisted, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, SlowIdleRestartAtQueueUsagePercent, 85, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(bool, section_name, EnableSlowActiveSecondaryRestartForVolatile, false, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(bool, section_name, EnableSlowActiveSecondaryRestartForPersisted, true, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, SlowActiveSecondaryRestartAtQueueUsagePercent, 90, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness, 0, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation, Common::TimeSpan::FromSeconds(120), Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(double, section_name, SecondaryProgressRateDecayFactor, 0.5, Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(Common::TimeSpan, section_name, IdleReplicaMaxLagDurationBeforePromotion, Common::TimeSpan::FromSeconds(60), Common::ConfigEntryUpgradePolicy::Dynamic); \
            DEPRECATED_CONFIG_ENTRY(uint, section_name, SecondaryReplicatorBatchTracingArraySize, 32, Common::ConfigEntryUpgradePolicy::Dynamic); \
            \
            \
            DEFINE_GETCONFIG_METHOD()
    }
}
