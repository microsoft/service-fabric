// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadSessionRequest 
            : public ServiceModel::ClientServerMessageBody
        {
            DENY_COPY(UploadSessionRequest);

        public:
            UploadSessionRequest();
            UploadSessionRequest(
                std::wstring const & storeRelativePath,
                Common::Guid const & sessionId);

            __declspec(property(get = get_StoreRelativePath)) std::wstring const & StoreRelativePath;
            inline std::wstring const & get_StoreRelativePath() const { return storeRelativePath_; }

            __declspec(property(get=get_SessionId)) Common::Guid const & SessionId;
            inline Common::Guid const & get_SessionId() const { return sessionId_; }
            
            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            FABRIC_FIELDS_02(storeRelativePath_, sessionId_);

        private:
            Common::Guid sessionId_;
            std::wstring storeRelativePath_;
        };
    }
}
