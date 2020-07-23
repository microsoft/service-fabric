// ----------------------------------------------------------------------
//  <copyright file="PrimaryMoveAnalysisEvent.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.PartitionInsight
{
    using System;
    using System.Globalization;
    using System.Linq;
    using System.Runtime.Serialization;
    using System.Text;
    using ClusterAnalysis.AnalysisCore.AnalysisEvents;
    using ClusterAnalysis.Common;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Assert = Common.Util.Assert;

    /// <summary>
    /// Container for storing all analysis related to a single Reconfiguration that has happened
    /// </summary>
    [DataContract]
    public class PrimaryMoveAnalysisEvent : FabricAnalysisEvent
    {
        private Duration duration;

        /// <summary>
        /// Create an instance of <see cref="PrimaryMoveAnalysisEvent"/>
        /// </summary>
        /// <param name="reconfigRecord"></param>
        public PrimaryMoveAnalysisEvent(ReconfigurationCompletedTraceRecord reconfigRecord) : base(reconfigRecord)
        {
            this.AnalysisStartTimeStamp = DateTime.UtcNow;
        }

        public ReconfigurationCompletedTraceRecord TriggerReconfigurationCompletedTraceRecord
        {
            get { return (ReconfigurationCompletedTraceRecord)this.OriginalInvocationEvent; }
        }

        public DateTime PrimaryMoveCompleteTimeStamp
        {
            get
            {
                return this.TriggerReconfigurationCompletedTraceRecord.TimeStamp;
            }
        }

        public TimeSpan Delay
        {
            get
            {
                return this.AnalysisStartTimeStamp.Subtract(this.PrimaryMoveCompleteTimeStamp);
            }
        }

        public TimeSpan Duration
        {
            get
            {
                return this.AnalysisEndTimeStamp.Subtract(this.AnalysisStartTimeStamp);
            }
        }

        public Guid PartitionId
        {
            get
            {
                return this.TriggerReconfigurationCompletedTraceRecord.PartitionId;
            }
        }

        [DataMember(IsRequired = true)]
        public PrimaryMoveReason Reason { get; internal set; }

        public string CorrelatedEventString
        {
            get
            {
                StringBuilder sb = new StringBuilder();

                if (this.CorrelatedEvents.Any())
                {
                    sb.AppendLine();
                    int index = 0;
                    foreach (var correlatedTraceRecord in this.CorrelatedTraceRecords)
                    {
                        sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "[Record #{0}]:", ++index));
                        sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "\t{0}", correlatedTraceRecord.ToPublicString()));
                    }
                }

                return sb.ToString();
            }
        }

        public string PreviousNode
        {
            get
            {
                if (this.PreviousPrimaryContext == null)
                {
                    return string.Empty;
                }

                return string.Format(CultureInfo.InvariantCulture, "{0}({1})", this.PreviousPrimaryContext.NodeName, this.PreviousPrimaryContext.NodeId);
            }
        }

        public string CurrentNode
        {
            get
            {
                if (this.CurrentPrimaryContext == null)
                {
                    return string.Empty;
                }

                return string.Format(CultureInfo.InvariantCulture, "{0}({1})", this.CurrentPrimaryContext.NodeName, this.CurrentPrimaryContext.NodeId);
            }
        }

        public override string FormattedMessage { get; protected set; }

        [DataMember(IsRequired = true)]
        internal DateTime AnalysisStartTimeStamp { get; set; }

        [DataMember(IsRequired = true)]
        internal DateTime AnalysisEndTimeStamp { get; set; }

        [DataMember(IsRequired = true)]
        internal PrimaryReplicaContext PreviousPrimaryContext { get; set; }

        [DataMember(IsRequired = true)]
        internal PrimaryReplicaContext CurrentPrimaryContext { get; set; }

        [DataMember(IsRequired = true)]
        internal Guid ReasonActivityId { get; set; }

        [DataMember(IsRequired = true)]
        internal ActivityType ReasonActivityType { get; set; }

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

        public override string ToString()
        {
            string typeName = this.GetType().Name;
            string timeStamp = this.TriggerReconfigurationCompletedTraceRecord.TimeStamp.ToString("o");
            string paritionId = this.TriggerReconfigurationCompletedTraceRecord.PartitionId.ToString();
            string reason = this.Reason.ToString();

            StringBuilder sb = new StringBuilder();
            string line = string.Format(
                    CultureInfo.InvariantCulture,
                    "[{0}]: TimeStamp={1}, PartitionId={2}, PreviousNode={3}, CurrentNode={4}, Reason={5}",
                    typeName,
                    timeStamp,
                    paritionId,
                    this.PreviousNode,
                    this.CurrentNode,
                    reason);

            sb.AppendLine(line);

            if (this.CorrelatedEvents.Any())
            {
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "Correlated trace records:"));
            }

            sb.Append(this.CorrelatedEventString);

            return sb.ToString();
        }

        internal Duration GetDuration()
        {
            if (this.duration == null)
            {
                Assert.IsTrue(this.PreviousPrimaryContext != null && this.CurrentPrimaryContext != null, "We need both previous primary timestamp and current primary time stamp.");

                long betweenMovesSpanTicks = this.CurrentPrimaryContext.DateTimeUtcTicks - this.PreviousPrimaryContext.DateTimeUtcTicks;

                if (betweenMovesSpanTicks > PrimaryReplicaContextConstants.TraceReadTimeSpanMaxTicks)
                {
                    long traceReadTimeSpanMaxEarlierTicks = this.CurrentPrimaryContext.DateTimeUtcTicks - PrimaryReplicaContextConstants.TraceReadTimeSpanMaxTicks;
                    this.duration = new Duration(traceReadTimeSpanMaxEarlierTicks, this.CurrentPrimaryContext.DateTimeUtcTicks);
                }
                else
                {
                    this.duration = new Duration(this.PreviousPrimaryContext.DateTimeUtcTicks, this.CurrentPrimaryContext.DateTimeUtcTicks);
                }
            }

            return this.duration;
        }
    }
}