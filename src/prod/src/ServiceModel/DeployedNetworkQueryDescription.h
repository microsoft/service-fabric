// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedNetworkQueryDescription
    {
        DENY_COPY(DeployedNetworkQueryDescription)

    public:
        DeployedNetworkQueryDescription();
        ~DeployedNetworkQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION const & deployedNetworkQueryDescription);

        __declspec(property(get = get_NodeName, put = set_NodeName)) std::wstring const &NodeName;

        std::wstring const& get_NodeName() const { return nodeName_; }
        void set_NodeName(std::wstring && value) { nodeName_ = move(value); }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        std::wstring nodeName_;        

        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
