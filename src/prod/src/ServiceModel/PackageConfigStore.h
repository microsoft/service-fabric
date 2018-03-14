// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class PackageConfigStore;
    typedef std::shared_ptr<ServiceModel::PackageConfigStore> PackageConfigStoreSPtr;

    // A configuration store implementation that is used by Fabric code and
    // it is based on the ServiceModel packaging format. It loads the configuration
    // settings from the config package declared in a ServicePackage.
    class PackageConfigStore : public Common::MonitoredConfigSettingsConfigStore
    {
        DENY_COPY(PackageConfigStore)

    public:
        PackageConfigStore(
            std::wstring const & servicePackageFileName, 
            std::wstring const & configPackageName,
            ConfigSettingsConfigStore::Preprocessor const & preprocessor = nullptr);
        virtual ~PackageConfigStore();

        static PackageConfigStoreSPtr Create(
            std::wstring const & servicePackageFileName, 
            std::wstring const & configPackageName,
            ConfigSettingsConfigStore::Preprocessor const & preprocessor = nullptr);

    private:
        virtual Common::ErrorCode LoadConfigSettings(__out Common::ConfigSettings & settings);

    private:
        std::wstring const servicePackageFileName_;
        std::wstring const configPackageName_;
    };
}
