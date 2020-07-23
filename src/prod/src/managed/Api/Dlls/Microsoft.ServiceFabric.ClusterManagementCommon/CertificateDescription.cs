// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.Collections.Generic;
    using System.ComponentModel.DataAnnotations;
    using System.Fabric.Management.ServiceModel;
    using System.Security.Cryptography.X509Certificates;

    [Serializable]
    public class CertificateDescription
    {
        public CertificateDescription()
        {
            this.X509StoreName = StoreName.My;
        }

        [Required(AllowEmptyStrings = false)]
        public string Thumbprint { get; set; }

        public string ThumbprintSecondary { get; set; }

        public StoreName X509StoreName { get; set; }

        public FabricCertificateType ToFabricCertificateType()
        {
            return new FabricCertificateType()
            {
                Name = "Certificate",
                X509FindType = FabricCertificateTypeX509FindType.FindByThumbprint,
                X509FindValue = this.Thumbprint,
                X509FindValueSecondary = this.ThumbprintSecondary,
                X509StoreName = this.X509StoreName.ToString()
            };
        }

        public List<string> ToThumbprintList()
        {
            if (string.IsNullOrWhiteSpace(this.ThumbprintSecondary))
            {
                return new List<string>() { this.Thumbprint };
            }
            else
            {
                return new List<string>() { this.Thumbprint, this.ThumbprintSecondary };
            }
        }
    }
}