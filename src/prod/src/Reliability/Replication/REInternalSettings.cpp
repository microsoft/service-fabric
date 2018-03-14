// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {
    
using std::shared_ptr;
using std::wstring;
using Common::AcquireReadLock;
using Common::AcquireExclusiveLock;
using Common::TimeSpan;
using Common::EventArgs;

REInternalSettingsSPtr REInternalSettings::Create(
    ReplicatorSettingsUPtr && userSettings, 
    shared_ptr<REConfig> const & globalConfig)
{
    auto object = shared_ptr<REInternalSettings>(new REInternalSettings(move(userSettings), globalConfig));
    return object;
}

REInternalSettings::REInternalSettings(
    ReplicatorSettingsUPtr && userSettings, 
    shared_ptr<REConfig> const & globalConfig)
    : lock_()
    , globalConfig_(globalConfig)
    , userSettings_(move(userSettings))
{
    LoadSettings();
}

int64 REInternalSettings::get_MaxPendingAcknowledgements() const
{
    AcquireReadLock grab(lock_);
    return maxPendingAcknowledgements_;
}

bool REInternalSettings::get_EnableReplicationOperationHeaderInBody() const 
{
    AcquireReadLock grab(lock_);
    return enableReplicationOperationHeaderInBody_;
}

TimeSpan REInternalSettings::get_TraceInterval() const
{
    AcquireReadLock grab(lock_);
    return traceInterval_;
}

TimeSpan REInternalSettings::get_QueueFullTraceInterval() const
{
    AcquireReadLock grab(lock_);
    return queueFullTraceInterval_;
}

TimeSpan REInternalSettings::get_QueueHealthMonitoringInterval() const
{
    AcquireReadLock grab(lock_);
    return queueHealthMonitoringInterval_;
}

TimeSpan REInternalSettings::get_SlowApiMonitoringInterval() const
{
    AcquireReadLock grab(lock_);
    return slowApiMonitoringInterval_;
}

int64 REInternalSettings::get_QueueHealthWarningAtUsagePercent() const
{
    AcquireReadLock grab(lock_);
    return queueHealthWarningAtUsagePercent_;
}

int64 REInternalSettings::get_CompleteReplicateThreadCount() const
{
    AcquireReadLock grab(lock_);
    return completeReplicateThreadCount_;
}

bool REInternalSettings::get_AllowMultipleQuorumSet() const
{
    AcquireReadLock grab(lock_);
    return allowMultipleQuorumSet_;
}

bool REInternalSettings::get_EnableSlowIdleRestartForVolatile() const
{
    AcquireReadLock grab(lock_);
    return enableSlowIdleRestartForVolatile_;
}

bool REInternalSettings::get_EnableSlowIdleRestartForPersisted() const
{
    AcquireReadLock grab(lock_);
    return enableSlowIdleRestartForPersisted_;
}

int64 REInternalSettings::get_SlowIdleRestartAtQueueUsagePercent() const
{
    AcquireReadLock grab(lock_);
    return slowIdleRestartAtQueueUsagePercent_;
}

bool REInternalSettings::get_EnableSlowActiveSecondaryRestartForVolatile() const
{
    AcquireReadLock grab(lock_);
    return enableSlowActiveSecondaryRestartForVolatile_;
}

bool REInternalSettings::get_EnableSlowActiveSecondaryRestartForPersisted() const
{
    AcquireReadLock grab(lock_);
    return enableSlowActiveSecondaryRestartForPersisted_;
}

int64 REInternalSettings::get_SlowActiveSecondaryRestartAtQueueUsagePercent() const
{
    AcquireReadLock grab(lock_);
    return slowActiveSecondaryRestartAtQueueUsagePercent_;
}

TimeSpan REInternalSettings::get_SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation() const
{
    AcquireReadLock grab(lock_);
    return slowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_;
}

int64 REInternalSettings::get_ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness() const
{
    AcquireReadLock grab(lock_);
    return activeSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_;
}

double REInternalSettings::get_SecondaryProgressRateDecayFactor() const
{
    AcquireReadLock grab(lock_);
    return secondaryProgressRateDecayFactor_;
}

TimeSpan REInternalSettings::get_IdleReplicaMaxLagDurationBeforePromotion() const
{
    AcquireReadLock grab(lock_);
    return idleReplicaMaxLagDurationBeforePromotion_;
}

int64 REInternalSettings::get_SecondaryReplicatorBatchTracingArraySize() const
{
    AcquireReadLock grab(lock_);
    return secondaryReplicatorBatchTracingArraySize_;
}

bool REInternalSettings::get_RequireServiceAck() const
{
    AcquireReadLock grab(lock_);
    return requireServiceAck_;
}

bool REInternalSettings::get_SecondaryClearAcknowledgedOperations() const
{
    AcquireReadLock grab(lock_);
    return secondaryClearAcknowledgedOperations_;
}

bool REInternalSettings::get_UseStreamFaultsAndEndOfStreamOperationAck() const
{
    AcquireReadLock grab(lock_);
    return useStreamFaultsAndEndOfStreamOperationAck_;
}

TimeSpan REInternalSettings::get_RetryInterval() const
{
    AcquireReadLock grab(lock_);
    return retryInterval_;
}

TimeSpan REInternalSettings::get_BatchAckInterval() const
{
    AcquireReadLock grab(lock_);
    return batchAckInterval_;
}

TimeSpan REInternalSettings::get_PrimaryWaitForPendingQuorumsTimeout() const
{
    AcquireReadLock grab(lock_);
    return primaryWaitForPendingQuorumsTimeout_;
}

int64 REInternalSettings::get_InitialReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return initialReplicationQueueSize_;
}

int64 REInternalSettings::get_MaxReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return maxReplicationQueueSize_;
}

int64 REInternalSettings::get_InitialCopyQueueSize() const
{
    AcquireReadLock grab(lock_);
    return initialCopyQueueSize_;
}

int64 REInternalSettings::get_MaxCopyQueueSize() const
{
    AcquireReadLock grab(lock_);
    return maxCopyQueueSize_;
}

int64 REInternalSettings::get_MaxReplicationQueueMemorySize() const
{
    AcquireReadLock grab(lock_);
    return maxReplicationQueueMemorySize_;
}

int64 REInternalSettings::get_MaxReplicationMessageSize() const
{
    AcquireReadLock grab(lock_);
    return maxReplicationMessageSize_;
}

int64 REInternalSettings::get_InitialPrimaryReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return initialPrimaryReplicationQueueSize_;
}

int64 REInternalSettings::get_MaxPrimaryReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return maxPrimaryReplicationQueueSize_;
}

int64 REInternalSettings::get_MaxPrimaryReplicationQueueMemorySize() const
{
    AcquireReadLock grab(lock_);
    return maxPrimaryReplicationQueueMemorySize_;
}

int64 REInternalSettings::get_InitialSecondaryReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return initialSecondaryReplicationQueueSize_;
}

int64 REInternalSettings::get_MaxSecondaryReplicationQueueSize() const
{
    AcquireReadLock grab(lock_);
    return maxSecondaryReplicationQueueSize_;
}

int64 REInternalSettings::get_MaxSecondaryReplicationQueueMemorySize() const
{
    AcquireReadLock grab(lock_);
    return maxSecondaryReplicationQueueMemorySize_;
}

wstring REInternalSettings::get_ReplicatorAddress() const
{
    AcquireReadLock grab(lock_);
    return replicatorAddress_;
}

wstring REInternalSettings::get_ReplicatorListenAddress() const
{
    AcquireReadLock grab(lock_);
    return replicatorListenAddress_;
}

wstring REInternalSettings::get_ReplicatorPublishAddress() const
{
    AcquireReadLock grab(lock_);
    return replicatorPublishAddress_;
}

void REInternalSettings::LoadSettings()
{
    AcquireExclusiveLock grab(lock_);

    // First load all global settings that cannot be over-ridden
    auto globalSettingsCount = LoadGlobalSettingsCallerHoldsLock();

    // Next load all the settings from the clustermanifest that can be over-ridden
    auto globalOverridableSettingsCount = LoadGlobalDefaultsForOverRidableSettingsCallerHoldsLock();

    if (userSettings_ == nullptr)
        return;

    // Finally load all the settings that are over-ridden
    LoadOverRidableSettingsCallerHoldsLock();
    
    // Sanity check that all the settings have loaded
    TESTASSERT_IF(globalSettingsCount != RE_GLOBAL_SETTINGS_COUNT, "Failed to load global settings as number of settings do not match");
    TESTASSERT_IF(globalOverridableSettingsCount != RE_OVERRIDABLE_SETTINGS_COUNT, "Failed to load global settings for overridable settings as number of setings do not match");
}

int REInternalSettings::LoadGlobalSettingsCallerHoldsLock()
{
    auto i = 0;

    this->completeReplicateThreadCount_ = globalConfig_->CompleteReplicateThreadCount;
    globalConfig_->CompleteReplicateThreadCountEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"CompleteReplicateThreadCount",
            Common::wformatString("{0}", this->completeReplicateThreadCount_),
            Common::wformatString("{0}", globalConfig_->CompleteReplicateThreadCount));

        this->completeReplicateThreadCount_ = globalConfig_->CompleteReplicateThreadCount;
    });
    i += 1;

    this->allowMultipleQuorumSet_ = globalConfig_->AllowMultipleQuorumSet;
    globalConfig_->AllowMultipleQuorumSetEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"AllowMultipleQuorumSet",
            Common::wformatString("{0}", this->allowMultipleQuorumSet_),
            Common::wformatString("{0}", globalConfig_->AllowMultipleQuorumSet));

        this->allowMultipleQuorumSet_ = globalConfig_->AllowMultipleQuorumSet;
    });
    i += 1;

    this->maxPendingAcknowledgements_ = globalConfig_->MaxPendingAcknowledgements; 
    globalConfig_->MaxPendingAcknowledgementsEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"MaxPendingAcknowledgements",
            Common::wformatString("{0}", this->maxPendingAcknowledgements_),
            Common::wformatString("{0}", globalConfig_->MaxPendingAcknowledgements));

        this->maxPendingAcknowledgements_ = globalConfig_->MaxPendingAcknowledgements;
    });
    i += 1;

    this->enableReplicationOperationHeaderInBody_ = globalConfig_->EnableReplicationOperationHeaderInBody; 
    globalConfig_->EnableReplicationOperationHeaderInBodyEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"EnableReplicationOperationHeaderInBody",
            Common::wformatString("{0}", this->enableReplicationOperationHeaderInBody_),
            Common::wformatString("{0}", globalConfig_->EnableReplicationOperationHeaderInBody));

        this->enableReplicationOperationHeaderInBody_ = globalConfig_->EnableReplicationOperationHeaderInBody;
    });
    i += 1;

    this->traceInterval_ = globalConfig_->TraceInterval;
    globalConfig_->TraceIntervalEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"TraceInterval",
            Common::wformatString("{0}", this->traceInterval_.TotalMilliseconds()),
            Common::wformatString("{0}", globalConfig_->TraceInterval.TotalMilliseconds()));

        this->traceInterval_ = globalConfig_->TraceInterval;
    });
    i += 1;

    this->queueFullTraceInterval_ = globalConfig_->QueueFullTraceInterval;
    globalConfig_->QueueFullTraceIntervalEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"QueueFullTraceInterval",
            Common::wformatString("{0}", this->queueFullTraceInterval_.TotalMilliseconds()),
            Common::wformatString("{0}", globalConfig_->QueueFullTraceInterval.TotalMilliseconds()));

        this->queueFullTraceInterval_ = globalConfig_->QueueFullTraceInterval;
    });
    i += 1;

    this->queueHealthMonitoringInterval_ = globalConfig_->QueueHealthMonitoringInterval;
    globalConfig_->QueueHealthMonitoringIntervalEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"QueueHealthMonitoringInterval",
            Common::wformatString("{0}", this->queueHealthMonitoringInterval_.TotalMilliseconds()),
            Common::wformatString("{0}", globalConfig_->QueueHealthMonitoringInterval.TotalMilliseconds()));

        this->queueHealthMonitoringInterval_ = globalConfig_->QueueHealthMonitoringInterval;
    });
    i += 1;

    this->queueHealthWarningAtUsagePercent_ = globalConfig_->QueueHealthWarningAtUsagePercent;
    globalConfig_->QueueHealthWarningAtUsagePercentEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"QueueHealthWarningAtUsagePercent",
            Common::wformatString("{0}", this->queueHealthWarningAtUsagePercent_),
            Common::wformatString("{0}", globalConfig_->QueueHealthWarningAtUsagePercent));

        this->queueHealthWarningAtUsagePercent_ = globalConfig_->QueueHealthWarningAtUsagePercent;
    });
    i += 1;

    this->slowApiMonitoringInterval_ = globalConfig_->SlowApiMonitoringInterval;
    globalConfig_->SlowApiMonitoringIntervalEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SlowApiMonitoringInterval",
            Common::wformatString("{0}", this->slowApiMonitoringInterval_.TotalMilliseconds()),
            Common::wformatString("{0}", globalConfig_->SlowApiMonitoringInterval.TotalMilliseconds()));

        this->slowApiMonitoringInterval_ = globalConfig_->SlowApiMonitoringInterval;
    });
    i += 1;

    this->enableSlowIdleRestartForVolatile_ = globalConfig_->EnableSlowIdleRestartForVolatile;
    globalConfig_->EnableSlowIdleRestartForVolatileEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"EnableSlowIdleRestartForVolatile",
            Common::wformatString("{0}", this->enableSlowIdleRestartForVolatile_),
            Common::wformatString("{0}", globalConfig_->EnableSlowIdleRestartForVolatile));

        this->enableSlowIdleRestartForVolatile_ = globalConfig_->EnableSlowIdleRestartForVolatile;
    });
    i += 1;

    this->enableSlowIdleRestartForPersisted_ = globalConfig_->EnableSlowIdleRestartForPersisted;
    globalConfig_->EnableSlowIdleRestartForPersistedEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"EnableSlowIdleRestartForPersisted",
            Common::wformatString("{0}", this->enableSlowIdleRestartForPersisted_),
            Common::wformatString("{0}", globalConfig_->EnableSlowIdleRestartForPersisted));

        this->enableSlowIdleRestartForPersisted_ = globalConfig_->EnableSlowIdleRestartForPersisted;
    });
    i += 1;

    this->slowIdleRestartAtQueueUsagePercent_ = globalConfig_->SlowIdleRestartAtQueueUsagePercent;
    globalConfig_->SlowIdleRestartAtQueueUsagePercentEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SlowIdleRestartAtQueueUsagePercent",
            Common::wformatString("{0}", this->slowIdleRestartAtQueueUsagePercent_),
            Common::wformatString("{0}", globalConfig_->SlowIdleRestartAtQueueUsagePercent));

        this->slowIdleRestartAtQueueUsagePercent_ = globalConfig_->SlowIdleRestartAtQueueUsagePercent;
    });
    i += 1;

    this->enableSlowActiveSecondaryRestartForVolatile_ = globalConfig_->EnableSlowActiveSecondaryRestartForVolatile;
    globalConfig_->EnableSlowActiveSecondaryRestartForVolatileEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"EnableSlowActiveSecondaryRestartForVolatile",
            Common::wformatString("{0}", this->enableSlowActiveSecondaryRestartForVolatile_),
            Common::wformatString("{0}", globalConfig_->EnableSlowActiveSecondaryRestartForVolatile));

        this->enableSlowActiveSecondaryRestartForVolatile_ = globalConfig_->EnableSlowActiveSecondaryRestartForVolatile;
    });
    i += 1;

    this->enableSlowActiveSecondaryRestartForPersisted_ = globalConfig_->EnableSlowActiveSecondaryRestartForPersisted;
    globalConfig_->EnableSlowActiveSecondaryRestartForPersistedEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"EnableSlowActiveSecondaryRestartForPersisted",
            Common::wformatString("{0}", this->enableSlowActiveSecondaryRestartForPersisted_),
            Common::wformatString("{0}", globalConfig_->EnableSlowActiveSecondaryRestartForPersisted));

        this->enableSlowActiveSecondaryRestartForPersisted_ = globalConfig_->EnableSlowActiveSecondaryRestartForPersisted;
    });
    i += 1;

    this->slowActiveSecondaryRestartAtQueueUsagePercent_ = globalConfig_->SlowActiveSecondaryRestartAtQueueUsagePercent;
    globalConfig_->SlowActiveSecondaryRestartAtQueueUsagePercentEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SlowActiveSecondaryRestartAtQueueUsagePercent",
            Common::wformatString("{0}", this->slowActiveSecondaryRestartAtQueueUsagePercent_),
            Common::wformatString("{0}", globalConfig_->SlowActiveSecondaryRestartAtQueueUsagePercent));

        this->slowActiveSecondaryRestartAtQueueUsagePercent_ = globalConfig_->SlowActiveSecondaryRestartAtQueueUsagePercent;
    });
    i += 1;

    this->activeSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_ = globalConfig_->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness;
    globalConfig_->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlownessEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness",
            Common::wformatString("{0}", this->activeSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_),
            Common::wformatString("{0}", globalConfig_->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness));

        this->activeSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness_ = globalConfig_->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness;
    });
    i += 1;

    this->slowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_ = globalConfig_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation;
    globalConfig_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperationEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation",
            Common::wformatString("{0}", this->slowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_),
            Common::wformatString("{0}", globalConfig_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation));

        this->slowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation_ = globalConfig_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation;
    });
    i += 1;

    this->secondaryProgressRateDecayFactor_ = globalConfig_->SecondaryProgressRateDecayFactor;
    globalConfig_->SecondaryProgressRateDecayFactorEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SecondaryProgressRateDecayFactor",
            Common::wformatString("{0}", this->secondaryProgressRateDecayFactor_),
            Common::wformatString("{0}", globalConfig_->SecondaryProgressRateDecayFactor));

        this->secondaryProgressRateDecayFactor_ = globalConfig_->SecondaryProgressRateDecayFactor;
    });
    i += 1;

    this->idleReplicaMaxLagDurationBeforePromotion_ = globalConfig_->IdleReplicaMaxLagDurationBeforePromotion;
    globalConfig_->IdleReplicaMaxLagDurationBeforePromotionEntry.AddHandler(
        [&] (EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"IdleReplicaMaxLagDurationBeforePromotion",
            Common::wformatString("{0}", this->idleReplicaMaxLagDurationBeforePromotion_),
            Common::wformatString("{0}", globalConfig_->IdleReplicaMaxLagDurationBeforePromotion));

        this->idleReplicaMaxLagDurationBeforePromotion_ = globalConfig_->IdleReplicaMaxLagDurationBeforePromotion;
    });
    i += 1;

    this->secondaryReplicatorBatchTracingArraySize_ = globalConfig_->SecondaryReplicatorBatchTracingArraySize;
    globalConfig_->SecondaryReplicatorBatchTracingArraySizeEntry.AddHandler(
        [&](EventArgs const &)
    {
        AcquireExclusiveLock grab(lock_);

        ReplicatorEventSource::Events->ReplicatorConfigUpdate(
            reinterpret_cast<uintptr_t>(this),
            L"SecondaryReplicatorBatchTracingArraySize",
            Common::wformatString("{0}", this->secondaryReplicatorBatchTracingArraySize_),
            Common::wformatString("{0}", globalConfig_->SecondaryReplicatorBatchTracingArraySize));

        this->secondaryReplicatorBatchTracingArraySize_ = globalConfig_->SecondaryReplicatorBatchTracingArraySize;
    });
    i += 1;

    return i;
}

int REInternalSettings::LoadGlobalDefaultsForOverRidableSettingsCallerHoldsLock()
{
    auto i = 0;

    this->replicatorAddress_ = globalConfig_->ReplicatorAddress;
    i += 1;
    this->replicatorListenAddress_ = globalConfig_->ReplicatorListenAddress;
    i += 1;
    this->replicatorPublishAddress_ = globalConfig_->ReplicatorPublishAddress;
    i += 1;

    //
    // WORK AROUND ALERT - Config subsystem replaces "0" in ReplicatorAddress if provided in cluster manifest to "IP:Port".
    // The Listen and Publish Address must be initialized to this to ensure customers using "0" are not broken if the default values are specified for the listen and publish addresses
    // 
    // Disable this workaround once the config subsystem logic moves into deployer
    // Also remove the test case "TestReplicatorListenAndPublishAddressWhenRESettingsIsNull" from ReplicatorSettings.Test.cpp
    //
    // RD: RDBug 10589649: Config subsystem loads ipaddress:port if config name ends with "Address" - Move this logic into deployer
    //
    if (this->replicatorListenAddress_ == globalConfig_->ReplicatorListenAddressEntry.DefaultValue)
    {
        this->replicatorListenAddress_ = this->replicatorAddress_;
    }
    if (this->replicatorPublishAddress_ == globalConfig_->ReplicatorPublishAddressEntry.DefaultValue)
    {
        this->replicatorPublishAddress_ = this->replicatorAddress_;
    }

    this->retryInterval_ = globalConfig_->RetryInterval;
    i += 1;
    this->batchAckInterval_ = globalConfig_->BatchAcknowledgementInterval;
    i += 1;
    this->requireServiceAck_ = globalConfig_->RequireServiceAck;
    i += 1;
    this->initialReplicationQueueSize_ = globalConfig_->InitialReplicationQueueSize;
    i += 1;
    this->initialPrimaryReplicationQueueSize_ = globalConfig_->InitialPrimaryReplicationQueueSize;
    i += 1;
    this->initialSecondaryReplicationQueueSize_ = globalConfig_->InitialSecondaryReplicationQueueSize;
    i += 1;
    this->maxReplicationQueueSize_ = globalConfig_->MaxReplicationQueueSize;
    i += 1;
    this->maxPrimaryReplicationQueueSize_ = globalConfig_->MaxPrimaryReplicationQueueSize;
    i += 1;
    this->maxSecondaryReplicationQueueSize_ = globalConfig_->MaxSecondaryReplicationQueueSize;
    i += 1;
    this->maxReplicationQueueMemorySize_ = globalConfig_->MaxReplicationQueueMemorySize;
    i += 1;
    this->maxPrimaryReplicationQueueMemorySize_ = globalConfig_->MaxPrimaryReplicationQueueMemorySize;
    i += 1;
    this->maxSecondaryReplicationQueueMemorySize_ = globalConfig_->MaxSecondaryReplicationQueueMemorySize;
    i += 1;
    this->initialCopyQueueSize_ = globalConfig_->InitialCopyQueueSize;
    i += 1;
    this->maxCopyQueueSize_ = globalConfig_->MaxCopyQueueSize;
    i += 1;
    this->secondaryClearAcknowledgedOperations_ = globalConfig_->SecondaryClearAcknowledgedOperations;
    i += 1;
    this->maxReplicationMessageSize_ = globalConfig_->MaxReplicationMessageSize;
    i += 1;
    this->useStreamFaultsAndEndOfStreamOperationAck_ = globalConfig_->UseStreamFaultsAndEndOfStreamOperationAck;
    i += 1;
    this->primaryWaitForPendingQuorumsTimeout_ = globalConfig_->PrimaryWaitForPendingQuorumsTimeout;
    i += 1;

    return i;
}

void REInternalSettings::LoadOverRidableSettingsCallerHoldsLock()
{
    DWORD flags = userSettings_->Flags;

    // relation between replicator address (legacy flag), relicator listen address and replicator publish address
    // replicator address provided: YES , listen and publish address provided: NO  => initialize listen and publish address to replicator address.
    // replicator address provided: YES , listen and publish address provided: YES => ignore replication address and use listen and publish address.
    // replicator address provided: NO  , listen and publish address provided: YES => use listen and publish address.
    // replicator address provided: NO  , listen and publish address provided: NO  => do not set anything here. default values will be used.
    if ((flags & FABRIC_REPLICATOR_ADDRESS) != 0)
    {
        this->replicatorAddress_ = userSettings_->ReplicatorAddress;

        this->replicatorListenAddress_ = this->replicatorAddress_;
        this->replicatorPublishAddress_ = this->replicatorAddress_;

        ASSERT_IF(userSettings_->ReplicatorAddress == L"", "Empty Replicator Address is not allowed if FABRIC_REPLICATOR_ADDRESS flag is specified");
    }

    if ((flags & FABRIC_REPLICATOR_LISTEN_ADDRESS) != 0)
    {
        this->replicatorListenAddress_ = userSettings_->ReplicatorListenAddress;
        ASSERT_IF(userSettings_->ReplicatorListenAddress == L"", "Empty Replicator Listen Address is not allowed if FABRIC_REPLICATOR_LISTEN_ADDRESS flag is specified");
    }

    if ((flags & FABRIC_REPLICATOR_PUBLISH_ADDRESS) != 0)
    {
        this->replicatorPublishAddress_ = userSettings_->ReplicatorPublishAddress;
        ASSERT_IF(userSettings_->ReplicatorPublishAddress == L"", "Empty Replicator Publish Address is not allowed if FABRIC_REPLICATOR_PUBLISH_ADDRESS flag is specified");
    }

    if ((flags & FABRIC_REPLICATOR_RETRY_INTERVAL) != 0)
    {
        this->retryInterval_ = userSettings_->RetryInterval;
    }

    if ((flags & FABRIC_REPLICATOR_BATCH_ACKNOWLEDGEMENT_INTERVAL) != 0)
    {
        this->batchAckInterval_ = userSettings_->BatchAcknowledgementInterval;
    }

    if ((flags & FABRIC_REPLICATOR_REQUIRE_SERVICE_ACK) != 0)
    {
        this->requireServiceAck_ = userSettings_->RequireServiceAck;
    }

    if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
    {
        this->initialReplicationQueueSize_ = static_cast<DWORD>(userSettings_->InitialReplicationQueueSize);
        this->initialPrimaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->InitialReplicationQueueSize);
        this->initialSecondaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->InitialReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) != 0)
    {
        this->maxReplicationQueueSize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueSize);
        this->maxPrimaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueSize);
        this->maxSecondaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
    {
        this->maxReplicationQueueMemorySize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueMemorySize);
        this->maxPrimaryReplicationQueueMemorySize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueMemorySize);
        this->maxSecondaryReplicationQueueMemorySize_ = static_cast<DWORD>(userSettings_->MaxReplicationQueueMemorySize);

        // if only memory limit is provided, the number of item will not be limited.
        if ((flags & FABRIC_REPLICATOR_REPLICATION_QUEUE_MAX_SIZE) == 0)
        {
            this->maxReplicationQueueSize_ = 0;
            this->maxPrimaryReplicationQueueSize_ = 0;
            this->maxSecondaryReplicationQueueSize_ = 0;
        }
    }
    if ((flags & FABRIC_REPLICATOR_COPY_QUEUE_INITIAL_SIZE) != 0)
    {
        this->initialCopyQueueSize_ = static_cast<DWORD>(userSettings_->InitialCopyQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_COPY_QUEUE_MAX_SIZE) != 0)
    {
        this->maxCopyQueueSize_ = static_cast<DWORD>(userSettings_->MaxCopyQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_SECONDARY_CLEAR_ACKNOWLEDGED_OPERATIONS) != 0)
    {
        this->secondaryClearAcknowledgedOperations_ = static_cast<DWORD>(userSettings_->SecondaryClearAcknowledgedOperations);
    }
    if ((flags & FABRIC_REPLICATOR_REPLICATION_MESSAGE_MAX_SIZE) != 0)
    {
        this->maxReplicationMessageSize_ = static_cast<DWORD>(userSettings_->MaxReplicationMessageSize);
    }
    if ((flags & FABRIC_REPLICATOR_USE_STREAMFAULTS_AND_ENDOFSTREAM_OPERATIONACK) != 0)
    {
        this->useStreamFaultsAndEndOfStreamOperationAck_ = userSettings_->UseStreamFaultsAndEndOfStreamOperationAck;
    }
    if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
    {
        this->initialSecondaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->InitialSecondaryReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
    {
        this->maxSecondaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->MaxSecondaryReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
    {
        this->maxSecondaryReplicationQueueMemorySize_ = static_cast<DWORD>(userSettings_->MaxSecondaryReplicationQueueMemorySize);
        
        // If only memory size is specified, we will allow any number of operations
        if ((flags & FABRIC_REPLICATOR_SECONDARY_REPLICATION_QUEUE_MAX_SIZE) == 0)
        {
            this->maxSecondaryReplicationQueueSize_ = 0;
        }
    }
    if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_INITIAL_SIZE) != 0)
    {
        this->initialPrimaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->InitialPrimaryReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) != 0)
    {
        this->maxPrimaryReplicationQueueSize_ = static_cast<DWORD>(userSettings_->MaxPrimaryReplicationQueueSize);
    }
    if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_MEMORY_SIZE) != 0)
    {
        this->maxPrimaryReplicationQueueMemorySize_ = static_cast<DWORD>(userSettings_->MaxPrimaryReplicationQueueMemorySize);

        // If only memory size is specified, we will allow any number of operations
        if ((flags & FABRIC_REPLICATOR_PRIMARY_REPLICATION_QUEUE_MAX_SIZE) == 0)
        {
            this->maxPrimaryReplicationQueueSize_ = 0;
        }
    }
    if ((flags & FABRIC_REPLICATOR_PRIMARY_WAIT_FOR_PENDING_QUORUMS_TIMEOUT) != 0)
    {
        this->primaryWaitForPendingQuorumsTimeout_ = userSettings_->PrimaryWaitForPendingQuorumsTimeout;
    }
}

wstring REInternalSettings::ToString() const
{
    wstring content;
    Common::StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));        
    return content;
}

wstring REInternalSettings::GetFormattedStringForTrailingSettingsToTrace() const
{
    wstring string1 = Common::wformatString(
        "{0}, SlowIdleRestartAtQueueUsagePercent = {1}, SlowActiveSecondaryRestartForPersisted = {2}, SlowActiveSecondaryRestartForVolatile = {3}, SlowActiveSecondaryRestartAtQueueUsagePercent = {4}, SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation = {5}seconds, SecondaryProgressRateDecayFactor = {6}, ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness = {7}, EnableReplicationOperationHeaderInBody = {8}, ReplicationListenAddress = {9}, ReplicationPublishAddress = {10}",
        this->EnableSlowIdleRestartForVolatile,
        this->SlowIdleRestartAtQueueUsagePercent,
        this->EnableSlowActiveSecondaryRestartForPersisted,
        this->EnableSlowActiveSecondaryRestartForVolatile,
        this->SlowActiveSecondaryRestartAtQueueUsagePercent,
        this->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation.TotalSeconds(),
        this->SecondaryProgressRateDecayFactor,
        this->ActiveSecondaryCountAdditionalToWriteQuorumNotRestaredDueToSlowness,
        this->EnableReplicationOperationHeaderInBody,
        this->ReplicatorListenAddress,
        this->ReplicatorPublishAddress);

    wstring string2 = Common::wformatString(
        "{0}, IdleReplicaMaxLagDurationBeforePromotion = {1} this = {2}", // Ensure "this =" is always at the end
        string1,
        this->IdleReplicaMaxLagDurationBeforePromotion,
        reinterpret_cast<uintptr_t>(this));
    
    return string2;
}

void REInternalSettings::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w.Write("AllowMultipleQuorumSet = {0}, ", this->AllowMultipleQuorumSet);
    w.Write("BatchAcknowledgementInterval = {0}, ", this->BatchAcknowledgementInterval);
    w.Write("CompleteReplicateThreadCount = {0}, ", this->CompleteReplicateThreadCount);
    w.Write("InitialCopyQueueSize = {0}, ", this->InitialCopyQueueSize);
    w.Write("InitialReplicationQueueSize = {0}, ", this->InitialReplicationQueueSize);
    w.Write("MaxCopyQueueSize = {0}, ", this->MaxCopyQueueSize);
    w.Write("MaxPendingAcknowledgements = {0}, ", this->MaxPendingAcknowledgements);
    w.Write("MaxReplicationMessageSize = {0}, ", this->MaxReplicationMessageSize);
    w.Write("MaxReplicationQueueMemorySize = {0}, ", this->MaxReplicationQueueMemorySize);
    w.Write("MaxReplicationQueueSize = {0}, ", this->MaxReplicationQueueSize);
    w.Write("ReplicatorAddress = {0}, ", this->ReplicatorAddress);
    w.Write("RequireServiceAck = {0}, ", this->RequireServiceAck);
    w.Write("RetryInterval = {0}, ", this->RetryInterval);
    w.Write("SecondaryClearAcknowledgedOperations = {0}, ", this->SecondaryClearAcknowledgedOperations);
    w.Write("TraceInterval = {0}, ", this->TraceInterval);        
    w.Write("UseStreamFaultsAndEndOfStreamOperationAck = {0} ", this->UseStreamFaultsAndEndOfStreamOperationAck);
    w.Write("InitialSecondaryReplicationQueueSize = {0} ", this->InitialSecondaryReplicationQueueSize);
    w.Write("MaxSecondaryReplicationQueueSize = {0} ", this->MaxSecondaryReplicationQueueSize);
    w.Write("MaxSecondaryReplicationQueueMemorySize = {0} ", this->MaxSecondaryReplicationQueueMemorySize);
    w.Write("InitialPrimaryReplicationQueueSize = {0} ", this->InitialPrimaryReplicationQueueSize);
    w.Write("MaxPrimaryReplicationQueueSize = {0} ", this->MaxPrimaryReplicationQueueSize);
    w.Write("MaxPrimaryReplicationQueueMemorySize = {0} ", this->MaxPrimaryReplicationQueueMemorySize);
    w.Write("PrimaryWaitForPendingQuorumsTimeout = {0} ", this->PrimaryWaitForPendingQuorumsTimeout);
    w.Write("QueueFullTraceInterval = {0}, ", this->QueueFullTraceInterval);
    w.Write("QueueHealthMonitoringInterval = {0}, ", this->QueueHealthMonitoringInterval);
    w.Write("QueueHealthWarningAtUsagePercent = {0}, ", this->QueueHealthWarningAtUsagePercent);
    w.Write("SlowApiMonitoringInterval = {0}, ", this->SlowApiMonitoringInterval);
    w.Write("EnableSlowIdleRestartForPersisted = {0}, ", this->EnableSlowIdleRestartForPersisted);
    w.Write("EnableSlowIdleRestartForVolatile = {0}, ", GetFormattedStringForTrailingSettingsToTrace());
}

std::string REInternalSettings::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "AllowMultipleQuorumSet={0},BatchAcknowledgmentInterval={1},CompleteReplicateThreadCount={2},InitialCopyQueueSize={3},InitialReplicationQueueSize={4},MaxCopyQueueSize={5},MaxPendingAcknowledgements={6},MaxReplicationMessageSize={7},MaxReplicationQueueMemorySize={8},MaxReplicationQueueSize={9},ReplicatorAddress={10},RequireServiceAck={11},RetryInterval={12},SecondaryClearAcknowledgedOperations={13},TraceInterval={14},UseStreamFaultsAndEndOfStreamOperationAck={15}, InitialSecondaryReplicationQueueSize={16}, MaxSecondaryReplicationQueueSize={17}, MaxSecondaryReplicationQueueMemorySize={18}, InitialPrimaryReplicationQueueSize = {19}, MaxPrimaryReplicationQueueSize = {20}, MaxPrimaryReplicationQueueMemorySize = {21}, PrimaryWaitForPendingQuorumsTimeout = {22}, QueueFullTraceInterval = {23}, QueueHealthMonitoringInterval = {24}, SlowApiMonitoringInterval = {25}, QueueHealthWarningAtUsagePercent = {26}, EnableSlowIdleRestartForPeristed = {27}, EnableSlowIdleRestartForVolatile = {28}";
    size_t index = 0;

    traceEvent.AddEventField<bool>(format, name + ".allowmultiplequorumset", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".batchackinterval", index);
    traceEvent.AddEventField<int64>(format, name + ".completereplicatethread", index);
    traceEvent.AddEventField<int64>(format, name + ".initialcopyqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".initialreplicationqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxcopyqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxpendingacknowledgement", index);
    traceEvent.AddEventField<int64>(format, name + ".maxreplicationmessagesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxreplicationqueuememsize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxreplicationqueuesize", index);
    traceEvent.AddEventField<wstring>(format, name + ".replicatoraddress", index);
    traceEvent.AddEventField<bool>(format, name + ".requireserviceack", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".retryinterval", index);
    traceEvent.AddEventField<bool>(format, name + ".secondaryclearacknowledgedops", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".traceint", index);
    traceEvent.AddEventField<bool>(format, name + ".usestreamfaultsandeosack", index);
    traceEvent.AddEventField<int64>(format, name + ".initialsecondaryreplicationqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxsecondaryreplicationqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxsecondaryreplicationqueuememorysize", index);
    traceEvent.AddEventField<int64>(format, name + ".initialPrimaryreplicationqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxPrimaryreplicationqueuesize", index);
    traceEvent.AddEventField<int64>(format, name + ".maxPrimaryreplicationqueuememorysize", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".primarywaitforpendingquorumstimeout", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".queuefulltraceinterval", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".queuehealthmonitoringinterval", index);
    traceEvent.AddEventField<TimeSpan>(format, name + ".slowapimonitoringinterval", index);
    traceEvent.AddEventField<int64>(format, name + ".queuehealthwarningatusagepercent", index);
    traceEvent.AddEventField<bool>(format, name + ".enableslowidlerestartforpersisted", index);
    traceEvent.AddEventField<wstring>(format, name + ".enableslowidlerestartforvolatile", index);

    return format;
}

void REInternalSettings::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<bool>(this->AllowMultipleQuorumSet); 
    context.Write<TimeSpan>(this->BatchAcknowledgementInterval);
    context.WriteCopy<int64>(this->CompleteReplicateThreadCount);
    context.WriteCopy<int64>(this->InitialCopyQueueSize);
    context.WriteCopy<int64>(this->InitialReplicationQueueSize);
    context.WriteCopy<int64>(this->MaxCopyQueueSize);
    context.WriteCopy<int64>(this->MaxPendingAcknowledgements);
    context.WriteCopy<int64>(this->MaxReplicationMessageSize);
    context.WriteCopy<int64>(this->MaxReplicationQueueMemorySize);
    context.WriteCopy<int64>(this->MaxReplicationQueueSize);
    context.WriteCopy<wstring>(this->ReplicatorAddress);
    context.Write<bool>(this->RequireServiceAck);
    context.Write<TimeSpan>(this->RetryInterval);
    context.Write<bool>(this->SecondaryClearAcknowledgedOperations);
    context.Write<TimeSpan>(this->TraceInterval);        
    context.Write<bool>(this->UseStreamFaultsAndEndOfStreamOperationAck);
    context.WriteCopy<int64>(this->InitialSecondaryReplicationQueueSize);
    context.WriteCopy<int64>(this->MaxSecondaryReplicationQueueSize);
    context.WriteCopy<int64>(this->MaxSecondaryReplicationQueueMemorySize);
    context.WriteCopy<int64>(this->InitialPrimaryReplicationQueueSize);
    context.WriteCopy<int64>(this->MaxPrimaryReplicationQueueSize);
    context.WriteCopy<int64>(this->MaxPrimaryReplicationQueueMemorySize);
    context.Write<TimeSpan>(this->PrimaryWaitForPendingQuorumsTimeout);
    context.Write<TimeSpan>(this->QueueFullTraceInterval);
    context.Write<TimeSpan>(this->QueueHealthMonitoringInterval);
    context.Write<TimeSpan>(this->SlowApiMonitoringInterval);
    context.WriteCopy<int64>(this->QueueHealthWarningAtUsagePercent);
    context.Write<bool>(this->EnableSlowIdleRestartForPersisted);
    context.WriteCopy<wstring>(GetFormattedStringForTrailingSettingsToTrace());
}

} // end namespace ReplicationComponent
} // end namespace Reliability
