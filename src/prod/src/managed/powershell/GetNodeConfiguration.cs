// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Management.Automation;
    
    [Cmdlet(VerbsCommon.Get, "ServiceFabricNodeConfiguration")]
    public sealed class GetNodeConfiguration : ClusterCmdletBase
    {
        protected override void ProcessRecord()
        {
            var clusterManifestType = NodeConfiguration.GetNodeConfiguration();
            if (clusterManifestType == null)
            {
                this.WriteWarning("Node is not configured.");
            }

            this.WriteObject(clusterManifestType);
        }
    }
}