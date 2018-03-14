// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
using namespace Common;
using namespace std;

namespace Store
{
    StringLiteral const TraceComponent("ReplicatedStoreSettings");    

    ReplicatedStoreSettings::ReplicatedStoreSettings()
    {
        this->InitializeCtor();
    }

    ReplicatedStoreSettings::ReplicatedStoreSettings(
        bool enableFlushOnDrain,
        SecondaryNotificationMode::Enum secondaryNotificationMode)
    {
        this->InitializeCtor();

        enableFlushOnDrain_ = enableFlushOnDrain;
        secondaryNotificationMode_ = secondaryNotificationMode;
    }

    ReplicatedStoreSettings::ReplicatedStoreSettings(
        int commitBatchingPeriod,
        int commitBatchingSizeLimit,
        int transactionLowWatermark,
        int transactionHighWatermark,
        int commitBatchingPeriodExtension,
        uint64 throttleReplicationQueueOperationCount,
        uint64 throttleReplicationQueueSizeBytes,
        bool enableFlushOnDrain,
        SecondaryNotificationMode::Enum secondaryNotificationMode)
    {
        this->InitializeCtor();

        commitBatchingPeriod_ = commitBatchingPeriod;
        commitBatchingSizeLimit_ = commitBatchingSizeLimit;
        transactionLowWatermark_ = transactionLowWatermark;
        transactionHighWatermark_ = transactionHighWatermark;
        commitBatchingPeriodExtension_ = commitBatchingPeriodExtension > 0 ? commitBatchingPeriodExtension : commitBatchingPeriod;
        throttleReplicationQueueOperationCount_ = throttleReplicationQueueOperationCount;
        throttleReplicationQueueSizeBytes_ = throttleReplicationQueueSizeBytes;
        enableFlushOnDrain_ = enableFlushOnDrain;
        secondaryNotificationMode_ = secondaryNotificationMode;
    }

    void ReplicatedStoreSettings::InitializeCtor()
    {
        // simple transactions disabled by default 
        //
        commitBatchingPeriod_ = 0;
        commitBatchingSizeLimit_ = 0;
        transactionLowWatermark_ = -1;
        transactionHighWatermark_ = -1;
        commitBatchingPeriodExtension_ = 0;

        // replication throttling disabled by default
        //
        throttleReplicationQueueOperationCount_ = 0;
        throttleReplicationQueueSizeBytes_ = 0;

        enableStreamFaults_ = StoreConfig::GetConfig().EnableEndOfStreamAckOverride;
        enableFlushOnDrain_ = StoreConfig::GetConfig().EnableSystemServiceFlushOnDrain;
        secondaryNotificationMode_ = SecondaryNotificationMode::None;
        transactionDrainTimeout_ = StoreConfig::GetConfig().TransactionDrainTimeout;
        enableCopyNotificationPrefetch_ = false;
        copyNotificationPrefetchTypes_.push_back(Constants::KeyValueStoreItemType); // Only support KVS prefetch
        enumerationPerfTraceThreshold_ = 0;
        enableTombstoneCleanup2_ = StoreConfig::GetConfig().EnableTombstoneCleanup2;
        fullCopyMode_ = FullCopyMode::Default;
        logTruncationInterval_ = TimeSpan::FromMinutes(StoreConfig::GetConfig().LogTruncationIntervalInMinutes);
    }

    ReplicatedStoreSettings & ReplicatedStoreSettings::operator=(ReplicatedStoreSettings const & other)
    {
        if (this != &other) 
        {
            commitBatchingPeriod_ = other.commitBatchingPeriod_;
            commitBatchingSizeLimit_ = other.commitBatchingSizeLimit_;
            transactionLowWatermark_ = other.transactionLowWatermark_;
            transactionHighWatermark_ = other.transactionHighWatermark_;
            commitBatchingPeriodExtension_ = other.commitBatchingPeriodExtension_;
            throttleReplicationQueueOperationCount_ = other.throttleReplicationQueueOperationCount_;
            throttleReplicationQueueSizeBytes_ = other.throttleReplicationQueueSizeBytes_;
            enableStreamFaults_ = other.enableStreamFaults_;
            enableFlushOnDrain_ = other.enableFlushOnDrain_;
            secondaryNotificationMode_ = other.secondaryNotificationMode_;
            transactionDrainTimeout_ = other.transactionDrainTimeout_;
            enableCopyNotificationPrefetch_ = other.enableCopyNotificationPrefetch_;
            copyNotificationPrefetchTypes_ = other.copyNotificationPrefetchTypes_;
            enumerationPerfTraceThreshold_ = other.enumerationPerfTraceThreshold_;
            enableTombstoneCleanup2_ = other.enableTombstoneCleanup2_;
            fullCopyMode_ = other.fullCopyMode_;
            logTruncationInterval_ = other.logTruncationInterval_;
        }

        return *this;
    }

    Api::IKeyValueStoreReplicaSettingsResultPtr ReplicatedStoreSettings::CreateDefaultSettings()
    {
        auto settings = make_shared<ReplicatedStoreSettings>();
        return RootedObjectPointer<Api::IKeyValueStoreReplicaSettingsResult>(settings.get(), settings->CreateComponentRoot());
    }

    ErrorCode ReplicatedStoreSettings::FromPublicApi(FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const & publicSettings)
    {
        secondaryNotificationMode_ = SecondaryNotificationMode::FromPublicApi(publicSettings.SecondaryNotificationMode);
        transactionDrainTimeout_ = TimeSpan::FromSeconds(publicSettings.TransactionDrainTimeoutInSeconds);

        if (publicSettings.Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto castedEx1 = static_cast<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX1*>(publicSettings.Reserved);

        enableCopyNotificationPrefetch_ = (castedEx1->EnableCopyNotificationPrefetch == TRUE);

        if (castedEx1->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto castedEx2 = static_cast<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX2*>(castedEx1->Reserved);

        auto error = FullCopyMode::FromPublicApi(castedEx2->FullCopyMode, fullCopyMode_);

        if (!error.IsSuccess())
        {
            return error;
        }

        if (castedEx2->Reserved != nullptr)
        {
            auto castedEx3 = static_cast<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX3*>(castedEx2->Reserved);
            logTruncationInterval_ = TimeSpan::FromMinutes(castedEx3->LogTruncationIntervalInMinutes);
        }
        
        return ErrorCodeValue::Success;
    }

    void ReplicatedStoreSettings::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS & publicSettings) const
    {
        publicSettings.SecondaryNotificationMode = SecondaryNotificationMode::ToPublicApi(secondaryNotificationMode_);
        publicSettings.TransactionDrainTimeoutInSeconds = static_cast<DWORD>(transactionDrainTimeout_.TotalSeconds());

        auto castedEx1 = heap.AddItem<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX1>();
        castedEx1->EnableCopyNotificationPrefetch = (enableCopyNotificationPrefetch_ ? TRUE : FALSE);

        auto castedEx2 = heap.AddItem<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX2>();
        castedEx2->FullCopyMode = FullCopyMode::ToPublicApi(fullCopyMode_);

        auto castedEx3 = heap.AddItem<FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_EX3>();
        castedEx3->LogTruncationIntervalInMinutes = static_cast<LONG>(logTruncationInterval_.TotalRoundedMinutes());

        publicSettings.Reserved = castedEx1.GetRawPointer();
        castedEx1->Reserved = castedEx2.GetRawPointer();
        castedEx2->Reserved = castedEx3.GetRawPointer();
    }

    void ReplicatedStoreSettings::EnsureReplicatorSettings(
        wstring const & traceId,
        Reliability::ReplicationComponent::ReplicatorSettingsUPtr const & replicatorSettings)
    {
        if (StoreConfig::GetConfig().EnableEndOfStreamAckOverride && !replicatorSettings->UseStreamFaultsAndEndOfStreamOperationAck)
        {
            Trace.WriteInfo(
                TraceComponent,
                "{0} ReplicatedStore/EnableEndOfStreamAckOverride is true - overriding EnableStreamFaultsAndEndOfStreamOperationAck on replicator settings",
                traceId);

            // Override must occur before creating replicator during open
            //
            replicatorSettings->UseStreamFaultsAndEndOfStreamOperationAck = true;
        }
        
        this->EnableStreamFaults = replicatorSettings->UseStreamFaultsAndEndOfStreamOperationAck;
    }
}
