// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
 
using namespace Common;
using namespace std;

using namespace Naming;

PsdCache::PsdCache(int cacheLimit)
    : cache_(cacheLimit)
    , servicesLsn_(0)
    , servicesLsnLock_()
{ 
}

void PsdCache::UpdateCacheEntry(StoreServicePsdCacheEntrySPtr & entry, FABRIC_SEQUENCE_NUMBER lsn)
{
    cache_.TryPutOrGet(entry);

    if (lsn > 0)
    {
        this->UpdateServicesLsn(lsn);
    }
}

void PsdCache::RemoveCacheEntry(std::wstring const & name, FABRIC_SEQUENCE_NUMBER lsn)
{
    cache_.TryRemove(name);

    if (lsn > 0)
    {
        this->UpdateServicesLsn(lsn);
    }
}

bool PsdCache::TryGetCacheEntry(std::wstring const & name, __out StoreServicePsdCacheEntrySPtr & entry)
{
    return cache_.TryGet(name, entry);
}

FABRIC_SEQUENCE_NUMBER PsdCache::GetServicesLsn()
{
    AcquireReadLock lock(servicesLsnLock_);

    return servicesLsn_;
}

void PsdCache::UpdateServicesLsn(FABRIC_SEQUENCE_NUMBER lsn)
{
    AcquireWriteLock lock(servicesLsnLock_);

    if (lsn > servicesLsn_)
    {
        servicesLsn_ = lsn;
    }
}
