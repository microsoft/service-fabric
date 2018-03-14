// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class LFUMMessageBody : public Serialization::FabricSerializable
    {
    public:
        LFUMMessageBody()
        {
        }

        LFUMMessageBody(
            GenerationNumber const & generation,
            NodeDescription && node,
            std::vector<FailoverUnitInfo> && failoverUnitInfos,
            bool anyReplicaFound)
            :   generation_(generation),
                node_(std::move(node)),
                failoverUnitInfos_(std::move(failoverUnitInfos)),
                anyReplicaFound_(anyReplicaFound)
        {
        }
            
        __declspec (property(get=get_Generation)) GenerationNumber const & Generation;
        GenerationNumber const & get_Generation() const { return generation_; }

        __declspec (property(get=get_Node)) NodeDescription const & Node;
        NodeDescription const & get_Node() const { return node_; }

        __declspec (property(get=get_FailoverUnitInfos)) std::vector<Reliability::FailoverUnitInfo> const& FailoverUnitInfos;
        std::vector<Reliability::FailoverUnitInfo> const& get_FailoverUnitInfos() const { return failoverUnitInfos_; }

        __declspec (property(get=get_AnyReplicaFound)) bool AnyReplicaFound;
        bool get_AnyReplicaFound() const { return anyReplicaFound_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.WriteLine("{0} {1} {2}", node_, generation_, anyReplicaFound_);
            
            for (Reliability::FailoverUnitInfo const& failoverUnitInfo : failoverUnitInfos_)
            {
                w.Write(failoverUnitInfo);
            }
        }

        FABRIC_FIELDS_04(generation_, node_, failoverUnitInfos_, anyReplicaFound_);

    private:
        GenerationNumber generation_;
        NodeDescription node_;
        std::vector<Reliability::FailoverUnitInfo> failoverUnitInfos_;
        bool anyReplicaFound_;
    };
}
