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
    public class X509October2017 : X509May2017
    {
        public List<CertificateIssuerStore> ClusterCertificateIssuerStores { get; set; }

        public List<CertificateIssuerStore> ServerCertificateIssuerStores { get; set; }

        public List<CertificateIssuerStore> ClientCertificateIssuerStores { get; set; }

        internal override X509 ToInternal()
        {
            X509 result = base.ToInternal();
            result.ClusterCertificateIssuerStores = this.ClusterCertificateIssuerStores;
            result.ServerCertificateIssuerStores = this.ServerCertificateIssuerStores;
            result.ClientCertificateIssuerStores = this.ClientCertificateIssuerStores;
            return result;
        }

        internal override void FromInternal(X509 certificateInformation)
        {
            base.FromInternal(certificateInformation);
            this.ClusterCertificateIssuerStores = certificateInformation.ClusterCertificateIssuerStores;
            this.ServerCertificateIssuerStores = certificateInformation.ServerCertificateIssuerStores;
            this.ClientCertificateIssuerStores = certificateInformation.ClientCertificateIssuerStores;
        }
    }
}