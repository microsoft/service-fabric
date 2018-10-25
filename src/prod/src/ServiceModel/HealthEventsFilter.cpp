// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace ServiceModel;

StringLiteral const TraceSource("HealthEventsFilter");

HealthEventsFilter::HealthEventsFilter()
    : healthStateFilter_(0)
{
}

HealthEventsFilter::HealthEventsFilter(DWORD healthStateFilter)
    : healthStateFilter_(healthStateFilter)
{
}

HealthEventsFilter::~HealthEventsFilter()
{
}

bool HealthEventsFilter::IsRespected(FABRIC_HEALTH_STATE eventState) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, eventState);
}

bool HealthEventsFilter::IsRespected(HealthEvent const & event) const
{
    return HealthStateFilterHelper::StateRespectsFilter(healthStateFilter_, event.State);
}

bool HealthStateFilterHelper::StateRespectsFilter(DWORD filter, FABRIC_HEALTH_STATE state, bool returnAllOnDefault)
{
    if (filter == FABRIC_HEALTH_STATE_FILTER_ALL)
    {
        return true;
    }
    
    if (filter == FABRIC_HEALTH_STATE_FILTER_DEFAULT)
    {
        // Default can mean either all (for health queries) or none (for health state chunk queries)
        return returnAllOnDefault;
    }

    if (filter == FABRIC_HEALTH_STATE_FILTER_NONE)
    {
        return false;
    }

    bool isRespected = false;
    if ((filter & FABRIC_HEALTH_STATE_FILTER_OK) != 0)
    {
        isRespected |= (state == FABRIC_HEALTH_STATE_OK);
    }

    if ((filter & FABRIC_HEALTH_STATE_FILTER_WARNING) != 0)
    {
        isRespected |= (state == FABRIC_HEALTH_STATE_WARNING);
    }

    if ((filter & FABRIC_HEALTH_STATE_FILTER_ERROR) != 0)
    {
        isRespected |= (state == FABRIC_HEALTH_STATE_ERROR);
    }

    return isRespected;
}

ErrorCode HealthEventsFilter::FromPublicApi(FABRIC_HEALTH_EVENTS_FILTER const & publicHealthEventsFilter)
{
    healthStateFilter_ = publicHealthEventsFilter.HealthStateFilter;

    return ErrorCode::Success();
}

wstring HealthEventsFilter::ToString() const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<HealthEventsFilter&>(*this), objectString);
    if (!error.IsSuccess())
    {
        return wstring();
    }

    return objectString;
}

ErrorCode HealthEventsFilter::FromString(
    wstring const & str, 
    __out HealthEventsFilter & eventsFilter)
{
    return JsonHelper::Deserialize(eventsFilter, str);
}
