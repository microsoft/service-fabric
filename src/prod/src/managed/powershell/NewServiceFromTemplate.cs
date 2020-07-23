// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Powershell
{
    using System;
    using System.Fabric.Description;
    using System.Management.Automation;

    [Cmdlet(VerbsCommon.New, "ServiceFabricServiceFromTemplate")]
    public sealed class NewServiceFromTemplate : ServiceCmdletBase
    {
        [Parameter(Mandatory = true, Position = 0)]
        public Uri ApplicationName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 1)]
        public Uri ServiceName
        {
            get;
            set;
        }

        [Parameter(Mandatory = true, Position = 2)]
        public string ServiceTypeName
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
        public ServicePackageActivationMode? ServicePackageActivationMode
        {
            get;
            set;
        }

        [Parameter(Mandatory = false)]
        public string ServiceDnsName
        {
            get;
            set;
        }

        protected override void ProcessRecord()
        {
            var activationMode = System.Fabric.Description.ServicePackageActivationMode.SharedProcess;
            if (this.ServicePackageActivationMode.HasValue)
            {
                activationMode = this.ServicePackageActivationMode.Value;
            }

            this.CreateServiceFromTemplate(
                this.ApplicationName, 
                this.ServiceName,
                this.ServiceDnsName,
                this.ServiceTypeName,
                activationMode);
        }
    }
}