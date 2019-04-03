// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Monitoring.Health
{
    using System.Collections.Generic;
    using System.Linq;
    using System.Text;
    using FabricMonSvc;

    /// <summary>
    /// Extensible wrapper for System.Fabric.Health.EntityHealth to support testability.
    /// </summary>
    internal abstract class EntityHealth 
    {
        internal EntityHealth(System.Fabric.Health.HealthState aggregatedHealthState)
        {
            this.AggregatedHealthState = aggregatedHealthState;
        }

        internal EntityHealth(
            System.Fabric.Health.HealthState aggregatedHealthState, 
            IEnumerable<EntityHealthEvent> healthEvents, 
            IList<System.Fabric.Health.HealthEvaluation> unhealthyEvaluations)
        {
            this.AggregatedHealthState = aggregatedHealthState;
            this.HealthEvents = healthEvents;
            this.UnhealthyEvaluations = unhealthyEvaluations;
        }

        internal EntityHealth(System.Fabric.Health.EntityHealth health)
        {
            Guard.IsNotNull(health, nameof(health));
            this.AggregatedHealthState = health.AggregatedHealthState;
            this.UnhealthyEvaluations = health.UnhealthyEvaluations;
            this.HealthEvents = health.HealthEvents.Select(ev => new EntityHealthEvent(ev));
        }

        internal virtual System.Fabric.Health.HealthState AggregatedHealthState { get; private set; }

        internal virtual IList<System.Fabric.Health.HealthEvaluation> UnhealthyEvaluations { get; private set; }

        internal virtual IEnumerable<EntityHealthEvent> HealthEvents { get; private set; }

        /// <summary>
        /// Joins the EntityHealth::UnhealthyEvaluations to create a string representation.
        /// </summary>
        /// <returns>String instance</returns>
        internal string GetHealthEvaluationString()
        {
            if (this.UnhealthyEvaluations == null)
            {
                return null;
            }

            bool isFirstString = true;
            var stringBuilder = new StringBuilder();
            foreach (var item in this.UnhealthyEvaluations)
            {
                stringBuilder.Append(isFirstString ? string.Empty : ", ");
                stringBuilder.Append(item.Description);
                isFirstString = false;
            }

            return stringBuilder.ToString();
        }
    }
}