// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsLifecycle.Register, "ServiceFabricApplicationType", DefaultParameterSetName = "UseStoreRelativePath")]
    public sealed class RegisterApplicationType : ApplicationCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "UseStoreRelativePath")]
        public string ApplicationPathInImageStore
        {
            get;
            set;
        }

        [Parameter(Mandatory = false, ParameterSetName = "UseStoreRelativePath")]
        public ApplicationPackageCleanupPolicy ApplicationPackageCleanupPolicy
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 0, ParameterSetName = "UseExternalSfpkg")]
        public Uri ApplicationPackageDownloadUri
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "UseExternalSfpkg")]
        public string ApplicationTypeName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, ParameterSetName = "UseExternalSfpkg")]
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

        protected override void ProcessRecord()
        {
            ProvisionApplicationTypeDescriptionBase description = null;
            if (this.ParameterSetName == "UseExternalSfpkg")
            {
                description = new ExternalStoreProvisionApplicationTypeDescription(
                    this.ApplicationPackageDownloadUri,
                    this.ApplicationTypeName,
                    this.ApplicationTypeVersion);
            }
            else
            {
                description = new ProvisionApplicationTypeDescription(this.ApplicationPathInImageStore) { ApplicationPackageCleanupPolicy = this.ApplicationPackageCleanupPolicy };
            }

            description.Async = this.Async;

            this.RegisterApplicationType(description);
        }
    }
}