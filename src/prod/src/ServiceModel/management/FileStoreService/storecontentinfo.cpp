// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

StoreContentInfo::StoreContentInfo()
    : storeFiles_()
    , storeFolders_()
{
}

StoreContentInfo::StoreContentInfo(
    std::vector<StoreFileInfo> const & storeFiles,
    std::vector<StoreFolderInfo> const & storeFolders)
    : storeFiles_(storeFiles)
    , storeFolders_(storeFolders)
{
}

StoreContentInfo & StoreContentInfo::operator = (StoreContentInfo && other)
{
    if (this != &other)
    {
        this->storeFiles_ = move(other.storeFiles_);
        this->storeFolders_ = move(other.storeFolders_);
    }

    return *this;
}

void StoreContentInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring StoreContentInfo::ToString() const
{
    return wformatString("StoreFilesCount={0}, StoreFoldersCount={1}", this->storeFiles_.size(), this->storeFolders_.size());
}
