// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    public class SecurityJanuary2017 : SecurityBase
    {
        public WindowsJanuary2017 WindowsIdentities 
        { 
            get; 
            set; 
        }

        internal override Security ToInternal()
        {
            var security = base.ToInternal();
            if (this.WindowsIdentities != null)
            {
                security.WindowsIdentities = this.WindowsIdentities.ToInternal();
            }

            return security;
        }

        internal override void FromInternal(Security securityConfiguration)
        {
            base.FromInternal(securityConfiguration);
            var windowsIdentities = new WindowsJanuary2017();
            if (securityConfiguration.WindowsIdentities != null)
            {
                windowsIdentities.FromInternal(securityConfiguration.WindowsIdentities);
            }

            this.WindowsIdentities = windowsIdentities;
        }
    }
}