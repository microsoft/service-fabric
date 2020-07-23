// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.AzureTableReader
{
    /// <summary>
    /// The Target Type
    /// </summary>
    public enum AzureReadTarget
    {
        /// <summary>
        /// Indicates that we are reading from Event Store.
        /// </summary>
        EventStore,

        /// <summary>
        /// Indicates that we are reading from Query Store.
        /// </summary>
        QueryStore
    }
}