// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("NetworkNodeQueryDescription");

NetworkNodeQueryDescription::NetworkNodeQueryDescription()
    : networkName_()
    , nodeNameFilter_()
    , queryPagingDescription_()
{
}

ErrorCode NetworkNodeQueryDescription::FromPublicApi(FABRIC_NETWORK_NODE_QUERY_DESCRIPTION const &networkNodeQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING(networkNodeQueryDescription.NetworkName, networkName_);

    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(networkNodeQueryDescription.NodeNameFilter, nodeNameFilter_);
    
    if (networkNodeQueryDescription.PagingDescription != nullptr)
    {
        QueryPagingDescription pagingDescription;
        
        ErrorCode error = pagingDescription.FromPublicApi(*(networkNodeQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void NetworkNodeQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    // NetworkName is a mandatory query parameter.
    argMap.Insert(
       QueryResourceProperties::Network::NetworkName,
       networkName_);

    if (!nodeNameFilter_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::Node::Name,
            nodeNameFilter_);
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}
