// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("ApplicationHealthStatesFilter");

ApplicationHealthStatesFilter::ApplicationHealthStatesFilter()
    : healthStateFilter_(0)
{
}

ApplicationHealthStatesFilter::ApplicationHealthStatesFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

ApplicationHealthStatesFilter::~ApplicationHealthStatesFilter()
{
}

bool ApplicationHealthStatesFilter::IsRespected(FABRIC_HEALTH_STATE healthState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, healthState);
}

ErrorCode ApplicationHealthStatesFilter::FromPublicApi(FABRIC_APPLICATION_HEALTH_STATES_FILTER const & publicApplicationHealthStatesFilter)
{
    healthStateFilter_ = publicApplicationHealthStatesFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring ApplicationHealthStatesFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<ApplicationHealthStatesFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode ApplicationHealthStatesFilter::FromString(
    wstring const & str,
    __out ApplicationHealthStatesFilter & filter)
{
    return JsonHelper::Deserialize(filter, str);
}
