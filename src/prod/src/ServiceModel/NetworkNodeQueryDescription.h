// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkNodeQueryDescription
    {
        DENY_COPY(NetworkNodeQueryDescription)

    public:
        NetworkNodeQueryDescription();
        ~NetworkNodeQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_NETWORK_NODE_QUERY_DESCRIPTION const & networkNodeQueryDescription);

        __declspec(property(get = get_NetworkName, put = set_NetworkName)) std::wstring const & NetworkName;

        std::wstring const& get_NetworkName() const { return networkName_; }
        void set_NetworkName(std::wstring && value) { networkName_ = move(value); }

        __declspec(property(get = get_NodeNameFilter, put = set_NodeNameFilter)) std::wstring const & NodeNameFilter;

        std::wstring const & get_NodeNameFilter() const { return nodeNameFilter_; }
        void set_NodeNameFilter(std::wstring && value) { nodeNameFilter_ = move(value); }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        std::wstring networkName_;
        std::wstring nodeNameFilter_;
        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
