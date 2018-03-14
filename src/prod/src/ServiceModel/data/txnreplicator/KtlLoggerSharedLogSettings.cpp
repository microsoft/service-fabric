// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace TxnReplicator;
using namespace ServiceModel;

GlobalWString const KtlLoggerSharedLogSettings::WindowsPathPrefix = make_global<wstring>(L"\\??\\");
GlobalWString const KtlLoggerSharedLogSettings::MockSharedLogPathForContainers = make_global<wstring>(L"X:\\mock_container_shared_log_path");
StringLiteral const TraceSource("KtlLoggerSharedLogSettings");

KtlLoggerSharedLogSettings::KtlLoggerSharedLogSettings()
    : containerPath_(L"")
    , containerId_(L"")
    , logSize_(0)
    , maximumNumberStreams_(0)
    , maximumRecordSize_(0)
    , createFlags_(0)
    , hashSharedLogIdFromPath_(false)
{
}

KtlLoggerSharedLogSettings::KtlLoggerSharedLogSettings(KtlLoggerSharedLogSettings const & other)
    : containerPath_(other.containerPath_)
    , containerId_(other.containerId_)
    , logSize_(other.logSize_)
    , maximumNumberStreams_(other.maximumNumberStreams_)
    , maximumRecordSize_(other.maximumRecordSize_)
    , createFlags_(other.createFlags_)
    , hashSharedLogIdFromPath_(other.hashSharedLogIdFromPath_)
{
}

ErrorCode KtlLoggerSharedLogSettings::FromPublicApi(
    __in KTLLOGGER_SHARED_LOG_SETTINGS const & publicSettings,
    __out KtlLoggerSharedLogSettingsUPtr & output)
{
    auto created = unique_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings());

    TRY_PARSE_PUBLIC_STRING_OUT(publicSettings.ContainerPath, created->containerPath_, true) // AllowNull
    TRY_PARSE_PUBLIC_STRING_OUT(publicSettings.ContainerId, created->containerId_, true) // AllowNull

    created->logSize_ = publicSettings.LogSize;
    created->maximumNumberStreams_ = publicSettings.MaximumNumberStreams;
    created->maximumRecordSize_ = publicSettings.MaximumRecordSize;
    created->createFlags_ = publicSettings.CreateFlags;
    created->hashSharedLogIdFromPath_ = false; // for testing only

    return MoveIfValid(move(created), output);
}

HRESULT KtlLoggerSharedLogSettings::FromConfig(
    __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in std::wstring const & configurationPackageName,
    __in std::wstring const & sectionName,
    __out IFabricSharedLogSettingsResult ** sharedLogSettingsResult)
{
    // Validate code package activation context is not null
    if (codePackageActivationContextCPtr.GetRawPointer() == NULL)
    {
        Trace.WriteError(TraceSource, "CodePackageActivationContext is NULL");
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    ComPointer<IFabricConfigurationPackage> configPackageCPtr;
    HRESULT hr = codePackageActivationContextCPtr->GetConfigurationPackage(
        configurationPackageName.c_str(),
        configPackageCPtr.InitializationAddress());

    if (FAILED(hr))
    {
        Trace.WriteError(TraceSource, "Failed to load Settings configuration package - {0}, HRESULT = {1}", configurationPackageName, hr);
        return ComUtility::OnPublicApiReturn(hr);
    }
    else if (configPackageCPtr.GetRawPointer() == NULL)
    {
        Trace.WriteError(TraceSource, "IFabricConfigurationPackage is NULL - {0}, HRESULT = {1}", configurationPackageName, hr);
        return ComUtility::OnPublicApiReturn(E_POINTER);
    }

    KtlLoggerSharedLogSettingsUPtr settings;
    hr = KtlLoggerSharedLogSettings::ReadFromConfigurationPackage(
        codePackageActivationContextCPtr,
        configPackageCPtr,
        sectionName,
        settings).ToHResult();

    if (hr == S_OK)
    {
        ASSERT_IFNOT(settings, "SharedLog settings cannot be null if validation succeeded");
        auto settingsSptr = shared_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings(*settings));
        auto isharedLogSettingsResult = RootedObjectPointer<Api::ISharedLogSettingsResult>(settingsSptr.get(), settingsSptr->CreateComponentRoot());
        auto comSharedLogSettingsResult = make_com<Api::ComSharedLogSettingsResult, IFabricSharedLogSettingsResult>(isharedLogSettingsResult);
        *sharedLogSettingsResult = comSharedLogSettingsResult.DetachNoRelease();
    }

    return ComUtility::OnPublicApiReturn(hr);
}

// Get sharedLog settings from configuration package after merging with cluster config settings
Common::ErrorCode KtlLoggerSharedLogSettings::ReadFromConfigurationPackage(
    __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
    __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
    __in std::wstring const & sectionName,
    __out KtlLoggerSharedLogSettingsUPtr & output)
{
    ASSERT_IF(codePackageActivationContextCPtr.GetRawPointer() == NULL, "activationContext is null");
    ASSERT_IF(configPackageCPtr.GetRawPointer() == NULL, "configPackageCPtr is null");

    auto created = unique_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings());
    KtlLoggerSharedLogSettings::GetDefaultSettingsFromClusterConfig(*created);
    KtlLoggerSharedLogSettings temp;

    bool hasValue = false;

    SLConfigValues configNames; // Dummy object used to retrieve key names of settings

    // ContainerId
    ErrorCode error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.ContainerIdEntry.Key,
        temp.containerId_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->containerId_ = temp.containerId_; }

    // ContainerPath
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.ContainerPathEntry.Key,
        temp.containerPath_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->containerPath_ = temp.containerPath_; }

    // CreateFlags
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.CreateFlagsEntry.Key,
        (int&)temp.createFlags_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->createFlags_ = temp.createFlags_; }

    // EnableHashSharedLogIdFromPath
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.EnableHashSharedLogIdFromPathEntry.Key,
        temp.hashSharedLogIdFromPath_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->hashSharedLogIdFromPath_ = temp.hashSharedLogIdFromPath_; }

    // LogSize
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.LogSizeEntry.Key,
        temp.logSize_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->logSize_ = temp.logSize_; }

    // MaximumNumberStreams
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaximumNumberStreamsEntry.Key,
        temp.maximumNumberStreams_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) { created->maximumNumberStreams_ = temp.maximumNumberStreams_; }

    // MaxRecordSize
    error = Parser::ReadSettingsValue(
        configPackageCPtr,
        sectionName,
        configNames.MaximumRecordSizeEntry.Key,
        temp.maximumRecordSize_,
        hasValue);

    if (!error.IsSuccess()) { return error; }
    if (hasValue) {	created->maximumRecordSize_ = temp.maximumRecordSize_; }

    return MoveIfValid(
        move(created),
        output);
}

void KtlLoggerSharedLogSettings::ToPublicApi(
    __in ScopedHeap & heap,
    __out KTLLOGGER_SHARED_LOG_SETTINGS & publicSettings) const
{
    publicSettings.ContainerPath = heap.AddString(containerPath_);
    publicSettings.ContainerId = heap.AddString(containerId_);
    publicSettings.LogSize = logSize_;
    publicSettings.MaximumNumberStreams = maximumNumberStreams_;
    publicSettings.MaximumRecordSize = maximumRecordSize_;
    publicSettings.CreateFlags = createFlags_;
    // publicSettings.HashSharedLogIdFromPath = hashSharedLogIdFromPath_; // for testing only
}

ErrorCode KtlLoggerSharedLogSettings::FromConfig(
    __in SLConfigBase const & configBase,
    __out KtlLoggerSharedLogSettingsUPtr & result)
{
    result = unique_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings());

    SLConfigValues configValues;
    configBase.GetTransactionalReplicatorSharedLogSettingsStructValues(configValues);

    result->containerPath_ = configValues.ContainerPath;
    result->containerId_ = configValues.ContainerId;
    result->logSize_ = configValues.LogSize;
    result->maximumNumberStreams_ = configValues.MaximumNumberStreams;
    result->maximumRecordSize_ = configValues.MaximumRecordSize;
    result->createFlags_ = configValues.CreateFlags;
    result->hashSharedLogIdFromPath_ = configValues.EnableHashSharedLogIdFromPath;

    return ErrorCodeValue::Success;
}

ErrorCode KtlLoggerSharedLogSettings::CreateForSystemService(
    __in SLConfigBase const & configBase,
    wstring const & workingDirectory,
    wstring const & sharedLogDirectory,
    wstring const & sharedLogFilename,
    Common::Guid const & logId,
    __out KtlLoggerSharedLogSettingsUPtr & result)
{
    auto error = KtlLoggerSharedLogSettings::FromConfig(configBase, result);
    if (!error.IsSuccess()) { return error; }

    if (result->ContainerPath.empty())
    {
        wstring sharedLogPath;
        error = GetSharedLogFullPath(
            workingDirectory,
            sharedLogDirectory,
            sharedLogFilename,
            sharedLogPath);
        if (!error.IsSuccess()) { return error; }

        result->ContainerPath = sharedLogPath;

        if (result->EnableHashSharedLogIdFromPath)
        {
            result->ContainerId = Guid::Test_FromStringHashCode(result->ContainerPath).ToString();
        }
        else
        {
            result->ContainerId = logId.ToString();
        }
    }

    return ErrorCodeValue::Success;
}

void KtlLoggerSharedLogSettings::GetDefaultSettingsFromClusterConfig(
    KtlLoggerSharedLogSettings& settings)
{
    auto sharedLogId = ServiceModelConfig::GetConfig().SharedLogId;
    if (sharedLogId.empty())
    {
        sharedLogId = ServiceModelConfig::GetConfig().DefaultApplicationSharedLogId;
    }

    settings.containerId_ = sharedLogId;
    settings.containerPath_ = L"";
    settings.logSize_ = static_cast<int64>(ServiceModelConfig::GetConfig().SharedLogSizeInMB) * 1024 * 1024;
    settings.maximumNumberStreams_ = ServiceModelConfig::GetConfig().SharedLogNumberStreams;
    settings.maximumRecordSize_ = ServiceModelConfig::GetConfig().SharedLogMaximumRecordSizeInKB;
    settings.createFlags_ = ServiceModelConfig::GetConfig().SharedLogCreateFlags;
}

Api::ISharedLogSettingsResultPtr KtlLoggerSharedLogSettings::GetDefaultReliableServiceSettings()
{
    auto settings = shared_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings());
    KtlLoggerSharedLogSettings::GetDefaultSettingsFromClusterConfig(*settings);

    return RootedObjectPointer<Api::ISharedLogSettingsResult>(settings.get(), settings->CreateComponentRoot());
}

Api::ISharedLogSettingsResultPtr KtlLoggerSharedLogSettings::GetKeyValueStoreReplicaDefaultSettings(
    wstring const & workingDirectory,
    wstring const & sharedLogDirectory,
    wstring const & sharedLogFilename,
    Guid const & sharedLogGuid)
{
    //
    // sharedLogGuid will be non-empty for managed KVS system services (i.e. FabricUS)
    //
    bool isSystemService = (sharedLogGuid != Guid::Empty());

    wstring sharedLogPath;
    auto error = GetSharedLogFullPath(
        workingDirectory,
        sharedLogDirectory,
        sharedLogFilename,
        sharedLogPath);

    if (!isSystemService && error.IsError(ErrorCodeValue::FabricDataRootNotFound))
    {
        // Fabric data root environment variable will not be available for user services from inside a container.
        // As long as we set the shared log ID to DefaultApplicationSharedLogId, then the underlying KTL logger
        // will look up the path associated with this ID. This should match the DefaultApplicationSharedLogId
        // in KtlLogger\Constants.cpp and result in using Fabric data root in Windows, node working directory in Linux.
        //
        sharedLogPath = *MockSharedLogPathForContainers;
        error = ErrorCodeValue::Success;
    }
    else
    {
        ASSERT_IFNOT(error.IsSuccess(), "GetSharedLogFullPath for KTL shared log settings failed: error={0}", error);
    }

    auto settings = shared_ptr<KtlLoggerSharedLogSettings>(new KtlLoggerSharedLogSettings());

    if (isSystemService)
    {
        settings->containerId_ = sharedLogGuid.ToString();
        settings->logSize_ = KvsSystemServiceConfig::GetConfig().LogSize;
        settings->maximumNumberStreams_ = KvsSystemServiceConfig::GetConfig().MaximumNumberStreams;
        settings->maximumRecordSize_ = KvsSystemServiceConfig::GetConfig().MaximumRecordSize;
        settings->createFlags_ = KvsSystemServiceConfig::GetConfig().CreateFlags;
    }
    else
    {
        KtlLoggerSharedLogSettings::GetDefaultSettingsFromClusterConfig(*settings);
    }
    settings->containerPath_ = sharedLogPath;

    return RootedObjectPointer<Api::ISharedLogSettingsResult>(settings.get(), settings->CreateComponentRoot());
}

ErrorCode KtlLoggerSharedLogSettings::GetSharedLogFullPath(
    wstring const & workingDirectory,
    wstring const & sharedLogDirectory,
    wstring const & sharedLogFilename,
    __out wstring & sharedLogPath)
{
#if !defined(PLATFORM_UNIX)
        UNREFERENCED_PARAMETER(workingDirectory);

        wstring fabricDataRoot;
        auto error = FabricEnvironment::GetFabricDataRoot(fabricDataRoot);
        if (!error.IsSuccess()) { return error; }

        sharedLogPath = fabricDataRoot;
#else
        ErrorCode error(ErrorCodeValue::Success);
        sharedLogPath = workingDirectory;
#endif

    if (sharedLogDirectory.empty() && sharedLogFilename.empty())
    {
        Path::CombineInPlace(sharedLogPath, ServiceModelConfig::GetConfig().DefaultApplicationSharedLogSubdirectory);
        Path::CombineInPlace(sharedLogPath, ServiceModelConfig::GetConfig().DefaultApplicationSharedLogFileName);
    }
    else
    {
        Path::CombineInPlace(sharedLogPath, sharedLogDirectory);
        Path::CombineInPlace(sharedLogPath, sharedLogFilename);
    }

#if !defined(PLATFORM_UNIX) 
    // KTL log open will fail with -1073741767 if WindowsPathPrefix isn't included.
    //
    if (sharedLogPath.find(*WindowsPathPrefix) != 0)
    {
        sharedLogPath = wformatString("{0}{1}", *WindowsPathPrefix, sharedLogPath);
    }
#endif

    return error;
}

void KtlLoggerSharedLogSettings::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write(
        "[path={0} id={1} logSize={2} maxStreams={3} maxRecordSize={4} createFlags={5} hashLogId={6}]",
        containerPath_,
        containerId_,
        logSize_,
        maximumNumberStreams_,
        maximumRecordSize_,
        createFlags_,
        hashSharedLogIdFromPath_);
}

std::wstring KtlLoggerSharedLogSettings::get_ContainerPath() const
{
    return containerPath_;
}

void KtlLoggerSharedLogSettings::set_ContainerPath(wstring const & value)
{
    containerPath_ = value;
}

std::wstring KtlLoggerSharedLogSettings::get_ContainerId() const
{
    return containerId_;
}

void KtlLoggerSharedLogSettings::set_ContainerId(wstring const & value)
{
    containerId_ = value;
}

int64 KtlLoggerSharedLogSettings::get_LogSize() const
{
    return logSize_;
}

int KtlLoggerSharedLogSettings::get_MaximumNumberStreams() const
{
    return maximumNumberStreams_;
}

int KtlLoggerSharedLogSettings::get_MaximumRecordSize() const
{
    return maximumRecordSize_;
}

uint KtlLoggerSharedLogSettings::get_CreateFlags() const
{
    return createFlags_;
}

bool KtlLoggerSharedLogSettings::get_EnableHashSharedLogIdFromPath() const
{
    return hashSharedLogIdFromPath_;
}

ErrorCode KtlLoggerSharedLogSettings::MoveIfValid(
    __in KtlLoggerSharedLogSettingsUPtr && toBeValidated,
    __in KtlLoggerSharedLogSettingsUPtr & output)
{   
    output = move(toBeValidated);
    return ErrorCode();
}
