// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

using namespace Hosting2;

ConfigureEndpointBindingAndFirewallPolicyRequest::ConfigureEndpointBindingAndFirewallPolicyRequest()
: nodeId_(),
  servicePackageId_(),
  endpointCertBindings_(),
  cleanup_(false),
  cleanupFirewallPolicy_(false),
  firewallPorts_(),
  nodeInstanceId_(0)
{
}

ConfigureEndpointBindingAndFirewallPolicyRequest::ConfigureEndpointBindingAndFirewallPolicyRequest(
    wstring const & nodeId,
    wstring const & servicePackageId,
    vector<EndpointCertificateBinding> const & certBindings,
    bool cleanup,
    bool cleanupFirewallPolicy,
    vector<LONG> const & firewallPorts,
    uint64 nodeInstanceId)
    : nodeId_(nodeId),
    servicePackageId_(servicePackageId),
    endpointCertBindings_(certBindings),
    cleanup_(cleanup),
    cleanupFirewallPolicy_(cleanupFirewallPolicy),
    firewallPorts_(firewallPorts),
    nodeInstanceId_(nodeInstanceId)
{
}

void ConfigureEndpointBindingAndFirewallPolicyRequest::WriteTo(TextWriter & w, FormatOptions const &) const
{
    w.Write("ConfigureEndpointBindingAndFirewallPolicyRequest { ");
    w.Write("NodeId = {0}", nodeId_);
    w.Write("ServicePackageId = {0}", servicePackageId_);
    w.Write("EndpointCertificateBindings {");
    for (auto iter = endpointCertBindings_.begin(); iter != endpointCertBindings_.end(); ++iter)
    {
        w.Write("EndpointCertificateBinding = {0}", *iter);
    }
    w.Write("}");
    w.Write("Cleanup = {0}", cleanup_);
    w.Write("CleanupFirewallPolicy = {0}", cleanupFirewallPolicy_);
    w.Write("FirewallPorts {");
    for (auto iter = firewallPorts_.begin(); iter != firewallPorts_.end(); ++iter)
    {
        w.Write("Port = {0}", *iter);
    }
    w.Write("}");
    w.Write("NodeInstanceId = {0}", nodeInstanceId_);
    w.Write("}");
}
