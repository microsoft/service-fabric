// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GetNetworkDeployedPackagesReply::GetNetworkDeployedPackagesReply()
: networkDeployedPackages_(),
codePackageInstanceIdentifierContainerInfoMap_()
{
}

GetNetworkDeployedPackagesReply::GetNetworkDeployedPackagesReply(
    std::map<std::wstring, std::vector<std::wstring>> const & networkDeployedPackages,
    std::map<std::wstring, std::wstring> const & codePackageInstanceIdentifierContainerInfoMap)
    : networkDeployedPackages_(networkDeployedPackages), 
    codePackageInstanceIdentifierContainerInfoMap_(codePackageInstanceIdentifierContainerInfoMap)
{
}

void GetNetworkDeployedPackagesReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetNetworkDeployedPackagesReply { ");

    for (auto const & network : networkDeployedPackages_)
    {
        wstring networkName;
        wstring servicePackageId;
        StringUtility::SplitOnce(network.first, networkName, servicePackageId, L",");
        w.Write("NetworkName = {0} ", networkName);
        w.Write("ServicePackageId = {0} ", servicePackageId);
        w.Write("CodePackageNames { ");
        for (auto const & cp : network.second)
        {
            w.Write("CodePackageName = {0} ", cp);
        }
        w.Write("} ");
    }

    for (auto const & cp : codePackageInstanceIdentifierContainerInfoMap_)
    {
        wstring codePackageInstanceIdentifier;
        wstring containerId;
        StringUtility::SplitOnce(cp.first, codePackageInstanceIdentifier, containerId, L",");
        w.Write("CodePackageInstanceIdentifier = {0} ", codePackageInstanceIdentifier);
        w.Write("ContainerId = {0} ", containerId);
        w.Write("ContainerAddress = {0} ", cp.second);
    }

    w.Write("}");
}
