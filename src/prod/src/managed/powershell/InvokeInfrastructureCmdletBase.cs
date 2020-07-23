// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;
    using System.Threading.Tasks;
    using Microsoft.PowerShell.Commands;

    public abstract class InvokeInfrastructureCmdletBase : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string Command
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        internal abstract string ErrorId
        {
            get;
        }

        // Must be internal instead of protected, because IClusterConnection is internal
        internal abstract Task<string> InvokeInfrastructureAsync(IClusterConnection clusterConnection);       

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                string response = this.InvokeInfrastructureAsync(clusterConnection).Result;

                if (!string.IsNullOrEmpty(response))
                {
                    // This is the same code used by the built-in ConvertFrom-Json cmdlet
                    ErrorRecord errorRecord;
                    object outputObject = JsonObject.ConvertFromJson(response, out errorRecord);

                    if (errorRecord != null)
                    {
                        this.ThrowTerminatingError(errorRecord);
                    }

                    this.WriteObject(outputObject, true);
                }
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        this.ErrorId,
                        clusterConnection);
                    return true;
                });
            }
            catch (Exception exception)
            {
                this.ThrowTerminatingError(
                    exception,
                    this.ErrorId,
                    clusterConnection);
            }
        }
    }
}