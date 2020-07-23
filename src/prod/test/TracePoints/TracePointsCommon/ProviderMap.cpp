// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

ProviderMap::ProviderMap(ServerEventSource & eventSource)
    : providerEvents_(),
    rwLock_(),
    eventSource_(eventSource)
{
}

ProviderMap::~ProviderMap()
{
}

void ProviderMap::ClearEventMap(ProviderId providerId)
{
    AcquireWriteLock grab(rwLock_);
    Map::iterator it = providerEvents_.find(providerId);
    if (it != providerEvents_.end())
    {
        it->second->Clear();
    }
}

bool ProviderMap::RemoveEventMap(ProviderId providerId)
{
    AcquireWriteLock grab(rwLock_);
    bool found = false;
    Map::iterator it = providerEvents_.find(providerId);
    if (it != providerEvents_.end())
    {
        found = true;
        providerEvents_.erase(it);
    }

    return found;
}

bool ProviderMap::RemoveEventMapEntry(ProviderId providerId, EventId eventId)
{
    AcquireWriteLock grab(rwLock_);
    bool found = false;
    Map::iterator it = providerEvents_.find(providerId);
    if (it != providerEvents_.end())
    {
        found = it->second->Remove(eventId);
    }

    return found;
}

bool ProviderMap::TryGetEventMapEntry(ProviderId providerId, EventId eventId, __out TracePointData & tracePointData) const
{
    AcquireReadLock grab(rwLock_);
    bool exists = false;
    Map::const_iterator it = providerEvents_.find(providerId);
    if (it != providerEvents_.end())
    {
        exists = it->second->TryGet(eventId, tracePointData);
    }

    return exists;
}

void ProviderMap::AddEventMapEntry(ProviderId providerId, EventId eventId, __in TracePointData const& tracePointData)
{
    AcquireWriteLock grab(rwLock_);
    Map::iterator it = providerEvents_.find(providerId);
    if (it == providerEvents_.end())
    {
        providerEvents_[providerId] = UniquePtr::Create<EventMap>();
        it = providerEvents_.find(providerId);
    }

    it->second->Add(eventId, tracePointData);
}
