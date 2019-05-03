// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
#if ServiceFabric
    using System;
    using System.Fabric;
#endif
    using System.Management.Automation;

#if ServiceFabric
    [Cmdlet(VerbsCommon.Set, "ServiceFabricReplicaPlacementHealthReporting")]
#else
    [Cmdlet(VerbsCommon.Set, "ServiceFabricReplicaPlacementHealthReporting")]
#endif
    public sealed class ToggleVerboseServicePlacementHealthReporting : ClusterCmdletBase
    {
        [Parameter(Position = 0, Mandatory = true, ValueFromPipelineByPropertyName = true, ParameterSetName = "Enabled")]
        public bool Enabled
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.InternalToggleVerboseServicePlacementHealthReporting(this.Enabled);
        }
    }
}