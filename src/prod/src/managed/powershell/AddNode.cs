// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Add, "ServiceFabricNode")]
    public sealed class AddNode : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string NodeType
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string IpAddressOrFQDN
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string UpgradeDomain
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string FaultDomain
        {
            get;
            set;
        }

        [Parameter(Mandatory = true)]
        public string FabricRuntimePackagePath
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();
            try
            {
                this.AddNode(
                    this.NodeName,
                    this.NodeType,
                    this.IpAddressOrFQDN,
                    this.UpgradeDomain,
                    this.FaultDomain,
                    this.FabricRuntimePackagePath);
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