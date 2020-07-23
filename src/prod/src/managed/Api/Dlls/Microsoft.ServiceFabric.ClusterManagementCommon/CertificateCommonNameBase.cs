// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    using System.ComponentModel.DataAnnotations;

    [Serializable]
    public class CertificateCommonNameBase
    {
        [Required(AllowEmptyStrings = false)]
        public string CertificateCommonName { get; set; }

        /// <summary>
        /// comma separated thumbprint list
        /// ideally it should be named XXXs, but for backward compatibility we have to live without s.
        /// </summary>
        [Required(AllowEmptyStrings = false)]
        public string CertificateIssuerThumbprint { get; set; }
    }
}