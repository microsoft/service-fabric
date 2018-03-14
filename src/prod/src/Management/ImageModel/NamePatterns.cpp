// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

GlobalWString FileNamePattern::ArchiveExtension = make_global<wstring>(L"zip");
GlobalWString FileNamePattern::SettingsFileName = make_global<wstring>(L"Settings.xml");

wstring FileNamePattern::GetApplicationPackageFileName(
    wstring const & applicationRolloutVersion)
{
    wstring applicationInstanceFileName(L"App.");
    applicationInstanceFileName.append(applicationRolloutVersion);
    applicationInstanceFileName.append(L".xml");

    return applicationInstanceFileName;
}

wstring FileNamePattern::GetServicePackageFileName(
    wstring const & servicePackageName,
    wstring const & servicePackageRolloutVersion)
{
    wstring packageFileName(servicePackageName);
    packageFileName.append(L".Package.");
    packageFileName.append(servicePackageRolloutVersion);
    packageFileName.append(L".xml");

    return packageFileName;
}

wstring FileNamePattern::GetServicePackageContextFileName(
    wstring const & servicePackageName,
    wstring const & servicePackageRolloutVersion)
{
    wstring packageContextFileName(servicePackageName);
    packageContextFileName.append(L".Context.");
    packageContextFileName.append(servicePackageRolloutVersion);
    packageContextFileName.append(L".txt");

    return packageContextFileName;
}

wstring FileNamePattern::GetServiceManifestFileName(
    wstring const & serviceManifestName,
    wstring const & serviceManifestVersion)
{
    wstring serviceManifestFileName(serviceManifestName);
    serviceManifestFileName.append(L".Manifest.");
    serviceManifestFileName.append(serviceManifestVersion);
    serviceManifestFileName.append(L".xml");

    return serviceManifestFileName;
}

wstring FileNamePattern::GetChecksumFileName(
    wstring const & name)
{
    wstring checksumFileName(name);
    checksumFileName.append(L".checksum");

    return checksumFileName;
}

bool FileNamePattern::IsArchiveFile(
    wstring const & fileName)
{
    return StringUtility::EndsWith(fileName, wformatString(".{0}", *ArchiveExtension));
}

wstring FileNamePattern::GetSubPackageArchiveFileName(
    wstring const & packageFolderName)
{
    return wformatString("{0}.{1}", packageFolderName, *ArchiveExtension);
}

wstring FileNamePattern::GetSubPackageExtractedFolderName(
    wstring const & archiveFileName)
{
    auto ext = wformatString(".{0}", *ArchiveExtension);
    auto startPos = archiveFileName.rfind(ext);
    if (startPos == string::npos)
    {
        return archiveFileName;
    }

    auto result = archiveFileName;
    StringUtility::Replace<wstring>(result, ext, L"", startPos);
    return result;
}

wstring FolderNamePattern::GetCodePackageFolderName(
   wstring const & serviceManifestOrPackageName,
   wstring const & codePackageName,
   wstring const & codePackageVersion)
{
    return GetSubPackageFolderName(
        serviceManifestOrPackageName,
        codePackageName,
        codePackageVersion);
}

wstring FolderNamePattern::GetDataPackageFolderName(
    wstring const & serviceManifestOrPackageName,
    wstring const & dataPackageName,
    wstring const & dataPackageVersion)
{
    return GetSubPackageFolderName(
        serviceManifestOrPackageName,
        dataPackageName,
        dataPackageVersion);
}

wstring FolderNamePattern::GetConfigPackageFolderName(
    wstring const & serviceManifestOrPackageName,
    wstring const & configPackageName,
    wstring const & configPackageVersion)
{
    return GetSubPackageFolderName(
        serviceManifestOrPackageName,
        configPackageName,
        configPackageVersion);
}

wstring FolderNamePattern::GetSubPackageFolderName(
    wstring const & serviceManifestOrPackageName,
    wstring const & subPackageName,
    wstring const & subPackageVersion)
{
    wstring subPackageFolderName(serviceManifestOrPackageName);
    subPackageFolderName.append(L".");
    subPackageFolderName.append(subPackageName);
    subPackageFolderName.append(L".");
    subPackageFolderName.append(subPackageVersion);

    return subPackageFolderName;
}
