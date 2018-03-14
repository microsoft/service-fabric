// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class ReplicatedStoreSettings
        : public Api::IKeyValueStoreReplicaSettingsResult
        , public Common::ComponentRoot
    {
    public:
        ReplicatedStoreSettings();
        ReplicatedStoreSettings(
            bool enableFlushOnDrain,
            SecondaryNotificationMode::Enum = SecondaryNotificationMode::None);
        ReplicatedStoreSettings(
            int commitBatchingPeriod,
            int commitBatchingSizeLimit,
            int transactionLowWatermark,
            int transactionHighWatermark,
            int commitBatchingPeriodExtension,
            uint64 throttleReplicationQueueOperationCount,
            uint64 throttleReplicationQueueSizeBytes,
            bool enableFlushOnDrain,
            SecondaryNotificationMode::Enum = SecondaryNotificationMode::None);

    private:

        void InitializeCtor();

    public:

        __declspec(property(get=get_CommitBatchingPeriod,put=set_CommitBatchingPeriod)) int CommitBatchingPeriod;
        __declspec(property(get=get_CommitBatchingSizeLimit)) int CommitBatchingSizeLimit;
        __declspec(property(get=get_TransactionLowWatermark)) int TransactionLowWatermark;
        __declspec(property(get=get_TransactionHighWatermark)) int TransactionHighWatermark;
        __declspec(property(get=get_CommitBatchingPeriodExtension)) int CommitBatchingPeriodExtension;
        __declspec(property(get=get_ThrottleReplicationQueueOperationCount)) uint64 ThrottleReplicationQueueOperationCount;
        __declspec(property(get=get_ThrottleReplicationQueueSizeBytes)) uint64 ThrottleReplicationQueueSizeBytes;
        __declspec(property(get=get_EnableStreamFaults,put=set_EnableStreamFaults)) bool EnableStreamFaults;
        __declspec(property(get=get_EnableFlushOnDrain,put=set_EnableFlushOnDrain)) bool EnableFlushOnDrain;
        __declspec(property(get=get_NotificationMode,put=set_NotificationMode)) SecondaryNotificationMode::Enum NotificationMode;
        __declspec(property(get=get_TransactionDrainTimeout)) Common::TimeSpan const & TransactionDrainTimeout;
        __declspec(property(get=get_EnableCopyNotificationPrefetch)) bool EnableCopyNotificationPrefetch;
        __declspec(property(get=get_CopyNotificationPrefetchTypes)) std::vector<std::wstring> const & CopyNotificationPrefetchTypes;
        __declspec(property(get=get_EnumerationPerfTraceThreshold,put=set_EnumerationPerfTraceThreshold)) int EnumerationPerfTraceThreshold;
        __declspec(property(get=get_EnableTombstoneCleanup2,put=set_EnableTombstoneCleanup2)) bool EnableTombstoneCleanup2;
        __declspec(property(get=get_FullCopyMode,put=set_FullCopyMode)) FullCopyMode::Enum FullCopyMode;
        __declspec(property(get=get_LogTruncationInterval, put=set_LogTruncationInterval)) Common::TimeSpan LogTruncationInterval;

        int get_CommitBatchingPeriod() const { return commitBatchingPeriod_; }
        int get_CommitBatchingSizeLimit() const { return commitBatchingSizeLimit_; }
        int get_TransactionLowWatermark() const { return transactionLowWatermark_; }
        int get_TransactionHighWatermark() const { return transactionHighWatermark_; }
        int get_CommitBatchingPeriodExtension() const { return commitBatchingPeriodExtension_; }
        uint64 get_ThrottleReplicationQueueOperationCount() const { return throttleReplicationQueueOperationCount_; }
        uint64 get_ThrottleReplicationQueueSizeBytes() const { return throttleReplicationQueueSizeBytes_; }
        bool get_EnableStreamFaults() const { return enableStreamFaults_; }
        bool get_EnableFlushOnDrain() const { return enableFlushOnDrain_; }
        SecondaryNotificationMode::Enum get_NotificationMode() const { return secondaryNotificationMode_; }
        Common::TimeSpan const & get_TransactionDrainTimeout() const { return transactionDrainTimeout_; }
        bool get_EnableCopyNotificationPrefetch() const { return enableCopyNotificationPrefetch_; }
        std::vector<std::wstring> const & get_CopyNotificationPrefetchTypes() const { return copyNotificationPrefetchTypes_; }
        int get_EnumerationPerfTraceThreshold() const { return enumerationPerfTraceThreshold_; }
        bool get_EnableTombstoneCleanup2() const { return enableTombstoneCleanup2_; }
        FullCopyMode::Enum get_FullCopyMode() const { return fullCopyMode_; }
        Common::TimeSpan get_LogTruncationInterval() { return logTruncationInterval_; }

        void set_CommitBatchingPeriod(int value) { commitBatchingPeriod_ = value; }
        void set_EnableStreamFaults(bool value) { enableStreamFaults_ = value; }
        void set_EnableFlushOnDrain(bool value) { enableFlushOnDrain_ = value; }
        void set_NotificationMode(SecondaryNotificationMode::Enum value) { secondaryNotificationMode_ = value; }
        void set_EnumerationPerfTraceThreshold(int value) { enumerationPerfTraceThreshold_ = value; }
        void set_EnableTombstoneCleanup2(bool value) { enableTombstoneCleanup2_ = value; }
        void set_FullCopyMode(FullCopyMode::Enum value) { fullCopyMode_ = value; }
        void set_LogTruncationInterval(Common::TimeSpan value) { logTruncationInterval_ = value; }

        ReplicatedStoreSettings & operator=(ReplicatedStoreSettings const &); 

        static Api::IKeyValueStoreReplicaSettingsResultPtr CreateDefaultSettings();
        Common::ErrorCode FromPublicApi(FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS const &);
        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS &) const override;

    public:

        void EnsureReplicatorSettings(std::wstring const & traceId, Reliability::ReplicationComponent::ReplicatorSettingsUPtr const &);

    private:
        int commitBatchingPeriod_;
        int commitBatchingSizeLimit_;
        int transactionLowWatermark_;
        int transactionHighWatermark_;
        int commitBatchingPeriodExtension_;
        uint64 throttleReplicationQueueOperationCount_;
        uint64 throttleReplicationQueueSizeBytes_;
        bool enableStreamFaults_;
        bool enableFlushOnDrain_;
        SecondaryNotificationMode::Enum secondaryNotificationMode_;
        Common::TimeSpan transactionDrainTimeout_;
        bool enableCopyNotificationPrefetch_;
        std::vector<std::wstring> copyNotificationPrefetchTypes_;
        int enumerationPerfTraceThreshold_;
        bool enableTombstoneCleanup2_;
        FullCopyMode::Enum fullCopyMode_;
        Common::TimeSpan logTruncationInterval_;
    };
}
