// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ClusterManagementCommon
{
    using System;
    
    [Serializable]
    public class ClientCertificateCommonName : CertificateCommonNameBase
    {
        public ClientCertificateCommonName()
        {
            this.IsAdmin = false;
        }

        public bool IsAdmin { get; set; }
    }
}