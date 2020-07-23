// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsData.Update, "ServiceFabricServiceGroup", SupportsShouldProcess = true, DefaultParameterSetName = "Stateless")]
    public sealed class UpdateServiceGroup : ServiceGroupCmdletBase
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

        [Parameter(Mandatory = true, Position = 1)]
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

        protected override void ProcessRecord()
        {
            ServiceGroupUpdateDescription serviceGroupUpdateDescription = new ServiceGroupUpdateDescription();

            if (this.Stateful)
            {
                var statefulServiceUpdateDescription = new StatefulServiceUpdateDescription();
                if (this.TargetReplicaSetSize.HasValue)
                {
                    statefulServiceUpdateDescription.TargetReplicaSetSize = this.TargetReplicaSetSize.Value;
                }

                if (this.MinReplicaSetSize.HasValue)
                {
                    statefulServiceUpdateDescription.MinReplicaSetSize = this.MinReplicaSetSize.Value;
                }

                if (this.ReplicaRestartWaitDuration.HasValue)
                {
                    statefulServiceUpdateDescription.ReplicaRestartWaitDuration = this.ReplicaRestartWaitDuration.Value;
                }

                if (this.QuorumLossWaitDuration.HasValue)
                {
                    statefulServiceUpdateDescription.QuorumLossWaitDuration = this.QuorumLossWaitDuration.Value;
                }

                serviceGroupUpdateDescription.ServiceUpdateDescription = statefulServiceUpdateDescription;
            }
            else
            {
                var statelessServiceUpdateDescription = new StatelessServiceUpdateDescription();
                if (this.InstanceCount.HasValue)
                {
                    statelessServiceUpdateDescription.InstanceCount = this.InstanceCount.Value;
                }

                serviceGroupUpdateDescription.ServiceUpdateDescription = statelessServiceUpdateDescription;
            }

            if (this.ShouldProcess(this.ServiceName.OriginalString))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.UpdateServiceGroup(this.ServiceName, serviceGroupUpdateDescription);
                }
            }
        }
    }
}