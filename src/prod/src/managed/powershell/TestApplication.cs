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

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricApplication")]
    public sealed class TestApplication : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Uri ApplicationName
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
                clusterConnection.TestApplicationAsync(
                    this.ApplicationName,
                    this.MaxStabilizationTimeoutSec,
                    this.GetCancellationToken()).Wait();

                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_TestApplicationSucceeded, this.ApplicationName));
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