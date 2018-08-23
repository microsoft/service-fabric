// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

GlobalWString RunLayoutSpecification::SharedFolderSuffix = make_global<wstring>(L".lk");
RunLayoutSpecification::RunLayoutSpecification()
    : runRoot_()
{
}

RunLayoutSpecification::RunLayoutSpecification(wstring const & runRoot)
    : runRoot_(runRoot)
{
}

wstring RunLayoutSpecification::GetApplicationFolder(
    wstring const & applicationId) const
{
    return Path::Combine(runRoot_, applicationId);
}

wstring RunLayoutSpecification::GetApplicationWorkFolder(
    wstring const & applicationId) const
{
    return Path::Combine(GetApplicationFolder(applicationId), L"work");
}

wstring RunLayoutSpecification::GetApplicationLogFolder(
    wstring const & applicationId) const
{
    return Path::Combine(GetApplicationFolder(applicationId), L"log");
}

wstring RunLayoutSpecification::GetApplicationTempFolder(
    wstring const & applicationId) const
{
    return Path::Combine(GetApplicationFolder(applicationId), L"temp");
}

wstring RunLayoutSpecification::GetApplicationSettingsFolder(
    wstring const & applicationId) const
{
    return Path::Combine(GetApplicationFolder(applicationId), L"settings");
}

wstring RunLayoutSpecification::GetApplicationPackageFile(
    wstring const & applicationId,
    wstring const & applicationRolloutVersion) const
{
    return Path::Combine(
        GetApplicationFolder(applicationId),
        FileNamePattern::GetApplicationPackageFileName(applicationRolloutVersion));
}

wstring RunLayoutSpecification::GetServicePackageFile(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & servicePackageRolloutVersion) const
{
    return Path::Combine(
        GetApplicationFolder(applicationId),
        FileNamePattern::GetServicePackageFileName(servicePackageName, servicePackageRolloutVersion));
}

wstring RunLayoutSpecification::GetCurrentServicePackageFile(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & servicePackageActivationId) const
{
    wstring servicePackageFileName(servicePackageName);

    if (!servicePackageActivationId.empty())
    {
        servicePackageFileName.append(L".");
        servicePackageFileName.append(servicePackageActivationId);
    }

    return GetServicePackageFile(
        applicationId,
        servicePackageFileName,
        L"Current");
}

wstring RunLayoutSpecification::GetEndpointDescriptionsFile(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & servicePackageActivationId) const
{
    wstring endpointDescriptionsFile(servicePackageName);

    if (!servicePackageActivationId.empty())
    {
        endpointDescriptionsFile.append(L".");
        endpointDescriptionsFile.append(servicePackageActivationId);
    }

    endpointDescriptionsFile.append(L".Endpoints.txt");

    return Path::Combine(
        GetApplicationFolder(applicationId),
        endpointDescriptionsFile);
}


wstring RunLayoutSpecification::GetPrincipalSIDsFile(
    wstring const & applicationId) const
{
    wstring sidsFile = L"Application.Sids.txt";

    return Path::Combine(
        GetApplicationFolder(applicationId),
        sidsFile);
}

wstring RunLayoutSpecification::GetServiceManifestFile(
    wstring const & applicationId,
    wstring const & serviceManifestName,
    wstring const & serviceManifestVersion) const
{
    return Path::Combine(
        GetApplicationFolder(applicationId),
        FileNamePattern::GetServiceManifestFileName(serviceManifestName, serviceManifestVersion));
}

wstring RunLayoutSpecification::GetCodePackageFolder(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & codePackageName,
    wstring const & codePackageVersion,
    bool isShared) const
{
    wstring path = Path::Combine(
        GetApplicationFolder(applicationId),
        FolderNamePattern::GetCodePackageFolderName(
            servicePackageName,
            codePackageName,
            codePackageVersion));

    if (isShared)
    {
        path.append(RunLayoutSpecification::SharedFolderSuffix);
    }
    return path;
}

wstring RunLayoutSpecification::GetConfigPackageFolder(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & configPackageName,
    wstring const & configPackageVersion,
    bool isShared) const
{
    wstring path = Path::Combine(
        GetApplicationFolder(applicationId),
        FolderNamePattern::GetConfigPackageFolderName(
            servicePackageName,
            configPackageName,
            configPackageVersion));

    if (isShared)
    {
        return path.append(RunLayoutSpecification::SharedFolderSuffix);
    }
    return path;
}

wstring RunLayoutSpecification::GetDataPackageFolder(
    wstring const & applicationId,
    wstring const & servicePackageName,
    wstring const & dataPackageName,
    wstring const & dataPackageVersion,
    bool isShared) const
{
    wstring path = Path::Combine(
        GetApplicationFolder(applicationId),
        FolderNamePattern::GetDataPackageFolderName(
            servicePackageName,
            dataPackageName,
            dataPackageVersion));

    if (isShared)
    {
        path.append(RunLayoutSpecification::SharedFolderSuffix);
    }
    return path;
}

wstring RunLayoutSpecification::GetSettingsFile(
    wstring const & configPackageFolder) const
{
    return Path::Combine(configPackageFolder, *(FileNamePattern::SettingsFileName));
}

wstring RunLayoutSpecification::GetSubPackageArchiveFile(
    wstring const & packageFolder) const
{
    return ImageModelUtility::GetSubPackageArchiveFileName(packageFolder);
}
