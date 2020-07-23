// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricService", SupportsShouldProcess = true, DefaultParameterSetName = "Stateless")]
    public sealed class UpdateService : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateful")]
        public SwitchParameter Stateful
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "Stateless")]
        public SwitchParameter Stateless
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful")]
        public int? TargetReplicaSetSize
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful")]
        public int? MinReplicaSetSize
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful")]
        public TimeSpan? ReplicaRestartWaitDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful")]
        public TimeSpan? QuorumLossWaitDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateful")]
        public TimeSpan? StandByReplicaKeepDuration
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "Stateless")]
        public int? InstanceCount
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string PlacementConstraints
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Metric
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] Correlation
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] PlacementPolicy
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        [ValidateSet("Zero", "Low", "Medium", "High")]
        public string DefaultMoveCost
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] PartitionNamesToAdd
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string[] PartitionNamesToRemove
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public List<ScalingPolicyDescription> ScalingPolicies
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            ServiceUpdateDescription serviceUpdateDescription;
            if (this.Stateful)
            {
                serviceUpdateDescription = this.GetStatefulServiceUpdateDescription(
                    this.TargetReplicaSetSize,
                    this.MinReplicaSetSize,
                    this.ReplicaRestartWaitDuration,
                    this.QuorumLossWaitDuration,
                    this.StandByReplicaKeepDuration,
                    this.Metric,
                    this.Correlation,
                    this.PlacementPolicy,
                    this.PlacementConstraints,
                    this.DefaultMoveCost,
                    this.PartitionNamesToAdd,
                    this.PartitionNamesToRemove,
                    this.ScalingPolicies);
            }
            else
            {
                serviceUpdateDescription = this.GetStatelessServiceUpdateDescription(
                    this.InstanceCount,
                    this.Metric,
                    this.Correlation,
                    this.PlacementPolicy,
                    this.PlacementConstraints,
                    this.DefaultMoveCost,
                    this.PartitionNamesToAdd,
                    this.PartitionNamesToRemove,
                    this.ScalingPolicies);
            }

            if (this.ShouldProcess(this.ServiceName.OriginalString))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.UpdateService(this.ServiceName, serviceUpdateDescription);
                }
            }
        }
    }
}