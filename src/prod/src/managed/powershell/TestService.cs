// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricService")]
    public sealed class TestService : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public int MaxStabilizationTimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                clusterConnection.TestServiceAsync(
                    this.ServiceName,
                    this.MaxStabilizationTimeoutSec,
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_TestServiceSucceeded, this.ServiceName));
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