// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedNetworkCodePackageQueryDescription
    {
        DENY_COPY(DeployedNetworkCodePackageQueryDescription)

    public:
        DeployedNetworkCodePackageQueryDescription();
        ~DeployedNetworkCodePackageQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_DESCRIPTION const & deployedNetworkCodePackageQueryDescription);

        __declspec(property(get = get_NodeName, put = set_NodeName)) std::wstring const &NodeName;

        std::wstring const& get_NodeName() const { return nodeName_; }
        void set_NodeName(std::wstring && value) { nodeName_ = move(value); }

        __declspec(property(get = get_NetworkName, put = set_NetworkName)) std::wstring const & NetworkName;

        std::wstring const& get_NetworkName() const { return networkName_; }
        void set_NetworkName(std::wstring && value) { networkName_ = move(value); }

        __declspec(property(get = get_ApplicationNameFilter, put = set_ApplicationNameFilter)) Common::NamingUri const & ApplicationNameFilter;

        Common::NamingUri const & get_ApplicationNameFilter() const { return applicationNameFilter_; }
        void set_ApplicationNameFilter(Common::NamingUri && value) { applicationNameFilter_ = move(value); }

        __declspec(property(get = get_ServiceManifestNameFilter, put = set_ServiceManifestNameFilter)) std::wstring const & ServiceManifestNameFilter;

        std::wstring const& get_ServiceManifestNameFilter() const { return serviceManifestNameFilter_; }
        void set_ServiceManifestNameFilter(std::wstring && value) { serviceManifestNameFilter_ = move(value); }

        __declspec(property(get = get_CodePackageNameFilter, put = set_CodePackageNameFilter)) std::wstring const & CodePackageNameFilter;

        std::wstring const& get_CodePackageNameFilter() const { return codePackageNameFilter_; }
        void set_CodePackageNameFilter(std::wstring && value) { codePackageNameFilter_ = move(value); }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        std::wstring nodeName_;
        std::wstring networkName_;
        Common::NamingUri applicationNameFilter_;
        std::wstring serviceManifestNameFilter_;
        std::wstring codePackageNameFilter_;
        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
