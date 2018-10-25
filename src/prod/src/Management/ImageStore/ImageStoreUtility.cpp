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
    std::wstring const & sfpkgName,
    __out std::wstring & sfpkgFilePath)
{
    if (!Directory::Exists(appPackageRootDirectory))
    {
        ErrorCode error(
            ErrorCodeValue::DirectoryNotFound,
            wformatString(GET_COMMON_RC(DirectoryNotFound), appPackageRootDirectory));
        Trace.WriteInfo(TraceComponent, "Generate sfpkg failed: {0} {1}", error, error.Message);
        return error;
    }

    wstring sourcePath = Path::GetFullPath(appPackageRootDirectory);
    wstring destPath = Path::GetFullPath(destinationDirectory);

    // The destination directory may not be inside the root directory, or create archive fails.
    // The error returned by zip only says that it can't access a file, so it's hard to figure out what is wrong.
    // Add a check so users get an error that it's clear.
    wstring sourcePathLower(sourcePath);
    wstring destPathLower(destPath);
    StringUtility::ToLower(sourcePathLower);
    StringUtility::ToLower(destPathLower);
    if (sourcePathLower == destPathLower ||
        StringUtility::StartsWith(destPathLower, sourcePathLower + Path::GetPathSeparatorWstr()))
    {
        ErrorCode error(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(ZipToChildDirectoryFails), destinationDirectory, appPackageRootDirectory));
        Trace.WriteInfo(TraceComponent, "Generate sfpkg failed: {0} {1}", error, error.Message);
        return error;
    }

    // If sfpkgName is empty, use the directory name instead.
    // Change or add .sfpkg extension.
    wstring sfPkgFileName = sfpkgName;
    if (sfpkgName.empty())
    {
        sfPkgFileName = Path::GetFileName(sourcePath);
    }

    ASSERT_IF(sfPkgFileName.empty(), "sfpkg name is not set. Input: '{0}, appPackageRootDirectory {1}", sfpkgName, appPackageRootDirectory);

    Trace.WriteInfo(TraceComponent, "Generate sfpkg from {0} to {1}, name {2}...", sourcePath, destPath, sfPkgFileName);

    Path::AddSfpkgExtension(sfPkgFileName);

    sfpkgFilePath = Path::Combine(destPath, sfPkgFileName);

    if (applyCompression)
    {
        ArchiveApplicationPackage(appPackageRootDirectory, INativeImageStoreProgressEventHandlerPtr());
    }

    // Create the destination directory if it doesn't exist.
    Directory::Create2(destinationDirectory);

    // Delete any previous file with the same name.
    ErrorCode error(ErrorCodeValue::Success);
    if (File::Exists(sfpkgFilePath))
    {
        Trace.WriteInfo(TraceComponent, "Delete previous sfpkg '{0}'.", sfpkgFilePath);
        error = File::Delete2(sfpkgFilePath);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    Trace.WriteInfo(TraceComponent, "Archive to sfpkg '{0}'.", sfpkgFilePath);
    error = Directory::CreateArchive(appPackageRootDirectory, sfpkgFilePath);
    if (!error.IsSuccess())
    {
        return error;
    }

    return error;
}

Common::ErrorCode ImageStoreUtility::ExpandSfpkg(
    std::wstring const & sfpkgFilePath,
    std::wstring const & appPackageRootDirectory)
{
    // Validate the sfpkg.
    if (!Path::IsSfpkg(sfpkgFilePath))
    {
        return ErrorCode(
            ErrorCodeValue::InvalidArgument,
            wformatString(GET_COMMON_RC(Invalid_Sfpkg_Name), sfpkgFilePath));
    }

    if (!File::Exists(sfpkgFilePath))
    {
        return ErrorCode(
            ErrorCodeValue::FileNotFound,
            wformatString(GET_COMMON_RC(FileNotFound), sfpkgFilePath));
    }

    ErrorCode error;

    // Delete sfpkg if it's in the appPackageRootDirectory, to ensure that the extracted package is a valid SF package.
    bool shouldDelete = false;

    // The destination folder should not contain other files.
    if (Directory::Exists(appPackageRootDirectory))
    {
        auto subDirs = Directory::GetSubDirectories(appPackageRootDirectory);
        bool hasUnexpectedFiles = false;
        if (!subDirs.empty())
        {
            // More folders in the app package, which is not expected.
            // The extracted app package should have no other files or folders.
            Trace.WriteInfo(TraceComponent, "There are {0} sub-directories in {1}, can't extract {2}", subDirs.size(), appPackageRootDirectory, sfpkgFilePath);
            hasUnexpectedFiles = true;
        }
        else
        {
            // Enumerate files top directory only.
            auto files = Directory::GetFiles(appPackageRootDirectory, L"*", false /*fullPath*/, true /*topDirectoryOnly*/);
            if (files.size() > 1u)
            {
                Trace.WriteInfo(TraceComponent, "There are {0} files in {1}, can't extract {2}", files.size(), appPackageRootDirectory, sfpkgFilePath);
                hasUnexpectedFiles = true;
            }
            else if (files.size() == 1u)
            {
                // Check if the file inside the directory is the sfpkg to expand.
                auto sfpkgFileName = Path::GetFileName(sfpkgFilePath);
                if (StringUtility::AreEqualCaseInsensitive(sfpkgFileName, files[0]))
                {
                    Trace.WriteInfo(TraceComponent, "Extract {0}: sfpkg is in directory {1} and will be deleted after successful expand", sfpkgFilePath, appPackageRootDirectory);
                    shouldDelete = true;
                }
                else
                {
                    // Different file, not supported.
                    Trace.WriteInfo(TraceComponent, "Extract {0}: directory {1} has unexpected file {2}. Can't extract, as the folder will not be a valid app folder.", sfpkgFilePath, appPackageRootDirectory, files[0]);
                    hasUnexpectedFiles = true;
                }
            }
        }

        if (hasUnexpectedFiles)
        {
            return ErrorCode(
                ErrorCodeValue::InvalidState,
                wformatString(GET_COMMON_RC(ExpandSfpkgDirectoryNotEmpty), appPackageRootDirectory));
        }
    }
    else
    {
        Directory::Create(appPackageRootDirectory);
    }

    error = Directory::ExtractArchive(sfpkgFilePath, appPackageRootDirectory);
    if (!error.IsSuccess())
    {
        return error;
    }

    // Delete the sfpkg since expand is successful.
    if (shouldDelete)
    {
        error = File::Delete2(sfpkgFilePath);
        if (!error.IsSuccess())
        {
            return error;
        }
    }

    return error;
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
