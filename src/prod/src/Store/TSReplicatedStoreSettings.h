// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Store
{
    class TSReplicatedStoreSettings 
        : public Api::IKeyValueStoreReplicaSettings_V2Result
        , public Common::ComponentRoot
        , public Common::TextTraceComponent<Common::TraceTaskCodes::ReplicatedStore>
    {
    public:
        TSReplicatedStoreSettings();

        TSReplicatedStoreSettings(
            std::wstring const & workingDirectory);

        TSReplicatedStoreSettings(
            std::wstring const & workingDirectory,
            TxnReplicator::KtlLoggerSharedLogSettingsUPtr &&);

        TSReplicatedStoreSettings(
            std::wstring const & workingDirectory,
            TxnReplicator::KtlLoggerSharedLogSettingsUPtr &&,
            TxnReplicator::TransactionalReplicatorSettingsUPtr &&);

        TSReplicatedStoreSettings(
            std::wstring const & workingDirectory,
            TxnReplicator::KtlLoggerSharedLogSettingsUPtr &&, 
            SecondaryNotificationMode::Enum const);

        TSReplicatedStoreSettings(
            std::wstring const & workingDirectory,
            TxnReplicator::KtlLoggerSharedLogSettingsUPtr &&,
            TxnReplicator::TransactionalReplicatorSettingsUPtr &&,
            SecondaryNotificationMode::Enum const);

        TSReplicatedStoreSettings(TSReplicatedStoreSettings const & other);

        __declspec(property(get=get_WorkingDirectory)) std::wstring const & WorkingDirectory;
        __declspec(property(get=get_SharedLogSettings)) TxnReplicator::KtlLoggerSharedLogSettingsUPtr const & SharedLogSettings;
        __declspec(property(get=get_TxnReplicatorSettings)) TxnReplicator::TransactionalReplicatorSettingsUPtr const & TxnReplicatorSettings;
        __declspec(property(get=get_NotificationMode)) SecondaryNotificationMode::Enum const & NotificationMode;
        __declspec(property(get=get_TStoreLockTimeout)) Common::TimeSpan const & TStoreLockTimeout;

        std::wstring const & get_WorkingDirectory() const { return workingDirectory_; }
        TxnReplicator::KtlLoggerSharedLogSettingsUPtr const & get_SharedLogSettings() const { return sharedLogSettings_; }
        TxnReplicator::TransactionalReplicatorSettingsUPtr const & get_TxnReplicatorSettings() const { return txnReplicatorSettings_; }
        SecondaryNotificationMode::Enum const & get_NotificationMode() const { return notificationMode_; }
        Common::TimeSpan const & get_TStoreLockTimeout() const { return tStoreLockTimeout_; }

        static Common::ErrorCode FromPublicApi(
            FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 const &, 
            __out std::unique_ptr<TSReplicatedStoreSettings> &);

        void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 &) const override;

        static Api::IKeyValueStoreReplicaSettings_V2ResultPtr GetKeyValueStoreReplicaDefaultSettings(
            std::wstring const & workingDirectory,
            std::wstring const & sharedLogDirectory,
            std::wstring const & sharedLogFilename,
            Common::Guid const & sharedLogGuid);

        Common::ErrorCode DeleteDatabaseFiles(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId);
        Common::ErrorCode DeleteDatabaseFiles(Common::Guid const & partitionId, FABRIC_REPLICA_ID replicaId, std::wstring const & sharedLogFilePath);
        Common::ErrorCode TraceAndGetError(std::wstring && msg, Common::ErrorCode const &);

    private:

        void InitializeCtor(std::wstring const & workingDirectory);

        //
        // public settings exposed to managed KVS
        //
        std::wstring workingDirectory_;
        TxnReplicator::KtlLoggerSharedLogSettingsUPtr sharedLogSettings_;
        TxnReplicator::TransactionalReplicatorSettingsUPtr txnReplicatorSettings_;
        SecondaryNotificationMode::Enum notificationMode_;

        //
        // internal settings
        //
        Common::TimeSpan tStoreLockTimeout_;
    };

    typedef std::unique_ptr<TSReplicatedStoreSettings> TSReplicatedStoreSettingsUPtr;
}
