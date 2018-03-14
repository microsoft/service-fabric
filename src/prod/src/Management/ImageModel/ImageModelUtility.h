// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace ImageModel
    {
        class ImageModelUtility
        {
        public:
            static std::vector<std::wstring> GetArchiveFiles(
                std::wstring const & folder,
                bool fullPath,
                bool topDirOnly);

            static bool IsArchiveFile(
                std::wstring const & fileName);

            static std::wstring GetSubPackageArchiveFileName(
                std::wstring const & packageFolderName);

            static std::wstring GetSubPackageExtractedFolderName(
                std::wstring const & archiveFileName);
        };
    }
}
