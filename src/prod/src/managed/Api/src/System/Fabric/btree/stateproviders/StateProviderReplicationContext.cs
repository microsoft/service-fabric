// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections.Generic;
    using System.Threading.Tasks;

    /// <summary>
    /// Replication context state.
    /// </summary>
    enum StateProviderReplicationContextStatus
    {
        /// <summary>
        /// Replication context was just created.
        /// </summary>
        Created,
        /// <summary>
        /// Replicaton context is in flight.
        /// </summary>
        InFlight,
        /// <summary>
        /// Replication context is completed.
        /// </summary>
        Completed,
        /// <summary>
        /// Replication context was sent out for apply.
        /// </summary>
        InApply
    }
    /// <summary>
    /// Replication context.
    /// </summary>
    class StateProviderReplicationContext
    {
        /// <summary>
        /// Operation part of this replication context.
        /// </summary>
        public StateProviderOperation Operation;
        /// <summary>
        /// Replication task.
        /// </summary>
        public Task Task;
        /// <summary>
        /// Replication task completion.
        /// </summary>
        public TaskCompletionSource<object> CompletionSource;
        /// <summary>
        /// State of this replication context.
        /// </summary>
        public StateProviderReplicationContextStatus Status = StateProviderReplicationContextStatus.Created;
        /// <summary>
        /// Exception encountered during the processing of this replication context.
        /// </summary>
        public Exception Exception;
    }
}