// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Management.ServiceModel;
    using System.Linq;
    using System.Security.Cryptography.X509Certificates;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    [Serializable]
    public class ServerCertificateCommonNames
    {
        public ServerCertificateCommonNames()
        {
            this.X509StoreName = StoreName.My;
        }

        public List<CertificateCommonNameBase> CommonNames { get; set; }

        public StoreName X509StoreName { get; set; }

        public FabricCertificateType ToFabricCertificateType()
        {
            return new FabricCertificateType()
            {
                Name = "Certificate",
                X509FindType = FabricCertificateTypeX509FindType.FindBySubjectName,
                X509FindValue = this.CommonNames[0].CertificateCommonName,
                X509FindValueSecondary = this.CommonNames.Count > 1 ? this.CommonNames[1].CertificateCommonName : null,
                X509StoreName = this.X509StoreName.ToString()
            };
        }

        public bool Any()
        {
            return this.CommonNames != null && this.CommonNames.Any();
        }
    }
}