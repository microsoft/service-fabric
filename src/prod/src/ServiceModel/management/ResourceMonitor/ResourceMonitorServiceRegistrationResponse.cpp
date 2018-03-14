// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include "ResourceMonitorServiceRegistrationResponse.h"

using namespace std;
using namespace Common;
using namespace Management::ResourceMonitor;
using namespace ServiceModel;


ResourceMonitorServiceRegistrationResponse::ResourceMonitorServiceRegistrationResponse(std::wstring const & fabricHostAddress)
    : fabricHostAddress_(fabricHostAddress)
{
}

void ResourceMonitorServiceRegistrationResponse::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ResourceMonitorServiceRegistrationResponse { ");
    w.Write("FabricHostAddress = {0}", fabricHostAddress_);
    w.Write("}");
}
