// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadSessionMetadata
        {
            DEFAULT_COPY_CONSTRUCTOR(UploadSessionMetadata)
            DEFAULT_MOVE_CONSTRUCTOR(UploadSessionMetadata)
        public:
            UploadSessionMetadata();
            UploadSessionMetadata(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId, 
                uint64 fileSize);

            virtual ~UploadSessionMetadata();

            __declspec(property(get=get_RelativePath)) std::wstring const & StoreRelativePath;
            std::wstring const & get_RelativePath() const { return storeRelativePath_; }

            __declspec(property(get=get_SessionId)) Common::Guid const & SessionId;
            Common::Guid const & get_SessionId() const { return sessionId_; }

            __declspec(property(get=get_FileSize)) uint64 FileSize;
            uint64 get_FileSize() const { return fileSize_; }
    
            __declspec(property(get = get_LastModifiedTime)) Common::DateTime const & LastModifiedTime;
            Common::DateTime const & get_LastModifiedTime() const { return lastModifiedTime_; }
          
            void GetNextExpectedRanges(__out std::vector<UploadSessionInfo::ChunkRangePair> & nextExpectedRanges);
            Common::ErrorCode GetSortedStagingLocation(__out std::vector<std::wstring> & sortedStagingLocation);
            void GetStagingLocation(__out std::vector<std::wstring> & stagingLocation);
            Common::ErrorCode AddRange(uint64 startPosition, uint64 endPosition, std::wstring const & stagingLocation);
            Common::ErrorCode CheckRange(uint64 startPosition, uint64 endPosition);

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        private:

            typedef enum 
            {
                Undefined = 0,
                Overlapped,
                Ahead,
                Behind,
                AdjacentAhead,
                AdjacentBehind
            } RangeRelativePosition;

            struct UploadChunk
            {
                uint64 StartPosition;
                uint64 EndPosition;
                std::wstring StagingLocation;

                UploadChunk(
                    uint64 startPosition,
                    uint64 endPosition,
                    std::wstring const & stagingLocation)
                    : StartPosition(startPosition)
                    , EndPosition(endPosition)
                    , StagingLocation(stagingLocation)
                {
                }

                bool operator <(const UploadChunk & rhs) const
                {
                    return StartPosition < rhs.StartPosition;
                }
            };

            RangeRelativePosition GetRangeRelativePosition(
                std::pair<uint64, uint64> const & existingRange, 
                std::pair<uint64, uint64> const & newRange);

            Common::Guid sessionId_;
            std::wstring storeRelativePath_;
            uint64 fileSize_;
            std::vector<UploadChunk> receivedChunks_;
            std::list<std::pair<uint64, uint64>> edges_;
            Common::DateTime lastModifiedTime_;
        };
    }
}
