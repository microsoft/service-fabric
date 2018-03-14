// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureEndpointSecurityRequest::ConfigureEndpointSecurityRequest() 
    : principalSid_(),
    port_(),
    isHttps_(),
    prefix_(),
    servicePackageId_(),
    nodeId_(),
    isSharedPort_()
{
}

ConfigureEndpointSecurityRequest::ConfigureEndpointSecurityRequest(
    wstring const & principalSid,
    UINT port,
    bool isHttps,
    wstring const & prefix,
    wstring const & servicePackageId,
    wstring const & nodeId,
    bool isSharedPort)
    : principalSid_(principalSid),
    port_(port),
    isHttps_(isHttps),
    prefix_(prefix),
    servicePackageId_(servicePackageId),
    nodeId_(nodeId),
    isSharedPort_(isSharedPort)
{
}

void ConfigureEndpointSecurityRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureEndpointSecurityRequest { ");
    w.Write("PrincipalSid = {0}", principalSid_);
    w.Write("Port = {0}", port_);
    w.Write("IsHttps = {0}", isHttps_);
    w.Write("Prefix = {0}", prefix_);
    w.Write("ServicePackageId = {0}", servicePackageId_);
    w.Write("NodeId = {0}", nodeId_);
    w.Write("IsSharedPort = {0}", isSharedPort_);
    w.Write("}");
}
