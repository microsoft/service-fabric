// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace TxnReplicator;

namespace Store
{
    StringLiteral const TraceComponent("TSReplicatedStoreSettings");    

    TSReplicatedStoreSettings::TSReplicatedStoreSettings()
    {
        this->InitializeCtor(L"");
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(
        std::wstring const & workingDirectory)
    {
        this->InitializeCtor(workingDirectory);
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(
        std::wstring const & workingDirectory,
        KtlLoggerSharedLogSettingsUPtr && sharedLogSettings)
    {
        this->InitializeCtor(workingDirectory);

        sharedLogSettings_ = move(sharedLogSettings);
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(
        std::wstring const & workingDirectory,
        TxnReplicator::KtlLoggerSharedLogSettingsUPtr && sharedLogSettings,
        TxnReplicator::TransactionalReplicatorSettingsUPtr && txnReplicatorSettings)
    {
        this->InitializeCtor(workingDirectory);

        sharedLogSettings_ = move(sharedLogSettings);
        txnReplicatorSettings_ = move(txnReplicatorSettings);
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(
        std::wstring const & workingDirectory,
        KtlLoggerSharedLogSettingsUPtr && sharedLogSettings, 
        SecondaryNotificationMode::Enum const notificationMode)
    {
        this->InitializeCtor(workingDirectory);

        sharedLogSettings_ = move(sharedLogSettings);
        notificationMode_ = notificationMode;
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(
        std::wstring const & workingDirectory,
        TxnReplicator::KtlLoggerSharedLogSettingsUPtr && sharedLogSettings,
        TxnReplicator::TransactionalReplicatorSettingsUPtr && txnReplicatorSettings,
        SecondaryNotificationMode::Enum const notificationMode)
    {
        this->InitializeCtor(workingDirectory);

        sharedLogSettings_ = move(sharedLogSettings);
        txnReplicatorSettings_ = move(txnReplicatorSettings);
        notificationMode_ = notificationMode;
    }

    TSReplicatedStoreSettings::TSReplicatedStoreSettings(TSReplicatedStoreSettings const & other)
        : workingDirectory_(other.workingDirectory_)
        , sharedLogSettings_(other.sharedLogSettings_.get() != nullptr 
            ? make_unique<KtlLoggerSharedLogSettings>(*other.sharedLogSettings_)
            : nullptr)
        , notificationMode_(other.notificationMode_)
        , tStoreLockTimeout_(other.tStoreLockTimeout_)
    {
    }

    void TSReplicatedStoreSettings::InitializeCtor(std::wstring const & workingDirectory)
    {
        workingDirectory_ = workingDirectory;
        sharedLogSettings_ = nullptr;
        notificationMode_ = SecondaryNotificationMode::None;
        tStoreLockTimeout_ = StoreConfig::GetConfig().TStoreLockTimeout;
    }

    ErrorCode TSReplicatedStoreSettings::FromPublicApi(
        FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 const & publicSettings,
        __out unique_ptr<TSReplicatedStoreSettings> & result)
    {
        result = make_unique<TSReplicatedStoreSettings>();

        TRY_PARSE_PUBLIC_STRING( publicSettings.WorkingDirectory, result->workingDirectory_ );

        if (publicSettings.SharedLogSettings == nullptr)
        {
            WriteWarning(TraceComponent, "SharedLogSettings cannot be null");
            return ErrorCodeValue::ArgumentNull;
        }

        auto error = KtlLoggerSharedLogSettings::FromPublicApi(*publicSettings.SharedLogSettings, result->sharedLogSettings_);
        if (!error.IsSuccess())
        {
            return error;
        }

        result->notificationMode_ = SecondaryNotificationMode::FromPublicApi(publicSettings.SecondaryNotificationMode);
        
        return ErrorCodeValue::Success;
    }

    void TSReplicatedStoreSettings::ToPublicApi(
        __in ScopedHeap & heap, 
        __out FABRIC_KEY_VALUE_STORE_REPLICA_SETTINGS_V2 & publicSettings) const
    {
        if (sharedLogSettings_.get() != nullptr)
        {
            auto publicSharedLogSettings = heap.AddItem<KTLLOGGER_SHARED_LOG_SETTINGS>();
            sharedLogSettings_->ToPublicApi(heap, *publicSharedLogSettings);
            publicSettings.SharedLogSettings = publicSharedLogSettings.GetRawPointer();
        }
        else
        {
            publicSettings.SharedLogSettings = NULL;
        }

        publicSettings.WorkingDirectory = workingDirectory_.empty() ? NULL : heap.AddString(workingDirectory_);
        publicSettings.SecondaryNotificationMode = SecondaryNotificationMode::ToPublicApi(notificationMode_);
    }

    Api::IKeyValueStoreReplicaSettings_V2ResultPtr TSReplicatedStoreSettings::GetKeyValueStoreReplicaDefaultSettings(
            wstring const & workingDirectory,
            wstring const & sharedLogDirectory,
            wstring const & sharedLogFilename,
            Guid const & sharedLogGuid)
    {
        auto sharedLogSettings = KtlLoggerSharedLogSettings::GetKeyValueStoreReplicaDefaultSettings(
            workingDirectory,
            sharedLogDirectory,
            sharedLogFilename,
            sharedLogGuid);

        auto castedLogSettings = dynamic_cast<KtlLoggerSharedLogSettings*>(sharedLogSettings.get());

        auto settings = make_shared<TSReplicatedStoreSettings>(
            workingDirectory,
            make_unique<KtlLoggerSharedLogSettings>(*castedLogSettings));
        return RootedObjectPointer<Api::IKeyValueStoreReplicaSettings_V2Result>(settings.get(), settings->CreateComponentRoot());
    }
}
