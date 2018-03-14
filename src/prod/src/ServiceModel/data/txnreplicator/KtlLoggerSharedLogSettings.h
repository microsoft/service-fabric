// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "../../../api/definitions/ISharedLogSettingsResult.h"
#include "../../../api/definitions/InterfacePointers.h"
#include "../../../api/wrappers/ComSharedLogSettingsResult.h"

namespace TxnReplicator
{
    class KtlLoggerSharedLogSettings;
    typedef std::unique_ptr<KtlLoggerSharedLogSettings> KtlLoggerSharedLogSettingsUPtr;

    class KtlLoggerSharedLogSettings 
        : public Api::ISharedLogSettingsResult
        , public Common::ComponentRoot
    {
    public:
        static const Common::GlobalWString WindowsPathPrefix;
        static const Common::GlobalWString MockSharedLogPathForContainers;

    public:

        KtlLoggerSharedLogSettings(KtlLoggerSharedLogSettings const&);

        // Load settings from public API
        static Common::ErrorCode FromPublicApi(
            __in KTLLOGGER_SHARED_LOG_SETTINGS const &,
            __out KtlLoggerSharedLogSettingsUPtr &);

        void ToPublicApi(
            __in Common::ScopedHeap &,
            __out KTLLOGGER_SHARED_LOG_SETTINGS &) const override;

        static Common::ErrorCode FromConfig(
            __in SLConfigBase const &,
            __out KtlLoggerSharedLogSettingsUPtr &);

        // Factory method that loads the settings from configuration package.
        static HRESULT FromConfig(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in std::wstring const & configurationPackageName,
            __in std::wstring const & sectionName,
            __out IFabricSharedLogSettingsResult ** sharedLogSettings);

        static Common::ErrorCode CreateForSystemService(
            __in SLConfigBase const &,
            std::wstring const & workingDirectory,
            std::wstring const & sharedLogDirectory,
            std::wstring const & sharedLogFilename,
            Common::Guid const & logId,
            __out KtlLoggerSharedLogSettingsUPtr &);

        static Api::ISharedLogSettingsResultPtr GetKeyValueStoreReplicaDefaultSettings(
            std::wstring const & workingDirectory,
            std::wstring const & sharedLogDirectory,
            std::wstring const & sharedLogFilename,
            Common::Guid const & sharedLogGuid);

        static Api::ISharedLogSettingsResultPtr GetDefaultReliableServiceSettings();

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        // Define config properties
        DEFINE_SL_OVERRIDEABLE_CONFIG_PROPERTIES();

    private:

        KtlLoggerSharedLogSettings();

        static Common::ErrorCode ReadFromConfigurationPackage(
            __in Common::ComPointer<IFabricCodePackageActivationContext> codePackageActivationContextCPtr,
            __in Common::ComPointer<IFabricConfigurationPackage> configPackageCPtr,
            __in std::wstring const & sectionName,
            __out KtlLoggerSharedLogSettingsUPtr & output);

        static Common::ErrorCode GetSharedLogFullPath(
            std::wstring const & workingDirectory,
            std::wstring const & sharedLogDirectory,
            std::wstring const & sharedLogFilename,
            __out std::wstring & sharedLogPath);

        static void GetDefaultSettingsFromClusterConfig(
            __out KtlLoggerSharedLogSettings& ktlLoggerSharedLogSettings);

        static Common::ErrorCode MoveIfValid(
            __in KtlLoggerSharedLogSettingsUPtr && toBeValidated,
            __out KtlLoggerSharedLogSettingsUPtr & output);

        DEFINE_SL_PRIVATE_CONFIG_MEMBERS();
    };    
}
