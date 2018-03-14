// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace Management::FileStoreService;

UploadSessionMetadata::UploadSessionMetadata(
    std::wstring const & storeRelativePath,
    Common::Guid const & sessionId,
    uint64 fileSize)
    : storeRelativePath_(storeRelativePath)
    , sessionId_(sessionId)
    , fileSize_(fileSize)
    , lastModifiedTime_(DateTime::get_Now())
{
}

UploadSessionMetadata::UploadSessionMetadata(UploadSessionMetadata const & uploadSessionMetadata)
{
    this->storeRelativePath_ = uploadSessionMetadata.StoreRelativePath;
    this->sessionId_ = uploadSessionMetadata.SessionId;
    this->fileSize_ = uploadSessionMetadata.FileSize;
    this->lastModifiedTime_ = uploadSessionMetadata.LastModifiedTime;
}

UploadSessionMetadata::~UploadSessionMetadata()
{
}

void UploadSessionMetadata::GetNextExpectedRanges(__out std::vector<UploadSessionInfo::ChunkRangePair> & nextExpectedRanges)
{
    uint64 startPosition = 0;
    if (this->edges_.empty())
    {
        nextExpectedRanges.push_back(UploadSessionInfo::ChunkRangePair(startPosition, this->fileSize_ - 1));
        return;
    }

    for (std::list<std::pair<uint64, uint64>>::const_iterator it = this->edges_.begin(); it != this->edges_.end(); ++it)
    {
        if (it->first > startPosition)
        {
            nextExpectedRanges.push_back(UploadSessionInfo::ChunkRangePair(startPosition, it->first - 1));
            if (it->second == this->fileSize_ - 1)
            {
                return;;
            }
        }
         
        startPosition = it->second + 1;
    }

    if (startPosition < this->fileSize_)
    {
        nextExpectedRanges.push_back(UploadSessionInfo::ChunkRangePair(startPosition, this->fileSize_ - 1));
    }
}

ErrorCode UploadSessionMetadata::GetSortedStagingLocation(__out std::vector<std::wstring> & sortedStagingLocation)
{
    if (this->edges_.size() != 1 
        || this->edges_.begin()->first != 0
        || this->edges_.begin()->second != this->fileSize_ - 1)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    std::sort(this->receivedChunks_.begin(), this->receivedChunks_.end());
    for (std::vector<UploadChunk>::const_iterator it = this->receivedChunks_.begin(); it != receivedChunks_.end(); ++it)
    {
        sortedStagingLocation.push_back(it->StagingLocation);
    }

    return ErrorCodeValue::Success;
}

void UploadSessionMetadata::GetStagingLocation(__out std::vector<std::wstring> & stagingLocation)
{
    for (std::vector<UploadChunk>::const_iterator it = this->receivedChunks_.begin(); it != receivedChunks_.end(); ++it)
    {
        stagingLocation.push_back(it->StagingLocation);
    }
}

ErrorCode UploadSessionMetadata::CheckRange(uint64 startPosition, uint64 endPosition, uint64 fileLength)
{
    if (startPosition > endPosition || fileLength != this->fileSize_)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto positions = std::pair<uint64, uint64>(startPosition, endPosition);
    for (std::list<std::pair<uint64, uint64>>::iterator it = this->edges_.begin(); it != this->edges_.end(); ++it)
    {
        if (this->GetRangeRelativePosition(*it, positions) == RangeRelativePosition::Overlapped)
        {
            return ErrorCodeValue::UploadSessionRangeNotSatisfiable;
        }
    }

    return ErrorCodeValue::Success;
}

ErrorCode UploadSessionMetadata::AddRange(uint64 startPosition, uint64 endPosition, std::wstring const & stagingLocation)
{
    if (startPosition > endPosition)
    {
        return ErrorCodeValue::InvalidArgument;
    }

    auto positions = std::pair<uint64, uint64>(startPosition, endPosition);
    if (this->edges_.empty())
    {
        this->edges_.push_front(positions);
    }
    else
    {
        bool updated = false;
        RangeRelativePosition previousRelateivePosition = RangeRelativePosition::Undefined;
        for (std::list<std::pair<uint64, uint64>>::iterator it = this->edges_.begin(); it != this->edges_.end(); ++it)
        {
            auto relativePosition = this->GetRangeRelativePosition(*it, positions);
            switch (relativePosition)
            {
            case RangeRelativePosition::AdjacentAhead:
                if (previousRelateivePosition == RangeRelativePosition::AdjacentBehind)
                {
                    uint64 currentEndPosition = it->second;
                    --it;
                    it->second = currentEndPosition;
                    this->edges_.erase(++it);
                }
                else
                {
                    it->first = startPosition;
                }

                updated = true;
                break;
            case RangeRelativePosition::Ahead:
                if (previousRelateivePosition == RangeRelativePosition::Undefined)
                {
                    this->edges_.push_front(std::pair<uint64, uint64>(startPosition, endPosition));
                }
                else if (previousRelateivePosition == RangeRelativePosition::Behind)
                {
                    edges_.insert(it, std::pair<int64, int64>(startPosition, endPosition));
                }
               
                updated = true;
                break;
            case RangeRelativePosition::AdjacentBehind:
                it->second = endPosition;
                break;
            case RangeRelativePosition::Behind:
                break;
            default:
                return ErrorCodeValue::UploadSessionRangeNotSatisfiable;
                break;
            }

            if (updated)
            {
                break;
            }
            else
            {
                previousRelateivePosition = relativePosition;
            }
        }

        if (previousRelateivePosition == RangeRelativePosition::Behind && !updated)
        {
            edges_.push_back(std::pair<uint64, uint64>(startPosition, endPosition));
        }
    }

    this->receivedChunks_.push_back(UploadChunk(startPosition, endPosition, stagingLocation));
    this->lastModifiedTime_ = Common::DateTime::Now();
    return ErrorCodeValue::Success;
}

void UploadSessionMetadata::WriteTo(TextWriter & w, FormatOptions const &) const
{    
    std::string builder = "[ ";
    for (std::vector<UploadChunk>::const_iterator it = this->receivedChunks_.begin(); it != this->receivedChunks_.end(); ++it)
    {
        builder.append(Common::formatString("{0}-{1} ", std::to_string(it->StartPosition), std::to_string(it->EndPosition)));
    }

    builder.append("]");
    w.Write("UploadSessionMetadata[{0}, {1}, {2}, {3}]", storeRelativePath_, fileSize_, builder, lastModifiedTime_.ToString());
}

UploadSessionMetadata::RangeRelativePosition UploadSessionMetadata::GetRangeRelativePosition(std::pair<uint64, uint64> const & existingRange, std::pair<uint64, uint64> const & newRange)
{ 
    if (existingRange.first == newRange.second + 1)
    {
        return RangeRelativePosition::AdjacentAhead;
    }

    if (existingRange.second == newRange.first - 1)
    {
        return RangeRelativePosition::AdjacentBehind;
    }

    if (existingRange.first > newRange.second + 1)
    {
        return RangeRelativePosition::Ahead;
    }

    if (existingRange.second < newRange.first - 1)
    {
        return RangeRelativePosition::Behind;
    }

    return RangeRelativePosition::Overlapped;
}

