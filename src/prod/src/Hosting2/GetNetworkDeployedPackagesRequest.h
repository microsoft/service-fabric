// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetNetworkDeployedPackagesRequest : public Serialization::FabricSerializable
    {
    public:
        GetNetworkDeployedPackagesRequest();
        GetNetworkDeployedPackagesRequest(
            std::vector<std::wstring> const & servicePackageIds,
            std::wstring const & codePackageName,
            std::wstring const & networkName,
            std::wstring const & nodeId,
            std::map<std::wstring, std::wstring> const & codePackageInstanceAppHostMap);

        __declspec(property(get = get_ServicePackageIds)) std::vector<std::wstring> const & ServicePackageIds;
        std::vector<std::wstring> const & get_ServicePackageIds() const { return servicePackageIds_; }

        __declspec(property(get = get_CodePackageName)) std::wstring const & CodePackageName;
        std::wstring const & get_CodePackageName() const { return codePackageName_; }

        __declspec(property(get = get_NetworkName)) std::wstring const & NetworkName;
        std::wstring const & get_NetworkName() const { return networkName_; }

        __declspec(property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }

        __declspec(property(get = get_CodePackageInstanceAppHostMap)) std::map<std::wstring, std::wstring> const & CodePackageInstanceAppHostMap;
        std::map<std::wstring, std::wstring> const & get_CodePackageInstanceAppHostMap() const { return codePackageInstanceAppHostMap_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_05(servicePackageIds_, codePackageName_, networkName_, nodeId_, codePackageInstanceAppHostMap_);

    private:
        std::vector<std::wstring> servicePackageIds_;
        std::wstring codePackageName_;
        std::wstring networkName_;
        std::wstring nodeId_;
        std::map<std::wstring, std::wstring> codePackageInstanceAppHostMap_;
    };
}
