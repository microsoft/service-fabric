// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace ServiceModel::ModelV2;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("GatewayResourceQueryDescription");

void GatewayResourceQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!gatewayNameFilter_.empty())
    {
        argMap.Insert(
            Query::QueryResourceProperties::GatewayResource::GatewayName,
            gatewayNameFilter_);
    }

    if (queryPagingDescriptionUPtr_ != nullptr)
    {
        queryPagingDescriptionUPtr_->SetQueryArguments(argMap);
    }
}

Common::ErrorCode GatewayResourceQueryDescription::GetDescriptionFromQueryArgumentMap(
    QueryArgumentMap const & queryArgs)
{
    // Find the GatewayName argument if it exists.
    queryArgs.TryGetValue(QueryResourceProperties::GatewayResource::GatewayName, gatewayNameFilter_);

    // This checks for the validity of the PagingQueryDescription as well as retrieving the data.
    QueryPagingDescription pagingDescription;
    auto error = pagingDescription.GetFromArgumentMap(queryArgs);
    if (error.IsSuccess())
    {
        queryPagingDescriptionUPtr_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return error;
}

void GatewayResourceQueryDescription::ToPublicApi(
    Common::ScopedHeap & heap,
    FABRIC_GATEWAY_RESOURCE_QUERY_DESCRIPTION & result) const
{
    result.Name = heap.AddString(gatewayNameFilter_);
    queryPagingDescriptionUPtr_->ToPublicApi(heap, result.PagingDescription);
}