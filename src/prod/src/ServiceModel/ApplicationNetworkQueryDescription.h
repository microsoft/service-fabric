// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationNetworkQueryDescription
    {
        DENY_COPY(ApplicationNetworkQueryDescription)

    public:
        ApplicationNetworkQueryDescription();
        ~ApplicationNetworkQueryDescription() = default;

        Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION const & applicationNetworkQueryDescription);

        __declspec(property(get = get_ApplicationName, put = set_ApplicationName)) Common::NamingUri const & ApplicationName;

        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
        void set_ApplicationName(Common::NamingUri && value) { applicationName_ = move(value); }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;

        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

    private:
        Common::NamingUri applicationName_;

        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}
