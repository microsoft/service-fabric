// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Collections.Generic;

    internal interface ICacheValue : IComparable
    {
        /// <summary>
        /// Data Map.
        /// </summary>
        IDictionary<string, string> DataMap { get; }

        DateTimeOffset ValueCreateTime { get; }
    }
}
