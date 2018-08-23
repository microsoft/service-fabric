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

    ErrorCode TSReplicatedStoreSettings::DeleteDatabaseFiles(Guid const & partitionId, FABRIC_REPLICA_ID replicaId)
    {
        return DeleteDatabaseFiles(partitionId, replicaId, L"");
    }

    // Since this function blindly deletes the shared log file, it should only be used by the migration workflow to clean up old copies of 
    // the database files that might exist. Replicas should continue to use ILocalStore::Cleanup to properly drop replicas.
    //
    ErrorCode TSReplicatedStoreSettings::DeleteDatabaseFiles(Guid const & partitionId, FABRIC_REPLICA_ID replicaId, wstring const & sharedLogFilePathArg)
    {
        auto sharedLogFilePath = sharedLogFilePathArg.empty() ? this->SharedLogSettings->ContainerPath : sharedLogFilePathArg;
        auto checkpointsFolder = Path::Combine(this->WorkingDirectory, wformatString("{0}_{1}", partitionId, replicaId));
        auto dedicatedLogFolder = this->WorkingDirectory;
        auto dedicatedLogFilePattern = L"*.SFLog";

        if (File::Exists(sharedLogFilePath))
        {
            auto error = File::Delete2(sharedLogFilePath);
            if (error.IsSuccess())
            {
                WriteInfo(TraceComponent, "deleted existing {0}", sharedLogFilePath);
            }
            else
            {
                return TraceAndGetError(wformatString("failed to delete existing {0}", sharedLogFilePath), error);
            }
        }
        else
        {
            WriteInfo(TraceComponent, "{0} already does not exist", sharedLogFilePath);
        }

        if (Directory::Exists(checkpointsFolder))
        {
            auto error = Directory::Delete(checkpointsFolder, true); // recursive
            if (error.IsSuccess())
            {
                WriteInfo(TraceComponent, "deleted existing {0}", checkpointsFolder);
            }
            else
            {
                return TraceAndGetError(wformatString("failed to delete existing {0}", checkpointsFolder), error);
            }
        }
        else
        {
            WriteInfo(TraceComponent, "{0} already does not exist", checkpointsFolder);
        }

        // fullPath - true
        // topDirectoryOnly - true
        //
        for (auto const & dedicatedLogFilePath : Directory::GetFiles(dedicatedLogFolder, dedicatedLogFilePattern, true, true))
        {
            auto error = File::Delete2(dedicatedLogFilePath);
            if (error.IsSuccess())
            {
                WriteInfo(TraceComponent, "deleted existing {0}", dedicatedLogFilePath);
            }
            else
            {
                return TraceAndGetError(wformatString("failed to delete existing {0}", dedicatedLogFilePath), error);
            }
        }

        return ErrorCodeValue::Success;
    }

    ErrorCode TSReplicatedStoreSettings::TraceAndGetError(wstring && msg, ErrorCode const & error)
    {
        if (!error.Message.empty())
        {
            msg.append(L"; ");
            msg.append(error.Message);
        }

        WriteWarning(TraceComponent, "{0}: {1}", msg, error);

        return ErrorCode(error.ReadValue(), move(msg));
    }
}
