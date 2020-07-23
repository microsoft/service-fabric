// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class NodeQueryDescription
    {
        DENY_COPY(NodeQueryDescription)

    public:
        NodeQueryDescription(); // There are places (like COM) which need the default constructor.

        NodeQueryDescription(NodeQueryDescription && other) = default;
        NodeQueryDescription & operator = (NodeQueryDescription && other) = default;

        ~NodeQueryDescription() = default;

        __declspec(property(get = get_NodeName, put = set_NodeName)) std::wstring const & NodeNameFilter;
        std::wstring const & get_NodeName() const { return nodeNameFilter_; }
        void set_NodeName(std::wstring && nodeName) { nodeNameFilter_ = std::move(nodeName); }
        void set_NodeName(std::wstring const & nodeName) { nodeNameFilter_ = nodeName; }

        __declspec(property(get = get_NodeStatus, put = set_NodeStatus)) DWORD const & NodeStatusFilter;
        DWORD const & get_NodeStatus() const { return nodeStatusFilter_; }
        void set_NodeStatus(DWORD & nodeStatus) { nodeStatusFilter_ = nodeStatus; }

        __declspec(property(get = get_QueryPagingDescription, put = set_QueryPagingDescription)) QueryPagingDescriptionUPtr const & PagingDescription;
        QueryPagingDescriptionUPtr const & get_QueryPagingDescription() const { return queryPagingDescription_; }
        void set_QueryPagingDescription(QueryPagingDescriptionUPtr && queryPagingDescription) { queryPagingDescription_ = std::move(queryPagingDescription); }

        Common::ErrorCode FromPublicApi(FABRIC_NODE_QUERY_DESCRIPTION const & deployedApplicationQueryDescription);

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

        void TakePagingDescription(__out QueryPagingDescriptionUPtr & pagingDescription) { pagingDescription = std::move(queryPagingDescription_); }

    private:
        std::wstring nodeNameFilter_;
        DWORD nodeStatusFilter_;
        QueryPagingDescriptionUPtr queryPagingDescription_;
    };
}

