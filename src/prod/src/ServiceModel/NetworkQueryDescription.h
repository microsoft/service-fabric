// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkQueryDescription
    {
        DENY_COPY(NetworkQueryDescription)

    public:
        NetworkQueryDescription();
        ~NetworkQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_NETWORK_QUERY_DESCRIPTION const & networkQueryDescription);

        __declspec(property(get = get_NetworkNameFilter, put = set_NetworkNameFilter)) std::wstring const &NetworkNameFilter;

        std::wstring const& get_NetworkNameFilter() const { return networkNameFilter_; }
        void set_NetworkNameFilter(std::wstring && value) { networkNameFilter_ = move(value); }

        __declspec(property(get = get_NetworkStatusFilter, put = set_NetworkStatusFilter)) DWORD const &NetworkStatusFilter;

        DWORD const& get_NetworkStatusFilter() const { return networkStatusFilter_; }
        void set_NetworkStatusFilter(DWORD const value) { networkStatusFilter_ = value; }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        std::wstring networkNameFilter_;
        DWORD networkStatusFilter_;
        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
