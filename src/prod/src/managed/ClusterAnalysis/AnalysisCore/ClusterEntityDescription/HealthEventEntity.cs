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
    [KnownType(typeof(HealthInformationEntity))]
    public class HealthEventEntity 
    {
        public HealthEventEntity(HealthEvent healthEvent)
        {
            Assert.IsNotNull(healthEvent, "Health Event can't be null");
            this.IsExpired = healthEvent.IsExpired;
            this.LastErrorTransitionAt = healthEvent.LastErrorTransitionAt;
            this.LastModifiedUtcTimestamp = healthEvent.LastModifiedUtcTimestamp;
            this.LastWarningTransitionAt = healthEvent.LastWarningTransitionAt;
            this.SourceUtcTimestamp = healthEvent.SourceUtcTimestamp;
            this.HealthInformation = new HealthInformationEntity(healthEvent.HealthInformation);
            this.LastOkTransitionAt = healthEvent.LastOkTransitionAt;
        }

        internal HealthEventEntity()
        {
        }

        [DataMember(IsRequired = true)]
        public HealthInformationEntity HealthInformation { get; internal set; }

        [DataMember(IsRequired = true)]
        public bool IsExpired { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTime LastErrorTransitionAt { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTime LastModifiedUtcTimestamp { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTime LastOkTransitionAt { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTime LastWarningTransitionAt { get; internal set; }

        [DataMember(IsRequired = true)]
        public DateTime SourceUtcTimestamp { get; internal set; }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(
                CultureInfo.InvariantCulture,
                "SourceUtcTime: {0} IsExpired: {1}, LastWarningTransitionAt: {2}, HealthInfo: {3}",
                this.SourceUtcTimestamp,
                this.IsExpired,
                this.LastWarningTransitionAt,
                this.HealthInformation);
        }
    }
}