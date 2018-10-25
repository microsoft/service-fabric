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
    std::wstring const & assignedIp,
    int64 timeoutTicks,
#if defined(PLATFORM_UNIX)
    ContainerPodDescription const & podDesc,
#endif
    ServicePackageResourceGovernanceDescription const & spRg)
    : servicePackageId_(servicePackageId),
    appId_(appId),
    appName_(appName),
    appfolder_(appfolder),
    nodeId_(nodeId),
    partitionId_(partitionId),
    servicePackageActivationId_(servicePackageActivationId),
    assignedIp_(assignedIp),
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
    w.Write("AppId={0}", appId_);
    w.Write("AppName={0}", appName_);
    w.Write("ServicePackageId={0}", servicePackageId_);
    w.Write("NodeId={0}", nodeId_);
    w.Write("PartitionId={0}", partitionId_);
    w.Write("ServicePackageActivationId={0}", servicePackageActivationId_);
    w.Write("AssignedIp={0}", assignedIp_);
    w.Write("AppFolder={0}", appfolder_);
    w.Write("Timeout ticks ={0}", ticks_);
    w.Write("ServicePackage RG={0}", spRg_);
#if defined(PLATFORM_UNIX)
    w.Write("ContainerPodDescription={0}", podDescription_);
#endif
    w.Write("}");
}
