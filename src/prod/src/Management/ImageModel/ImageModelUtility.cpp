// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management;
using namespace ImageModel;

vector<wstring> ImageModelUtility::GetArchiveFiles(
    wstring const & folder,
    bool fullPath,
    bool topDirOnly)
{
    return Directory::GetFiles(
        folder,
        wformatString("*.{0}", *FileNamePattern::ArchiveExtension),
        fullPath,
        topDirOnly);
}

bool ImageModelUtility::IsArchiveFile(
    wstring const & fileName)
{
    return FileNamePattern::IsArchiveFile(fileName);
}

wstring ImageModelUtility::GetSubPackageArchiveFileName(
    wstring const & packageFolderName)
{
    return FileNamePattern::GetSubPackageArchiveFileName(packageFolderName);
}

wstring ImageModelUtility::GetSubPackageExtractedFolderName(
    wstring const & archiveFileName)
{
    return FileNamePattern::GetSubPackageExtractedFolderName(archiveFileName);
}
