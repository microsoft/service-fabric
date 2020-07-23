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

    [Cmdlet(VerbsLifecycle.Start, "ServiceFabricNode")]
    [Obsolete("This cmdlet is deprecated.  Use Start-ServiceFabricNodeTransition -Start instead.")]
    public sealed class StartNode : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 1)]
        public BigInteger? NodeInstanceId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 2)]
        public string IpAddressOrFQDN
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true, Position = 3)]
        public int? ClusterConnectionPort
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public CompletionMode? CommandCompletionMode
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var startNodeResult = clusterConnection.StartNodeAsync(
                                          this.NodeName,
                                          this.NodeInstanceId ?? 0,
                                          this.IpAddressOrFQDN ?? string.Empty,
                                          this.ClusterConnectionPort ?? 0,
                                          this.TimeoutSec,
                                          this.CommandCompletionMode ?? CompletionMode.Verify,
                                          this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(startNodeResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.StartNodeErrorId,
                    clusterConnection);
            }
        }

        protected override object FormatOutput(object output)
        {
            if (!(output is StartNodeResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as StartNodeResult;

            var itemPsObj = new PSObject(item);
            return itemPsObj;
        }
    }
}