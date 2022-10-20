// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricPackageSharingPolicy")]
    public sealed class NewPackageSharingPolicy : CommonCmdletBase
    {
        [Parameter(Mandatory = false, Position = 0)]
        public string PackageName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "All")]
        public SwitchParameter SharingScopeAll
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Code")]
        public SwitchParameter SharingScopeCode
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Config")]
        public SwitchParameter SharingScopeConfig
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "Data")]
        public SwitchParameter SharingScopeData
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            PackageSharingPolicy sharingPolicy;
            if (this.SharingScopeAll.IsPresent)
            {
                sharingPolicy = new PackageSharingPolicy(this.PackageName, PackageSharingPolicyScope.All);
            }
            else if (this.SharingScopeCode.IsPresent)
            {
                sharingPolicy = new PackageSharingPolicy(this.PackageName, PackageSharingPolicyScope.Code);
            }
            else if (this.SharingScopeConfig.IsPresent)
            {
                sharingPolicy = new PackageSharingPolicy(this.PackageName, PackageSharingPolicyScope.Config);
            }
            else if (this.SharingScopeData.IsPresent)
            {
                sharingPolicy = new PackageSharingPolicy(this.PackageName, PackageSharingPolicyScope.Data);
            }
            else
            {
                sharingPolicy = new PackageSharingPolicy(this.PackageName, PackageSharingPolicyScope.None);
            }

            this.WriteObject(new PSObject(sharingPolicy));
        }
    }
}