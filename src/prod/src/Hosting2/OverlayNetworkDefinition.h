// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class OverlayNetworkDefinition : 
        public Common::IFabricJsonSerializable,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;

        DENY_COPY(OverlayNetworkDefinition)

    public:
        OverlayNetworkDefinition::OverlayNetworkDefinition() {}

        OverlayNetworkDefinition(
            wstring networkName,
            wstring networkId,
            wstring networkType,
            wstring subnet,
            wstring gateway,
            int mask,
            int vxlanId,
            wstring startMacPoolAddress,
            wstring endMacPoolAddress,
            wstring nodeIpAddress,
            wstring nodeId,
            wstring nodeName,
            std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded,
            std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved);

        virtual ~OverlayNetworkDefinition();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::NetworkNameParameter, networkName_, !networkName_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::NetworkTypeParameter, networkType_, !networkType_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::NetworkAdapterNameParameter, networkAdapterName_, !networkAdapterName_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::StartMacPoolAddressParameter, startMacPoolAddress_, !startMacPoolAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::EndMacPoolAddressParameter, endMacPoolAddress_, !endMacPoolAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::SubnetParameter, subnet_, !subnet_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::GatewayAddressParameter, gateway_, !gateway_.empty())
            SERIALIZABLE_PROPERTY(OverlayNetworkDefinition::VxlanIdParameter, vxlanId_)
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkDefinition::NodeIpAddressParameter, nodeIpAddress_, !nodeIpAddress_.empty())
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        // Get the name of the overlay network.
        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return this->networkName_; };

        // Get the id of the overlay network.
        __declspec(property(get = get_NetworkId)) std::wstring const & NetworkId;
        std::wstring const & get_NetworkId() const { return this->networkId_; };

         // Get the network type.
         __declspec(property(get = get_NetworkType)) std::wstring const & NetworkType;
         std::wstring const & get_NetworkType() const { return this->networkType_; };

        // Get the network adapter name used for the overlay network.
        __declspec(property(get = get_NetworkAdapterName, put = set_NetworkAdapterName)) std::wstring const & NetworkAdapterName;
        std::wstring const & get_NetworkAdapterName() const { return this->networkAdapterName_; };
        inline void set_NetworkAdapterName(std::wstring const & value) { networkAdapterName_ = value; }

        // Get the subnet of the overlay network.
        __declspec(property(get = get_Subnet)) std::wstring const & Subnet;
        std::wstring const & get_Subnet() const { return this->subnet_; };

        // Get the gateway of the overlay network.
        __declspec(property(get = get_Gateway)) std::wstring const & Gateway;
        std::wstring const & get_Gateway() const { return this->gateway_; };

        // Get the mask of the overlay network.
        __declspec(property(get = get_Mask)) int Mask;
        int get_Mask() const { return this->mask_; };

        // Get the vxlan id of the overlay network.
        __declspec(property(get = get_VxlanId)) int VxlanId;
        int get_VxlanId() const { return this->vxlanId_; };

        // Get start mac pool address of the overlay network.
        __declspec(property(get = get_StartMacPoolAddress)) std::wstring const & StartMacPoolAddress;
        std::wstring const & get_StartMacPoolAddress() const { return this->startMacPoolAddress_; };

        // Get end mac pool address of the overlay network.
        __declspec(property(get = get_EndMacPoolAddress)) std::wstring const & EndMacPoolAddress;
        std::wstring const & get_EndMacPoolAddress() const { return this->endMacPoolAddress_; };

        // Get the node ip address.
        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return this->nodeIpAddress_; };

        // Get the node id.
        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return this->nodeId_; };

        // Get the node name.
        __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return this->nodeName_; };

        // Get the ip/mac address collection to be added.
        __declspec(property(get = get_IpMacAddressMapToBeAdded)) std::map<std::wstring, std::wstring> const & IpMacAddressMapToBeAdded;
        std::map<std::wstring, std::wstring> const & get_IpMacAddressMapToBeAdded() const { return this->ipMacAddressMapToBeAdded_; };

        // Get the ip/mac address collection to be removed.
        __declspec(property(get = get_IpMacAddressMapToBeRemoved)) std::map<std::wstring, std::wstring> const & IpMacAddressMapToBeRemoved;
        std::map<std::wstring, std::wstring> const & get_IpMacAddressMapToBeRemoved() const { return this->ipMacAddressMapToBeRemoved_; };

    private:

        // Populate network resources to be added/removed
        void PopulateNetworkResources(
            std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded,
            std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved);

        // Network name
        // Passed in from the network inventory manager
        std::wstring networkName_;

        // Network id
        // Passed in from the network inventory manager
        std::wstring networkId_;

        // Network adapter name
        std::wstring networkAdapterName_;

        // Subnet in CIDR format
        // Passed in from the network inventory manager
        std::wstring subnet_;

        // Gateway in dot format
        // Passed in from the network inventory manager
        std::wstring gateway_;

        // Subnet mask
        // Passed in from the network inventory manager
        int mask_;

        // Start Mac pool address
        // Passed in from the network inventory manager
        std::wstring startMacPoolAddress_;

        // End Mac pool address
        // Passed in from the network inventory manager
        std::wstring endMacPoolAddress_;

        // Node ip address in dot format
        std::wstring nodeIpAddress_;

        // Node id
        std::wstring nodeId_;

        // Node name
        std::wstring nodeName_;

        // Network type
        std::wstring networkType_;

        // Vxlan id
        // Passed in from the network inventory manager
        int vxlanId_;

        // Current batch of ip and mac supplied for container activation
        // Passed in from the network inventory manager
        std::map<std::wstring, std::wstring> ipMacAddressMapToBeAdded_;

        // Batch of ip and mac removed for container activation
        // Passed in from the network inventory manager
        std::map<std::wstring, std::wstring> ipMacAddressMapToBeRemoved_;

        static WStringLiteral const NetworkNameParameter;
        static WStringLiteral const NetworkTypeParameter;
        static WStringLiteral const NetworkAdapterNameParameter;
        static WStringLiteral const StartMacPoolAddressParameter;
        static WStringLiteral const EndMacPoolAddressParameter;
        static WStringLiteral const SubnetParameter;
        static WStringLiteral const GatewayAddressParameter;
        static WStringLiteral const VxlanIdParameter;
        static WStringLiteral const NodeIpAddressParameter;
    };
}
