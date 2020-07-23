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

    [Cmdlet(VerbsCommon.New, "ServiceFabricComposeDeployment")]
    public sealed class NewComposeDeployment : ComposeDeploymentCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public string DeploymentName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public string Compose
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string RegistryUserName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string RegistryPassword
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public SwitchParameter PasswordEncrypted
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            ComposeDeploymentDescription description =
                new ComposeDeploymentDescription(this.DeploymentName, this.GetAbsolutePath(this.Compose))
                {
                    ContainerRegistryUserName = this.RegistryUserName,
                    ContainerRegistryPassword = this.RegistryPassword,
                    IsRegistryPasswordEncrypted = this.PasswordEncrypted
                };

            this.CreateComposeDeployment(description);
        }
    }
}