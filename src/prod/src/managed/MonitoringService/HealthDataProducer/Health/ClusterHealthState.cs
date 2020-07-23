// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.ClusterHealthState to support testability.
    /// </summary>
    internal class ClusterHealthState : EntityHealthState
    {
        public ClusterHealthState(HealthState state) 
            : base(state)
        {
        }
    }
}