// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Unregister, "ServiceFabricApplicationType", SupportsShouldProcess = true)]
    public sealed class UnregisterApplicationType : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 1)]
        public string ApplicationTypeVersion
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Async
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter Force
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(this.ApplicationTypeName))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.UnregisterApplicationType(new UnprovisionApplicationTypeDescription(
                        this.ApplicationTypeName,
                        this.ApplicationTypeVersion)
                    {
                        Async = this.Async
                    });
                }
            }
        }
    }
}