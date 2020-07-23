// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricService", SupportsShouldProcess = true)]
    public sealed class RemoveService : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public Uri ServiceName
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

        [Parameter(Mandatory = false)]
        public SwitchParameter ForceRemove
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(this.ServiceName.OriginalString))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.RemoveService(
                        new DeleteServiceDescription(this.ServiceName)
                    {
                        ForceDelete = this.ForceRemove
                    });
                }
            }
        }
    }
}