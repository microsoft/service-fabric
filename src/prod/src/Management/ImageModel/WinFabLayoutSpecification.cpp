// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

GlobalWString WinFabStoreLayoutSpecification::WindowsFabricStore = make_global<wstring>(L"WindowsFabricStore");

WinFabStoreLayoutSpecification::WinFabStoreLayoutSpecification()
	: windowsFabRoot_(WindowsFabricStore)
{
}

WinFabStoreLayoutSpecification::WinFabStoreLayoutSpecification(wstring const & storeRoot)
	: windowsFabRoot_(Path::Combine(storeRoot, *WindowsFabricStore))
{
}

wstring WinFabStoreLayoutSpecification::GetPatchFile(wstring const & version) const
{
    return Path::Combine(
        windowsFabRoot_, 
        GetPatchFileName(version, false));
}

wstring WinFabStoreLayoutSpecification::GetCabPatchFile(wstring const & version) const
{
    return Path::Combine(
        windowsFabRoot_, 
        GetPatchFileName(version, true));
}


wstring WinFabStoreLayoutSpecification::GetPatchFileName(wstring const & version, bool useCabFile)
{
    FabricCodeVersion codeVersion;
    auto error = FabricCodeVersion::FromString(version, codeVersion);
    ASSERT_IF(!error.IsSuccess(), "{0} is not FabricCodeVersion", version);    

#if defined(PLATFORM_UNIX)
    wstring fileName(L"servicefabric_");
#else
    wstring fileName(L"WindowsFabric.");
#endif
    fileName.append(version);
    if (useCabFile)
    {
        fileName.append(L".cab");
    }
    else
    {
#if defined(PLATFORM_UNIX)
        Common::LinuxPackageManagerType::Enum packageManagerType;
        auto error = FabricEnvironment::GetLinuxPackageManagerType(packageManagerType);
        ASSERT_IF(!error.IsSuccess(), "GetLinuxPackageManagerType failed. Type: {0}. Error: {1}", packageManagerType, error);
        CODING_ERROR_ASSERT(packageManagerType != Common::LinuxPackageManagerType::Enum::Unknown);
        switch (packageManagerType)
        {
        case Common::LinuxPackageManagerType::Enum::Deb:
            fileName.append(L".deb");
            break;
        case Common::LinuxPackageManagerType::Enum::Rpm:
            fileName.append(L".rpm");
            break;
        }
#else
        fileName.append(L".msi");
#endif

    }

    return fileName;
}

wstring WinFabStoreLayoutSpecification::GetCodePackageFolder(wstring const & version) const
{
    FabricCodeVersion codeVersion;
    auto error = FabricCodeVersion::FromString(version, codeVersion);
    ASSERT_IF(!error.IsSuccess(), "{0} is not FabricCodeVersion", version);

    wstring wfCodePackageDirName(L"WindowsFabric.");
    wfCodePackageDirName.append(version);

    return Path::Combine(
        windowsFabRoot_,
        wfCodePackageDirName);
}

wstring WinFabStoreLayoutSpecification::GetClusterManifestFile(wstring const & clusterManifestVersion) const
{
    return Path::Combine(
        windowsFabRoot_, 
        GetClusterManifestFileName(clusterManifestVersion));
}

wstring WinFabStoreLayoutSpecification::GetClusterManifestFileName(wstring const & clusterManifestVersion)
{
    wstring cmName(L"ClusterManifest.");
    cmName.append(clusterManifestVersion);
    cmName.append(L".xml");

    return cmName;
}
