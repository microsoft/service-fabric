// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    [Serializable]
    public class X509
    {
        public CertificateDescription ClusterCertificate { get; set; }

        public CertificateDescription ServerCertificate { get; set; }

        public CertificateDescription ReverseProxyCertificate { get; set; }
         
        public List<ClientCertificateThumbprint> ClientCertificateThumbprints { get; set; }

        public List<ClientCertificateCommonName> ClientCertificateCommonNames { get; set; }

        public ServerCertificateCommonNames ServerCertificateCommonNames { get; set; }

        public ServerCertificateCommonNames ReverseProxyCertificateCommonNames { get; set; }

        public ServerCertificateCommonNames ClusterCertificateCommonNames { get; set; }

        public List<CertificateIssuerStore> ClusterCertificateIssuerStores { get; set; }

        public List<CertificateIssuerStore> ServerCertificateIssuerStores { get; set; }

        public List<CertificateIssuerStore> ClientCertificateIssuerStores { get; set; }
    }
}