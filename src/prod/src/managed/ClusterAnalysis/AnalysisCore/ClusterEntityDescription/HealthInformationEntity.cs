// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.ClusterEntityDescription
{
    using System;
    using System.Fabric.Health;
    using System.Globalization;
    using System.Runtime.Serialization;

    using ClusterAnalysis.Common.Util;

    /// <summary>
    /// Wrapper over Cluster Partition for easy unit testability. All props are internal so that
    /// they can be easily set by Unit test.
    /// </summary>
    [DataContract]
    public class HealthInformationEntity
    {
        public HealthInformationEntity(HealthInformation healthInfo)
        {
            Assert.IsNotNull(healthInfo, "HealthInformation can't be null");
            this.Description = healthInfo.Description;
            this.HealthState = healthInfo.HealthState;
            this.Property = healthInfo.Property;
            this.RemoveWhenExpired = healthInfo.RemoveWhenExpired;
            this.SequenceNumber = healthInfo.SequenceNumber;
            this.SourceId = healthInfo.SourceId;
            this.TimeToLive = healthInfo.TimeToLive;
        }

        internal HealthInformationEntity()
        {
        }

        [DataMember(IsRequired = true)]
        public string Description { get; internal set; }

        [DataMember(IsRequired = true)]
        public HealthState HealthState { get; internal set; }

        [DataMember(IsRequired = true)]
        public string Property { get; internal set; }

        [DataMember(IsRequired = true)]
        public bool RemoveWhenExpired { get; internal set; }

        [DataMember(IsRequired = true)]
        public long SequenceNumber { get; internal set; }

        [DataMember(IsRequired = true)]
        public string SourceId { get; internal set; }

        [DataMember(IsRequired = true)]
        public TimeSpan TimeToLive { get; internal set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "SourceId: {0}, Property: {1}, TTL: {2}, HealthState: {3}', Sequence: {4}, Description: {5}",
                this.SourceId,
                this.Property,
                this.TimeToLive,
                this.HealthState,
                this.SequenceNumber,
                this.Description);
        }
    }
}