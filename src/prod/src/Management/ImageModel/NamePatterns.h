// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        //
        // Contains naming patterns for file and folder that can be used across different layout.
        // Similar pattern across layout ensures consistency for debugging but it is strictly not needed.
        // Different layouts can use different naming patterns.
        // 
        struct FileNamePattern
        {
            static Common::GlobalWString ArchiveExtension;
            static Common::GlobalWString SettingsFileName;

            static std::wstring GetApplicationPackageFileName(
                std::wstring const & applicationRolloutVersion);

            static std::wstring GetServicePackageFileName(
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageRolloutVersion);            

            static std::wstring GetServicePackageContextFileName(
                std::wstring const & servicePackageName,
                std::wstring const & servicePackageRolloutVersion);

            static std::wstring GetServiceManifestFileName(
                std::wstring const & serviceManifestName,
                std::wstring const & serviceManifestVersion);

            static std::wstring GetChecksumFileName(
                std::wstring const & name);

            static bool IsArchiveFile(
                std::wstring const & fileName);

            static std::wstring GetSubPackageArchiveFileName(
                std::wstring const & packageFolderName);

            static std::wstring GetSubPackageExtractedFolderName(
                std::wstring const & archiveFileName);
        };

        struct FolderNamePattern
        {
            static std::wstring GetCodePackageFolderName(
                std::wstring const & serviceManifestOrPackageName,
                std::wstring const & codePackageName,
                std::wstring const & codePackageVersion);

            static std::wstring GetDataPackageFolderName(
                std::wstring const & serviceManifestOrPackageName,
                std::wstring const & dataPackageName,
                std::wstring const & dataPackageVersion);

            static std::wstring GetConfigPackageFolderName(
                std::wstring const & serviceManifestOrPackageName,
                std::wstring const & configPackageName,
                std::wstring const & configPackageVersion);

            static std::wstring GetSubPackageFolderName(
                std::wstring const & serviceManifestOrPackageName,
                std::wstring const & subPackageName,
                std::wstring const & subPackageVersion);
        };
    }
}
