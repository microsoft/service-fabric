// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Copy, "ServiceFabricServicePackageToNode")]
    public sealed class DeployServicePackageToNode : CommonCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string ServiceManifestName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 4)]
        public string NodeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public PackageSharingPolicy[] PackageSharingPolicies
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var clusterConnection = this.GetClusterConnection();

            try
            {
                PackageSharingPolicyList sharingPolicyList = null;
                if (this.PackageSharingPolicies != null)
                {
                    sharingPolicyList = new PackageSharingPolicyList(this.PackageSharingPolicies);
                }

                clusterConnection.DeployServicePackageToNodeAsync(
                    this.ApplicationTypeName,
                    this.ApplicationTypeVersion,
                    this.ServiceManifestName,
                    sharingPolicyList,
                    this.NodeName,
                    this.GetTimeout(),
                    this.GetCancellationToken()).Wait();
                this.WriteObject(string.Format(CultureInfo.CurrentCulture, StringResources.Info_ServicePackageDeployToNodeSucceeded, this.ServiceManifestName, this.NodeName));
            }
            catch (AggregateException aggregateException)
            {
                aggregateException.Handle((ae) =>
                {
                    this.ThrowTerminatingError(
                        ae,
                        Constants.DeployServicePackageToNodeErrorId,
                        clusterConnection);
                    return true;
                });
            }
        }
    }
}