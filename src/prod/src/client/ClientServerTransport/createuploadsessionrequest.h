// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class CreateUploadSessionRequest 
            : public ServiceModel::ClientServerMessageBody
        {
        public:

            CreateUploadSessionRequest();

            CreateUploadSessionRequest(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId,
                uint64 fileSize);
                
            __declspec(property(get = get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            inline Common::Guid const & get_SessionId() const { return sessionId_; }

            __declspec(property(get = get_FileSize)) uint64 FileSize;
            inline uint64 get_FileSize() const { return fileSize_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(storeRelativePath_, sessionId_, fileSize_);

        private:
            std::wstring storeRelativePath_;
            Common::Guid sessionId_;
            uint64 fileSize_;
        };
    }
}
