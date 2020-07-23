// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;
using namespace Query;

StringLiteral const TraceComponent("ApplicationNetworkQueryDescription");

ApplicationNetworkQueryDescription::ApplicationNetworkQueryDescription()    
    : applicationName_(NamingUri::RootNamingUri)
    , queryPagingDescription_()
{
}

ErrorCode ApplicationNetworkQueryDescription::FromPublicApi(FABRIC_APPLICATION_NETWORK_QUERY_DESCRIPTION const & applicationNetworkQueryDescription)
{
    ErrorCode error = ErrorCodeValue::Success;
    if (applicationNetworkQueryDescription.ApplicationName != nullptr)
    {
        error = NamingUri::TryParse(applicationNetworkQueryDescription.ApplicationName, "ApplicationName", applicationName_);
    }

    if (!error.IsSuccess())
    {
        Trace.WriteWarning(
            TraceComponent,
            "ApplicationNetworkQueryDescription::FromPublicApi failed to parse ApplicationName = {0} error = {1}. errormessage = {2}",
            applicationNetworkQueryDescription.ApplicationName,
            error,
            error.Message);

        return error;
    }
    
    if (applicationNetworkQueryDescription.PagingDescription != nullptr)
    {
        QueryPagingDescription pagingDescription;
        
        error = pagingDescription.FromPublicApi(*(applicationNetworkQueryDescription.PagingDescription));
        if (!error.IsSuccess())
        {
            Trace.WriteInfo(TraceComponent, "PagingDescription::FromPublicApi error: {0}", error);
            return error;
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCode::Success();
}

void ApplicationNetworkQueryDescription::GetQueryArgumentMap(__out QueryArgumentMap & argMap) const
{
    if (!applicationName_.IsRootNamingUri)
    {
        argMap.Insert(
            Query::QueryResourceProperties::Application::ApplicationName,
            applicationName_.ToString());
    }

    if (queryPagingDescription_ != nullptr)
    {
        queryPagingDescription_->SetQueryArguments(argMap);
    }
}
