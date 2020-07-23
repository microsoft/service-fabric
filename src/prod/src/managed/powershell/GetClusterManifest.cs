// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricClusterManifest")]
    public sealed class GetClusterManifest : ClusterCmdletBase
    {
        [Parameter(Mandatory = false)]
        public string ClusterManifestVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var clusterManifest = clusterConnection.GetClusterManifestAsync(
                      new ClusterManifestQueryDescription() { ClusterManifestVersion = this.ClusterManifestVersion },
                      this.GetTimeout(),
                      this.GetCancellationToken()).Result;
                this.WriteObject(clusterManifest);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterManifestErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}