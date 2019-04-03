// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("NetworkQueryDescription");

NetworkQueryDescription::NetworkQueryDescription()
    : networkNameFilter_()
    , networkStatusFilter_(0)
    , queryPagingDescription_()
{
}

ErrorCode NetworkQueryDescription::FromPublicApi(FABRIC_NETWORK_QUERY_DESCRIPTION const &networkQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(networkQueryDescription.NetworkNameFilter, networkNameFilter_);

    networkStatusFilter_ = networkQueryDescription.NetworkStatusFilter;
    
    if (networkQueryDescription.PagingDescription != NULL)
    {
        QueryPagingDescription pagingDescription;
        
        auto error = pagingDescription.FromPublicApi(*(networkQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void NetworkQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!networkNameFilter_.empty())
    {
        argMap.Insert(
            QueryResourceProperties::Network::NetworkName,
            networkNameFilter_);
    }

    if (networkStatusFilter_ != (DWORD)FABRIC_NETWORK_STATUS_FILTER_DEFAULT)
    {
        argMap.Insert(
            QueryResourceProperties::Network::NetworkStatusFilter,
            StringUtility::ToWString<DWORD>(networkStatusFilter_));
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}
