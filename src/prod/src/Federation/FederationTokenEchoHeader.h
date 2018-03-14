// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct FederationTokenEchoHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationTokenEcho>, public Serialization::FabricSerializable
    {
    public:
        FederationTokenEchoHeader()
        {
        }

        FederationTokenEchoHeader(NodeIdRange const & range, uint64 sourceVersion, uint64 targetVersion, NodeInstance const & origin)
            : range_(range), sourceVersion_(sourceVersion), targetVersion_(targetVersion), origin_(origin)
        {
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        { 
            w.Write("[{0},{1},{2},{3}]", origin_, range_, sourceVersion_, targetVersion_); 
        }

        FABRIC_FIELDS_04(origin_, range_, sourceVersion_, targetVersion_);

        // Properties
        __declspec (property(get=getRange)) NodeIdRange const& Range;
        __declspec (property(get=getSourceVersion)) uint64 SourceVersion;
        __declspec (property(get=getTargetVersion)) uint64 TargetVersion;
        __declspec (property(get=getOrigin)) NodeInstance & Origin;

        //Getter functions for properties.
        NodeInstance & getOrigin() { return origin_; }
        NodeIdRange getRange() { return range_; }
        uint64 getSourceVersion() { return sourceVersion_; }
        uint64 getTargetVersion() { return targetVersion_; }

    private:
        NodeInstance origin_;
        NodeIdRange range_;
        uint64 sourceVersion_;
        uint64 targetVersion_;
    };
}
