// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

BuildLayoutSpecification::BuildLayoutSpecification()
    : buildRoot_()
{
}

BuildLayoutSpecification::BuildLayoutSpecification(wstring const & buildRoot)
    : buildRoot_(buildRoot)
{
}

wstring BuildLayoutSpecification::GetApplicationManifestFile() const
{
    return Path::Combine(buildRoot_, L"ApplicationManifest.xml");
}

// Same scheme for code/config/data packages
wstring BuildLayoutSpecification::GetSubPackageArchiveFile(
    wstring const & packageFolder) const
{
    return ImageModelUtility::GetSubPackageArchiveFileName(packageFolder);
}

wstring BuildLayoutSpecification::GetSubPackageExtractedFolder(wstring const & archiveFile) const
{
    return ImageModelUtility::GetSubPackageExtractedFolderName(archiveFile);
}

bool BuildLayoutSpecification::IsArchiveFile(wstring const & fileName) const
{
    return FileNamePattern::IsArchiveFile(fileName);
}

wstring BuildLayoutSpecification::GetServiceManifestFile(
    wstring const & serviceManifestName) const
{
    return Path::Combine(
        GetServiceManifestFolder(serviceManifestName), 
        L"ServiceManifest.xml");
}

wstring BuildLayoutSpecification::GetServiceManifestChecksumFile(
    wstring const & serviceManifestName) const
{
    return FileNamePattern::GetChecksumFileName(GetServiceManifestFile(serviceManifestName));
}

wstring BuildLayoutSpecification::GetCodePackageFolder(
    wstring const & serviceManifestName,
    wstring const & codePackageName) const
{
    return Path::Combine(
        GetServiceManifestFolder(serviceManifestName), 
        codePackageName);
}

wstring BuildLayoutSpecification::GetCodePackageChecksumFile(
    wstring const & serviceManifestName,
    wstring const & codePackageName) const
{
    wstring codePackageFolder = GetCodePackageFolder(
        serviceManifestName, 
        codePackageName);

    return FileNamePattern::GetChecksumFileName(codePackageFolder);
}

wstring BuildLayoutSpecification::GetConfigPackageFolder(
    wstring const & serviceManifestName,
    wstring const & configPackageName) const
{
    return Path::Combine(
        GetServiceManifestFolder(serviceManifestName), 
        configPackageName);
}

wstring BuildLayoutSpecification::GetConfigPackageChecksumFile(
    wstring const & serviceManifestName,
    wstring const & configPackageName) const
{
    wstring configPackageFolder = GetConfigPackageFolder(
        serviceManifestName, 
        configPackageName);

    return FileNamePattern::GetChecksumFileName(configPackageFolder);
}

wstring BuildLayoutSpecification::GetDataPackageFolder(
    wstring const & serviceManifestName,
    wstring const & dataPackageName) const
{
   return Path::Combine(
        GetServiceManifestFolder(serviceManifestName), 
        dataPackageName);
}

wstring BuildLayoutSpecification::GetDataPackageChecksumFile(
    wstring const & serviceManifestName,
    wstring const & dataPackageName) const
{
    wstring dataPackageFolder = GetConfigPackageFolder(
        serviceManifestName,
        dataPackageName);

   return FileNamePattern::GetChecksumFileName(dataPackageFolder);
}

wstring BuildLayoutSpecification::GetSettingsFile(
    wstring const & configPackageFolder) const
{
    return Path::Combine(configPackageFolder, *(FileNamePattern::SettingsFileName));
}

wstring BuildLayoutSpecification::GetChecksumFile(wstring const & fileOrDirectoryName) const
{
    return FileNamePattern::GetChecksumFileName(fileOrDirectoryName);
}

wstring BuildLayoutSpecification::GetServiceManifestFolder(
    wstring const & serviceManifestName) const
{
    return Path::Combine(buildRoot_, serviceManifestName);
}
