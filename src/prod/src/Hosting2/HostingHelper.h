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

        static Common::ErrorCode DecryptAndGetSecretStoreRef(
            _Inout_ std::map<std::wstring, Common::ConfigSettings> & configSettingsMapResult,
            _Out_ std::vector<std::wstring> & secretStoreRef);

        static void ReplaceSecretStoreRef(_Inout_ std::map<std::wstring, Common::ConfigSettings> & configSettingsMapResult, std::map<std::wstring, std::wstring> const& secrteStoreRef);

        static Common::ErrorCode WriteSettingsToFile(std::wstring const& settingName, std::wstring const& settingValue, std::wstring const& fileName);
    };
}
