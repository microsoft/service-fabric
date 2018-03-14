// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    // Structure represents information about a particular instance of a node.
    class FederationPartnerNodeHeader : public Transport::MessageHeader<Transport::MessageHeaderId::FederationPartnerNode>, public Serialization::FabricSerializable
    {
    public:
        FederationPartnerNodeHeader() : phase_(NodePhase::Shutdown), flags_(0)
        {
        }

        FederationPartnerNodeHeader(PartnerNode const& node, bool endToEnd = false)
           : nodeInstance_(node.Instance),
             phase_(node.Phase),
             address_(node.Address),
             leaseAgentAddress_(node.LeaseAgentAddress),
             leaseAgentInstanceId_(node.LeaseAgentInstanceId),
             token_(node.Token),
             nodeFaultDomainId_(node.NodeFaultDomainId),
             endToEnd_(endToEnd),
             ringName_(node.RingName),
             flags_(node.Flags)
        {
        }

        FederationPartnerNodeHeader(NodeInstance const& instance,
                            NodePhase::Enum phase,
                            std::wstring const & address,
                            std::wstring const & leaseAgentAddress,
                            int64 leaseAgentInstanceId,
                            RoutingToken token,
                            Common::Uri const & nodeFaultDomainId,
                            bool endToEnd,
                            std::wstring const & ringName,
                            int flags)
           : nodeInstance_(instance),
             phase_(phase),
             address_(address),
             leaseAgentAddress_(leaseAgentAddress),
             leaseAgentInstanceId_(leaseAgentInstanceId),
             token_(token),
             nodeFaultDomainId_(nodeFaultDomainId),
             endToEnd_(endToEnd),
             ringName_(ringName),
             flags_(flags)
        {
        }
        
        FederationPartnerNodeHeader(FederationPartnerNodeHeader && other)
           : nodeInstance_(other.nodeInstance_),
             phase_(other.phase_),
             address_(std::move(other.address_)),
             leaseAgentAddress_(std::move(other.leaseAgentAddress_)),
             leaseAgentInstanceId_(other.leaseAgentInstanceId_),
             token_(other.token_),
             nodeFaultDomainId_(std::move(other.nodeFaultDomainId_)),
             endToEnd_(other.endToEnd_),
             ringName_(std::move(other.ringName_)),
             flags_(other.flags_)
        {
        }

        FederationPartnerNodeHeader & operator = (FederationPartnerNodeHeader && other)
        {
            if (this != &other)
            {
                nodeInstance_ = other.nodeInstance_;
                phase_ = other.phase_;
                address_ = std::move(other.address_);
                leaseAgentAddress_ = std::move(other.leaseAgentAddress_);
                leaseAgentInstanceId_ = other.leaseAgentInstanceId_;
                token_ = other.token_;
                nodeFaultDomainId_ = other.nodeFaultDomainId_;
                endToEnd_ = other.endToEnd_;
                ringName_ = std::move(other.ringName_);
                flags_ = other.flags_;
            }

            return *this;
        }

        FABRIC_FIELDS_10(nodeInstance_, phase_, address_, token_, leaseAgentAddress_, leaseAgentInstanceId_, endToEnd_, nodeFaultDomainId_, ringName_, flags_);

        // Properties
        __declspec (property(get=getInstance)) NodeInstance const & Instance;
        __declspec (property(get=getPhase)) NodePhase::Enum Phase;
        __declspec (property(get=getAddress)) std::wstring Address;
        __declspec (property(get=getIsAvailable)) bool IsAvailable;
        __declspec (property(get=getToken)) RoutingToken Token;
        __declspec (property(get=getLeaseAgentAddress)) std::wstring LeaseAgentAddress;
        __declspec (property(get=getLeaseAgentInstanceId)) int64 LeaseAgentInstanceId;
        __declspec (property(get=getNodeFaultDomainId)) Common::Uri const& NodeFaultDomainId;
        __declspec (property(get=getRingName)) std::wstring const& RingName;
        __declspec (property(get = getFlags)) int Flags;

        //Getter functions for properties.
        NodeInstance const & getInstance() const { return nodeInstance_; }

        NodePhase::Enum getPhase() const { return phase_; }

        std::wstring getAddress() const { return address_; }

        RoutingToken getToken() const { return token_; }

        bool getIsAvailable() const { return phase_ == NodePhase::Inserting || phase_ == NodePhase::Routing; }

        std::wstring getLeaseAgentAddress() const { return leaseAgentAddress_; }

        Common::Uri const& getNodeFaultDomainId() const { return nodeFaultDomainId_; }

        int64 getLeaseAgentInstanceId() const { return leaseAgentInstanceId_; }

        bool IsEndToEnd() const { return endToEnd_; }

        std::wstring getRingName() const { return ringName_; }

        int getFlags() const { return flags_; }

        void WriteTo (Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0},{1},{2}", nodeInstance_, phase_, address_);
        }

    private:       
        NodeInstance nodeInstance_; // The node instance of the node which this information is of.
        NodePhase::Enum phase_; // The phase of the node.
        std::wstring address_; // The address of the node.
        std::wstring leaseAgentAddress_; // The address of the lease agent for this node.
        int64 leaseAgentInstanceId_; // Instance of the lease agent.
        RoutingToken token_; // The token owned by this node.
        Common::Uri nodeFaultDomainId_; // The fault domain setting of this node.
        bool endToEnd_;
        std::wstring ringName_;
        int flags_;
    };
}
