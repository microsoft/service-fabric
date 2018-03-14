// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

AssignIpAddressesRequest::AssignIpAddressesRequest() 
: nodeId_(),
  servicePackageId_(),
  codePackageNames_(),
  cleanup_(false)
{
}

AssignIpAddressesRequest::AssignIpAddressesRequest(
    wstring const & nodeId,
    wstring const & servicePackageId,
    vector<wstring> const & codepackageNames,
    bool cleanup)
    : nodeId_(nodeId),
    servicePackageId_(servicePackageId),
    codePackageNames_(codepackageNames),
    cleanup_(cleanup)
{
}

void AssignIpAddressesRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AssignIpAddressesRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("ServicePackageId = {0}", servicePackageId_);
    w.Write("CodePackageNames {");
    for (auto iter = codePackageNames_.begin(); iter != codePackageNames_.end(); ++iter)
    {
        w.Write("CodePackageName = {0}", *iter);
    }
    w.Write("}");
    w.Write("Cleanup = {0}", cleanup_);
    w.Write("}");
}
