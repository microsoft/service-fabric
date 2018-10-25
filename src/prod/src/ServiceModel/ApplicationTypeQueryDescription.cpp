// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;

StringLiteral const TraceComponent("ApplicationTypeQueryDescription");

ApplicationTypeQueryDescription::ApplicationTypeQueryDescription()
    : applicationTypeNameFilter_()
    , applicationTypeVersionFilter_()
    , applicationTypeDefinitionKindFilter_(0)
    , excludeApplicationParameters_(false)
    , maxResults_(0)
    , continuationToken_()
{
}

ApplicationTypeQueryDescription::~ApplicationTypeQueryDescription()
{
}

// Convert the passed in value to this object
ErrorCode ApplicationTypeQueryDescription::FromPublicApi(PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION const & pagedAppTypeQueryDesc)
{
    excludeApplicationParameters_ = pagedAppTypeQueryDesc.ExcludeApplicationParameters ? true : false;
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(pagedAppTypeQueryDesc.ContinuationToken, continuationToken_);
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(pagedAppTypeQueryDesc.ApplicationTypeNameFilter, applicationTypeNameFilter_);
    maxResults_ = pagedAppTypeQueryDesc.MaxResults;

    if (pagedAppTypeQueryDesc.Reserved != nullptr)
    {
        auto ex1 = reinterpret_cast<PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION_EX1 *>(pagedAppTypeQueryDesc.Reserved);

        TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(ex1->ApplicationTypeVersionFilter, applicationTypeVersionFilter_);

        // If you have app type version without a name, throw an error
        if (!applicationTypeVersionFilter_.empty() && applicationTypeNameFilter_.empty())
        {
            return ErrorCode(ErrorCodeValue::InvalidArgument, GET_CM_RC(Version_Given_Without_Type));
        }

        if (ex1->Reserved != nullptr)
        {
            auto ex2 = reinterpret_cast<PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION_EX2 *>(ex1->Reserved);

            applicationTypeDefinitionKindFilter_ = ex2->ApplicationTypeDefinitionKindFilter;
        }
    }

    return ErrorCodeValue::Success;
}
