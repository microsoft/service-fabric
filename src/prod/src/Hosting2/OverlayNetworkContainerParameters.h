// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Hosting2
{
    class OverlayNetworkContainerParameters :
        public Common::IFabricJsonSerializable,
        private Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>
    {
        using Common::TextTraceComponent<Common::TraceTaskCodes::Hosting>::WriteInfo;

        DENY_COPY(OverlayNetworkContainerParameters)

    public:

        OverlayNetworkContainerParameters(
            OverlayNetworkDefinitionSPtr networkDefinition,
            std::wstring ipAddress,
            std::wstring macAddress,
            std::wstring containerId,
            std::wstring dnsServerList);
        
        OverlayNetworkContainerParameters(
            std::wstring containerId,
            std::wstring networkName,
            bool outboundNAT);

        ~OverlayNetworkContainerParameters();

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::ContainerIdParameter, containerId_, !containerId_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::NetworkNameParameter, networkName_, !networkName_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::MacAddressParameter, macAddress_, !macAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::IPV4AddressParameter, ipAddress_, !ipAddress_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::GatewayAddressParameter, gateway_, !gateway_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::DNSServerListParameter, dnsServerList_, !dnsServerList_.empty())
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::NodeIpAddressParameter, nodeIpAddress_, !nodeIpAddress_.empty())
            SERIALIZABLE_PROPERTY(OverlayNetworkContainerParameters::OutboundNATParameter, outboundNAT_)
            SERIALIZABLE_PROPERTY_IF(OverlayNetworkContainerParameters::NetworkDefinitionParameter, networkDefinition_, networkDefinition_ != nullptr)
        END_JSON_SERIALIZABLE_PROPERTIES()

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NetworkId)) std::wstring const & NetworkId;
        std::wstring const & get_NetworkId() const { return networkId_; }

        __declspec(property(get = get_ContainerId)) std::wstring const & ContainerId;
        std::wstring const & get_ContainerId() const { return containerId_; }

        __declspec(property(get = get_IPV4Address)) std::wstring const & IPV4Address;
        std::wstring const & get_IPV4Address() const { return ipAddress_; }

        __declspec(property(get = get_MacAddress)) std::wstring const & MacAddress;
        std::wstring const & get_MacAddress() const { return macAddress_; }

        __declspec(property(get = get_NodeIpAddress)) std::wstring const & NodeIpAddress;
        std::wstring const & get_NodeIpAddress() const { return nodeIpAddress_; }

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }

        __declspec(property(get = get_Subnet)) std::wstring const & Subnet;
        std::wstring const & get_Subnet() const { return subnet_; }

        __declspec(property(get = get_Gateway)) std::wstring const & Gateway;
        std::wstring const & get_Gateway() const { return gateway_; }

        __declspec(property(get = get_DnsServerList)) std::wstring const & DnsServerList;
        std::wstring const & get_DnsServerList() const { return dnsServerList_; }

        __declspec(property(get = get_OutboundNAT)) bool const & OutboundNAT;
        bool const & get_OutboundNAT() const { return outboundNAT_; }

    private:

        // Network definition
        OverlayNetworkDefinitionSPtr networkDefinition_;

        // Container id
        std::wstring containerId_;

        // Container ip address
        std::wstring ipAddress_;

        // Container mac address
        std::wstring macAddress_;

        // Dns server list
        std::wstring dnsServerList_;

        // Network name
        std::wstring networkName_;

        // Network id
        std::wstring networkId_;

        // Node ip address
        std::wstring nodeIpAddress_;

        // Node id
        std::wstring nodeId_;

        // Node name
        std::wstring nodeName_;

        // Subnet
        std::wstring subnet_;

        // Gateway
        std::wstring gateway_;

        // Outbound NAT network policy requested
        bool outboundNAT_;

        static WStringLiteral const ContainerIdParameter;
        static WStringLiteral const NetworkNameParameter;
        static WStringLiteral const MacAddressParameter;
        static WStringLiteral const IPV4AddressParameter;
        static WStringLiteral const GatewayAddressParameter;
        static WStringLiteral const DNSServerListParameter;
        static WStringLiteral const NodeIpAddressParameter;
        static WStringLiteral const OutboundNATParameter;
        static WStringLiteral const NetworkDefinitionParameter;
    };
}
