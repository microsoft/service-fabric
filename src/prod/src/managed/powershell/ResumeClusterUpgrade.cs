// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Resume, "ServiceFabricClusterUpgrade")]
    public sealed class ResumeClusterUpgrade : ClusterCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string UpgradeDomainName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.ResumeClusterUpgrade(this.UpgradeDomainName);
        }
    }
}