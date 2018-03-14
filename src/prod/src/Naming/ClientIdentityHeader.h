// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    //
    // This class represents the identity of the client for use within the gateway.
    //
    class ClientIdentityHeader
        : public Transport::MessageHeader<Transport::MessageHeaderId::ClientIdentity>
        , public Serialization::FabricSerializable
    {
    public:
        ClientIdentityHeader()
        {
        }

        ClientIdentityHeader(Transport::ISendTarget::SPtr const &sendTarget)
            : targetId_()
            , friendlyName_()
        {
            targetId_ = Common::wformatString("{0}::{1}", sendTarget->Id(), Common::Guid::NewGuid().ToString());
        }

        ClientIdentityHeader(Transport::ISendTarget::SPtr const &sendTarget, std::wstring const &name)
            : targetId_()
            , friendlyName_(name)
        {
            targetId_ = Common::wformatString("{0}::{1}", sendTarget->Id(), Common::Guid::NewGuid().ToString());
        }

        ClientIdentityHeader(std::wstring const &targetId, std::wstring const &name)
            : targetId_(targetId)
            , friendlyName_(name)
        {
        }

        __declspec (property(get=get_TargetId)) std::wstring const& TargetId;
        std::wstring const& get_TargetId() const { return targetId_; }

        __declspec (property(get=get_FriendlyName)) std::wstring const& FriendlyName;
        std::wstring const& get_FriendlyName() const { return friendlyName_; }

        bool operator < (ClientIdentityHeader const & other) const { return targetId_ < other.targetId_; }
        bool operator <= (ClientIdentityHeader const & other) const { return targetId_ <= other.targetId_; }
        bool operator > (ClientIdentityHeader const & other) const { return targetId_ > other.targetId_; }
        bool operator >= (ClientIdentityHeader const & other) const { return targetId_ >= other.targetId_; }
        bool operator == (ClientIdentityHeader const & other) const { return targetId_ == other.targetId_; }
        bool operator != (ClientIdentityHeader const & other) const { return targetId_ != other.targetId_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write(
            "{0}#{1}",
            targetId_,
            friendlyName_);
        }

        FABRIC_FIELDS_02(targetId_, friendlyName_);

    private:
        std::wstring targetId_; // maps to the send target id
        std::wstring friendlyName_;
    };
}
