// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Fabric.Chaos.Common;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Health;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricChaos")]
    public sealed class StartChaos : CommonCmdletBase
    {
        /// <summary>
        /// Time duration for which random scenario should run in minutes.
        /// Non-positive value means infinite.
        /// </summary>
        [Parameter(Mandatory = false)]
        [ValidateRange(0, UInt32.MaxValue / 60)]
        public uint? TimeToRunMinute
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two actions for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        [ValidateRange(0, UInt32.MaxValue)]
        public uint? MaxConcurrentFaults
        {
            get;
            set;
        }

        /// <summary>
        /// The maximum amount of time to wait for all cluster entities to become stable and healthy.
        /// </summary>
        [Parameter(Mandatory = false)]
        [ValidateRange(0, UInt32.MaxValue)]
        public uint? MaxClusterStabilizationTimeoutSec
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two iterations for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        [ValidateRange(0, UInt32.MaxValue)]
        public uint? WaitTimeBetweenIterationsSec
        {
            get;
            set;
        }

        /// <summary>
        /// Pause between two actions for a random duration bound by this value.
        /// </summary>
        [Parameter(Mandatory = false)]
        [ValidateRange(0, UInt32.MaxValue)]
        public uint? WaitTimeBetweenFaultsSec
        {
            get;
            set;
        }

        /// <summary>
        /// Whether to enable move primary or secondary replica faults
        /// </summary>
        [Parameter(Mandatory = false)]
        public SwitchParameter EnableMoveReplicaFaults
        {
            get;
            set;
        }

        /// <summary>
        /// This can be used to record detailed context about why Chaos is being started.
        /// </summary>
        [Parameter(Mandatory = false)]
        public Hashtable Context
        {
            get;
            set;
        }

        /// <summary>
        /// ClusterHealthPolicy determines the state of the health of the entities that Chaos would
        /// ensure before going onto the next set of faults. Setting 'ConsiderWarningAsError' to false
        /// would let Chaos go onto the next set of faults while there are entities in the cluster with
        /// healthState in warning (although Chaos will skip the entities in warning while choosing
        /// fault-able entities.)
        /// </summary>
        [Parameter(Mandatory = false)]
        public ClusterHealthPolicy ClusterHealthPolicy
        {
            get;
            set;
        }

        /// <summary>
        /// List of cluster entities to target for Chaos faults.
        /// </summary>
        [Parameter(Mandatory = false)]
        public ChaosTargetFilter ChaosTargetFilter
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.StartChaosAsync(
                    this.CreateChaosScenarioParameters(),
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.StartChaosCommandErrorId,
                    clusterConnection);
            }
        }

        private ChaosParameters CreateChaosScenarioParameters()
        {
            var chaosParametersObject = new ChaosParameters(
                    this.MaxClusterStabilizationTimeoutSec.HasValue
                        ? TimeSpan.FromSeconds(this.MaxClusterStabilizationTimeoutSec.Value)
                        : ChaosConstants.DefaultClusterStabilizationTimeout,
                    this.MaxConcurrentFaults ?? ChaosConstants.MaxConcurrentFaultsDefault,
                    this.EnableMoveReplicaFaults.IsPresent,
                    this.TimeToRunMinute.HasValue ? TimeSpan.FromMinutes(this.TimeToRunMinute.Value) : TimeSpan.MaxValue,
                    this.GetContextDictionary(this.Context),
                    this.WaitTimeBetweenIterationsSec.HasValue ? TimeSpan.FromSeconds(this.WaitTimeBetweenIterationsSec.Value) : ChaosConstants.WaitTimeBetweenIterationsDefault,
                    this.WaitTimeBetweenFaultsSec.HasValue ? TimeSpan.FromSeconds(this.WaitTimeBetweenFaultsSec.Value) : ChaosConstants.WaitTimeBetweenFaultsDefault,
                    this.ClusterHealthPolicy);

            chaosParametersObject.ChaosTargetFilter = this.ChaosTargetFilter;

            return chaosParametersObject;
        }

        private Dictionary<string, string> GetContextDictionary(Hashtable context)
        {
            if (context == null)
            {
                return null;
            }

            var contextDictionary = new Dictionary<string, string>();

            try
            {
                foreach (var key in context.Keys)
                {
                    contextDictionary.Add((string)key, (string)context[key]);
                }
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    Constants.GetNameValueCollectionErrorId,
                    context);
            }

            return contextDictionary;
        }
    }
}