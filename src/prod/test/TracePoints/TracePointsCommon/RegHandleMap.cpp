// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace TracePoints;

RegHandleMap::RegHandleMap()
    : regHandles_(),
    rwLock_()
{
}

RegHandleMap::~RegHandleMap()
{
}

void RegHandleMap::Add(REGHANDLE regHandle, ProviderId providerId)
{
    AcquireWriteLock grab(rwLock_);
    regHandles_[regHandle] = providerId;
}

bool RegHandleMap::Remove(REGHANDLE regHandle)
{
    AcquireWriteLock grab(rwLock_);
    Map::iterator it = regHandles_.find(regHandle);
    if (it == regHandles_.end())
    {
        return false;
    }

    regHandles_.erase(it);
    return true;
}

ProviderId RegHandleMap::Get(REGHANDLE regHandle) const
{
    AcquireReadLock grab(rwLock_);
    Map::const_iterator it = regHandles_.find(regHandle);

    if (it == regHandles_.end())
    {
        throw Error::WindowsError(ERROR_INVALID_PARAMETER, "The specified provider was not registered.");
    }

    return it->second;
}
