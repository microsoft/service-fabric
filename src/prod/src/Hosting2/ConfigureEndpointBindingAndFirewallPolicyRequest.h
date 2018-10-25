// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureEndpointBindingAndFirewallPolicyRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureEndpointBindingAndFirewallPolicyRequest();
        ConfigureEndpointBindingAndFirewallPolicyRequest(
            std::wstring const & nodeId,
            std::wstring const & servicePackageId,
            std::vector<EndpointCertificateBinding> const & endpointCertBindings,
            bool cleanup,
            bool cleanupFirewallPolicy,
            std::vector<LONG> const & firewallPorts,
            uint64 nodeInstanceId);

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get=get_EndpointCertificateBindings)) std::vector<EndpointCertificateBinding> const & EndpointCertificateBindings;
        std::vector<EndpointCertificateBinding> const & get_EndpointCertificateBindings() const { return endpointCertBindings_; }

        __declspec(property(get = get_Cleanup)) bool Cleanup;
        bool get_Cleanup() const { return cleanup_; }

        __declspec(property(get = get_CleanupFirewallPolicy)) bool CleanupFirewallPolicy;
        bool get_CleanupFirewallPolicy() const { return cleanupFirewallPolicy_; }

        __declspec(property(get = get_FirewallPorts)) std::vector<LONG> FirewallPorts;
        std::vector<LONG> get_FirewallPorts() const { return firewallPorts_; }

        __declspec(property(get = get_NodeInstanceId)) uint64 NodeInstanceId;
        uint64 get_NodeInstanceId() const { return nodeInstanceId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(nodeId_, servicePackageId_, endpointCertBindings_, cleanup_, cleanupFirewallPolicy_, firewallPorts_, nodeInstanceId_);

    private:
        std::wstring nodeId_;
        std::wstring servicePackageId_;
        std::vector<EndpointCertificateBinding> endpointCertBindings_;
        bool cleanup_;
        bool cleanupFirewallPolicy_;
        std::vector<LONG> firewallPorts_;
        uint64 nodeInstanceId_;
    };
}
