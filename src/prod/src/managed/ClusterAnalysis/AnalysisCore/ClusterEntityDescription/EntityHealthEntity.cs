// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System.Collections.Generic;
    using System.Fabric.Health;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wrapper over Cluster Partition for easy unit testability. All props are internal so that
    /// they can be easily set by Unit test.
    /// </summary>
    [DataContract]
    [KnownType(typeof(HealthEventEntity))]
    public class EntityHealthEntity
    {
        public EntityHealthEntity(EntityHealth entityHealth)
        {
            Assert.IsNotNull(entityHealth, "EntityHealth can't be null");
            this.AggregatedHealthState = entityHealth.AggregatedHealthState;
            this.HealthEvents = entityHealth.HealthEvents.Select(item => new HealthEventEntity(item)).ToList();
        }

        internal EntityHealthEntity()
        {
        }

        [DataMember(IsRequired = true)]
        public HealthState AggregatedHealthState { get; internal set; }

        [DataMember(IsRequired = true)]
        public IList<HealthEventEntity> HealthEvents { get; internal set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "Aggregated Health: '{0}', Health Events: '{1}'",
                this.AggregatedHealthState,
                string.Join("\n", this.HealthEvents));
        }
    }
}