// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;

    internal sealed class CachePolicy
    {
        public CachePolicy(TimeSpan maxCacheDuration, int maxCacheItem, TimeSpan cacheBuildIncrement)
        {
            this.MaxCacheItemCount = maxCacheItem;
            this.MaxCacheMaintainDuration = maxCacheDuration;
            this.CacheBuildIncrement = cacheBuildIncrement;
        }

        /// <summary>
        /// Max duration for which cache needs to be maintained.
        /// </summary>
        public TimeSpan MaxCacheMaintainDuration { get; }

        /// <summary>
        /// Max number of items in the cache.
        /// </summary>
        public int MaxCacheItemCount { get; }

        /// <summary>
        /// During cache build, we build in this increment.
        /// </summary>
        public TimeSpan CacheBuildIncrement { get; }
    }
}