// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

ApplicationUnhealthyEvaluation::ApplicationUnhealthyEvaluation(
    wstring const & applicationName,
    FABRIC_HEALTH_STATE aggregatedHealthState,
    vector<HealthEvaluation> && unhealthyEvaluations)
    : ApplicationAggregatedHealthState(
        applicationName,
        aggregatedHealthState)
    , unhealthyEvaluations_(move(unhealthyEvaluations))
{
}
