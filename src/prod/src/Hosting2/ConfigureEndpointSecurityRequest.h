// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureEndpointSecurityRequest : public Serialization::FabricSerializable
    {
    public:
        ConfigureEndpointSecurityRequest();
        ConfigureEndpointSecurityRequest(
            std::wstring const & principalSid,
            UINT port,
            bool isHttps,
            std::wstring const & additionalPrefix,
            std::wstring const & servicePackageId,
            std::wstring const & nodeId,
            bool isSharedPort);

        __declspec(property(get=get_PrincipalSid)) std::wstring const & PrincipalSid;
        std::wstring const & get_PrincipalSid() const { return principalSid_; }

        __declspec(property(get=get_Port)) UINT Port;
        UINT get_Port() const { return port_; }

        __declspec(property(get=get_IsHttps)) bool IsHttps;
        bool get_IsHttps() const { return isHttps_; }

        __declspec(property(get = get_IsSharedPort)) bool IsSharedPort;
        bool get_IsSharedPort() const { return isSharedPort_; }


        __declspec(property(get = get_AdditionalPrefix)) std::wstring const & Prefix;
        std::wstring const & get_AdditionalPrefix() const { return prefix_; }

        __declspec(property(get = get_ServicePackageId)) std::wstring const & ServicePackageId;
        std::wstring const & get_ServicePackageId() const { return servicePackageId_; }

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_07(principalSid_, port_, isHttps_, prefix_, servicePackageId_, nodeId_, isSharedPort_);

    private:
        std::wstring principalSid_;
        UINT port_;
        bool isHttps_;
        std::wstring prefix_;
        std::wstring servicePackageId_;
        std::wstring nodeId_;
        bool isSharedPort_;
    };
}
