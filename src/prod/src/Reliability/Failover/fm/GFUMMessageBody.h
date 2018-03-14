// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class GFUMMessageBody : public Serialization::FabricSerializable
    {
    public:

        GFUMMessageBody()
        {
        }

        GFUMMessageBody(
            GenerationNumber const& generation,
            std::vector<FailoverManagerComponent::NodeInfo> && nodes,
            FailoverManagerComponent::FailoverUnit const& failoverUnit,
            Reliability::ServiceDescription const& serviceDescription,
            int64 endLookupVersion)
            : generation_(generation),
              nodes_(std::move(nodes)),
              failoverUnit_(failoverUnit),
              serviceDescription_(serviceDescription),
              endLookupVersion_(endLookupVersion)
        {
        }
            
        __declspec (property(get=get_Generation)) GenerationNumber const& Generation;
        GenerationNumber const& get_Generation() const { return generation_; }

        __declspec (property(get=get_Nodes)) std::vector<FailoverManagerComponent::NodeInfo> const& Nodes;
        std::vector<FailoverManagerComponent::NodeInfo> const& get_Nodes() const { return nodes_; }

        __declspec (property(get=get_FailoverUnit)) FailoverManagerComponent::FailoverUnit const& FailoverUnit;
        FailoverManagerComponent::FailoverUnit const& get_FailoverUnit() const { return failoverUnit_; }

        __declspec(property(get = get_ServiceDescription)) Reliability::ServiceDescription const& ServiceDescription;
        Reliability::ServiceDescription const& get_ServiceDescription() const { return serviceDescription_; }

        __declspec (property(get=get_EndLookupVersion)) int64 EndLookupVersion;
        int64 get_EndLookupVersion() const { return endLookupVersion_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.WriteLine("Generation: {0}", generation_);

            w.WriteLine("Nodes:");
            for (auto const& nodeInfo : nodes_)
            {
                w.WriteLine("  {0} {1}", nodeInfo.NodeInstance, (nodeInfo.IsUp ? L"Up" : L"Down"));
            }

            w.WriteLine();
            w.WriteLine(
                "FailoverUnit: {0} {1}", 
                failoverUnit_.FailoverUnitDescription, failoverUnit_.LookupVersion);

            for (auto it = failoverUnit_.BeginIterator; it != failoverUnit_.EndIterator; ++it)
            {
                w.Write("  {0}", it->ReplicaDescription);
            }

            w.WriteLine();
            if (!serviceDescription_.Name.empty())
            {
                w.WriteLine("ServiceDescription: {0}", serviceDescription_);
            }

            w.WriteLine();
            w.WriteLine("EndLookupVersion: {0}", endLookupVersion_);
        }

        FABRIC_FIELDS_05(generation_, nodes_, failoverUnit_, endLookupVersion_, serviceDescription_);

    private:

        GenerationNumber generation_;
        std::vector<FailoverManagerComponent::NodeInfo> nodes_;
        FailoverManagerComponent::FailoverUnit failoverUnit_;
        Reliability::ServiceDescription serviceDescription_;
        int64 endLookupVersion_;
    };
}
