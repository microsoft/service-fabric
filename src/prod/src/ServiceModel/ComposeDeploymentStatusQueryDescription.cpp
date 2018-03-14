// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ServiceModel;
using namespace std;
using namespace Common;

StringLiteral const TraceComponent("ComposeDeploymentStatusQueryDescription");

ComposeDeploymentStatusQueryDescription::ComposeDeploymentStatusQueryDescription()
    : deploymentNameFilter_()
    , continuationToken_()
    , maxResults_(0)
{
}

ComposeDeploymentStatusQueryDescription::ComposeDeploymentStatusQueryDescription(
    wstring && continuationToken,
    int64 maxResults)
    : deploymentNameFilter_()
    , continuationToken_(move(continuationToken))
    , maxResults_(maxResults)
{
}

ComposeDeploymentStatusQueryDescription::ComposeDeploymentStatusQueryDescription(
    wstring && deploymentNameFilter,
    wstring && continuationToken)
    : deploymentNameFilter_(move(deploymentNameFilter))
    , continuationToken_(move(continuationToken))
    , maxResults_(0)
{
}

ErrorCode ComposeDeploymentStatusQueryDescription::FromPublicApi(FABRIC_COMPOSE_DEPLOYMENT_STATUS_QUERY_DESCRIPTION const & queryDescription)
{
    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(queryDescription.DeploymentNameFilter, deploymentNameFilter_);

    TRY_PARSE_PUBLIC_STRING_ALLOW_NULL(queryDescription.ContinuationToken, continuationToken_);

    maxResults_ = queryDescription.MaxResults;

    return ErrorCodeValue::Success;
}
