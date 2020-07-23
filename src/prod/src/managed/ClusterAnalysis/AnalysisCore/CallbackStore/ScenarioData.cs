// ----------------------------------------------------------------------
//  <copyright file="ScenarioData.cs" company="Microsoft">
//       Copyright (c) Microsoft Corporation. All rights reserved.
//  </copyright>
// ----------------------------------------------------------------------

namespace ClusterInsight.InsightCore.DataSetLayer.CallbackStore
{
    using System;
    using System.Globalization;
    using System.Runtime.Serialization;
    using ClusterInsight.Common;
    using ClusterInsight.Common.Util;
    using ClusterInsight.InsightCore.DataSetLayer.DataModel;

    [DataContract]
    public class ScenarioData : IUniquelyIdentifiable
    {
        public ScenarioData(Scenario scenario, FabricEvent fabricEvent, DateTime eventSeenTime)
        {
            Assert.IsNotNull(fabricEvent, "fabricEvent");
            this.Scenario = scenario;
            this.FabricEvent = fabricEvent;
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
        public FabricEvent FabricEvent { get; private set; }

        /// <inheritdoc />
        public int GetUniqueIdentity()
        {
            unchecked
            {
                int hash = 17;
                hash = (hash * 23) + this.Scenario.GetHashCode();
                hash = (hash * 23) + this.FabricEvent.GetUniqueIdentity();
                return hash;
            }
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return string.Format(CultureInfo.InvariantCulture, "Scenario: {0}, Event: {1}", this.Scenario, this.FabricEvent);
        }
    }
}