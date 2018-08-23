// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Defines methods a reliable state provider replica must implement for Service Fabric to interact with it.
    /// </summary>
    public interface IStateProviderReplica2 : IStateProviderReplica
    {
        /// <summary>
        /// Function called after restore has been performed on the replica.
        /// </summary>
        /// <value>
        /// Function called when the replica's state has been restored successfully by the framework
        /// </value>
        Func<CancellationToken, Task> OnRestoreCompletedAsync { set; }
    }
}