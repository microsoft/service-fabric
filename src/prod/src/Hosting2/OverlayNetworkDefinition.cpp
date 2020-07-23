//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

namespace Hosting2
{
    StringLiteral const TraceType_OverlayNetworkDefinition("OverlayNetworkDefinition");

    Common::WStringLiteral const OverlayNetworkDefinition::NetworkNameParameter(L"Name");
    Common::WStringLiteral const OverlayNetworkDefinition::NetworkTypeParameter(L"Type");
    Common::WStringLiteral const OverlayNetworkDefinition::NetworkAdapterNameParameter(L"NetworkAdapterName");
    Common::WStringLiteral const OverlayNetworkDefinition::StartMacPoolAddressParameter(L"StartMacAddress");
    Common::WStringLiteral const OverlayNetworkDefinition::EndMacPoolAddressParameter(L"EndMacAddress");
    Common::WStringLiteral const OverlayNetworkDefinition::SubnetParameter(L"Subnet");
    Common::WStringLiteral const OverlayNetworkDefinition::GatewayAddressParameter(L"GatewayAddress");
    Common::WStringLiteral const OverlayNetworkDefinition::VxlanIdParameter(L"VxlanID");
    Common::WStringLiteral const OverlayNetworkDefinition::NodeIpAddressParameter(L"InfraIPAddress");

    OverlayNetworkDefinition::OverlayNetworkDefinition(
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
        std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved) :
        networkName_(networkName),
        networkId_(networkId),
        networkType_(networkType),
        subnet_(subnet),
        gateway_(gateway),
        mask_(mask),
        vxlanId_(vxlanId),
        startMacPoolAddress_(startMacPoolAddress),
        endMacPoolAddress_(endMacPoolAddress),
        nodeIpAddress_(nodeIpAddress),
        nodeId_(nodeId),
        nodeName_(nodeName)
    {
        PopulateNetworkResources(ipMacAddressMapToBeAdded, ipMacAddressMapToBeRemoved);
    }

    OverlayNetworkDefinition::~OverlayNetworkDefinition()
    {
        WriteInfo(TraceType_OverlayNetworkDefinition, "Destructing OverlayNetworkDefinition.");
    }

    void OverlayNetworkDefinition::PopulateNetworkResources(
        std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeAdded,
        std::map<std::wstring, std::wstring> const & ipMacAddressMapToBeRemoved)
    {
        this->ipMacAddressMapToBeAdded_.clear();
        this->ipMacAddressMapToBeRemoved_.clear();

        for (auto const & nr : ipMacAddressMapToBeAdded)
        {
            this->ipMacAddressMapToBeAdded_.insert(make_pair(nr.first, nr.second));
        }

        for (auto const & nr : ipMacAddressMapToBeRemoved)
        {
            this->ipMacAddressMapToBeRemoved_.insert(make_pair(nr.first, nr.second));
        }
    }

    void OverlayNetworkDefinition::WriteTo(TextWriter & w, FormatOptions const &) const
    {
        w.Write("OverlayNetworkDefinition { ");
        w.Write("NetworkName = {0} ", networkName_);
        w.Write("NetworkId = {0} ", networkId_);
        w.Write("NetworkAdapterName = {0} ", networkAdapterName_);
        w.Write("Subnet = {0} ", subnet_);
        w.Write("Gateway = {0} ", gateway_);
        w.Write("Mask = {0} ", mask_);
        w.Write("StartMacPoolAddress = {0} ", startMacPoolAddress_);
        w.Write("EndMacPoolAddress = {0} ", endMacPoolAddress_);
        w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
        w.Write("NodeId = {0} ", nodeId_);
        w.Write("NodeName = {0} ", nodeName_);
        w.Write("NetworkType = {0} ", networkType_);
        w.Write("VxlanId = {0} ", vxlanId_);
        w.Write("IpMacAddressMapToBeAdded { ");
        for (auto iter = ipMacAddressMapToBeAdded_.begin(); iter != ipMacAddressMapToBeAdded_.end(); ++iter)
        {
            w.Write("IpAddressToBeAdded = {0} ", iter->first);
            w.Write("MacAddressToBeAdded = {0} ", iter->second);
        }
        w.Write("}");
        w.Write("IpMacAddressMapToBeRemoved { ");
        for (auto iter = ipMacAddressMapToBeRemoved_.begin(); iter != ipMacAddressMapToBeRemoved_.end(); ++iter)
        {
            w.Write("IpAddressToBeRemoved = {0} ", iter->first);
            w.Write("MacAddressToBeRemoved = {0} ", iter->second);
        }
        w.Write("}");
        w.Write("}");
    }
}
