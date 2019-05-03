// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
#if ServiceFabric
    using System;
#endif
    using System.Fabric.Query;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Get, "ServiceFabricUnplacedReplicaInformation")]
    public sealed class GetUnplacedReplicaInformation : CommonCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        [ValidateNotNullOrEmpty]
        public string ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public Guid PartitionId
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ValueFromPipelineByPropertyName = true)]
        public bool OnlyPrimaries
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                var queryResult = clusterConnection.GetUnplacedReplicaInformationAsync(
                                      this.ServiceName,
                                      this.PartitionId,
                                      this.OnlyPrimaries,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                foreach (string reason in queryResult.UnplacedReplicaReasons)
                {
                    this.WriteObject(reason);
                }
            }
            catch (AggregateException aggregateException)
            {
                this.ThrowTerminatingError(
                    aggregateException.InnerException,
                    Constants.GetReplicaHealthErrorId,
                    clusterConnection);
            }
        }

        protected override object FormatOutput(object output)
        {
            return base.FormatOutput(output);
        }
    }
}