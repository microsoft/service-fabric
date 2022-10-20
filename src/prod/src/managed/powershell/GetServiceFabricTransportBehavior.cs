// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Common;
using System.Management.Automation;
using Microsoft.ServiceFabric.Powershell;

namespace Microsoft.ServiceFabric.PowerTools
{
    [Cmdlet(VerbsCommon.Get, "ServiceFabricTransportBehavior")]
    public sealed class GetServiceFabricTransportBehavior : Microsoft.ServiceFabric.Powershell.CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string NodeName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            try
            {
                if (this.GetClusterConnection() == null)
                {
                    throw new Exception("Connection to Cluster is null, Connect to Cluster before invoking the cmdlet");
                }

                Microsoft.ServiceFabric.Powershell.InternalClusterConnection clusterConnection =
                    new Microsoft.ServiceFabric.Powershell.InternalClusterConnection(this.GetClusterConnection());

                var queryResult = clusterConnection.GetTransportBehavioursAsync(
                                      this.NodeName,
                                      this.GetTimeout(),
                                      this.GetCancellationToken()).Result;

                foreach (var item in queryResult)
                {
                    this.WriteObject(this.FormatOutput(item));
                }

            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        ConstantsInternal.GetTransportBehaviorList,
                        this.GetClusterConnection());
                    return true;
                });
            }

        }
    }
}