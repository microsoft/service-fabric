// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    // Store Service PSD cache maintains some additional state
    // (highest service description LSN) in addition to the
    // actual PSD cache itself.
    //
    class PsdCache
    {
    public:
        PsdCache(int cacheLimit);

        __declspec(property(get=get_IsCacheLimitEnabled)) bool IsCacheLimitEnabled;
        bool get_IsCacheLimitEnabled() const { return cache_.IsCacheLimitEnabled; }

        __declspec(property(get=get_CacheLimit)) size_t CacheLimit;
        size_t get_CacheLimit() const { return cache_.CacheLimit; }

        __declspec(property(get=get_Size)) size_t Size;
        size_t get_Size() const { return cache_.Size; }

        void UpdateCacheEntry(StoreServicePsdCacheEntrySPtr & entry, FABRIC_SEQUENCE_NUMBER lsn = 0);
        void RemoveCacheEntry(std::wstring const & name, FABRIC_SEQUENCE_NUMBER lsn);
        bool TryGetCacheEntry(std::wstring const & name, __out StoreServicePsdCacheEntrySPtr & entry);

        FABRIC_SEQUENCE_NUMBER GetServicesLsn();
        void UpdateServicesLsn(FABRIC_SEQUENCE_NUMBER lsn);

    private:
        StoreServicePsdCache cache_;
        FABRIC_SEQUENCE_NUMBER servicesLsn_;
        Common::RwLock servicesLsnLock_;
    };
}
