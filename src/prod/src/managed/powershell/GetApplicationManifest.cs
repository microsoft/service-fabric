// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricApplicationManifest")]
    public sealed class GetApplicationManifest : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var applicationManifest = clusterConnection.GetApplicationManifestAsync(
                                              this.ApplicationTypeName,
                                              this.ApplicationTypeVersion,
                                              this.GetTimeout(),
                                              this.GetCancellationToken()).Result;
                this.WriteObject(applicationManifest);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetApplicationManifestErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}