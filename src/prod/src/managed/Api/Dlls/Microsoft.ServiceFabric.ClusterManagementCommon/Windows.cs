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
    public class Windows
    {
        public string ClusterSPN { get; set; }

        public string ClusterIdentity { get; set; }

        public string ClustergMSAIdentity { get; set; }

        public List<ClientIdentity> ClientIdentities { get; set; }

        public string FabricHostSpn { get; set; }
    }
}