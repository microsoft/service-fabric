// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceComponent("ServiceQueryDescription");

ServiceQueryDescription::ServiceQueryDescription()
    : applicationName_(NamingUri::RootNamingUri)
    , serviceNameFilter_(NamingUri::RootNamingUri)
    , serviceTypeNameFilter_()
    , queryPagingDescription_()
{
}

ServiceQueryDescription::ServiceQueryDescription(
    NamingUri && applicationName,
    NamingUri && serviceNameFilter,
    wstring && serviceTypeNameFilter,
    QueryPagingDescription && queryPagingDescription)
    : applicationName_(move(applicationName))
    , serviceNameFilter_(move(serviceNameFilter))
    , serviceTypeNameFilter_(move(serviceTypeNameFilter))
    , queryPagingDescription_(make_unique<QueryPagingDescription>(move(queryPagingDescription)))
{
}

ErrorCode ServiceQueryDescription::FromPublicApi(FABRIC_SERVICE_QUERY_DESCRIPTION const & svcQueryDesc)
{
    ErrorCode err = ErrorCodeValue::Success;
    if (svcQueryDesc.ApplicationName != nullptr)
    {
        err = NamingUri::TryParse(svcQueryDesc.ApplicationName, "ApplicationName", applicationName_);
    }

    if (!err.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ServiceQueryDescription::FromPublicApi failed to parse ApplicationName = {0} error = {1} errormessage = {2}.", svcQueryDesc.ApplicationName, err, err.Message);
        return err;
    }

    if (svcQueryDesc.ServiceNameFilter != nullptr)
    {
        err = NamingUri::TryParse(svcQueryDesc.ServiceNameFilter, "ServiceNameFilter", serviceNameFilter_);
    }

    if (!err.IsSuccess())
    {
        Trace.WriteWarning(TraceComponent, "ServiceQueryDescription::FromPublicApi failed to parse ServiceNameFilter = {0} error = {1}. errormessage = {2}", svcQueryDesc.ServiceNameFilter, err, err.Message);
        return err;
    }

    if (svcQueryDesc.Reserved != nullptr)
    {
        QueryPagingDescription pagingDescription;
        wstring continuationToken;

        auto svcQueryDescEx1 = static_cast<FABRIC_SERVICE_QUERY_DESCRIPTION_EX1 *>(svcQueryDesc.Reserved);

        TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(svcQueryDescEx1->ContinuationToken, continuationToken)

        if (!continuationToken.empty())
        {
            pagingDescription.ContinuationToken = move(continuationToken);
        }

        if (svcQueryDescEx1->Reserved != nullptr)
        {
            auto svcQueryDescEx2 = static_cast<FABRIC_SERVICE_QUERY_DESCRIPTION_EX2 *>(svcQueryDescEx1->Reserved);

            TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(svcQueryDescEx2->ServiceTypeNameFilter, serviceTypeNameFilter_)

            if (svcQueryDescEx2->Reserved != nullptr)
            {
                auto svcQueryDescEx3 = static_cast<FABRIC_SERVICE_QUERY_DESCRIPTION_EX3 *>(svcQueryDescEx2->Reserved);

                if (svcQueryDescEx3->MaxResults < 0)
                {
                    return ErrorCode(
                        ErrorCodeValue::InvalidArgument,
                        wformatString(GET_COMMON_RC(Invalid_Max_Results), svcQueryDescEx3->MaxResults));
                }

                pagingDescription.MaxResults = svcQueryDescEx3->MaxResults;
            }
        }

        queryPagingDescription_ = make_unique<QueryPagingDescription>(move(pagingDescription));
    }

    return ErrorCodeValue::Success;
}