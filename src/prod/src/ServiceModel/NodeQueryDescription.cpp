//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("NodeQueryDescription");

NodeQueryDescription::NodeQueryDescription()
    : nodeNameFilter_()
    , nodeStatusFilter_()
    , queryPagingDescription_()
{
}

Common::ErrorCode NodeQueryDescription::FromPublicApi(FABRIC_NODE_QUERY_DESCRIPTION const & nodeQueryDescription)
{
    // Node name filter
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(nodeQueryDescription.NodeNameFilter, nodeNameFilter_)

    // Continuation token
    if (nodeQueryDescription.Reserved != nullptr)
    {
        QueryPagingDescription pagingDescription;
        wstring continuationToken;

        auto ex1 = reinterpret_cast<FABRIC_NODE_QUERY_DESCRIPTION_EX1*>(nodeQueryDescription.Reserved);
        TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(ex1->ContinuationToken, continuationToken)

        if (!continuationToken.empty())
        {
            pagingDescription.ContinuationToken = move(continuationToken);
        }

        // Node status filter
        if (ex1->Reserved != NULL)
        {
            auto ex2 = reinterpret_cast<FABRIC_NODE_QUERY_DESCRIPTION_EX2*>(ex1->Reserved);
            nodeStatusFilter_ = ex2->NodeStatusFilter;

            // Max Results
            if (ex2->Reserved != NULL)
            {
                auto ex3 = reinterpret_cast<FABRIC_NODE_QUERY_DESCRIPTION_EX3*>(ex2->Reserved);

                if (ex3->MaxResults < 0)
                {
                    return ErrorCode(
                        ErrorCodeValue::InvalidArgument,
                        wformatString(GET_COMMON_RC(Invalid_Max_Results), ex3->MaxResults));
                }

                pagingDescription.MaxResults = ex3->MaxResults;
            }
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void NodeQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!nodeNameFilter_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::Node::Name,
            nodeNameFilter_);
    }

    if (nodeStatusFilter_ != (DWORD)FABRIC_QUERY_NODE_STATUS_FILTER_DEFAULT)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Node::NodeStatusFilter,
            StringUtility::ToWString<DWORD>(nodeStatusFilter_));
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}