// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// Settings that configure the store
    /// </summary>
    public class StoreSettings
    {
        /// <summary>
        /// State providers cache read items in memory. Once a threshold of cached items is reached, items can be swept
        /// from the cache via an LRU algorithm. 0 (the default) tells Service Fabric to automatically determine the
        /// maximum number of items to cache before sweeping.
        /// -1 disables sweeping (all items are cached and none are evicted).
        /// Other valid cached item limits are between 1 and 5000000, inclusive
        /// Enumerations are not cached.
        /// </summary>
        /// <returns>Number of read operations that cache items in memory (excluding enumerations).</returns>
        public int? SweepThreshold { get; set; }

        /// <summary>
        /// This configurations is to enable or disable Strict 2PL.
        /// When Strict 2PL is enabled, read locks will be released at the beginning of commit without waiting for the commit to become stable. 
        /// When Strict 2PL is disabled, Rigourous 2PL is used.
        /// This will cause read locks to be released when the commit is stable.
        /// </summary>
        public bool? EnableStrict2PL { get; set; }
    }
}