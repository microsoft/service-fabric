// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;

    /// <summary>
    /// This class defines all the test parameters to configure the FailoverTestScenario.
    /// </summary>
    [Serializable]
    [Obsolete("FailoverTestScenario and FailoverTestScenarioParameters are deprecated.  Please use Chaos instead https://docs.microsoft.com/azure/service-fabric/service-fabric-controlled-chaos")]
    public class FailoverTestScenarioParameters : TestScenarioParameters
    {
        /// <summary>
        /// Constructor for the FailoverTestScenarioParameters.
        /// </summary>
        /// <param name="partitionSelector">PartitionSelector which gives the partition being targeted for the test.</param>
        /// <param name="timeToRun">The total time for which the failover test will run.</param>
        /// <param name="maxServiceStabilizationTimeout">The maximum amount of time to wait for the service to stabilize after a fault before failing the test.</param>
        public FailoverTestScenarioParameters(PartitionSelector partitionSelector, TimeSpan timeToRun, TimeSpan maxServiceStabilizationTimeout)
            : base(timeToRun)
        {
            this.PartitionSelector = partitionSelector;
            this.MaxServiceStabilizationTimeout = maxServiceStabilizationTimeout;
        }

        /// <summary>
        /// The PartitionSelector which gives the partition that needs to be targeted for the test.
        /// </summary>
        /// <value>
        /// Returns <see cref="System.Fabric.PartitionSelector" />
        /// </value>
        public PartitionSelector PartitionSelector { get; set; }

        /// <summary>
        /// The maximum amount of time to wait for the service to stabilize after a fault before failing the test.
        /// </summary>
        /// <value>
        /// Before start exercising failover scenarios, FailoverTestScenario waits at most this amount of time for the service to stabilize and to become healthy.
        /// </value>
        public TimeSpan MaxServiceStabilizationTimeout { get; set; }
    }
}