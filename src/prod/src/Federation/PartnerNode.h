// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class FederationPartnerNodeHeader;

    class PartnerNode
    {
    public:

        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Takes a node info header and creates a partner node object from
        //      that.
        //
        // Arguments:
        //      info - The node infromation header.
        //
        PartnerNode(NodeConfig const & nodeConfig, SiteNode & siteNode);
        PartnerNode(FederationPartnerNodeHeader const& info, SiteNode & siteNode);
        PartnerNode(std::shared_ptr<PartnerNode const> const & oldNode, NodePhase::Enum phase, RoutingToken token);
        PartnerNode(PartnerNode const & other);
        
        //---------------------------------------------------------------------
        // Constructor.
        //
        // Description:
        //      Takes a node id and listener address to create a partner for a 
        //      site node. 
        //
        //      This normally would only called from inside SiteNode constructor.
        //      However, bootstrap needs to send messages too and the current messaging API
        //      is PartnerNode-based. If we create a request/reply layer that operates on
        //      addresses/reply targets, then this ctor doesn't need to be public.
        //
        // Arguments:
        //        id - the node id.
        //      address - the address of listener.
        //
        PartnerNode(
            NodeId const& id,
            uint64 instanceId,
            std::wstring const & address,
            std::wstring const & leaseAgentAddress,
            Common::Uri const & nodeFaultDomainId,
            std::wstring const & ringName);

        void SetAsUnknown() const;
        void OnSend(bool expectsReply) const;
        void OnReceive(bool expectsReply) const;
        void SetGlobalTimeUpperLimit(Common::TimeSpan value) const;
        void IncreaseGlobalTimeUpperLimit(Common::TimeSpan value) const;

        void IncrementInstanceId();

        // Properties
        __declspec (property(get=getNodeId)) NodeId Id;
        __declspec (property(get=getIdString)) std::wstring const & IdString;
        __declspec (property(get=getNodeInstance)) NodeInstance const & Instance;
        __declspec (property(get=getAddress, put=setAddress)) std::wstring const & Address;
        __declspec (property(get=getLeaseAgentAddress)) std::wstring const & LeaseAgentAddress;
        __declspec (property(get=getLeaseAgentInstanceId)) int64 LeaseAgentInstanceId;
        __declspec (property(get=getPhase,put=setPhase)) NodePhase::Enum Phase;
        __declspec (property(get=getTarget)) Transport::ISendTarget::SPtr const & Target;
        __declspec (property(get=getIsRouting)) bool IsRouting;
        __declspec (property(get=getIsAvailable)) bool IsAvailable;
        __declspec (property(get=getIsShutdown)) bool IsShutdown;
        __declspec (property(get=getIsUnknown)) bool IsUnknown;
        __declspec (property(get=getRoutingToken)) RoutingToken const & Token;
        __declspec (property(get=getLastUpdated)) Common::StopwatchTime LastUpdated;
        __declspec (property(get=getLastAccessed)) Common::StopwatchTime LastAccessed;
        __declspec (property(get=getLastConsider)) Common::StopwatchTime LastConsider;
        __declspec (property(get=getNextLivenessUpdate)) Common::StopwatchTime NextLivenessUpdate;
        __declspec (property(get=getNodeFaultDomainId)) Common::Uri const& NodeFaultDomainId;
        __declspec (property(get=getGlobalTimeUpperLimit)) Common::TimeSpan GlobalTimeUpperLimit;
        __declspec (property(get=getRingName)) std::wstring const & RingName;
        __declspec (property(get=getFlags)) int Flags;

        // Getter function for properties.
        NodeId getNodeId() const {return nodeInstance_.Id;}
        std::wstring const & getIdString() const { return idString_; }
        NodeInstance const & getNodeInstance() const {return nodeInstance_;}
        NodePhase::Enum getPhase() const {return phase_;}
        void setPhase(NodePhase::Enum value) {phase_ = value;}
        std::wstring const & getAddress() const {return address_;}
        void setAddress(std::wstring const & value) {address_ = value;}
        bool getIsRouting() const {return phase_ == NodePhase::Routing;}
        std::wstring const & getLeaseAgentAddress() const {return leaseAgentAddress_;}
        int64 getLeaseAgentInstanceId() const {return leaseAgentInstanceId_;}
        bool getIsAvailable() const {return phase_ == NodePhase::Inserting || phase_ == NodePhase::Routing;}
        bool getIsShutdown() const {return phase_ == NodePhase::Shutdown;}
        bool getIsUnknown() const;
        Common::StopwatchTime getLastUpdated() const { return lastUpdated_; }
        Common::StopwatchTime getLastAccessed() const { return lastAccessed_; }
        Common::StopwatchTime getLastConsider() const { return lastConsider_; }
        Common::StopwatchTime getNextLivenessUpdate() const { return nextLivenessUpdate_; }
        void UpdateLastAccess(Common::StopwatchTime time) const { lastAccessed_= time;}
        void UpdateLastConsider(Common::StopwatchTime time) const { lastConsider_ = time; }
        Transport::ISendTarget::SPtr const & getTarget() const {return target_;}
        RoutingToken const & getRoutingToken() const { return token_; }
        Common::Uri const& getNodeFaultDomainId() const { return nodeFaultDomainId_; }
        Common::TimeSpan getGlobalTimeUpperLimit() const;
        std::wstring const & getRingName() const { return ringName_; }
        int getFlags() const { return flags_; }

        void Test_SetToken(RoutingToken const & token)
        {
            token_ = token;
        }

        bool IsExtendedArbitrationEnabled() const
        {
            return ((flags_ & 0x01) != 0);
        }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const& options) const;

        virtual ~PartnerNode();

    protected:
        void SetTarget(Transport::ISendTarget::SPtr && target)
        {
            target_ = std::move(target);
        }

        void SetLeaseAgentInstanceId(int64 instanceId)
        {
            leaseAgentInstanceId_ = instanceId;
        }

        RoutingToken & GetTokenForUpdate()
        {
            return token_;
        }
        
    private:
        NodeId siteId_;
        NodeInstance nodeInstance_; // Instance information for this node instance.
        NodePhase::Enum phase_; // The phase of this partner node.
        RoutingToken token_; // The routing token.
        std::wstring address_; // The listener address for this partner node.
        std::wstring leaseAgentAddress_; // The listener address for the lease agent of this partner node.
        Common::Uri nodeFaultDomainId_; // The fault domain setting of this partner node.
        int64 leaseAgentInstanceId_; // Instance of the lease agent.
        std::wstring idString_;
        Common::StopwatchTime lastUpdated_; // Last time phase changed.
        mutable Common::StopwatchTime lastAccessed_; // Last time accessed from routing table.
        mutable Common::StopwatchTime lastConsider_; // Last time considered in routing table.
        mutable Common::StopwatchTime unknownStart_;
        mutable Common::StopwatchTime nextLivenessUpdate_;
        mutable bool isUnknown_;
        mutable Transport::ISendTarget::SPtr target_; // The channel to this remote node from the current site node.
        mutable Common::TimeSpan globalTimeUpperLimit_;
        mutable Common::StopwatchTime lastGlobalTimeRefresh_;
        std::wstring ringName_;
        int flags_;
    };
}
