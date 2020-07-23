// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Reset, "ServiceFabricPartitionLoad")]
    public sealed class ResetPartitionLoad : ClusterCmdletBase
    {
        [Parameter(Position = 0, Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Partition")]
        public Guid PartitionId
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.InternalResetPartitionLoad(this.PartitionId);
        }
    }
}