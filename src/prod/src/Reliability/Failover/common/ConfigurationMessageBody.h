// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    // Contains information about the configuration of the FailoverUnit
    class ConfigurationMessageBody : public Serialization::FabricSerializable
    {
    public:

        ConfigurationMessageBody()
        {}

        ConfigurationMessageBody(
            Reliability::FailoverUnitDescription const & fudesc,
            Reliability::ServiceDescription const & serviceDescription,
            std::vector<Reliability::ReplicaDescription> && replicas) 
            : fudesc_(fudesc),
              replicas_(std::move(replicas)),
              serviceDesc_(serviceDescription)
        {}

        __declspec (property(get=get_FailoverUnitDescription)) Reliability::FailoverUnitDescription const & FailoverUnitDescription;
        Reliability::FailoverUnitDescription const & get_FailoverUnitDescription() const { return fudesc_; }

        __declspec(property(get=get_ServiceDescription)) Reliability::ServiceDescription const & ServiceDescription;
        Reliability::ServiceDescription const & get_ServiceDescription() const { return serviceDesc_; }

        __declspec (property(get=get_ReplicaDescriptions)) std::vector<Reliability::ReplicaDescription> const & ReplicaDescriptions;
        std::vector<Reliability::ReplicaDescription> const & get_ReplicaDescriptions() const { return replicas_; }
        std::vector<Reliability::ReplicaDescription> & get_ReplicaDescriptions() { return replicas_; }

        __declspec (property(get=get_ReplicaCount)) size_t ReplicaCount;
        size_t get_ReplicaCount() const { return replicas_.size(); }

        __declspec (property(get=get_BeginReplicaDescriptionIterator)) ReplicaDescriptionConstIterator BeginIterator;
        ReplicaDescriptionConstIterator get_BeginReplicaDescriptionIterator() const { return replicas_.begin(); }       

        __declspec (property(get=get_EndReplicaDescriptionIterator)) ReplicaDescriptionConstIterator EndIterator;
        ReplicaDescriptionConstIterator get_EndReplicaDescriptionIterator() const { return replicas_.end(); }

        ReplicaDescription const* GetReplica(Federation::NodeId nodeId) const
        {
            for (ReplicaDescription const& desc : replicas_)
            {
                if (desc.FederationNodeId == nodeId)
                {
                    return &desc;
                }
            }

            return nullptr;
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;
        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_03(fudesc_, replicas_, serviceDesc_);

    private:

        Reliability::FailoverUnitDescription fudesc_;
        Reliability::ServiceDescription serviceDesc_;
        std::vector<Reliability::ReplicaDescription> replicas_;

    };
}
