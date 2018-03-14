// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadSession 
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {
            DEFAULT_COPY_CONSTRUCTOR(UploadSession)

        public:

            UploadSession();
            explicit UploadSession(std::vector<UploadSessionInfo> const & uploadSessions);
            UploadSession& operator = (UploadSession const & other);
            UploadSession& operator = (std::vector<UploadSessionInfo> const & sessions);
            UploadSession& operator = (UploadSession && other);
            UploadSession& operator = (std::vector<UploadSessionInfo> && sessions);

            __declspec(property(get = get_UploadSession, put = put_UploadSession)) std::vector<UploadSessionInfo> & UploadSessions;
            std::vector<UploadSessionInfo> & get_UploadSession() { return uploadSessions_; }
            void put_UploadSession(std::vector<UploadSessionInfo> const & uploadSessions)
            { 
                std::copy(uploadSessions.begin(), uploadSessions.end(), back_inserter(this->uploadSessions_));
                //this->uploadSessions_ = uploadSessions; 
            }

            std::wstring ToString() const;
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(uploadSessions_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::UploadSessions, uploadSessions_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            std::vector<UploadSessionInfo> uploadSessions_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::FileStoreService::UploadSessionInfo);
