// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadChunkRequest 
            : public ServiceModel::ClientServerMessageBody
        {
            DENY_COPY(UploadChunkRequest)

        public:

            UploadChunkRequest();
            UploadChunkRequest(
                std::wstring const & stagingRelativePath,
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition);

            __declspec(property(get = get_StagingRelativePath)) std::wstring const & StagingRelativePath;
            inline std::wstring const & get_StagingRelativePath() const { return stagingRelativePath_; }

            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            inline Common::Guid const & get_SessionId() const { return sessionId_; }

            __declspec(property(get = get_StartPosition)) uint64 StartPosition;
            inline uint64 get_StartPosition() const { return startPosition_; }

            __declspec(property(get = get_EndPosition)) uint64 EndPosition;
            inline uint64 get_EndPosition() const { return endPosition_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(stagingRelativePath_, sessionId_, startPosition_, endPosition_);

        private:
            std::wstring stagingRelativePath_;
            Common::Guid sessionId_;
            uint64 startPosition_;
            uint64 endPosition_;
        };
    }
}
