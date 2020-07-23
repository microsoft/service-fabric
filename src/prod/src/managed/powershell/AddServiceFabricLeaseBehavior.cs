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
    [Cmdlet(VerbsCommon.Add, "ServiceFabricLeaseBehavior", SupportsShouldProcess = true)]
    public sealed class AddServiceFabricLeaseBehavior : Microsoft.ServiceFabric.Powershell.CommonCmdletBase
    {
        [Parameter(Mandatory = true)]
        public string BehaviorString
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

        [Parameter(Mandatory = true)]
        public string Alias
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.WriteObject(string.Format(
                                 "Note:Adding Lease behavior causes reliability issues on the cluster.\n" +
                                 "Note: The behavior {0} is applied on the node until it is removed with Remove-ServiceFabricLeaseBehavior cmdlet.", BehaviorString));

            try
            {
                if (this.GetClusterConnection() == null)
                {
                    throw new Exception("Connection to Cluster is null, Connect to Cluster before invoking the cmdlet");
                }

                Microsoft.ServiceFabric.Powershell.InternalClusterConnection clusterConnection =
                    new Microsoft.ServiceFabric.Powershell.InternalClusterConnection(this.GetClusterConnection());

                clusterConnection.AddUnreliableLeaseAsync(
                    NodeName,
                    BehaviorString,
                    Alias,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();

                // localization of string not needed, since this cmdlet is an internal use.
                this.WriteObject(string.Format(
                                     "Added Unreliable Lease for behavior: {0}.\n" +
                                     "Note: The behavior is applied on the node until it is removed with Remove-UnreliableLease cmdlet.", BehaviorString));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        ConstantsInternal.AddUnreliableLeaseId,
                        this.GetClusterConnection());
                    return true;
                });
            }
        }
    }
}