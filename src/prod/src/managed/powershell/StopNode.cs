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

    [Cmdlet(VerbsLifecycle.Stop, "ServiceFabricNode")]
    [Obsolete("This cmdlet is deprecated.  Use Start-ServiceFabricNodeTransition -Stop instead.")]
    public sealed class StopNode : CommonCmdletBase
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
                var stopNodeResult = clusterConnection.StopNodeAsync(
                                         this.NodeName,
                                         this.NodeInstanceId ?? 0,
                                         this.TimeoutSec,
                                         this.CommandCompletionMode ?? CompletionMode.Verify,
                                         this.GetCancellationToken()).Result;

                this.WriteObject(this.FormatOutput(stopNodeResult));
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.NodeOperationCommandErrorId,
                    clusterConnection);
            }
        }

        protected override object FormatOutput(object output)
        {
            if (!(output is StopNodeResult))
            {
                return base.FormatOutput(output);
            }

            var item = output as StopNodeResult;
            var itemPsObj = new PSObject(item);
            return itemPsObj;
        }
    }
}