// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Fabric.Health;
    using FabricMonSvc;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.HealthEvent to support testability.
    /// </summary>
    internal class EntityHealthEvent
    {
        internal EntityHealthEvent(
            HealthState state, 
            string desc, 
            string property, 
            long seqNum, 
            string source, 
            bool isExpired)
        {
            this.HealthState = state;
            this.Description = desc;
            this.Property = property;
            this.SequenceNumber = seqNum;
            this.SourceId = source;
            this.IsExpired = isExpired;
        }

        internal EntityHealthEvent(HealthEvent healthEvent)
        {
            Guard.IsNotNull(healthEvent, nameof(healthEvent));
            this.HealthState = healthEvent.HealthInformation.HealthState;
            this.Description = healthEvent.HealthInformation.Description;
            this.Property = healthEvent.HealthInformation.Property;
            this.SequenceNumber = healthEvent.HealthInformation.SequenceNumber;
            this.SourceId = healthEvent.HealthInformation.SourceId;
            this.IsExpired = healthEvent.IsExpired;
        }

        internal virtual HealthState HealthState { get; private set; }

        internal virtual string Description { get; private set; }

        internal virtual string Property { get; private set; }

        internal virtual long SequenceNumber { get; private set; }

        internal virtual string SourceId { get; private set; }

        internal virtual bool IsExpired { get; private set; }

        /// <summary>
        /// Checks if the healthState on the healthEvent is warning or error state.
        /// Returns false if the healthEvent has expired.
        /// </summary>
        /// <returns>Boolean result.</returns>
        internal bool IsWarningOrError()
        {
            return this.IsExpired
                ? false
                : this.HealthState.IsWarningOrError();
        }
    }
}