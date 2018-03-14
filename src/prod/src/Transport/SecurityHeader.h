// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class SecurityHeader : public MessageHeader<MessageHeaderId::MessageSecurity>, public Serialization::FabricSerializable
    {
        DENY_COPY_ASSIGNMENT(SecurityHeader);

    public:
        SecurityHeader() { }

        SecurityHeader(
            ProtectionLevel::Enum headerSecurity,
            ProtectionLevel::Enum bodySecurity)
        : headerSecurity_(headerSecurity)
        , bodySecurity_(bodySecurity)
        {
        }

        SecurityHeader(SecurityHeader && rhs)
            :   headerSecurity_(rhs.headerSecurity_),
                bodySecurity_(rhs.bodySecurity_),
                headerToken_(std::move(rhs.headerToken_)),
                headerData_(std::move(rhs.headerData_)),
                headerPadding_(std::move(rhs.headerPadding_)),
                bodyToken_(std::move(rhs.bodyToken_)),
                bodyPadding_(std::move(rhs.bodyPadding_))
        {
        }

        SecurityHeader & operator=(SecurityHeader && rhs)
        {
            if (this != &rhs)
            {
                headerSecurity_ = rhs.headerSecurity_;
                bodySecurity_ = rhs.bodySecurity_;
                headerToken_ = std::move(rhs.headerToken_);
                headerData_ = std::move(rhs.headerData_);
                headerPadding_ = std::move(rhs.headerPadding_);
                bodyToken_ = std::move(rhs.bodyToken_);
                bodyPadding_ = std::move(rhs.bodyPadding_);
            }

            return *this;
        }

        __declspec(property(get=getHeaderSecurity)) ProtectionLevel::Enum MessageHeaderProtectionLevel;
        __declspec(property(get=getBodySecurity)) ProtectionLevel::Enum MessageBodyProtectionLevel;
        __declspec(property(get=getHeaderToken)) std::vector<byte> & HeaderToken;
        __declspec(property(get=getHeaderData)) std::vector<byte> & HeaderData;
        __declspec(property(get=getHeaderPadding)) std::vector<byte> & HeaderPadding;
        __declspec(property(get=getBodyToken)) std::vector<byte> & BodyToken;
        __declspec(property(get=getBodyPadding)) std::vector<byte> & BodyPadding;

        ProtectionLevel::Enum getHeaderSecurity() const { return headerSecurity_; }
        ProtectionLevel::Enum getBodySecurity() const { return bodySecurity_; }
        std::vector<byte> & getHeaderToken() { return headerToken_; }
        std::vector<byte> & getHeaderData() { return headerData_; }
        std::vector<byte> & getHeaderPadding() { return headerPadding_; }
        std::vector<byte> & getBodyToken() { return bodyToken_; }
        std::vector<byte> & getBodyPadding() { return bodyPadding_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w << headerSecurity_;
            w << bodySecurity_;
            w << headerToken_;
            w << headerData_;
            w << headerPadding_;
            w << bodyToken_;
            w << bodyPadding_;
        }

        FABRIC_FIELDS_07(
                   headerSecurity_,
                   bodySecurity_,
                   headerToken_,
                   headerData_,
                   headerPadding_,
                   bodyToken_,
                   bodyPadding_);

    private:
        ProtectionLevel::Enum headerSecurity_;
        ProtectionLevel::Enum bodySecurity_;
        std::vector<byte> headerToken_;
        std::vector<byte> headerData_;
        std::vector<byte> headerPadding_;
        std::vector<byte> bodyToken_;
        std::vector<byte> bodyPadding_;

    };
}

