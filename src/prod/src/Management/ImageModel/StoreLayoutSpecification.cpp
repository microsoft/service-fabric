// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

GlobalWString StoreLayoutSpecification::Store = make_global<wstring>(L"Store");

StoreLayoutSpecification::StoreLayoutSpecification()
	: storeRoot_(Store)
{
}

StoreLayoutSpecification::StoreLayoutSpecification(wstring const & storeRoot)
	: storeRoot_(Path::Combine(storeRoot, *Store))
{
}

wstring StoreLayoutSpecification::GetApplicationManifestFile(
    wstring const & applicationTypeName,
    wstring const & applicationTypeVersion) const
{
    wstring applicationManifestFileName(L"ApplicationManifest.");
    applicationManifestFileName.append(applicationTypeVersion);
    applicationManifestFileName.append(L".xml");

    return Path::Combine(
        GetApplicationTypeFolder(applicationTypeName), 
        applicationManifestFileName);
}

wstring StoreLayoutSpecification::GetApplicationInstanceFile(
    wstring const & applicationTypeName,
    wstring const & applicationId,
    wstring const & applicationInstanceVersion) const
{
    wstring applicationInstanceFileName(L"ApplicationInstance.");
    applicationInstanceFileName.append(applicationInstanceVersion);
    applicationInstanceFileName.append(L".xml");

    return Path::Combine(
        GetApplicationInstanceFolder(applicationTypeName, applicationId),
        applicationInstanceFileName);
}

wstring StoreLayoutSpecification::GetApplicationPackageFile(
    wstring const & applicationTypeName,
    wstring const & applicationId,
    wstring const & applicationRolloutVersion) const
{
    wstring applicationInstanceFileName(L"ApplicationPackage.");
    applicationInstanceFileName.append(applicationRolloutVersion);
    applicationInstanceFileName.append(L".xml");

    return Path::Combine(
        GetApplicationInstanceFolder(applicationTypeName, applicationId),
        applicationInstanceFileName);
}

wstring StoreLayoutSpecification::GetServicePackageFile(
    wstring const & applicationTypeName,
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & servicePackageRolloutVersion) const
{
    return Path::Combine(
        GetApplicationInstanceFolder(applicationTypeName, applicationId),
        FileNamePattern::GetServicePackageFileName(servicePackageName, servicePackageRolloutVersion));
}

wstring StoreLayoutSpecification::GetServiceManifestFile(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & serviceManifestVersion) const
{
    return Path::Combine(
        GetApplicationTypeFolder(applicationTypeName),
        FileNamePattern::GetServiceManifestFileName(serviceManifestName, serviceManifestVersion));
}

wstring StoreLayoutSpecification::GetServiceManifestChecksumFile(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & serviceManifestVersion) const
{
    return FileNamePattern::GetChecksumFileName(GetServiceManifestFile(
        applicationTypeName,
        serviceManifestName,
        serviceManifestVersion));
}

wstring StoreLayoutSpecification::GetCodePackageFolder(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & codePackageName,
    wstring const & codePackageVersion) const
{
    return Path::Combine(
        GetApplicationTypeFolder(applicationTypeName),
        FolderNamePattern::GetCodePackageFolderName(
            serviceManifestName,
            codePackageName,
            codePackageVersion));
}

wstring StoreLayoutSpecification::GetCodePackageChecksumFile(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & codePackageName,
    wstring const & codePackageVersion) const
{
    return FileNamePattern::GetChecksumFileName(GetCodePackageFolder(
        applicationTypeName,
        serviceManifestName,
        codePackageName,
        codePackageVersion));
}

wstring StoreLayoutSpecification::GetConfigPackageFolder(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & configPackageName,
    wstring const & configPackageVersion) const
{
    return Path::Combine(
        GetApplicationTypeFolder(applicationTypeName),
        FolderNamePattern::GetConfigPackageFolderName(
            serviceManifestName,
            configPackageName,
            configPackageVersion));
}

wstring StoreLayoutSpecification::GetConfigPackageChecksumFile(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & configPackageName,
    wstring const & configPackageVersion) const
{
    return FileNamePattern::GetChecksumFileName(GetConfigPackageFolder(
        applicationTypeName,
        serviceManifestName,
        configPackageName,
        configPackageVersion));
}

wstring StoreLayoutSpecification::GetDataPackageFolder(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & dataPackageName,
    wstring const & dataPackageVersion) const
{
    return Path::Combine(
        GetApplicationTypeFolder(applicationTypeName),
        FolderNamePattern::GetDataPackageFolderName(
            serviceManifestName,
            dataPackageName,
            dataPackageVersion));
}

wstring StoreLayoutSpecification::GetDataPackageChecksumFile(
    wstring const & applicationTypeName,
    wstring const & serviceManifestName,
    wstring const & dataPackageName,
    wstring const & dataPackageVersion) const
{
    return FileNamePattern::GetChecksumFileName(GetDataPackageFolder(
        applicationTypeName,
        serviceManifestName,
        dataPackageName,
        dataPackageVersion));
}

wstring StoreLayoutSpecification::GetSettingsFile(
    wstring const & configPackageFolder) const
{
    return Path::Combine(configPackageFolder, *(FileNamePattern::SettingsFileName));
}

wstring StoreLayoutSpecification::GetApplicationTypeFolder(
    wstring const & applicationTypeName) const
{
    return Path::Combine(storeRoot_, applicationTypeName);
}

wstring StoreLayoutSpecification::GetApplicationInstanceFolder(
    wstring const & applicationTypeName, 
    wstring const & applicationId) const
{
    wstring applicationFolder = Path::Combine(GetApplicationTypeFolder(applicationTypeName), L"apps");
    return Path::Combine(applicationFolder, applicationId);
}

wstring StoreLayoutSpecification::GetSubPackageArchiveFile(
    wstring const & packageFolder) const
{
    return ImageModelUtility::GetSubPackageArchiveFileName(packageFolder);
}

wstring StoreLayoutSpecification::GetSubPackageExtractedFolder(
    wstring const & archiveFile) const
{
    return ImageModelUtility::GetSubPackageExtractedFolderName(archiveFile);
}
