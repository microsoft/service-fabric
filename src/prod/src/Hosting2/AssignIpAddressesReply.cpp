// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

AssignIpAddressesReply::AssignIpAddressesReply() 
: error_(),
  assignedIps_()
{
}

AssignIpAddressesReply::AssignIpAddressesReply(
    ErrorCode const & error,
    vector<wstring> const & assignedIps)
    : error_(error),
    assignedIps_(assignedIps)
{
}

void AssignIpAddressesReply::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("AssignIpAddressesReply { ");
    w.Write("Error = {0}", error_);
    w.Write("AssignedIps {");
    for (auto iter = assignedIps_.begin(); iter != assignedIps_.end(); ++iter)
    {
        w.Write("AssignedIp = {0}", *iter);
    }
    w.Write("}");
}
