// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class ServerAuthHeader : public MessageHeader<MessageHeaderId::ServerAuth>, public Serialization::FabricSerializable
    {
    public:
        ServerAuthHeader();
        ServerAuthHeader(bool willSendConnectionAuthStatus);

        bool WillSendConnectionAuthStatus() const;
        void SetClaimsRetrievalMetadata(ClaimsRetrievalMetadataSPtr const & metadata) { metadata_ = metadata; }
        ClaimsRetrievalMetadataSPtr && TakeClaimsRetrievalMetadata() { return std::move(metadata_); }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(willSendConnectionAuthStatus_, metadata_);

    private:
        bool willSendConnectionAuthStatus_;
        ClaimsRetrievalMetadataSPtr metadata_;
    };
}

