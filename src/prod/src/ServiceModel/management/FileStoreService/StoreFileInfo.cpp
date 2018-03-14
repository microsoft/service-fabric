// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

INITIALIZE_SIZE_ESTIMATION(StoreFileInfo)

StoreFileInfo::StoreFileInfo()
: storeRelativePath_()
, version_()
, size_()
, modifiedDate_()
{
}

StoreFileInfo::StoreFileInfo(
    std::wstring const & storeRelativePath, 
    StoreFileVersion const & version, 
    uint64 size, 
    DateTime const & modifiedDate)
: storeRelativePath_(storeRelativePath)
, version_(version)
, size_(size)
, modifiedDate_(modifiedDate)
{
}

StoreFileInfo const & StoreFileInfo::operator = (StoreFileInfo const & other)
{
    if (this != &other)
    {
        this->storeRelativePath_ = other.storeRelativePath_;
        this->version_ = other.version_;
        this->size_ = other.size_;
        this->modifiedDate_ = other.modifiedDate_;
    }

    return *this;
}

StoreFileInfo const & StoreFileInfo::operator = (StoreFileInfo && other)
{
    if (this != &other)
    {
        this->storeRelativePath_ = move(other.storeRelativePath_);
        this->version_ = other.version_;
        this->size_ = other.size_;
        this->modifiedDate_ = other.modifiedDate_;
    }

    return *this;
}

bool StoreFileInfo::operator == (StoreFileInfo const & other) const
{
    bool equals = true;

    equals = StringUtility::AreEqualCaseInsensitive(this->storeRelativePath_, other.storeRelativePath_);
    if (!equals) { return equals; }

    equals = this->version_ == other.version_;
    if (!equals) { return equals; }

    equals = this->size_ == other.size_;
    if (!equals) { return equals; }

    equals = this->modifiedDate_ == other.modifiedDate_;
    return equals;
}

bool StoreFileInfo::operator != (StoreFileInfo const & other) const
{
    return !(*this == other);
}

void StoreFileInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

void StoreFileInfo::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_IMAGE_STORE_FILE_INFO_QUERY_RESULT_ITEM & publicImageStoreFileInfo) const
{
    publicImageStoreFileInfo.StoreRelativePath = heap.AddString(this->storeRelativePath_);
    publicImageStoreFileInfo.FileVersion = heap.AddString(this->get_StoreFileVersion().ToString());
    publicImageStoreFileInfo.FileSize = heap.AddString(std::to_wstring(this->size_));
    publicImageStoreFileInfo.ModifiedDate = heap.AddString(this->modifiedDate_.ToString(true));
}

wstring StoreFileInfo::ToString() const
{
    return wformatString("StoreFileInfo={{0}, {1}}", this->storeRelativePath_, this->version_);
}

