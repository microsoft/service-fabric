// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::ImageStore;
using namespace Management::ImageModel;

StringLiteral const TraceComponent("ImageStoreUtility");

Common::ErrorCode ImageStoreUtility::GenerateSfpkg(
    std::wstring const & appPackageRootDirectory,
    std::wstring const & destinationDirectory,
    bool applyCompression,
    std::wstring const & sfPkgName,
    __out std::wstring & sfPkgFilePath)
{
    // If sfPkgName is empty, use the directory name instead.
    // Change or add .sfpkg extension.
    wstring sfPkgFileName = sfPkgName;
    if (sfPkgName.empty())
    {
        sfPkgFileName = Path::GetDirectoryName(appPackageRootDirectory);
    }

    Path::ChangeExtension(sfPkgFileName, *ServiceModel::Constants::SFApplicationPackageExtension);

    sfPkgFilePath = Path::Combine(destinationDirectory, sfPkgFileName);

    if (applyCompression)
    {
        ArchiveApplicationPackage(appPackageRootDirectory, INativeImageStoreProgressEventHandlerPtr());
    }

    Trace.WriteInfo(TraceComponent, "Generate sfpkg from {0} to {1} ...", appPackageRootDirectory, sfPkgFilePath);
    ErrorCode error(ErrorCodeValue::Success);
    if (File::Exists(sfPkgFilePath))
    {
        error = File::Delete2(sfPkgFilePath);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return Directory::CreateArchive(appPackageRootDirectory, sfPkgFilePath);
}

Common::ErrorCode ImageStoreUtility::ExpandSfpkg(
    std::wstring const & sfPkgFilePath,
    std::wstring const & appPackageRootDirectory)
{
    return Directory::ExtractArchive(sfPkgFilePath, appPackageRootDirectory);
}

ErrorCode ImageStoreUtility::ArchiveApplicationPackage(
    wstring const & appPackageRootDirectory, 
    INativeImageStoreProgressEventHandlerPtr const & progressHandler)
{
    Trace.WriteInfo(TraceComponent, "Compressing {0} ...", appPackageRootDirectory);

    BuildLayoutSpecification layout(appPackageRootDirectory);

    vector<wstring> sources;

    for (auto const & servicePackageName : Directory::GetSubDirectories(appPackageRootDirectory))
    {
        auto servicePackageDir = Path::Combine(appPackageRootDirectory, servicePackageName);

        // Archive individual Code/Config/Data packages 
        //
        for (auto const & dir : Directory::GetSubDirectories(servicePackageDir))
        {
            sources.push_back(Path::Combine(servicePackageDir, dir));
        }
    }

    size_t completedItems = 0;

    UpdateProgress(progressHandler, completedItems, sources.size());

    for (auto const & src : sources)
    {
        auto dest = layout.GetSubPackageArchiveFile(src);

        if (File::Exists(dest))
        {
            auto error = File::Delete2(dest);
            if (!error.IsSuccess())
            {
                return error;
            }
        }

        auto error = Directory::CreateArchive(src, dest);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = Directory::Delete(src, true); // recursive
        if (!error.IsSuccess())
        {
            return error;
        }

        UpdateProgress(progressHandler, ++completedItems, sources.size());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ImageStoreUtility::TryExtractApplicationPackage(
    wstring const & appPackageRootDirectory,
    INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    __out bool & archiveExists)
{
    archiveExists = false;

    Trace.WriteInfo(TraceComponent, "Extracting {0} ...", appPackageRootDirectory);

    BuildLayoutSpecification layout(appPackageRootDirectory);

    vector<wstring> sources;

    for (auto const & servicePackageName : Directory::GetSubDirectories(appPackageRootDirectory))
    {
        auto servicePackageDir = Path::Combine(appPackageRootDirectory, servicePackageName);

        for (auto const & file : Directory::GetFiles(servicePackageDir))
        {
            if (!layout.IsArchiveFile(file))
            {
                continue;
            }

            archiveExists = true;

            sources.push_back(Path::Combine(servicePackageDir, file));
        }
    }

    size_t completedItems = 0;

    UpdateProgress(progressHandler, completedItems, sources.size());

    for (auto const & src : sources)
    {
        auto dest = layout.GetSubPackageExtractedFolder(src);

        auto error = Directory::ExtractArchive(src, dest);
        if (!error.IsSuccess())
        {
            return error;
        }

        error = File::Delete2(src);
        if (!error.IsSuccess())
        {
            return error;
        }

        UpdateProgress(progressHandler, ++completedItems, sources.size());
    }

    return ErrorCodeValue::Success;
}

void ImageStoreUtility::UpdateProgress(
    INativeImageStoreProgressEventHandlerPtr const & progressHandler,
    size_t completedItems,
    size_t totalItems)
{
    if (progressHandler.get() != nullptr)
    {
        auto error = progressHandler->OnUpdateProgress(completedItems, totalItems, ProgressUnitType::ServiceSubPackages);

        if (!error.IsSuccess())
        {
            Trace.WriteWarning(TraceComponent, "OnUpdateProgress failed: completed={0} total={1} error={2}", completedItems, totalItems, error);
        }
    }
}
