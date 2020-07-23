// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class GatewayResourceQueryDescription
        {
            DENY_COPY(GatewayResourceQueryDescription)

        public:
            GatewayResourceQueryDescription() = default;

            void GetQueryArgumentMap(__out ServiceModel::QueryArgumentMap & argMap) const;
            Common::ErrorCode GetDescriptionFromQueryArgumentMap(ServiceModel::QueryArgumentMap const & queryArgs);

            __declspec(property(get = get_GatewayNameFilter, put = set_GatewayNameFilter)) std::wstring const & GatewayNameFilter;
            __declspec(property(get = get_QueryPagingDescriptionUPtr, put = set_QueryPagingDescriptionUPtr)) std::unique_ptr<ServiceModel::QueryPagingDescription> const & QueryPagingDescriptionUPtr;

            // Get
            std::wstring const & get_GatewayNameFilter() const { return gatewayNameFilter_; }
            std::unique_ptr<ServiceModel::QueryPagingDescription> const & get_QueryPagingDescriptionUPtr() const { return queryPagingDescriptionUPtr_; }

            // Set
            void set_GatewayNameFilter(std::wstring && value) { gatewayNameFilter_ = move(value); }
            void set_QueryPagingDescriptionUPtr(std::unique_ptr<ServiceModel::QueryPagingDescription> && queryPagingDescription) { queryPagingDescriptionUPtr_ = std::move(queryPagingDescription); }

            void ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_GATEWAY_RESOURCE_QUERY_DESCRIPTION &) const;

        private:
            std::wstring gatewayNameFilter_;
            std::unique_ptr<ServiceModel::QueryPagingDescription> queryPagingDescriptionUPtr_;
        };
    }
}
