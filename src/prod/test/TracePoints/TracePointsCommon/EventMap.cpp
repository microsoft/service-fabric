// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace TracePoints;

EventMap::EventMap()
    : eventFunctions_()
{
}

EventMap::~EventMap()
{
}

void EventMap::Clear()
{
    eventFunctions_.clear();
}

void EventMap::Add(EventId eventId, __in TracePointData const& tracePointData)
{
    eventFunctions_[eventId] = tracePointData;
}

bool EventMap::Remove(EventId eventId)
{
    bool found = false;
    Map::iterator it = eventFunctions_.find(eventId);
    if (it != eventFunctions_.end())
    {
        eventFunctions_.erase(it);
        found = true;
    }

   return found;
}

bool EventMap::TryGet(EventId eventId, TracePointData & tracePointData) const
{
    bool found = false;
    Map::const_iterator it = eventFunctions_.find(eventId);
    if (it != eventFunctions_.end())
    {
        tracePointData = (it->second);  
        found = true;
    }

    return found;
}
