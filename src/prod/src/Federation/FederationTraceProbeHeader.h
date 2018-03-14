// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct FederationTraceProbeHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationTraceProbe>, public Serialization::FabricSerializable
    {
    public:
        FederationTraceProbeHeader()
        {
        }

        FederationTraceProbeHeader(TraceProbeNode const & startNode, uint64 sourceVersion, bool isSuccDirection, NodeIdRange const & range)
            : traceProbeNodes_(), isSuccDirection_(isSuccDirection), sourceVersion_(sourceVersion), range_(range)
        {
            AddNode(startNode);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        { 
            w.Write("[{0},{1},{2},{3}]", isSuccDirection_, range_, sourceVersion_, traceProbeNodes_); 
        }

        FABRIC_FIELDS_04(traceProbeNodes_, isSuccDirection_, sourceVersion_, range_);

        // Properties
        __declspec (property(get=getTraceProbeNodes)) std::vector<TraceProbeNode> const & TraceProbeNodes;
        __declspec (property(get=getIsSuccDirection)) bool IsSuccDirection;
        __declspec (property(get=getRange)) NodeIdRange const & Range;

        //Getter functions for properties.
        std::vector<TraceProbeNode> const & getTraceProbeNodes() const { return traceProbeNodes_; }
        bool getIsSuccDirection() const { return isSuccDirection_; }
        NodeIdRange const & getRange() const { return range_; }

        void AddNode(TraceProbeNode const & node)
        {
            traceProbeNodes_.push_back(node);
        }

        void Remove()
        {
            traceProbeNodes_.pop_back();
        }

        void Replace(size_t index, TraceProbeNode const & node)
        {
            traceProbeNodes_[index] = node;
        }

        uint64 GetSourceVersion()
        {
            return sourceVersion_;
        }

    private:
        std::vector<TraceProbeNode> traceProbeNodes_;
        bool isSuccDirection_;
        uint64 sourceVersion_;
        NodeIdRange range_;
    };
}
