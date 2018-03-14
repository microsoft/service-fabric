// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;

StringLiteral const TraceComponent("EseLocalStoreSettings");

#define READ_SETTING( SettingType, ParamName, FieldName ) \
{ \
    SettingType value; \
    bool hasValue = false; \
    auto error = Parser::ReadSettingsValue(configPackageCPtr, sectionName, ParamName, value, hasValue); \
    if (!error.IsSuccess()) { return error.ToHResult(); } \
    if (hasValue) \
    { \

#define READ_CONFIG_SETTING( SettingType, ParamName, FieldName ) \
    READ_SETTING( SettingType, cfg.ParamName##Entry.Key, FieldName )

#define SET_VALUE( FieldName ) \
        settingsSPtr->FieldName = value; \
    } \
} \

#define SET_CONVERTED_VALUE( FieldName, Conversion ) \
        settingsSPtr->FieldName = Conversion(value); \
    } \
} \

#define READ_INT_SETTING( ParamName, FieldName ) \
    READ_CONFIG_SETTING( int, ParamName, FieldName ) \
    SET_VALUE( FieldName ) \

#define READ_BOOL_SETTING( ParamName, FieldName ) \
    READ_SETTING( bool, ParamName, FieldName ) \
    SET_VALUE( FieldName ) \

#define READ_WSTR_SETTING( ParamName, FieldName ) \
    READ_SETTING( wstring, ParamName, FieldName ) \
    SET_VALUE( FieldName ) \

#define READ_TIMESPAN_MILLIS_SETTING( ParamName, FieldName ) \
    READ_CONFIG_SETTING( int, ParamName, FieldName ) \
    SET_CONVERTED_VALUE( FieldName, TimeSpan::FromMilliseconds ) \

#define READ_TIMESPAN_MINUTES_SETTING( ParamName, FieldName ) \
    READ_CONFIG_SETTING( int, ParamName, FieldName ) \
    SET_CONVERTED_VALUE( FieldName, TimeSpan::FromMinutes ) \

namespace Store
{
    EseLocalStoreSettings::EseLocalStoreSettings()
    {
        this->InitializeCtor(L"", L"");
    }

    EseLocalStoreSettings::EseLocalStoreSettings(
        wstring const & storeName,
        wstring const & dbFolderPath,
        bool enableIncrementalBackup)
    {
        this->InitializeCtor(storeName, dbFolderPath);

        enableIncrementalBackup_ = enableIncrementalBackup;
    }

    EseLocalStoreSettings::EseLocalStoreSettings(
        wstring const & storeName,
        wstring const & dbFolderPath,
        int compactionThresholdInMB)
    {
        this->InitializeCtor(storeName, dbFolderPath);

        compactionThresholdInMB_ = compactionThresholdInMB;
    }

    EseLocalStoreSettings::EseLocalStoreSettings(
        wstring const & storeName,
        wstring const & dbFolderPath,
        int compactionThresholdInMB,
        int maxCursors)
    {
        this->InitializeCtor(storeName, dbFolderPath);

        compactionThresholdInMB_ = compactionThresholdInMB;
        maxCursors_ = maxCursors;
    }

    EseLocalStoreSettings::EseLocalStoreSettings(
        wstring const & storeName,
        wstring const & dbFolderPath,
        int compactionThresholdInMB,
        int maxCursors,
        int maxCacheSizeInMB)
    {
        this->InitializeCtor(storeName, dbFolderPath);

        compactionThresholdInMB_ = compactionThresholdInMB;
        maxCursors_ = maxCursors;
        maxCacheSizeInMB_ = maxCacheSizeInMB;
    }

    EseLocalStoreSettings::EseLocalStoreSettings(
        wstring const & storeName,
        wstring const & dbFolderPath,
        int compactionThresholdInMB,
        int maxCursors,
        int maxCacheSizeInMB,
        int minCacheSizeInMB)
    {
        this->InitializeCtor(storeName, dbFolderPath);

        compactionThresholdInMB_ = compactionThresholdInMB;
        maxCursors_ = maxCursors;
        maxCacheSizeInMB_ = maxCacheSizeInMB;
        minCacheSizeInMB_ = minCacheSizeInMB;
    }


    void EseLocalStoreSettings::InitializeCtor(wstring const & storeName, wstring const & dbFolderPath)
    {
        storeName_ = storeName;
        dbFolderPath_ = dbFolderPath;
        logFileSizeInKB_ = StoreConfig::GetConfig().EseLogFileSizeInKB;
        logBufferSizeInKB_ = StoreConfig::GetConfig().EseLogBufferSizeInKB;
        maxCursors_ = StoreConfig::GetConfig().MaxCursors;
        maxVerPages_ = StoreConfig::GetConfig().MaxVerPages;
        maxAsyncCommitDelay_ = TimeSpan::FromMilliseconds(StoreConfig::GetConfig().MaxAsyncCommitDelayInMilliseconds);
        enableIncrementalBackup_ = false;
        maxCacheSizeInMB_ = StoreConfig::GetConfig().MaxCacheSizeInMB;
        minCacheSizeInMB_ = StoreConfig::GetConfig().MinCacheSizeInMB;
        optimizeTableScans_ = false;
        maxDefragFrequency_ = TimeSpan::FromMinutes(StoreConfig::GetConfig().MaxDefragFrequencyInMinutes);
        defragThresholdInMB_ = StoreConfig::GetConfig().DefragThresholdInMB;
        dbPageSizeInKB_ = StoreConfig::GetConfig().DatabasePageSizeInKB;
        compactionThresholdInMB_ = StoreConfig::GetConfig().CompactionThresholdInMB;
        intrinsicValueThresholdInBytes_ = StoreConfig::GetConfig().IntrinsicValueThresholdInBytes;
        assertOnFatalError_ = StoreConfig::GetConfig().AssertOnFatalError;
        enableOverwriteOnUpdate_ = StoreConfig::GetConfig().EnableOverwriteOnUpdate;
    }

    HRESULT EseLocalStoreSettings::FromConfig(
        __in IFabricCodePackageActivationContext const * codePackageActivationContextPtr,
        __in LPCWSTR configurationPackageNamePtr,
        __in LPCWSTR sectionNamePtr,
        __out Api::IEseLocalStoreSettingsResultPtr & settingsResult)
    {
        if (codePackageActivationContextPtr == NULL) 
        { 
            WriteWarning(TraceComponent, "IFabricCodePackageActivationContext is NULL");
            return E_POINTER; 
        }
        
        ComPointer<IFabricConfigurationPackage> configPackageCPtr;
        auto hr = const_cast<IFabricCodePackageActivationContext *>(codePackageActivationContextPtr)->GetConfigurationPackage(
            configurationPackageNamePtr,
            configPackageCPtr.InitializationAddress());

        if (FAILED(hr)) 
        { 
            WriteWarning(TraceComponent, "GetConfigurationPackage failed: hr={0}", hr);
            return hr; 
        }
        else if (configPackageCPtr.GetRawPointer() == NULL)
        {
            WriteWarning(TraceComponent, "IFabricConfigurationPackage is NULL");
            return E_POINTER;
        }

        wstring configurationPackageName;
        hr = StringUtility::LpcwstrToWstring(configurationPackageNamePtr, false, configurationPackageName);
        if (FAILED(hr))
        {
            WriteWarning(TraceComponent, "failed to convert configurationPackageName: hr={0}", hr);
            return hr; 
        }

        wstring sectionName;
        hr = StringUtility::LpcwstrToWstring(sectionNamePtr, false, sectionName);
        if (FAILED(hr))
        {
            WriteWarning(TraceComponent, "failed to convert sectionName: hr={0}", hr);
            return hr; 
        }

        auto const & cfg = StoreConfig::GetConfig();
        auto settingsSPtr = make_shared<EseLocalStoreSettings>();

        // These settings appear in the cluster manifest and are set system wide, but
        // can be overridden by the application at runtime.
        //
        READ_INT_SETTING( EseLogFileSizeInKB, logFileSizeInKB_ )
        READ_INT_SETTING( EseLogBufferSizeInKB, logBufferSizeInKB_ )
        READ_INT_SETTING( MaxCursors, maxCursors_ )
        READ_INT_SETTING( MaxVerPages, maxVerPages_ )
        READ_INT_SETTING( MaxCacheSizeInMB, maxCacheSizeInMB_ )
        READ_INT_SETTING( MinCacheSizeInMB, minCacheSizeInMB_ )
        READ_INT_SETTING( DatabasePageSizeInKB, dbPageSizeInKB_ )
        READ_INT_SETTING( DefragThresholdInMB, defragThresholdInMB_ )
        READ_INT_SETTING( CompactionThresholdInMB, compactionThresholdInMB_ )
        READ_INT_SETTING( IntrinsicValueThresholdInBytes, intrinsicValueThresholdInBytes_ )
        READ_TIMESPAN_MILLIS_SETTING( MaxAsyncCommitDelayInMilliseconds, maxAsyncCommitDelay_ )
        READ_TIMESPAN_MINUTES_SETTING( MaxDefragFrequencyInMinutes, maxDefragFrequency_ )

        // These settings do not appear in the cluster manifest but can still be loaded 
        // through the application's config package.
        //
        READ_WSTR_SETTING( L"DbFolderPath", dbFolderPath_ )
        READ_BOOL_SETTING( L"EnableIncrementalBackup", enableIncrementalBackup_ )
        READ_BOOL_SETTING( L"EnableOverwriteOnUpdate", enableOverwriteOnUpdate_ )

        settingsResult = RootedObjectPointer<IEseLocalStoreSettingsResult>(
            settingsSPtr.get(), 
            settingsSPtr->CreateComponentRoot());

        return S_OK;
    }

    ErrorCode EseLocalStoreSettings::FromPublicApi(FABRIC_ESE_LOCAL_STORE_SETTINGS const & publicSettings)
    {
        if (publicSettings.DbFolderPath != NULL)
        {
            HRESULT hr = StringUtility::LpcwstrToWstring(publicSettings.DbFolderPath, false, dbFolderPath_);

            if (FAILED(hr))
            {
                return ErrorCode::FromHResult(hr);
            }
        }
        else
        {
            dbFolderPath_ = std::wstring(Directory::GetCurrentDirectoryW());
        }

        // Keep default settings from config if value is <= 0
        if (publicSettings.LogFileSizeInKB > 0)
        {
            logFileSizeInKB_ = publicSettings.LogFileSizeInKB;
        }

        if (publicSettings.LogBufferSizeInKB > 0)
        {
            logBufferSizeInKB_ = publicSettings.LogBufferSizeInKB;
        }

        if (publicSettings.MaxCursors > 0)
        {
            maxCursors_ = publicSettings.MaxCursors;
        }

        if (publicSettings.MaxVerPages > 0)
        {
            maxVerPages_ = publicSettings.MaxVerPages;
        }

        if (publicSettings.MaxAsyncCommitDelayInMilliseconds > 0)
        {
            maxAsyncCommitDelay_ = TimeSpan::FromMilliseconds(publicSettings.MaxAsyncCommitDelayInMilliseconds);
        }
        
        if (publicSettings.Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex1Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX1*>(publicSettings.Reserved);
        enableIncrementalBackup_ = ex1Settings->EnableIncrementalBackup ? true : false;

        if (ex1Settings->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex2Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX2*>(ex1Settings->Reserved);

        if (ex2Settings->MaxCacheSizeInMB > 0)
        {
            maxCacheSizeInMB_ = ex2Settings->MaxCacheSizeInMB;
        }

        //
        // MinCacheSizeInMB is currently not exposed to user services.
        // Opportunistically expose it whenever the public struct
        // gets extended for a new required setting
        //

        if (ex2Settings->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex3Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX3*>(ex2Settings->Reserved);

        if (ex3Settings->MaxDefragFrequencyInMinutes > 0)
        {
            maxDefragFrequency_ = TimeSpan::FromMinutes(ex3Settings->MaxDefragFrequencyInMinutes);
        }

        // < 0: disable defragmentation
        // = 0: use default from config
        // > 0: valid override value
        //
        if (ex3Settings->DefragThresholdInMB != 0)
        {
            defragThresholdInMB_ = ex3Settings->DefragThresholdInMB;
        }

        if (ex3Settings->DatabasePageSizeInKB > 0)
        {
            dbPageSizeInKB_ = ex3Settings->DatabasePageSizeInKB;
        }

        if (ex3Settings->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex4Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX4*>(ex3Settings->Reserved);

        if (ex4Settings->CompactionThresholdInMB > 0)
        {
            compactionThresholdInMB_ = ex4Settings->CompactionThresholdInMB;
        }

        if (ex4Settings->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex5Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX5*>(ex4Settings->Reserved);

        if (ex5Settings->IntrinsicValueThresholdInBytes > 0)
        {
            intrinsicValueThresholdInBytes_ = ex5Settings->IntrinsicValueThresholdInBytes;
        }

        if (ex5Settings->Reserved == nullptr)
        {
            return ErrorCodeValue::Success;
        }

        auto ex6Settings = static_cast<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX6*>(ex5Settings->Reserved);
        enableOverwriteOnUpdate_ = ex6Settings->EnableOverwriteOnUpdate ? true : false;
        
        return ErrorCodeValue::Success;
    }

    void EseLocalStoreSettings::ToPublicApi(__in ScopedHeap & heap, __out FABRIC_ESE_LOCAL_STORE_SETTINGS & publicSettings) const
    {
        publicSettings.DbFolderPath = heap.AddString(dbFolderPath_, true); // nullIfEmpty = true
        publicSettings.LogFileSizeInKB = logFileSizeInKB_;
        publicSettings.LogBufferSizeInKB = logBufferSizeInKB_;
        publicSettings.MaxCursors = maxCursors_;
        publicSettings.MaxVerPages = maxVerPages_;
        publicSettings.MaxAsyncCommitDelayInMilliseconds = static_cast<LONG>(maxAsyncCommitDelay_.TotalMilliseconds());

        ReferencePointer<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX1> ex1Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX1>();
        ex1Settings->EnableIncrementalBackup = enableIncrementalBackup_;

        publicSettings.Reserved = ex1Settings.GetRawPointer();

        auto ex2Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX2>();
        ex2Settings->MaxCacheSizeInMB = maxCacheSizeInMB_;

        ex1Settings->Reserved = ex2Settings.GetRawPointer();

        auto ex3Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX3>();
        ex3Settings->MaxDefragFrequencyInMinutes = static_cast<LONG>(maxDefragFrequency_.TotalRoundedMinutes());
        ex3Settings->DefragThresholdInMB = defragThresholdInMB_;
        ex3Settings->DatabasePageSizeInKB = dbPageSizeInKB_;

        ex2Settings->Reserved = ex3Settings.GetRawPointer();

        auto ex4Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX4>();
        ex4Settings->CompactionThresholdInMB = compactionThresholdInMB_;

        ex3Settings->Reserved = ex4Settings.GetRawPointer();

        auto ex5Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX5>();
        ex5Settings->IntrinsicValueThresholdInBytes = intrinsicValueThresholdInBytes_;

        ex4Settings->Reserved = ex5Settings.GetRawPointer();

        auto ex6Settings = heap.AddItem<FABRIC_ESE_LOCAL_STORE_SETTINGS_EX6>();
        ex6Settings->EnableOverwriteOnUpdate = enableOverwriteOnUpdate_;

        ex5Settings->Reserved = ex6Settings.GetRawPointer();
    }

    void EseLocalStoreSettings::WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
    {
        w << L"LogFileSizeInKB=" << logFileSizeInKB_ << L" ";
        w << L"LogBufferSizeInKB=" << logBufferSizeInKB_ << L" ";
        w << L"MaxCursors=" << maxCursors_ << L" ";
        w << L"MaxVerPages=" << maxVerPages_ << L" ";
        w << L"MaxAsyncCommitDelay=" << maxAsyncCommitDelay_ << L" ";
        w << L"EnableIncrementalBackup=" << enableIncrementalBackup_ << L" ";
        w << L"MaxCacheSizeInMB=" << maxCacheSizeInMB_ << L" ";
        w << L"MinCacheSizeInMB=" << minCacheSizeInMB_ << L" ";
        w << L"OptimizeTableScans=" << optimizeTableScans_ << L" ";
        w << L"MaxDefragFrequency=" << maxDefragFrequency_ << L" ";
        w << L"DefragThresholdInMB=" << defragThresholdInMB_ << L" ";
        w << L"DatabasePageSizeInKB=" << dbPageSizeInKB_ << L" ";
        w << L"CompactionThresholdInMB=" << compactionThresholdInMB_ << L" ";
        w << L"IntrinsicValueThresholdInBytes=" << intrinsicValueThresholdInBytes_ << L" ";
        w << L"AssertOnFatalError=" << assertOnFatalError_ << L" ";
        w << L"EnableOverwriteOnUpdate=" << enableOverwriteOnUpdate_ << L" ";
    }
}
