// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

SetupContainerGroupRequest::SetupContainerGroupRequest()
    : servicePackageId_(),
    appId_(),
	appName_(),
    nodeId_(),
    partitionId_(),
    servicePackageActivationId_(),
    appfolder_(),
    assignedIp_(),
    networkType_(),
    openNetworkAssignedIp_(),
    overlayNetworkResources_(),
    dnsServers_(),
    ticks_(0),
#if defined(PLATFORM_UNIX)
    podDescription_(),
#endif
    spRg_()
{
}

SetupContainerGroupRequest::SetupContainerGroupRequest(
    std::wstring const & servicePackageId,
    std::wstring const & appId,
    std::wstring const & appName,
    std::wstring const & appfolder,
    std::wstring const & nodeId,
    std::wstring const & partitionId,
    std::wstring const & servicePackageActivationId,
    ServiceModel::NetworkType::Enum networkType,
    std::wstring const & openNetworkAssignedIp,
    std::map<std::wstring, std::wstring> const & overlayNetworkResources,
    std::vector<std::wstring> const & dnsServers,
    int64 timeoutTicks,
#if defined(PLATFORM_UNIX)
    ContainerPodDescription const & podDesc,
#endif
    ServicePackageResourceGovernanceDescription const & spRg)
    : servicePackageId_(servicePackageId),
    appId_(appId),
    appName_(appName),
    appfolder_(appfolder),
    assignedIp_(),
    nodeId_(nodeId),
    partitionId_(partitionId),
    servicePackageActivationId_(servicePackageActivationId),
    networkType_(networkType),
    openNetworkAssignedIp_(openNetworkAssignedIp),
    overlayNetworkResources_(overlayNetworkResources),
    dnsServers_(dnsServers),
    ticks_(timeoutTicks),
#if defined(PLATFORM_UNIX)
    podDescription_(podDesc),
#endif
    spRg_(spRg)
{
}

void SetupContainerGroupRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("SetupContainerGroupRequest { ");
    w.Write("AppId={0} ", appId_);
    w.Write("AppName={0} ", appName_);
    w.Write("ServicePackageId={0} ", servicePackageId_);
    w.Write("NodeId={0} ", nodeId_);
    w.Write("PartitionId={0} ", partitionId_);
    w.Write("ServicePackageActivationId={0} ", servicePackageActivationId_);
    w.Write("Network Type={0} ", networkType_);
    w.Write("Open Network Assigned Ip={0} ", openNetworkAssignedIp_);
    w.Write("Overlay Network Resources { ");
    for (auto const & onr : overlayNetworkResources_)
    {
        w.Write("Overlay Network Name={0} ", onr.first);
        w.Write("Overlay Network Assigned Resource={0} ", onr.second);
    }
    w.Write("} ");
    w.Write("Dns servers { ");
    for (auto const & dnsServer : dnsServers_)
    {
        w.Write("Dns server={0} ", dnsServer);
    }
    w.Write("} ");
    w.Write("AppFolder={0} ", appfolder_);
    w.Write("Timeout ticks={0} ", ticks_);
    w.Write("ServicePackage RG={0} ", spRg_);
#if defined(PLATFORM_UNIX)
    w.Write("ContainerPodDescription={0} ", podDescription_);
#endif
    w.Write("}");
}