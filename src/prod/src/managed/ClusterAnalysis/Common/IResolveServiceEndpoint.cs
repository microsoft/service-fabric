// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Contract for Endpoint Resolver
    /// </summary>
    public interface IResolveServiceEndpoint
    {
        /// <summary>
        /// Resolve a given endpoint
        /// </summary>
        /// <param name="serviceUri"></param>
        /// <param name="hostNodeId"></param>
        /// <param name="resourceToAccess"></param>
        /// <param name="token"></param>
        /// <returns></returns>
        Task<Uri> GetServiceEndpointAsync(Uri serviceUri, string hostNodeId, string resourceToAccess, CancellationToken token);
    }
}