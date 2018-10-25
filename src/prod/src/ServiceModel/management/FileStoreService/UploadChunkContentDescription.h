// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace FileStoreService
    {
        class UploadChunkContentDescription
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
        {

            DENY_COPY(UploadChunkContentDescription)
            DEFAULT_MOVE_CONSTRUCTOR(UploadChunkContentDescription)
            DEFAULT_MOVE_ASSIGNMENT(UploadChunkContentDescription)

        public:

            UploadChunkContentDescription();

            UploadChunkContentDescription(
                Common::Guid const & sessionId,
                uint64 startPosition,
                uint64 endPosition);

            __declspec(property(get = get_SessionId)) Common::Guid const & SessionId;
            Common::Guid const & get_SessionId() const { return sessionId_; }

            __declspec(property(get = get_StartPosition)) uint64 StartPosition;
            inline uint64 get_StartPosition() const { return startPosition_; }

            __declspec(property(get = get_EndPosition)) uint64 EndPosition;
            inline uint64 get_EndPosition() const { return endPosition_; }
        
            virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_03(sessionId_, startPosition_, endPosition_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::SessionId, sessionId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::StartPosition, startPosition_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::EndPosition, endPosition_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            Common::Guid sessionId_;
            uint64 startPosition_;
            uint64 endPosition_;
        };
    }
}
