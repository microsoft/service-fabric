// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Api;
using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("DeadlockDetector");

// Avoid building trace strings in hot paths where a GUID is provided (commits).
// Only trace infrequent replica lifecycle events (Open, ChangeRole, Close).
//
#define TRACE_LIFECYCLE( FormatString, ... ) \
    if (eventId == Guid::Empty()) \
    { \
        WriteInfo( \
            TraceComponent, \
            FormatString, \
            __VA_ARGS__); \
    } \

DeadlockDetector::DeadlockDetector(PartitionedReplicaTraceComponent const & traceComponent)
    : PartitionedReplicaTraceComponent(traceComponent)
    , stringEvents_()
    , guidEvents_()
    , eventsLock_()
{
}

void DeadlockDetector::TrackAssertEvent(
    wstring const & eventName, 
    TimeSpan const timeout) 
{
    return this->TrackAssertEvent(eventName, Guid::Empty(), timeout);
}

void DeadlockDetector::TrackAssertEvent(
    wstring const & eventName, 
    Guid const & eventId,
    TimeSpan const timeout)
{
    if (timeout <= TimeSpan::Zero)
    {
        TRACE_LIFECYCLE(
            "{0} skip tracking assert event: {1} ({2}) timeout={3}", 
            this->TraceId,
            eventName, 
            eventId,
            timeout);

        return;
    }

    auto root = this->CreateComponentRoot();
    auto timer = Timer::Create(TraceComponent, [this, root, eventName, eventId, timeout](TimerSPtr const &)
    {
        this->OnAssertEvent(eventName, eventId, timeout);
    });

    AcquireWriteLock lock(eventsLock_);

    bool inserted = (eventId == Guid::Empty())
        ? stringEvents_.insert(make_pair(eventName, timer)).second
        : guidEvents_.insert(make_pair(eventId, timer)).second;

    if (inserted)
    {
        timer->Change(timeout);
            
        TRACE_LIFECYCLE(
            "{0} Tracking assert event: {1} ({2}) timeout={3}", 
            this->TraceId,
            eventName, 
            eventId,
            timeout);
    }
    else
    {
        timer->Cancel();

        TRACE_LIFECYCLE(
            "{0} Assert event already tracked: {1} ({2})", 
            this->TraceId,
            eventName,
            eventId);
    }
}

void DeadlockDetector::UntrackAssertEvent(wstring const & eventName)
{
    return this->UntrackAssertEvent(eventName, Guid::Empty());
}

void DeadlockDetector::UntrackAssertEvent(
    wstring const & eventName, 
    Guid const & eventId)
{
    TimerSPtr timer;
    {
        AcquireWriteLock lock(eventsLock_);

        if (eventId == Guid::Empty())
        {
            auto findIt = stringEvents_.find(eventName);
            if (findIt != stringEvents_.end())
            {
                timer = findIt->second;
                stringEvents_.erase(findIt);
            }
        }
        else
        {
            auto findIt = guidEvents_.find(eventId);
            if (findIt != guidEvents_.end())
            {
                timer = findIt->second;
                guidEvents_.erase(findIt);
            }
        }
    }

    if (timer.get() != nullptr)
    {
        timer->Cancel();

        TRACE_LIFECYCLE(
            "{0} Assert event untracked: {1} ({2})", 
            this->TraceId,
            eventName,
            eventId);
    }
    else
    {
        TRACE_LIFECYCLE(
            "{0} Assert event not found: {1} ({2})", 
            this->TraceId,
            eventName,
            eventId);
    }
}

void DeadlockDetector::OnAssertEvent(wstring const & eventName, Guid const & eventId, TimeSpan const timeout)
{
    TRACE_ERROR_AND_ASSERT(
        TraceComponent,
        "{0}: asserting on expired event for self-mitigation: event={1} ({2}) timeout={3}",
        this->TraceId,
        eventName,
        eventId,
        timeout);
}
