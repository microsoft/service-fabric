//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

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

        __declspec(property(get = get_QueryPagingDescriptionUPtr, put = set_QueryPagingDescriptionUPtr)) std::unique_ptr<QueryPagingDescription> const & QueryPagingDescriptionUPtr;
        std::unique_ptr<QueryPagingDescription> const & get_QueryPagingDescriptionUPtr() const { return queryPagingDescriptionUPtr_; }
        void set_QueryPagingDescriptionUPtr(std::unique_ptr<QueryPagingDescription> && queryPagingDescription) { queryPagingDescriptionUPtr_ = std::move(queryPagingDescription); }

        Common::ErrorCode FromPublicApi(FABRIC_NODE_QUERY_DESCRIPTION const & deployedApplicationQueryDescription);

        void GetQueryArgumentMap(__out QueryArgumentMap & argMap) const;

        void MovePagingDescription(__out unique_ptr<QueryPagingDescription> & pagingDescription) { pagingDescription = std::move(queryPagingDescriptionUPtr_); }

    private:
        std::wstring nodeNameFilter_;
        DWORD nodeStatusFilter_;
        std::unique_ptr<QueryPagingDescription> queryPagingDescriptionUPtr_;
    };
}

