// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ManageOverlayNetworkResourcesRequest::ManageOverlayNetworkResourcesRequest(
    wstring const & nodeId,
    wstring const & nodeName,
    wstring const & nodeIpAddress,
    wstring const & servicePackageId,
    std::map<std::wstring, std::vector<std::wstring>> const & codePackageNetworkNames,
    ManageOverlayNetworkAction::Enum action)
    : nodeId_(nodeId),
    nodeName_(nodeName),
    nodeIpAddress_(nodeIpAddress),
    servicePackageId_(servicePackageId),
    codePackageNetworkNames_(codePackageNetworkNames),
    action_(action)
{
}

ManageOverlayNetworkResourcesRequest::ManageOverlayNetworkResourcesRequest()
    : nodeId_(),
    nodeName_(),
    nodeIpAddress_(),
    servicePackageId_(),
    codePackageNetworkNames_(),
    action_()
{
}

ManageOverlayNetworkResourcesRequest::~ManageOverlayNetworkResourcesRequest()
{
}

void ManageOverlayNetworkResourcesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    if (action_ == ManageOverlayNetworkAction::Enum::Assign)
    {
        w.Write("AssignOverlayNetworkResourcesRequest { ");
        w.Write("NodeId = {0} ", nodeId_);
        w.Write("NodeName = {0} ", nodeName_);
        w.Write("NodeIpAddress = {0} ", nodeIpAddress_);
        w.Write("ServicePackageId = {0} ", servicePackageId_);
        w.Write("CodePackageNames { ");
        for (auto iter = codePackageNetworkNames_.begin(); iter != codePackageNetworkNames_.end(); ++iter)
        {
            w.Write("CodePackageName = {0} ", iter->first);
            for (auto const & network : iter->second)
            {
                w.Write("NetworkName = {0} ", network);
            }
        }
        w.Write("}");
        w.Write("}");
    }
    else if (action_ == ManageOverlayNetworkAction::Enum::Unassign)
    {
        w.Write("UnassignOverlayNetworkResourcesRequest { ");
        w.Write("NodeId = {0} ", nodeId_);
        w.Write("ServicePackageId = {0} ", servicePackageId_);
        w.Write("CodePackageNames { ");
        for (auto iter = codePackageNetworkNames_.begin(); iter != codePackageNetworkNames_.end(); ++iter)
        {
            w.Write("CodePackageName = {0} ", iter->first);
            for (auto const & network : iter->second)
            {
                w.Write("NetworkName = {0} ", network);
            }
        }
        w.Write("}");
        w.Write("}");
    }
}
