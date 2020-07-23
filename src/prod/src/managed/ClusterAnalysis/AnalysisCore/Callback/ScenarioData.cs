// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace ClusterAnalysis.AnalysisCore.Callback
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterAnalysis.Common;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;
    using Assert = ClusterAnalysis.Common.Util.Assert;

    [DataContract]
    internal class ScenarioData
    {
        public ScenarioData(Scenario scenario, TraceRecord record, DateTime eventSeenTime)
        {
            Assert.IsNotNull(record, "TraceRecord can't be null");
            this.Scenario = scenario;
            this.TraceRecord = record;
            this.EventSeenTime = eventSeenTime;
        }

        /// <summary>
        /// Gets the scenario this data represents
        /// </summary>
        [DataMember(IsRequired = true)]
        public Scenario Scenario { get; private set; }

        /// <summary>
        /// Gets the time this event was seen by runtime.
        /// </summary>
        [DataMember(IsRequired = true)]
        public DateTime EventSeenTime { get; private set; }

        /// <summary>
        /// Gets the event for the scenario
        /// </summary>
        [DataMember(IsRequired = true)]
        public TraceRecord TraceRecord { get; private set; }

        /// <summary>
        ///  Get Unique Identity
        /// </summary>
        /// <returns></returns>
        public Guid GetUniqueIdentity()
        {
            return this.TraceRecord.GetUniqueId();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Scenario: {0}, Event: {1}", this.Scenario, this.TraceRecord);
        }
    }
}