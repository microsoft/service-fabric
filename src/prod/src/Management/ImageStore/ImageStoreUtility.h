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
            static Common::ErrorCode GenerateSfpkg(
                std::wstring const & appPackageRootDirectory,
                std::wstring const & destinationDirectory,
                bool applyCompression,
                std::wstring const & sfPkgName,
                __out std::wstring & sfPkgFilePath);

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
