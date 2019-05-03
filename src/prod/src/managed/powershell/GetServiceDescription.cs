// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricServiceDescription")]
    public sealed class GetServicDescription : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Uri ServiceName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                var queryResult = clusterConnection.GetServiceDescriptionAsync(
                                      this.ServiceName,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(queryResult));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetServiceDescriptionErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}