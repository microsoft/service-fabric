// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Description;
    using System.Management.Automation;
    using System.Numerics;

    [Cmdlet(VerbsLifecycle.Start, Constants.StartNodeTransitionName)]
    public sealed class StartNodeTransition : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ParameterSetName = "Stop")]
        public SwitchParameter Stop
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Start")]
        public SwitchParameter Start
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Start")]
        [Parameter(Mandatory = true, ParameterSetName = "Stop")]
        public Guid OperationId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Start")]
        [Parameter(Mandatory = true, ParameterSetName = "Stop")]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Start")]
        [Parameter(Mandatory = true, ParameterSetName = "Stop")]
        public BigInteger NodeInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Stop")]
        public int StopDurationInSeconds
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            NodeTransitionDescription description = this.CreateDescription();

            try
            {
                clusterConnection.StartNodeTransitionAsync(
                    description,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.StartNodeTransitionCommandErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }

        private NodeTransitionDescription CreateDescription()
        {
            if (this.Start)
            {
                return new NodeStartDescription(this.OperationId, this.NodeName, this.NodeInstanceId);
            }
            else if (this.Stop)
            {
                return new NodeStopDescription(this.OperationId, this.NodeName, this.NodeInstanceId, this.StopDurationInSeconds);
            }
            else
            {
                throw new NotImplementedException("NodeTransitionType not implemented");
            }
        }
    }
}