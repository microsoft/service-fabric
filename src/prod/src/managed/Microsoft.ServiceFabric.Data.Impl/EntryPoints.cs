// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System.Fabric;

    /// <summary>
    /// Entry points from the SDK into the implementation
    /// </summary>
    public static class EntryPoints
    {
        /// <summary>
        /// Create a new ReliableStateManager
        /// </summary>
        /// <param name="serviceContext">A <see cref="StatefulServiceContext"/> that describes the service context.</param>
        /// <param name="configuration">Configuration parameters.</param>
        public static IReliableStateManagerReplica CreateReliableStateManager(
            StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration)
        {
            return new ReliableStateManagerImpl(serviceContext, configuration);
        }

        /// <summary>
        /// Create a new ReliableStateManager
        /// </summary>
        /// <param name="serviceContext">A <see cref="StatefulServiceContext"/> that describes the service context.</param>
        /// <param name="configuration">Configuration parameters.</param>
        public static IReliableStateManagerReplica2 CreateReliableStateManager2(
            StatefulServiceContext serviceContext, ReliableStateManagerConfiguration configuration)
        {
            return new ReliableStateManagerImpl(serviceContext, configuration);
        }
    }
}