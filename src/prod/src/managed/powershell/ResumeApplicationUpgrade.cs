// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Resume, "ServiceFabricApplicationUpgrade")]
    public sealed class ResumeApplicationUpgrade : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string UpgradeDomainName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            this.MoveNextApplicationUpgradeDomain(this.ApplicationName, this.UpgradeDomainName);
        }
    }
}