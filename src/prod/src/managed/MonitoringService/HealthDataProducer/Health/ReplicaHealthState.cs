// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.ReplicaHealthState to support testability.
    /// </summary>
    internal class ReplicaHealthState : EntityHealthState
    {
        [SuppressMessage("StyleCop.CSharp.ReadabilityRules", "SA1126:PrefixCallsCorrectly", Justification = "This rule breaks for nameof() operator.")]
        public ReplicaHealthState(Guid partitionId, long replicaId, HealthState state) 
            : base(state)
        {
            this.PartitionId = partitionId;
            this.ReplicaId = replicaId;
        }

        public virtual Guid PartitionId { get; private set; }

        public virtual long ReplicaId { get; private set; }
    }
}