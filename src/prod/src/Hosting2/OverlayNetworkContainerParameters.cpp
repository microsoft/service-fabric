// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

namespace Hosting2
{
    StringLiteral const TraceType_OverlayNetworkContainerParameters("OverlayNetworkContainerParameters");

    Common::WStringLiteral const OverlayNetworkContainerParameters::ContainerIdParameter(L"ContainerId");
    Common::WStringLiteral const OverlayNetworkContainerParameters::NetworkNameParameter(L"NetworkName");
    Common::WStringLiteral const OverlayNetworkContainerParameters::MacAddressParameter(L"MacAddress");
    Common::WStringLiteral const OverlayNetworkContainerParameters::IPV4AddressParameter(L"IPAddress");
    Common::WStringLiteral const OverlayNetworkContainerParameters::GatewayAddressParameter(L"GatewayAddress");
    Common::WStringLiteral const OverlayNetworkContainerParameters::DNSServerListParameter(L"DNSServerList");
    Common::WStringLiteral const OverlayNetworkContainerParameters::NodeIpAddressParameter(L"InfraIPAddress");
    Common::WStringLiteral const OverlayNetworkContainerParameters::OutboundNATParameter(L"OutBoundNAT");
    Common::WStringLiteral const OverlayNetworkContainerParameters::NetworkDefinitionParameter(L"NetworkDefinition");

    OverlayNetworkContainerParameters::OverlayNetworkContainerParameters(
        OverlayNetworkDefinitionSPtr networkDefinition,
        std::wstring ipAddress,
        std::wstring macAddress,
        std::wstring containerId,
        std::wstring dnsServerList) :
        networkDefinition_(networkDefinition),
        networkName_(networkDefinition_->NetworkName),
        networkId_(networkDefinition_->NetworkId),
        nodeIpAddress_(networkDefinition_->NodeIpAddress),
        nodeId_(networkDefinition_->NodeId),
        nodeName_(networkDefinition_->NodeName),
        subnet_(networkDefinition_->Subnet),
        gateway_(networkDefinition_->Gateway),
        ipAddress_(ipAddress),
        macAddress_(macAddress),
        containerId_(containerId),
        dnsServerList_(dnsServerList),
        outboundNAT_(true)
    {}

    OverlayNetworkContainerParameters::OverlayNetworkContainerParameters(
        std::wstring containerId,
        std::wstring networkName,
        bool outboundNAT) :
        containerId_(containerId),
        networkName_(networkName),
        outboundNAT_(outboundNAT)
    {}

    OverlayNetworkContainerParameters::~OverlayNetworkContainerParameters() 
    {
        WriteInfo(TraceType_OverlayNetworkContainerParameters, "Destructing OverlayNetworkContainerParameters.");
    }

    void OverlayNetworkContainerParameters::WriteTo(TextWriter & w, FormatOptions const &) const
    {
        w.Write("OverlayNetworkContainerParameters { ");
        w.Write("NetworkDefinition = {0} ", networkDefinition_);
        w.Write("ContainerId = {0} ", containerId_);
        w.Write("NetworkName = {0} ", networkName_);
        w.Write("NetworkId = {0} ", networkId_);
        w.Write("IpAddress = {0} ", ipAddress_);
        w.Write("MacAddress = {0} ", macAddress_);
        w.Write("Subnet = {0} ", subnet_);
        w.Write("Gateway = {0} ", gateway_);
        w.Write("DnsServers = {0} ", dnsServerList_);
        w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
        w.Write("NodeId = {0} ", nodeId_);
        w.Write("NodeName = {0} ", nodeName_);
        w.Write("OutboundNAT = {0} ", outboundNAT_);
        w.Write("}");
    }
}
