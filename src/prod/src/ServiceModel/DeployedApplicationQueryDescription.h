// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class DeployedApplicationQueryDescription
    {
        DENY_COPY(DeployedApplicationQueryDescription)

    public:
        DeployedApplicationQueryDescription(); // There are places (like COM) which need the default constructor.

        DeployedApplicationQueryDescription(DeployedApplicationQueryDescription && other) = default;
        DeployedApplicationQueryDescription & operator = (DeployedApplicationQueryDescription && other) = default;

        ~DeployedApplicationQueryDescription() = default;

        __declspec(property(get = get_NodeName, put = set_NodeName)) std::wstring const & NodeName;
        std::wstring const & get_NodeName() const { return nodeName_; }
        void set_NodeName(std::wstring && nodeName) { nodeName_ = std::move(nodeName); }

        __declspec(property(get = get_ApplicationName, put = set_ApplicationName)) Common::NamingUri const & ApplicationName;
        Common::NamingUri  const & get_ApplicationName() const { return applicationName_; }
        void set_ApplicationName(Common::NamingUri  && applicationName) { applicationName_ = std::move(applicationName); }

        __declspec(property(get = get_IncludeHealthState, put = set_IncludeHealthState)) bool IncludeHealthState;
        bool get_IncludeHealthState() const { return includeHealthState_; }
        void set_IncludeHealthState(bool includeHealthState) { includeHealthState_ = includeHealthState; }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionObject;
        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        Common::ErrorCode FromPublicApi(FABRIC_PAGED_DEPLOYED_APPLICATION_QUERY_DESCRIPTION const & deployedApplicationQueryDescription);

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;
        Common::ErrorCode GetDescriptionFromQueryArgumentMap(QueryArgumentMap const & argMap);

        bool IsExclusiveFilterHelper(bool const isValid, bool & hasFilterSet);

        void MovePagingDescription(__out unique_ptr<QueryPagingDescription> & pagingDescription) { pagingDescription = std::move(queryPagingDescription_); }

        std::wstring GetApplicationNameString() const;

    private:
        std::wstring nodeName_;
        Common::NamingUri applicationName_;
        bool includeHealthState_;
        std::unique_ptr<QueryPagingDescription> queryPagingDescription_;
    };
}

