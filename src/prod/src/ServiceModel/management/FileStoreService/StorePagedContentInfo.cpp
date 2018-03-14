// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Management::FileStoreService;

INITIALIZE_SIZE_ESTIMATION(StorePagedContentInfo)

StorePagedContentInfo::StorePagedContentInfo()
    : storeFiles_()
    , storeFolders_()
    , continuationToken_()
    , maxAllowedSize_(static_cast<size_t>(
        static_cast<double>(ServiceModelConfig::GetConfig().MaxMessageSize) *
        ServiceModelConfig::GetConfig().MessageContentBufferRatio *
        ServiceModelConfig::GetConfig().QueryPagerContentRatio))
    , currentSize_(0)
{
}

StorePagedContentInfo::StorePagedContentInfo(
    std::vector<StoreFileInfo> const & storeFiles,
    std::vector<StoreFolderInfo> const & storeFolders,
    std::wstring && continuationToken)
    : storeFiles_(storeFiles)
    , storeFolders_(storeFolders)
    , continuationToken_(move(continuationToken))
{
}

StorePagedContentInfo & StorePagedContentInfo::operator = (StorePagedContentInfo && other)
{
    if (this != &other)
    {
        this->storeFiles_ = move(other.storeFiles_);
        this->storeFolders_ = move(other.storeFolders_);
        this->continuationToken_ = move(other.continuationToken_);
    }

    return *this;
}

void StorePagedContentInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write(ToString());
}

wstring StorePagedContentInfo::ToString() const
{
    return wformatString("StoreFilesCount={0}, StoreFoldersCount={1}, ContinuationToken={2}",this->storeFiles_.size(), this->storeFolders_.size(), this->continuationToken_);
}

ErrorCode StorePagedContentInfo::TryAdd(StoreFileInfo && file)
{
    auto error = CheckEstimatedSize(file.EstimateSize(), file.StoreRelativePath);
    if (error.IsSuccess())
    {
        storeFiles_.push_back(move(file));
    }

    return error;
}

ErrorCode StorePagedContentInfo::TryAdd(StoreFolderInfo && folder)
{
    auto error = CheckEstimatedSize(folder.EstimateSize(), folder.StoreRelativePath);
    if (error.IsSuccess())
    {
        storeFolders_.push_back(move(folder));
    }

    return error;
}

ErrorCode StorePagedContentInfo::EscapeContinuationToken()
{
    wstring escapedContinuationToken;
    auto error = NamingUri::EscapeString(this->continuationToken_, escapedContinuationToken);
    if (error.IsSuccess())
    {
        this->continuationToken_ = escapedContinuationToken;
    }

    return error;
}

ErrorCode StorePagedContentInfo::CheckEstimatedSize(size_t contentSize, wstring const & storeRelativePath)
{
    size_t estimatedSize = currentSize_ + contentSize;
    if (estimatedSize >= maxAllowedSize_)
    {
        auto error = ErrorCode(
            ErrorCodeValue::EntryTooLarge,
            wformatString("{0} {1} {2}", StringResource::Get(IDS_COMMON_ENTRY_TOO_LARGE), estimatedSize, maxAllowedSize_));
        this->continuationToken_ = storeRelativePath;
        return error;
    }

    currentSize_ = estimatedSize;
    return ErrorCodeValue::Success;
}
