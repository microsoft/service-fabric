// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

INITIALIZE_SIZE_ESTIMATION(StoreFolderInfo)

StoreFolderInfo::StoreFolderInfo()
    : storeRelativePath_()
    , fileCount_()
{
}

StoreFolderInfo::StoreFolderInfo(std::wstring const & storeRelativePath, unsigned long long const & fileCount)
: storeRelativePath_(storeRelativePath)
, fileCount_(fileCount)
{
}

StoreFolderInfo const & StoreFolderInfo::operator = (StoreFolderInfo const & other)
{
    if (this != &other)
    {
        this->storeRelativePath_ = other.storeRelativePath_;
        this->fileCount_ = other.fileCount_;
    }

    return *this;
}

StoreFolderInfo const & StoreFolderInfo::operator = (StoreFolderInfo && other)
{
    if (this != &other)
    {
        this->storeRelativePath_ = move(other.storeRelativePath_);
        this->fileCount_ = other.fileCount_;
    }

    return *this;
}

bool StoreFolderInfo::operator == (StoreFolderInfo const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->storeRelativePath_, other.storeRelativePath_);
    if (!equals) { return equals; }

    equals = this->fileCount_ == other.fileCount_;
    return equals;
}

bool StoreFolderInfo::operator != (StoreFolderInfo const & other) const
{
    return !(*this == other);
}

void StoreFolderInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

void StoreFolderInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_IMAGE_STORE_FOLDER_INFO_QUERY_RESULT_ITEM & publicImageStoreFolderInfo) const
{
    publicImageStoreFolderInfo.StoreRelativePath = heap.AddString(this->storeRelativePath_);
    publicImageStoreFolderInfo.FileCount = heap.AddString(std::to_wstring(this->fileCount_));
}

wstring StoreFolderInfo::ToString() const
{
    return wformatString("StoreFolderInfo={{0}, {1}}", this->storeRelativePath_, std::to_wstring(this->fileCount_));
}
