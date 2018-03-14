// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

// ********************************************************************************************************************
// PackageConfigStore Implementation
//

PackageConfigStore::PackageConfigStore(
    wstring const & servicePackageFileName, 
    wstring const & configPackageName,
    ConfigSettingsConfigStore::Preprocessor const & preprocessor)
    : MonitoredConfigSettingsConfigStore(servicePackageFileName, preprocessor),
    servicePackageFileName_(servicePackageFileName),
    configPackageName_(configPackageName)
{
    MonitoredConfigSettingsConfigStore::Initialize();
}

PackageConfigStore::~PackageConfigStore()
{
}

PackageConfigStoreSPtr PackageConfigStore::Create(
    wstring const & servicePackageFileName, 
    wstring const & configPackageName,
    PackageConfigStore::Preprocessor const & preprocessor)
{
    auto store = make_shared<PackageConfigStore>(servicePackageFileName, configPackageName, preprocessor);
    auto error = store->EnableChangeMonitoring();
    if (!error.IsSuccess())
    {
        Assert::CodingError(
            "Failed to enable change monitoring on PackageConfigStore. ErrorCode={0}, ServicePackageFileName={1}, ConfigPackageName={2}",
            error,
            servicePackageFileName,
            configPackageName);
    }

    return store;
}

ErrorCode PackageConfigStore::LoadConfigSettings(__out ConfigSettings & settings)
{
    ServicePackageDescription packageDescription;
    auto error = ServiceModel::Parser::ParseServicePackage(servicePackageFileName_, packageDescription);
    if (!error.IsSuccess()) 
    { 
        return error; 
    }

    wstring configPackageVersion;
    bool found = false;
    for(auto iter = packageDescription.DigestedConfigPackages.begin(); 
        iter != packageDescription.DigestedConfigPackages.end(); 
        ++iter)
    {
        if (StringUtility::AreEqualCaseInsensitive(iter->ConfigPackage.Name, configPackageName_))
        {
            configPackageVersion = iter->ConfigPackage.Version;
            found = true;
            break;
        }
    }

    if (!found) 
    { 
        return ErrorCode(ErrorCodeValue::NotFound); 
    }

    wstring appFolderPath = Path::GetDirectoryName(servicePackageFileName_);
    wstring configPackageFolder = Path::Combine(appFolderPath, wformatString("{0}.{1}", configPackageName_, configPackageVersion));
    wstring configSettingsFile = Path::Combine(configPackageFolder, L"Settings.xml");

    error = ServiceModel::Parser::ParseConfigSettings(configSettingsFile, settings);
    if (!error.IsSuccess()) 
    { 
        return error; 
    }

    ConfigEventSource::Events.PackageConfigStoreSettingsLoaded(configSettingsFile);

    return ErrorCode(ErrorCodeValue::Success);
}
