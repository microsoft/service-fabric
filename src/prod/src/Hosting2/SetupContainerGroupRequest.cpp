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
    nodeId_(),
    appfolder_(),
    assignedIp_(),
    ticks_(0),
    spRg_()
{
}

SetupContainerGroupRequest::SetupContainerGroupRequest(
    std::wstring const & servicePackageId,
    std::wstring const & appId,
    std::wstring const & appfolder,
    std::wstring const & nodeId,
    std::wstring const & assignedIp,
    int64 timeoutTicks,
    ServicePackageResourceGovernanceDescription const & spRg)
    : servicePackageId_(servicePackageId),
    appId_(appId),
    appfolder_(appfolder),
    nodeId_(nodeId),
    assignedIp_(assignedIp),
    ticks_(timeoutTicks),
    spRg_(spRg)
{
}

void SetupContainerGroupRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("SetupContainerGroupRequest { ");
    w.Write("AppId={0}", appId_);
    w.Write("ServicePackageId={0}", servicePackageId_);
    w.Write("NodeId={0}", nodeId_);
    w.Write("AssignedIp={0}", assignedIp_);
    w.Write("AppFolder={0}", appfolder_);
    w.Write("Timeout ticks ={0}", ticks_);
    w.Write("ServicePackage RG={0}", spRg_);
    w.Write("}");
}
