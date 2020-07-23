// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    [Serializable]
    public class SecurityOctober2017 : SecurityBase
    {
        public new X509October2017 CertificateInformation
        {
            get;
            set;
        }

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

            if (this.CertificateInformation != null)
            {
                security.CertificateInformation = this.CertificateInformation.ToInternal();
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

            X509October2017 certificationInformation = new X509October2017();
            if (securityConfiguration.CertificateInformation != null)
            {
                certificationInformation.FromInternal(securityConfiguration.CertificateInformation);
            }

            this.CertificateInformation = certificationInformation;            
        }
    }
}