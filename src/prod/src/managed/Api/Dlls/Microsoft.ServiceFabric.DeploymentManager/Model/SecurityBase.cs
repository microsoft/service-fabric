// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;
    
    /// <summary>
    /// The input portion from CSM: i.e. what can be specified in a put request
    /// for a cluster resource.
    /// </summary>
    [JsonObject(IsReference = true)]
    public abstract class SecurityBase
    {
        public CredentialType ClusterCredentialType { get; set; }

        public CredentialType ServerCredentialType { get; set; }

        public X509GA CertificateInformation { get; set; }

        public AzureActiveDirectory AzureActiveDirectory { get; set; }

        // Ensure all overriden implementations of ToInternal cover all newly added properties.
        internal virtual Security ToInternal()
        {
            Security result = new Security()
            {
                AzureActiveDirectory = this.AzureActiveDirectory,
                ClusterCredentialType = this.ClusterCredentialType,
                ServerCredentialType = this.ServerCredentialType,
            };

            if (this.CertificateInformation != null)
            {
                result.CertificateInformation = this.CertificateInformation.ToInternal();
            }

            return result;
        }

        // Ensure all overriden implementations of FromInternal cover all newly added properties.
        internal virtual void FromInternal(Security securityConfiguration)
        {
            this.AzureActiveDirectory = securityConfiguration.AzureActiveDirectory;
            this.ClusterCredentialType = securityConfiguration.ClusterCredentialType;
            this.ServerCredentialType = securityConfiguration.ServerCredentialType;

            if (securityConfiguration.CertificateInformation != null)
            {
                this.CertificateInformation = new X509GA();
                this.CertificateInformation.FromInternal(securityConfiguration.CertificateInformation);
            }
        }
    }
}