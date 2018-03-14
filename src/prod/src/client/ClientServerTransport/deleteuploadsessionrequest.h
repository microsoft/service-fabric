// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class DeleteUploadSessionRequest : public ServiceModel::ClientServerMessageBody
        {
            DENY_COPY(DeleteUploadSessionRequest)

        public:
            DeleteUploadSessionRequest();
            DeleteUploadSessionRequest(Common::Guid const & sessionId);

            __declspec(property(get=get_SessionId)) Common::Guid const & SessionId;
            inline Common::Guid const & get_SessionId() const { return sessionId_; }
            
            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
            FABRIC_FIELDS_01(sessionId_);

        private:
            Common::Guid sessionId_;
        };
    }
}
