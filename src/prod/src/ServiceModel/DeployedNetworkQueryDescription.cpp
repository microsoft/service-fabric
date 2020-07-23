// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("DeployedNetworkQueryDescription");

DeployedNetworkQueryDescription::DeployedNetworkQueryDescription()
    : nodeName_()
    , queryPagingDescription_()
{
}

ErrorCode DeployedNetworkQueryDescription::FromPublicApi(FABRIC_DEPLOYED_NETWORK_QUERY_DESCRIPTION const &deployedNetworkQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING(deployedNetworkQueryDescription.NodeName, nodeName_);    
    
    if (deployedNetworkQueryDescription.PagingDescription != NULL)
    {
        QueryPagingDescription pagingDescription;
        
        auto error = pagingDescription.FromPublicApi(*(deployedNetworkQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void DeployedNetworkQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    // NodeName is a mandatory query parameter.
    argMap.Insert(
        QueryResourceProperties::Node::Name,
        nodeName_);

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}
