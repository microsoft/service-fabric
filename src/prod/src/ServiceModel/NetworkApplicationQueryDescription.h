// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NetworkApplicationQueryDescription
    {
        DENY_COPY(NetworkApplicationQueryDescription)

    public:
        NetworkApplicationQueryDescription();
        ~NetworkApplicationQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION const & networkApplicationQueryDescription);

        __declspec(property(get = get_NetworkName, put = set_NetworkName)) std::wstring const & NetworkName;

        std::wstring const& get_NetworkName() const { return networkName_; }
        void set_NetworkName(std::wstring && value) { networkName_ = move(value); }

        __declspec(property(get = get_ApplicationNameFilter, put = set_ApplicationNameFilter)) Common::NamingUri const & ApplicationNameFilter;

        Common::NamingUri const & get_ApplicationNameFilter() const { return applicationNameFilter_; }
        void set_ApplicationNameFilter(Common::NamingUri && value) { applicationNameFilter_ = move(value); }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        std::wstring networkName_;

        Common::NamingUri applicationNameFilter_;

        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
