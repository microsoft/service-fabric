// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Management.Automation;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;

    [Cmdlet(VerbsCommon.Remove, "ServiceFabricComposeDeployment", SupportsShouldProcess = true)]
    public sealed class RemoveComposeDeployment : ComposeDeploymentCmdletBase
    {
        [Parameter(Mandatory = true, ValueFromPipelineByPropertyName = true, Position = 0)]
        public string DeploymentName
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
            if (this.ShouldProcess(this.DeploymentName))
            {
                if (this.Force || this.ShouldContinue(string.Empty, string.Empty))
                {
                    this.DeleteComposeDeployment(new DeleteComposeDeploymentDescription(this.DeploymentName));
                }
            }
        }
    }
}