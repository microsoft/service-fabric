// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Repair, "ServiceFabricPartition", SupportsShouldProcess = true, DefaultParameterSetName = "Partition")]
    public sealed class RepairPartition : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, ParameterSetName = "Service")]
        public SwitchParameter Service
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "All")]
        public SwitchParameter All
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "System")]
        public SwitchParameter System
        {
            get;
            set;
        }

        [Parameter]
        public SwitchParameter Force
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Service")]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Position = 0, Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Partition")]
        public Guid PartitionId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(string.Empty))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    if (this.Service)
                    {
                        this.InternalRepairServicePartitions(this.ServiceName);
                    }
                    else if (this.System)
                    {
                        this.InternalRepairSystemPartitions();
                    }
                    else if (this.All)
                    {
                        this.InternalRepairAllPartitions();
                    }
                    else
                    {
                        this.InternalRepairPartition(this.PartitionId);
                    }
                }
            }
        }
    }
}