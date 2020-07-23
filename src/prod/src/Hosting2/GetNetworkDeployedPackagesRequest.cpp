// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;
using namespace Hosting2;

GetNetworkDeployedPackagesRequest::GetNetworkDeployedPackagesRequest()
: servicePackageIds_(),
  codePackageName_(),
  networkName_(),
  nodeId_(),
  codePackageInstanceAppHostMap_()
{
}

GetNetworkDeployedPackagesRequest::GetNetworkDeployedPackagesRequest(
    vector<wstring> const & servicePackageIds,
    wstring const & codepackageName,
    wstring const & networkName,
    wstring const & nodeId,
    map<wstring, wstring> const & codePackageInstanceAppHostMap)
    : servicePackageIds_(servicePackageIds),
    codePackageName_(codepackageName),
    networkName_(networkName),
    nodeId_(nodeId),
    codePackageInstanceAppHostMap_(codePackageInstanceAppHostMap)
{
}

void GetNetworkDeployedPackagesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("GetNetworkDeployedPackagesRequest { ");
    w.Write("ServicePackageIds { ");
    for (auto const & servicePackageId : servicePackageIds_)
    {
        w.Write("ServicePackageId = {0} ", servicePackageId);
    }
    w.Write("} ");
    w.Write("CodePackageName = {0} ", codePackageName_);
    w.Write("NetworkName = {0} ", networkName_);
    w.Write("NodeId = {0} ", nodeId_);
    w.Write("CodePackageInstanceAppHostMap { ");
    for (auto const & codePackageInstanceAppHost : codePackageInstanceAppHostMap_)
    {
        w.Write("CodePackageInstanceIdentifier = {0} ", codePackageInstanceAppHost.first);
        w.Write("ApplicationHostId = {0} ", codePackageInstanceAppHost.second);
    }
    w.Write("}");
}
