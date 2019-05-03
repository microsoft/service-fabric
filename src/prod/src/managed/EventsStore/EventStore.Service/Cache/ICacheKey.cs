// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;

    internal interface ICacheKey : IComparable
    {
        /// <summary>
        /// In cache data is fetched from somewhere. This fetch may happen in intervals and this field
        /// captures the start of the interval in which this particular key was fetched.
        /// </summary>
        DateTimeOffset DataFetchStartTime { get; }

        DateTimeOffset DataFetchEndTime { get; }

        /// <summary>
        /// Captures the original time the value matching this key was created
        /// </summary>
        DateTimeOffset OriginalCreateTime { get; }

        /// <summary>
        /// The time this key was created in cache.
        /// </summary>
        DateTimeOffset CreatedTime { get; }
    }
}
