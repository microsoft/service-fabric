// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Collections;
    using System.Management.Automation;

    [Cmdlet(VerbsDiagnostic.Test, "ServiceFabricApplicationPackage", DefaultParameterSetName = SFApplicationPackageParameterSet)]
    public sealed class TestApplicationPackage : ApplicationCmdletBase
    {
        private const string SFApplicationPackageParameterSet = "SFApplicationPackage";
        private const string ComposeParameterSet = "Compose";

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = SFApplicationPackageParameterSet)]
        public string ApplicationPackagePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = SFApplicationPackageParameterSet)]
        public Hashtable ApplicationParameter
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = ComposeParameterSet)]
        public string ComposeFilePath
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = ComposeParameterSet)]
        public string RegistryUserName
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = ComposeParameterSet)]
        public string RegistryPassword
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = ComposeParameterSet)]
        public SwitchParameter PasswordEncrypted
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ImageStoreConnectionString
        {
            get;
            set;
        }

        private new int? TimeoutSec
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            if (string.Equals(this.ParameterSetName, SFApplicationPackageParameterSet))
            {
                this.TestSFApplicationPackage(
                    this.ApplicationPackagePath, 
                    this.ApplicationParameter, 
                    this.ImageStoreConnectionString);
            }
            else if (string.Equals(this.ParameterSetName, ComposeParameterSet))
            {
                this.TestComposeDeploymentPackage(
                    this.ComposeFilePath,
                    this.RegistryUserName,
                    this.RegistryPassword,
                    this.PasswordEncrypted,
                    this.ImageStoreConnectionString);
            }
            else
            {
                // Unsupported parameter set
                throw new NotSupportedException();
            }
        }
    }
}