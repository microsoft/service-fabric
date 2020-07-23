// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

ManageOverlayNetworkResourcesReply::ManageOverlayNetworkResourcesReply() :
    action_(),
    error_(),
    assignedNetworkResources_()
{
}

ManageOverlayNetworkResourcesReply::ManageOverlayNetworkResourcesReply(
    ManageOverlayNetworkAction::Enum action,
    Common::ErrorCode const & error,
    std::map<wstring, std::map<std::wstring, std::wstring>> const & assignedNetworkResources)
    : action_(action),
    error_(error),
    assignedNetworkResources_()
{
    for (auto const & cp : assignedNetworkResources)
    {
        for (auto const & nwk : cp.second)
        {
            auto cpNetwork = wformatString("{0},{1}", cp.first, nwk.first);
            assignedNetworkResources_.insert(make_pair(cpNetwork, nwk.second));
        }
    }
}

ManageOverlayNetworkResourcesReply::~ManageOverlayNetworkResourcesReply()
{
}

void ManageOverlayNetworkResourcesReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    if (action_ == ManageOverlayNetworkAction::Enum::Assign)
    {
        w.Write("AssignOverlayNetworkResourcesReply { ");
        w.Write("Error = {0} ", error_);
        w.Write("AssignedOverlayNetworkResources { ");
        for (auto iter = assignedNetworkResources_.begin(); iter != assignedNetworkResources_.end(); ++iter)
        {
            wstring codePackageName;
            wstring networkName;
            StringUtility::SplitOnce(iter->first, codePackageName, networkName, L",");
            w.Write("CodePackage = {0} ", codePackageName);
            w.Write("NetworkName = {0} ", networkName);
            w.Write("AssignedOverlayNetworkResource = {0} ", iter->second);
        }
        w.Write("}");
        w.Write("}");
    }
    else if (action_ == ManageOverlayNetworkAction::Enum::Unassign)
    {
        w.Write("UnassignOverlayNetworkResourcesReply { ");
        w.Write("Error = {0} ", error_);
        w.Write("UnassignOverlayNetworkResources { ");
        for (auto iter = assignedNetworkResources_.begin(); iter != assignedNetworkResources_.end(); ++iter)
        {
            wstring codePackageName;
            wstring networkName;
            StringUtility::SplitOnce(iter->first, codePackageName, networkName, L",");
            w.Write("CodePackage = {0} ", codePackageName);
            w.Write("NetworkName = {0} ", networkName);
        }
        w.Write("}");
        w.Write("}");
    }
}
