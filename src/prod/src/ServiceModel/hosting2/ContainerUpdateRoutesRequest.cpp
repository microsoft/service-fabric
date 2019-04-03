// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ContainerUpdateRoutesRequest::ContainerUpdateRoutesRequest()
    : containerId_(),
    containerName_(),
    applicationId_(),
    applicationName_(),
    networkType_(),
    gatewayIpAddresses_(),
    autoRemove_(),
    isContainerRoot_(),
    cgroupName_(),
    timeoutTicks_()
{
}

ContainerUpdateRoutesRequest::ContainerUpdateRoutesRequest(
    std::wstring const & containerId,
    std::wstring const & containerName,
    std::wstring const & applicationId,
    std::wstring const & applicationName,
    ServiceModel::NetworkType::Enum networkType,
    std::vector<std::wstring> const & gatewayIpAddresses,
    bool autoRemove,
    bool isContainerRoot,
    std::wstring const & cgroupName,
    int64 timeoutTicks)
    : containerId_(containerId), 
    containerName_(containerName),
    applicationId_(applicationId),
    applicationName_(applicationName),
    networkType_(networkType),
    gatewayIpAddresses_(gatewayIpAddresses),
    autoRemove_(autoRemove),
    isContainerRoot_(isContainerRoot),
    cgroupName_(cgroupName),
    timeoutTicks_(timeoutTicks)
{
}

void ContainerUpdateRoutesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("Container Description { ");

    w.Write("Container Id = {0}", containerId_);
    w.Write("Container Name = {0}", containerName_);
    w.Write("Application Id = {0}", applicationId_);
    w.Write("Application Name = {0}", applicationName_);
    w.Write("Network Type = {0}", networkType_);

    for (auto const & gatewayIpAddres : gatewayIpAddresses_)
    {
        w.Write("Gateway ip address = {0}", gatewayIpAddres);
    }

    w.Write("AutoRemove = {0}", autoRemove_);
    w.Write("IsContainerRoot = {0}", isContainerRoot_);
    w.Write("Group Container Name = {0}", cgroupName_);
    w.Write("TimeoutTicks = {0}", timeoutTicks_);

    w.Write("}");
}
