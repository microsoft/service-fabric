// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct FederationRoutingTokenHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationRoutingToken>, public Serialization::FabricSerializable
    {
    public:
        FederationRoutingTokenHeader()
            : range_(), sourceVersion_(0), targetVersion_(0)
        {
        }

        FederationRoutingTokenHeader(NodeIdRange const & range, uint64 sourceVersion, uint64 const & targetVersion);

        __declspec (property(get=getIsEmpty)) bool IsEmpty;
        __declspec (property(get=getToken)) RoutingToken const& Token;
        __declspec (property(get=getRange)) NodeIdRange const& Range;
        __declspec (property(get=getSourceVersion)) uint64 SourceVersion; // Used at the source to to reaccept rejected token.
        __declspec (property(get=getTargetVersion)) uint64 TargetVersion; // Used at the target for stale message discovery.
        __declspec (property(get=getExpiryTime)) Common::StopwatchTime ExpiryTime;

        NodeIdRange getRange() const { return range_; }
        uint64 getSourceVersion() const { return sourceVersion_; }
        uint64 getTargetVersion() const { return targetVersion_; }
        bool getIsEmpty() const { return range_.IsEmpty; }
        RoutingToken getToken() const { return RoutingToken(range_, targetVersion_);}
        Common::StopwatchTime getExpiryTime() const { return expiryTime_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(range_, sourceVersion_, targetVersion_);

    private:
        NodeIdRange range_;
        uint64 sourceVersion_;
        uint64 targetVersion_;
        Common::StopwatchTime expiryTime_;
    };
}
