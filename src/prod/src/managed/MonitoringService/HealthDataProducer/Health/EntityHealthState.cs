// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Fabric.Health;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.HealthState to support testability.
    /// </summary>
    internal class EntityHealthState
    {
        internal EntityHealthState(HealthState state)
        {
            this.AggregatedHealthState = state;
        }

        internal HealthState AggregatedHealthState { get; private set; }

        public override string ToString()
        {
            return this.AggregatedHealthState.ToString();
        }

        /// <summary>
        /// Checks if the healthState is warning or error state.
        /// </summary>
        /// <returns>Boolean result.</returns>
        internal bool IsWarningOrError()
        {
            return this.AggregatedHealthState.IsWarningOrError();
        }
    }
}