// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetNetworkDeployedPackagesReply : public Serialization::FabricSerializable
    {
    public:
        GetNetworkDeployedPackagesReply();
        GetNetworkDeployedPackagesReply(
            std::map<std::wstring, std::vector<std::wstring>> const & networkDeployedPackages,
            std::map<std::wstring, std::wstring> const & codePackageInstanceIdentifierContainerInfoMap);

        __declspec(property(get=get_NetworkDeployedPackages)) std::map<std::wstring, std::vector<std::wstring>> const & NetworkDeployedPackages;
        std::map<std::wstring, std::vector<std::wstring>> const & get_NetworkDeployedPackages() const { return networkDeployedPackages_; }

        __declspec(property(get = get_CodePackageInstanceIdentifierContainerInfoMap)) std::map<std::wstring, std::wstring> const & CodePackageInstanceIdentifierContainerInfoMap;
        std::map<std::wstring, std::wstring> const & get_CodePackageInstanceIdentifierContainerInfoMap() const { return codePackageInstanceIdentifierContainerInfoMap_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(networkDeployedPackages_, codePackageInstanceIdentifierContainerInfoMap_);

    private:
        std::map<std::wstring, std::vector<std::wstring>> networkDeployedPackages_;
        std::map<std::wstring, std::wstring> codePackageInstanceIdentifierContainerInfoMap_;
    };
}