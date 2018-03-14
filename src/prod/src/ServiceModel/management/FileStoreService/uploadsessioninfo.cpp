// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Management::FileStoreService;

UploadSessionInfo::UploadSessionInfo()
: storeRelativeLocation_()
, sessionId_()
, fileSize_()
, modifiedDate_()
, expectedRanges_()
{
}

UploadSessionInfo::UploadSessionInfo(
    std::wstring storeRelativeLocation,
    Common::Guid sessionId,
    uint64 fileSize,
    Common::DateTime modifiedDate,
    std::vector<ChunkRangePair> && expectedRanges)
    : storeRelativeLocation_(storeRelativeLocation)
    , sessionId_(sessionId)
    , fileSize_(fileSize)
    , modifiedDate_(modifiedDate)
    , expectedRanges_(std::move(expectedRanges))
{
}

UploadSessionInfo::UploadSessionInfo(
    UploadSessionInfo && other)
    : storeRelativeLocation_(move(other.storeRelativeLocation_))
    , sessionId_(move(other.sessionId_))
    , fileSize_(other.fileSize_)
    , modifiedDate_(move(other.modifiedDate_))
    , expectedRanges_(move(other.expectedRanges_))
{
}

UploadSessionInfo const & UploadSessionInfo::operator = (UploadSessionInfo && other)
{
    if (this != &other)
    {
        this->storeRelativeLocation_ = move(other.storeRelativeLocation_);
        this->sessionId_ = move(other.sessionId_);
        this->fileSize_ = other.fileSize_;
        this->modifiedDate_ = move(other.modifiedDate_);
        this->expectedRanges_ = move(other.expectedRanges_);
    }

    return *this;
}

UploadSessionInfo::UploadSessionInfo(
    UploadSessionInfo const & other)
    : storeRelativeLocation_(other.storeRelativeLocation_)
    , sessionId_(other.sessionId_)
    , fileSize_(other.fileSize_)
    , modifiedDate_(other.modifiedDate_)
    , expectedRanges_(other.expectedRanges_)
{
}

UploadSessionInfo const & UploadSessionInfo::operator = (UploadSessionInfo const & other)
{
    if (this != &other)
    {
        this->storeRelativeLocation_ = other.storeRelativeLocation_;
        this->sessionId_ = other.sessionId_;
        this->fileSize_ = other.fileSize_;
        this->modifiedDate_ = other.modifiedDate_;
        this->expectedRanges_ = other.expectedRanges_;
    }

    return *this;
}

void UploadSessionInfo::WriteTo(TextWriter & w, FormatOptions const &) const
{
	w.Write("UploadSessionInfo { ");
    w.Write("StoreRelativeLocation = {0}", this->storeRelativeLocation_);
    w.Write("SessionId = {0}", this->sessionId_);
    w.Write("FileSize = {0}", this->fileSize_);
    w.Write("ModifiedDate = {0}", this->modifiedDate_.ToString());
    w.Write("ExpectedRanges { ");
    for (auto it = this->expectedRanges_.begin(); it != this->expectedRanges_.end(); ++it)
    {
        w.Write("StartPosition = {0}", it->StartPosition);
        w.Write("EndPosition = {0}", it->EndPosition);
    }

    w.Write("}");
    w.Write("}");
}
