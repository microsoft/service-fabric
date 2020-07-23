// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.Common.Config
{
    using System;
    
    // TODO: I may refactor this a bit in future.
    [Flags]
    public enum RunMode
    {
        /// <summary>
        /// It's a once box cluster
        /// </summary>
        OneBoxCluster = 1,

        /// <summary>
        /// It's a multi node environment.
        /// </summary>
        MultiNodeCluster = 2,

        /// <summary>
        /// We are supposed to use Azure Event Store
        /// </summary>
        AzureEventStore = 4,

        /// <summary>
        /// We are supposed to use Local Event Store
        /// </summary>
        LocalEventStore = 8
    }
}