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
    public class CertificateIssuerStore
    {
        public string IssuerCommonName { get; set; }

        public string X509StoreNames { get; set; }
    }
}