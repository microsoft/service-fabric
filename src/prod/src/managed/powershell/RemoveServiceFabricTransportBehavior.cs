// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Fabric.Common;
using System.Management.Automation;

namespace Microsoft.ServiceFabric.PowerTools
{
    [Cmdlet(VerbsCommon.Remove, "ServiceFabricTransportBehavior")]
    public sealed class RemoveServiceFabricTransportBehavior : Microsoft.ServiceFabric.Powershell.CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string BehaviorName
        {
            get;
            set;
        }

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
                clusterConnection.RemoveUnreliableTransportAsync(
                    NodeName,
                    BehaviorName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                // localization of string not needed, since this cmdlet is an internal use.
                this.WriteObject(string.Format("Removed Unreliable transport for behavior: {0}.\n", BehaviorName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        ConstantsInternal.RemoveUnreliableTransportId,
                        this.GetClusterConnection());
                    return true;
                });
            }
        }
    }
}