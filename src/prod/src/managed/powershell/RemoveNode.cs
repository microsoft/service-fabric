// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricNode")]
    [Obsolete("This cmdlet is deprecated. Use Start-ServiceFabricClusterConfigurationUpgrade to remove nodes from the cluster.")]
    public sealed class RemoveNode : ClusterCmdletBase
    {
        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (!this.Force &&
                !this.ShouldContinue(StringResources.Warning_RemoveServiceFabricNode, string.Empty))
            {
                return;
            }

            var clusterConnection = this.GetClusterConnection();

            try
            {
                this.RemoveNode();
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.StartNodeErrorId,
                    clusterConnection);
            }
        }
    }
}