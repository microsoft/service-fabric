// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Scenario
{
    using System;
    using System.Fabric.Chaos.RandomActionGenerator;

    /// <summary>
    /// This class defines all the test parameters to configure the ChaosTestScenario.
    /// </summary>
    [Serializable]
    [Obsolete("This class is deprecated. Please use System.Fabric.Chaos.DataStructures.ChaosParameters instead.")]
    public class ChaosTestScenarioParameters : TestScenarioParameters
    {
        private static readonly TimeSpan WaitTimeBetweenIterationsDefault = TimeSpan.FromSeconds(30);

        internal ChaosTestScenarioParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun,
            bool disableStartStopNodeFaults)
            : base(timeToRun)
        {
            if (maxConcurrentFaults < 0)
            {
                throw new ArgumentOutOfRangeException("maxConcurrentFaults", "Value cannot be negative");
            }

            this.MaxClusterStabilizationTimeout = maxClusterStabilizationTimeout;
            this.WaitTimeBetweenIterations = WaitTimeBetweenIterationsDefault;

            this.ActionGeneratorParameters = new ActionGeneratorParameters();

            // Set default values for action generator parameters.
            this.ActionGeneratorParameters.MaxConcurrentFaults = maxConcurrentFaults;

            if (disableStartStopNodeFaults)
            {
                // This is disable start/stop node
                this.ActionGeneratorParameters.NodeFaultActionsParameters.RestartNodeFaultWeight = 1.0;
                this.ActionGeneratorParameters.NodeFaultActionsParameters.StartStopNodeFaultWeight = 0;
            }

            this.ActionGeneratorParameters.NodeFaultActionWeight = 40.0;
            this.ActionGeneratorParameters.ServiceFaultActionWeight = 60.0;

            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RemoveReplicaFaultWeight = 100.0;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartReplicaFaultWeight = 100.0;
            this.ActionGeneratorParameters.ServiceFaultActionsParameters.RestartCodePackageFaultWeight = 100.0;
            if (enableMoveReplicaFaults)
            {
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight = 100.0;
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight = 100.0;
            }
            else
            {
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MovePrimaryFaultWeight = 0.0;
                this.ActionGeneratorParameters.ServiceFaultActionsParameters.MoveSecondaryFaultWeight = 0.0;
            }
        }

        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.Testability.Scenario.ChaosTestScenarioParameters" /> class.</para>
        /// </summary>
        /// <param name="maxClusterStabilizationTimeout">The maximum amount of time to wait for the entire cluster to stabilize after a fault iteration before failing the test.</param>
        /// <param name="maxConcurrentFaults">Maximum number of concurrent faults induced per iteration with the lowest being 1. The higher the concurrency the more aggressive the failovers 
        /// thus inducing more complex series of failures to uncover bugs. using 2 or 3 for this is recommended.</param>
        /// <param name="enableMoveReplicaFaults">Enables or disables the MovePrimary and MoveSecondary faults.</param>
        /// <param name="timeToRun"></param>
        /// <returns>The object containing the Chaos scenario parameters, typed as ChaosScenarioParameters</returns>
        public ChaosTestScenarioParameters(
            TimeSpan maxClusterStabilizationTimeout,
            long maxConcurrentFaults,
            bool enableMoveReplicaFaults,
            TimeSpan timeToRun)
            : this(maxClusterStabilizationTimeout, maxConcurrentFaults, enableMoveReplicaFaults, timeToRun, true)
        {
        }

        /// <summary>
        /// Wait time between two iterations for a random duration bound by this value.
        /// </summary>
        /// <value>
        /// The time-separation between two consecutive iterations of the ChaosTestScenario
        /// </value>
        public TimeSpan WaitTimeBetweenIterations { get; set; }

        internal ActionGeneratorParameters ActionGeneratorParameters { get; set; }

        /// <summary>
        /// The maximum amount of time to wait for the cluster to stabilize after a fault before failing the test.
        /// </summary>
        /// <value>
        /// After each iteration, the ChaosTestScenario waits for at most this amount of time for the cluster to become healthy
        /// </value>
        public TimeSpan MaxClusterStabilizationTimeout { get; set; }
    }
}