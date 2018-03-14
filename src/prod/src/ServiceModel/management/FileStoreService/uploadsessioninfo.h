// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadSessionInfo
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
           
        public:

            struct ChunkRangePair
                : public Serialization::FabricSerializable
                , public Common::IFabricJsonSerializable
            {
                uint64 StartPosition;
                uint64 EndPosition;

                ChunkRangePair() :StartPosition(), EndPosition() {}

                ChunkRangePair(uint64 startPosition, uint64 endPosition)
                    : StartPosition(startPosition), EndPosition(endPosition) {}

                FABRIC_FIELDS_02(StartPosition, EndPosition)
                
                BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                    SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartPosition, StartPosition)
                    SERIALIZABLE_PROPERTY(ServiceModel::Constants::EndPosition, EndPosition)
                END_JSON_SERIALIZABLE_PROPERTIES()
            };

        public:

            UploadSessionInfo();
            UploadSessionInfo(
                std::wstring storeRelativeLocation,
                Common::Guid sessionId,
                uint64 fileSize,
                Common::DateTime modifiedDate,
                std::vector<ChunkRangePair> && expectedRanges);

            UploadSessionInfo(UploadSessionInfo const & other);
            UploadSessionInfo const & operator = (UploadSessionInfo const & other);

            UploadSessionInfo(UploadSessionInfo && other);
            UploadSessionInfo const & operator = (UploadSessionInfo && other);

            __declspec(property(get = get_StoreRelativeLocation)) std::wstring const & StoreRelativeLocation;
            std::wstring const & get_StoreRelativeLocation() const { return storeRelativeLocation_; }
          
            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            Common::Guid const & get_SessionId() const { return sessionId_; }

            __declspec(property(get = get_FileSize)) uint64 FileSize;
            uint64 get_FileSize() const { return fileSize_; }

            __declspec(property(get = get_ModifiedDate)) Common::DateTime const & ModifiedDate;
            Common::DateTime const & get_ModifiedDate() const { return modifiedDate_; }

            __declspec(property(get = get_ExpectedRanges)) std::vector<ChunkRangePair> const & ExpectedRanges;
            std::vector<ChunkRangePair> const & get_ExpectedRanges() const { return expectedRanges_; }
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_05(storeRelativeLocation_, sessionId_, fileSize_, modifiedDate_, expectedRanges_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StoreRelativePath, storeRelativeLocation_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::SessionId, sessionId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::FileSize, fileSize_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ModifiedDate, modifiedDate_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ExpectedRanges, expectedRanges_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            std::wstring storeRelativeLocation_;
            Common::Guid sessionId_;
            uint64 fileSize_;
            Common::DateTime modifiedDate_;
            std::vector<ChunkRangePair> expectedRanges_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FileStoreService::UploadSessionInfo::ChunkRangePair);

