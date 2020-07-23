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
    public class X509May2017 : X509GA
    {
        public ServerCertificateCommonNames ServerCertificateCommonNames { get; set; }

        public ServerCertificateCommonNames ReverseProxyCertificateCommonNames { get; set; }

        public ServerCertificateCommonNames ClusterCertificateCommonNames { get; set; }

        internal override X509 ToInternal()
        {
            X509 result = base.ToInternal();
            result.ClusterCertificateCommonNames = this.ClusterCertificateCommonNames;
            result.ServerCertificateCommonNames = this.ServerCertificateCommonNames;
            result.ReverseProxyCertificateCommonNames = this.ReverseProxyCertificateCommonNames;

            return result;
        }

        internal override void FromInternal(X509 certificateInformation)
        {
            base.FromInternal(certificateInformation);
            this.ClusterCertificateCommonNames = certificateInformation.ClusterCertificateCommonNames;
            this.ServerCertificateCommonNames = certificateInformation.ServerCertificateCommonNames;
            this.ReverseProxyCertificateCommonNames = certificateInformation.ReverseProxyCertificateCommonNames;
        }
    }
}