// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class HostingHelper
    {

    public:
        static Common::ErrorCode GenerateAllConfigSettings(
            std::wstring const& applicationId,
            std::wstring const& servicePackageName,
            Management::ImageModel::RunLayoutSpecification const& runLayout,
            std::vector<ServiceModel::DigestedConfigPackageDescription> const& digestedConfigPackages,
            _Out_ std::map<std::wstring, Common::ConfigSettings> & configSettingsMapResult);

        static Common::ErrorCode DecryptAndGetSecretStoreRefForConfigSetting(
            _Inout_ std::map<std::wstring, Common::ConfigSettings> & configSettingsMapResult,
            _Out_ std::vector<std::wstring> & secretStoreRef);
        static Common::ErrorCode WriteSettingsToFile(std::wstring const& settingName, std::wstring const& settingValue, std::wstring const& fileName);

        static Common::AsyncOperationSPtr BeginGetSecretStoreRef(
            _In_ HostingSubsystem & hosting,
            std::vector<std::wstring> const & secretStoreRefs,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent);

        static Common::ErrorCode EndGetSecretStoreRef(
            Common::AsyncOperationSPtr const & operation,
            _Out_ map<std::wstring, std::wstring> & decryptedSecretStoreRef);

    private:
        class GetSecretStoreRefValuesAsyncOperation;
    };
}
