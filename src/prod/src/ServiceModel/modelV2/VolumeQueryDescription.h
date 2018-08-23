// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class VolumeQueryDescription
        {
            DENY_COPY(VolumeQueryDescription)

        public:
            VolumeQueryDescription() = default;

            void GetQueryArgumentMap(__out ServiceModel::QueryArgumentMap & argMap) const;
            Common::ErrorCode GetDescriptionFromQueryArgumentMap(ServiceModel::QueryArgumentMap const & queryArgs);

            __declspec(property(get = get_VolumeNameFilter, put = set_VolumeNameFilter)) std::wstring const & VolumeNameFilter;
            __declspec(property(get = get_QueryPagingDescriptionUPtr, put = set_QueryPagingDescriptionUPtr)) std::unique_ptr<ServiceModel::QueryPagingDescription> const & QueryPagingDescriptionUPtr;

            // Get
            std::wstring const & get_VolumeNameFilter() const { return volumeNameFilter_; }
            std::unique_ptr<ServiceModel::QueryPagingDescription> const & get_QueryPagingDescriptionUPtr() const { return queryPagingDescriptionUPtr_; }

            // Set
            void set_VolumeNameFilter(std::wstring && value) { volumeNameFilter_ = move(value); }
            void set_QueryPagingDescriptionUPtr(std::unique_ptr<ServiceModel::QueryPagingDescription> && queryPagingDescription) { queryPagingDescriptionUPtr_ = std::move(queryPagingDescription); }

        private:
            std::wstring volumeNameFilter_;
            std::unique_ptr<ServiceModel::QueryPagingDescription> queryPagingDescriptionUPtr_;
        };
    }
}
