// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;
    using Threading.Tasks;

    /// <summary>
    /// Wrapper over Service Fabric's QueryClient for unit testability.
    /// </summary>
    internal interface IQueryClient
    {
        /// <summary>
        /// Gets the node list and sets a time when the node was up. 
        /// If <c>referenceTime</c> is <c>null</c>, it is taken to be <see cref="DateTimeOffset.UtcNow"/>.
        /// When we query Service Fabric's query client for the node list, it gives us a TimeSpan for
        /// the 'node up' duration. E.g. node was up since 1 hour 15 min. 
        /// In this wrapper, we convert the TimeSpan to a DateTimeOffset with
        /// a reference time (node was up since 1 hour 15 min measured at reference time, say 2016-05-01 10:30:00).
        /// </summary>
        Task<IList<INode>> GetNodeListAsync(DateTimeOffset? referenceTime = null);
    }
}