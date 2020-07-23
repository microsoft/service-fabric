// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Result;
    using System.Management.Automation;
    using System.Numerics;

    [Cmdlet(VerbsCommon.Get, Constants.GetClusterConfiguration)]

    public sealed class GetClusterConfiguration : CommonCmdletBase
    {
        [Parameter(Mandatory = false, ParameterSetName = "UseApiVersion")]
        public SwitchParameter UseApiVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UseApiVersion")]
        public string ApiVersion
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                if (this.UseApiVersion != !string.IsNullOrWhiteSpace(this.ApiVersion))
                {
                    throw new ArgumentException("UseApiVersion and ApiVersion must be both specified or unspecified");
                }

                var clusterConfiguration = clusterConnection.GetClusterConfigurationAsync(
                                               this.ApiVersion,
                                               this.GetTimeout(),
                                               this.GetCancellationToken()).Result;
                this.WriteObject(clusterConfiguration);
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.GetClusterConfigurationErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception ex)
            {
                this.ThrowTerminatingError(
                    ex,
                    Constants.GetClusterConfigurationErrorId,
                    clusterConnection);
            }
        }
    }
}