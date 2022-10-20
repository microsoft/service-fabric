// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System;
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.PartitionHealthState to support testability.
    /// </summary>
    internal class PartitionHealthState : EntityHealthState
    {
        public PartitionHealthState(Guid partitionId, HealthState state) 
            : base(state)
        {
            this.PartitionId = partitionId;
        }

        public virtual Guid PartitionId { get; private set; }
    }
}