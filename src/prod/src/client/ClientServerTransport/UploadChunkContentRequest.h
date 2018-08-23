// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadChunkContentRequest 
            : public ServiceModel::ClientServerMessageBody
            , public Common::TextTraceComponent<Common::TraceTaskCodes::Client>
        {
            DENY_COPY(UploadChunkContentRequest)
            DEFAULT_MOVE_CONSTRUCTOR(UploadChunkContentRequest)
            DEFAULT_MOVE_ASSIGNMENT(UploadChunkContentRequest)
        public:

            UploadChunkContentRequest();

            UploadChunkContentRequest(
                Transport::MessageUPtr && chunkContentMessage,
                Management::FileStoreService::UploadChunkContentDescription && uploadChunkContentDescription);

            Common::ErrorCode InitializeBuffer();

            __declspec(property(get = get_UploadChunkContent)) Management::FileStoreService::UploadChunkContentDescription const & UploadChunkContentDescription;
            inline Management::FileStoreService::UploadChunkContentDescription const & get_UploadChunkContent() const { return uploadChunkContentDescription_; }

            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            inline Common::Guid const & get_SessionId() const { return uploadChunkContentDescription_.SessionId; }

            __declspec(property(get = get_StartPosition)) uint64 StartPosition;
            inline uint64 get_StartPosition() const { return uploadChunkContentDescription_.StartPosition; }

            __declspec(property(get = get_EndPosition)) uint64 EndPosition;
            inline uint64 get_EndPosition() const { return uploadChunkContentDescription_.EndPosition; }

            __declspec(property(get = get_ChunkContent)) std::vector<BYTE> const & ChunkContent;
            inline std::vector<BYTE> const & get_ChunkContent() const { return chunkContent_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_02(uploadChunkContentDescription_, chunkContent_);

        private:
            Management::FileStoreService::UploadChunkContentDescription uploadChunkContentDescription_;
            std::vector<BYTE> chunkContent_;
            Transport::MessageUPtr chunkContentMessage_;
        };
    }
}
