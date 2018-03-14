// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    struct TraceProbeNode : public Serialization::FabricSerializable
    {
    public:

        TraceProbeNode()
            : nodeInstance_(), isShutdown_(), neighborhoodVersion_(0)
        {
        }

        TraceProbeNode(NodeInstance const& instance, bool isShutdown, uint neighborhoodVersion = 0)
            : nodeInstance_(instance), isShutdown_(isShutdown), neighborhoodVersion_(neighborhoodVersion)
        {
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
        { 
            w.Write(nodeInstance_);
            if (isShutdown_)
            {
                w.Write("(S)");
            }
            w.Write(" ");
            w.Write(neighborhoodVersion_);
        }

        FABRIC_FIELDS_03(nodeInstance_, isShutdown_, neighborhoodVersion_);

        // Properties
        __declspec (property(get=getInstance)) NodeInstance const & Instance;
        __declspec (property(get=getIsShutdown)) bool IsShutdown;
        __declspec (property(get=getNeighborhoodVersion)) uint NeighborhoodVersion;

        //Getter functions for properties.
        NodeInstance const & getInstance() const { return nodeInstance_; }
        bool getIsShutdown() const { return isShutdown_; }
        uint getNeighborhoodVersion() const { return neighborhoodVersion_; }

        bool Match(PartnerNodeSPtr partner) const
        {
            if (nodeInstance_ == partner->Instance)
            {
                // If instance is the same, the only way it can be unsafe
                // is when the current state is shutdown and previously
                // it was not.
                return (isShutdown_ || (!partner->IsShutdown));
            }
            else
            {
                // If the instance seen in the first pass is older, the node
                // must have restarted (even more than once) and there is no
                // way to be sure that it has never become routing.
                return (nodeInstance_.InstanceId > partner->Instance.InstanceId);
            }
         }

    private:       
        NodeInstance nodeInstance_;
        bool isShutdown_;
        uint neighborhoodVersion_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Federation::TraceProbeNode);
