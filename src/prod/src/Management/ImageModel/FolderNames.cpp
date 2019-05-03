// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        struct FolderNames
        {
            static std::wstring GetCodePackageFolderName(
                std::wstring const & serviceManifestName,
                std::wstring const & codePackageName,
                std::wstring const & codePackageVersion);

            static std::wstring GetDataPackageFolderName(
                std::wstring const & serviceManifestName,
                std::wstring const & dataPackageName,
                std::wstring const & dataPackageVersion);

            static std::wstring GetConfigPackageFolderName(
                std::wstring const & serviceManifestName,
                std::wstring const & configPackageName,
                std::wstring const & configPackageVersion);
        }
    }
}
