// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.Fabric.InfrastructureWrapper
{
    using System;
    using System.ComponentModel;    
    using System.Configuration.Install;
    using System.ServiceProcess;

    [RunInstaller(true)]
    public class WindowsFabricAzureWrapperServiceInstaller : Installer
    {
        private ServiceInstaller serviceInstaller;
        private ServiceProcessInstaller serviceProcessInstaller;

        public WindowsFabricAzureWrapperServiceInstaller()
        {
            this.serviceInstaller = new ServiceInstaller
            {
                ServiceName = WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperServiceName,
                DisplayName = WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperDisplayName,
                Description = WindowsFabricAzureWrapperServiceCommon.WindowsFabricAzureWrapperDescription,
                StartType = ServiceStartMode.Manual,
            };

            this.serviceProcessInstaller = new ServiceProcessInstaller()
            {
                Account = ServiceAccount.LocalSystem,
            };

            Installers.Add(this.serviceInstaller);

            Installers.Add(this.serviceProcessInstaller);
        }
    }
}