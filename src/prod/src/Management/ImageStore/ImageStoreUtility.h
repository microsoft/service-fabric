// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ImageStore
    {
        class ImageStoreUtility
        {
        public:
            // Generates an sfpkg from the specified app package root directory.
            // The sfpkg is placed inside the destination directory, which is created if it doesn't exist.
            // The destination directory can't be a child of the source directory, because extraction fails.
            // If desired, compression is applied before generating sfpkg.
            // If sfpkgName is not specified, the sfpkg uses the app package root directory name.
            // Returns the full path to the generated sfpkg.
            static Common::ErrorCode GenerateSfpkg(
                std::wstring const & appPackageRootDirectory,
                std::wstring const & destinationDirectory,
                bool applyCompression,
                std::wstring const & sfPkgName,
                __out std::wstring & sfPkgFilePath);

            // Expands a given sfpkg file in the specified directory.
            // The sfpkg file must exist and have '.sfpkg' extension.
            // If the sfpkg is inside appPackageRootDirectory, after the package is expanded, the sfpkg is deleted,
            // so the resulting directory is a valid application package.
            // The appPackageRootDirectory must not have other files/folders.
            // If the sfpkg is not in the appPackageRootDirectory, the sfpkg is not deleted.
            static Common::ErrorCode ExpandSfpkg(
                std::wstring const & sfPkgFilePath,
                std::wstring const & appPackageRootDirectory);

            static Common::ErrorCode ArchiveApplicationPackage(
                std::wstring const & appPackageRootDirectory, 
                Api::INativeImageStoreProgressEventHandlerPtr const &);
            
            static Common::ErrorCode TryExtractApplicationPackage(
                std::wstring const & appPackageRootDirectory, 
                Api::INativeImageStoreProgressEventHandlerPtr const &,
                __out bool & archiveExists);

        private:

            static void UpdateProgress(
                Api::INativeImageStoreProgressEventHandlerPtr const &,
                size_t completedItems,
                size_t totalItems);
        };
    }
}
