//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceContext("SingleInstanceDeploymentQueryDescription");

SingleInstanceDeploymentQueryDescription::SingleInstanceDeploymentQueryDescription()
    : deploymentNameFilter_()
    , deploymentTypeFilter_(FABRIC_SINGLE_INSTANCE_DEPLOYMENT_TYPE_FILTER_DEFAULT)
    , continuationToken_()
    , maxResults_(0)
{
}

SingleInstanceDeploymentQueryDescription::SingleInstanceDeploymentQueryDescription(
    wstring const & deploymentNameFilter)
    : deploymentNameFilter_(deploymentNameFilter)
    , deploymentTypeFilter_(FABRIC_SINGLE_INSTANCE_DEPLOYMENT_TYPE_FILTER_DEFAULT)
    , continuationToken_()
    , maxResults_(0)
{
}

SingleInstanceDeploymentQueryDescription::SingleInstanceDeploymentQueryDescription(
    DWORD const deploymentTypeFilter)
    : deploymentNameFilter_()
    , deploymentTypeFilter_(deploymentTypeFilter)
    , continuationToken_()
    , maxResults_(0)
{
}

SingleInstanceDeploymentQueryDescription::SingleInstanceDeploymentQueryDescription(
    DWORD const deploymentTypeFilter,
    wstring && continuationToken,
    int64 maxResults)
    : deploymentNameFilter_()
    , deploymentTypeFilter_(deploymentTypeFilter)
    , continuationToken_(move(continuationToken))
    , maxResults_(maxResults)
{
}
