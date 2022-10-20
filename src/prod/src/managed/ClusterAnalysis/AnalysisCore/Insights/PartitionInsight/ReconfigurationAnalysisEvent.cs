// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.Serialization;
    using ClusterAnalysis.AnalysisCore.AnalysisEvents;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;

    /// <summary>
    /// Container for storing all analysis related to a single Reconfiguration that has happened
    /// </summary>
    [DataContract]
    public class ReconfigurationAnalysisEvent : FabricAnalysisEvent
    {
        /// <summary>
        /// Create an instance of <see cref="ReconfigurationAnalysisEvent"/>
        /// </summary>
        /// <param name="reconfigRecord"></param>
        public ReconfigurationAnalysisEvent(ReconfigurationCompletedTraceRecord reconfigRecord) : base(reconfigRecord)
        {
            this.ReadUnavailabilityDuration = TimeSpan.MinValue;
            this.WriteUnavailabilityDuration = TimeSpan.MinValue;
        }

        public ReconfigurationCompletedTraceRecord TriggerReconfigurationCompletedTraceRecord
        {
            get { return (ReconfigurationCompletedTraceRecord)this.OriginalInvocationEvent; }
        }

        /// <summary>
        /// Time for which we couldn't read.
        /// </summary>
        [DataMember(IsRequired = true)]
        public TimeSpan ReadUnavailabilityDuration { get; internal set; }

        /// <summary>
        /// Time for which we couldn't write.
        /// </summary>
        [DataMember(IsRequired = true)]
        public TimeSpan WriteUnavailabilityDuration { get; internal set; }

        /// <summary>
        /// Start time of the Reconfiguration
        /// </summary>
        [DataMember(IsRequired = true)]
        public DateTime ReconfigStartTime { get; internal set; }

        /// <summary>
        /// End time of the Reconfiguration
        /// </summary>
        [DataMember(IsRequired = true)]
        public DateTime ReconfigFinishTime { get; internal set; }

        public override string FormattedMessage { get; protected set; }

        /// <inheritdoc />
        public override int GetHashCode()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.OriginalInvocationEvent.GetHashCode();
                hash = (hash * 23) + this.OriginTime.GetHashCode();
                return hash;
            }
        }
    }
}