// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricServiceGroup", SupportsShouldProcess = true)]
    public sealed class RemoveServiceGroup : ServiceGroupCmdletBase
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

        protected override void ProcessRecord()
        {
            if (this.ShouldProcess(this.ServiceName.OriginalString))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.RemoveServiceGroup(this.ServiceName);
                }
            }
        }
    }
}