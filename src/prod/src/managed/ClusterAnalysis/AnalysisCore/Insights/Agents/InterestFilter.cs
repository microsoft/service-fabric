// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Insights.Agents
{
    using System;
    using System.Runtime.Serialization;
    using ClusterAnalysis.AnalysisCore.Callback;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Definitions.TypedTraceRecords.RA;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    /// <summary>
    /// Allows Agents to Register interest in future events.
    /// </summary>
    /// <remarks>
    /// Ideally, I can register a lambda and that would be perfect, however, there is no good way to serialize a lambda, something that we need to do.
    /// So, we have this friendly wrapper class where agents can register interest in a way that can be persisted. This class, and the different filter abstractions
    /// will grow as per agents needs. Today, we have some simple filter that are sufficient for our needs.
    /// </remarks>
    [DataContract]
    internal class InterestFilter
    {
        [DataMember] private TraceRecord traceRecord;

        [DataMember] private Scenario interestingScenario;

        [DataMember] private Guid partitionId;

        private InterestFilter()
        {
        }

        public static InterestFilter CreateFilter()
        {
            return new InterestFilter();
        }

        /// <summary>
        /// Register interest in an Event
        /// </summary>
        /// <param name="traceRecord"></param>
        public InterestFilter RegisterInterestForEvent(TraceRecord traceRecord)
        {
            Assert.IsNotNull(traceRecord, "traceRecord");
            this.traceRecord = traceRecord;
            return this;
        }

        /// <summary>
        /// Register interest in Reconfiguration Scenario with partition Filter.
        /// </summary>
        /// <param name="partitionIdentifier"></param>
        /// <remarks>
        /// An example of usage is where an agent wants to analyze all the Reconfiguration events for a specific partition as part of single analysis.
        /// </remarks>
        public InterestFilter RegisterInterestForReconfigurationScenario(Guid partitionIdentifier)
        {
            Assert.IsFalse(partitionIdentifier == Guid.Empty, "PartitionId is empty");
            this.interestingScenario = Scenario.Reconfiguration;
            this.partitionId = partitionIdentifier;
            return this;
        }

        /// <summary>
        /// Check if current filter matches this event.
        /// </summary>
        /// <param name="fabricEvent"></param>
        /// <returns></returns>
        public bool IsMatch(TraceRecord fabricEvent)
        {
            Assert.IsNotNull(fabricEvent, "traceRecord");
            if (this.traceRecord != null)
            {
                return this.traceRecord.Equals(fabricEvent);
            }

            if (this.interestingScenario == Scenario.Reconfiguration)
            {
                var reconfigEvent = fabricEvent as ReconfigurationCompletedTraceRecord;
                return reconfigEvent != null && reconfigEvent.PartitionId == this.partitionId;
            }

            return false;
        }
    }
}