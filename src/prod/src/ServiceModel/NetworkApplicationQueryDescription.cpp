// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("NetworkApplicationQueryDescription");

NetworkApplicationQueryDescription::NetworkApplicationQueryDescription()
    : networkName_()
    , applicationNameFilter_(NamingUri::RootNamingUri)
    , queryPagingDescription_()
{
}

ErrorCode NetworkApplicationQueryDescription::FromPublicApi(FABRIC_NETWORK_APPLICATION_QUERY_DESCRIPTION const &networkApplicationQueryDescription)
{
    TRY_PARSE_PUBLIC_STRING(networkApplicationQueryDescription.NetworkName, networkName_);

    ErrorCode error = ErrorCodeValue::Success;
    if (networkApplicationQueryDescription.ApplicationNameFilter != nullptr)
    {
        error = NamingUri::TryParse(networkApplicationQueryDescription.ApplicationNameFilter, "ApplicationNameFilter", applicationNameFilter_);
    }

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(
            TraceComponent,
            "NetworkApplicationQueryDescription::FromPublicApi failed to parse ApplicationNameFilter = {0} error = {1}. errormessage = {2}",
            networkApplicationQueryDescription.ApplicationNameFilter,
            error,
            error.Message);

        return error;
    }
    
    if (networkApplicationQueryDescription.PagingDescription != NULL)
    {
        QueryPagingDescription pagingDescription;
        
        error = pagingDescription.FromPublicApi(*(networkApplicationQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void NetworkApplicationQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    // NetworkName is a mandatory query parameter.
    argMap.Insert(
       QueryResourceProperties::Network::NetworkName,
       networkName_);

    if (!applicationNameFilter_.IsRootNamingUri)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Application::ApplicationName,
            applicationNameFilter_.ToString());
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}
