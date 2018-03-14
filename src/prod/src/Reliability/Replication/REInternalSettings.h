// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Internal Representation of the replicator settings used by all the components in the replicator
        class REInternalSettings :
            public REConfigBase
        {
            DENY_COPY_CONSTRUCTOR(REInternalSettings)

        public:

            static REInternalSettingsSPtr Create(
                ReplicatorSettingsUPtr && userSettings,
                std::shared_ptr<REConfig> const & globalConfig);

            __declspec(property(get = get_TestInternalConfigObject)) std::shared_ptr<REConfig> TestInternalConfigObject;
            std::shared_ptr<REConfig> get_TestInternalConfigObject() const
            {
                // No lock needed as this is a const
                return globalConfig_;
            }
            
            DECLARE_RE_GLOBAL_SETTINGS_PROPERTIES()
            DECLARE_RE_OVERRIDABLE_SETTINGS_PROPERTIES()
            DEFINE_GETCONFIG_METHOD()

            std::wstring ToString() const;
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            void FillEventData(Common::TraceEventContext & context) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);

        private:
            REInternalSettings(
                ReplicatorSettingsUPtr && userSettings,
                std::shared_ptr<REConfig> const & globalConfig);

            // Loads global defaults from the global config and also over rides any user provided values for settings
            // Step 1: Invokes LoadGlobalSettingsCallerHoldsLock()
            // Step 2: Invokes LoadGlobalDefaultsForOverRidableSettingsCallerHoldsLock()
            // Step 3: Invokes LoadOverRidableSettingsCallerHoldsLock()
            void LoadSettings();
            
            // Loads only the global defaults from the global config which are NOT over-rideable
            int LoadGlobalSettingsCallerHoldsLock();

            // Loads the global defaults from the global config which are over-rideable by the user via CreateReplicator API
            int LoadGlobalDefaultsForOverRidableSettingsCallerHoldsLock();
        
            // Loads the settings provided by the user 
            void LoadOverRidableSettingsCallerHoldsLock();

            std::wstring GetFormattedStringForTrailingSettingsToTrace() const;

            MUTABLE_RWLOCK(REInternalSettings, lock_);

            std::shared_ptr<REConfig> const globalConfig_;
            ReplicatorSettingsUPtr const userSettings_;

            // The following are global settings
            int64 maxPendingAcknowledgements_;
            bool enableReplicationOperationHeaderInBody_;
            Common::TimeSpan traceInterval_;
            Common::TimeSpan queueFullTraceInterval_;
            Common::TimeSpan queueHealthMonitoringInterval_ ;
            Common::TimeSpan slowApiMonitoringInterval_ ;
            int64 queueHealthWarningAtUsagePercent_;
            int64 completeReplicateThreadCount_ ;
            bool allowMultipleQuorumSet_;
            bool enableSlowIdleRestartForVolatile_;
            bool enableSlowIdleRestartForPersisted_;
            int64 slowIdleRestartAtQueueUsagePercent_ ;
            bool enableSlowActiveSecondaryRestartForVolatile_;
            bool enableSlowActiveSecondaryRestartForPersisted_;
            int64 slowActiveSecondaryRestartAtQueueUsagePercent_ ;
            int64 activeSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_ ;
            Common::TimeSpan slowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_ ;
            double secondaryProgressRateDecayFactor_ ;
            Common::TimeSpan idleReplicaMaxLagDurationBeforePromotion_;
            int64 secondaryReplicatorBatchTracingArraySize_;

            // The following are over-ridable settings
            Common::TimeSpan retryInterval_;
            Common::TimeSpan batchAckInterval_;
            Common::TimeSpan primaryWaitForPendingQuorumsTimeout_;
            std::wstring replicatorAddress_;
            std::wstring replicatorListenAddress_;
            std::wstring replicatorPublishAddress_;
            bool requireServiceAck_;
            bool secondaryClearAcknowledgedOperations_;
            bool useStreamFaultsAndEndOfStreamOperationAck_;
            int64 initialReplicationQueueSize_;
            int64 maxReplicationQueueSize_;
            int64 initialCopyQueueSize_;
            int64 maxCopyQueueSize_;
            int64 maxReplicationQueueMemorySize_;
            int64 maxReplicationMessageSize_;
            int64 initialPrimaryReplicationQueueSize_;
            int64 maxPrimaryReplicationQueueSize_;
            int64 maxPrimaryReplicationQueueMemorySize_;
            int64 initialSecondaryReplicationQueueSize_;
            int64 maxSecondaryReplicationQueueSize_;
            int64 maxSecondaryReplicationQueueMemorySize_;
        };
    }
}
