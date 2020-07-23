// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Fabric.Testability.Scenario;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Invoke, "ServiceFabricChaosTestScenario")]
    [Obsolete("This cmdlet is deprecated.  To manage Chaos in your cluster, please use Start-ServiceFabricChaos, Stop-ServiceFabricChaos, and Get-ServiceFabricChaosReport instead.  These APIs require the FaultAnalysisService")]
    public sealed class InvokeChaosTestScenario : CommonCmdletBase
    {
        /// <summary>
        /// Time duration for which random scenario should run in minutes.
        /// Non-positive value means infinite.
        /// </summary>
        [Parameter(Mandatory = true, Position = 0)]
        public uint TimeToRunMinute
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public uint MaxClusterStabilizationTimeoutSec
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two iterations for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        public uint? WaitTimeBetweenIterationsSec
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two actions for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        public uint? WaitTimeBetweenFaultsSec
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two actions for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        public uint? MaxConcurrentFaults
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter EnableMoveReplicaFaults
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.InvokeChaosTestScenarioAsync(
                    this.CreateChaosScenarioParameters(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(StringResources.Info_ChaosTestScenarioSucceeded);
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.InvokeChaosTestCommandErrorId,
                    clusterConnection);
            }
        }

        private ChaosTestScenarioParameters CreateChaosScenarioParameters()
        {
            uint maxConcurrentFaults = this.MaxConcurrentFaults.HasValue ? this.MaxConcurrentFaults.Value : 1;

            ChaosTestScenarioParameters chaosScenarioParameters = new ChaosTestScenarioParameters(
                TimeSpan.FromSeconds(this.MaxClusterStabilizationTimeoutSec),
                maxConcurrentFaults,
                this.EnableMoveReplicaFaults.IsPresent,
                TimeSpan.FromMinutes(this.TimeToRunMinute));

            if (this.WaitTimeBetweenFaultsSec.HasValue)
            {
                chaosScenarioParameters.WaitTimeBetweenFaults = TimeSpan.FromSeconds(this.WaitTimeBetweenFaultsSec.Value);
            }

            if (this.WaitTimeBetweenIterationsSec.HasValue)
            {
                chaosScenarioParameters.WaitTimeBetweenIterations = TimeSpan.FromSeconds(this.WaitTimeBetweenIterationsSec.Value);
            }

            return chaosScenarioParameters;
        }
    }
}