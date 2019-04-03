// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System;
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    [Serializable]
    public class X509GA
    {
        public CertificateDescription ClusterCertificate { get; set; }

        public CertificateDescription ServerCertificate { get; set; }

        public CertificateDescription ReverseProxyCertificate { get; set; }

        public List<ClientCertificateThumbprint> ClientCertificateThumbprints { get; set; }

        public List<ClientCertificateCommonName> ClientCertificateCommonNames { get; set; }

        internal virtual X509 ToInternal()
        {
            return new X509
            {
                ClusterCertificate = this.ClusterCertificate,
                ServerCertificate = this.ServerCertificate,
                ReverseProxyCertificate = this.ReverseProxyCertificate,
                ClientCertificateThumbprints = this.ClientCertificateThumbprints,
                ClientCertificateCommonNames = this.ClientCertificateCommonNames
            };
        }
        
        internal virtual void FromInternal(X509 certificateInformation)
        {
            if (certificateInformation != null)
            {
                this.ClusterCertificate = certificateInformation.ClusterCertificate;
                this.ServerCertificate = certificateInformation.ServerCertificate;
                this.ReverseProxyCertificate = certificateInformation.ReverseProxyCertificate;
                this.ClientCertificateThumbprints = certificateInformation.ClientCertificateThumbprints;
                this.ClientCertificateCommonNames = certificateInformation.ClientCertificateCommonNames;
            }
        }
    }
}