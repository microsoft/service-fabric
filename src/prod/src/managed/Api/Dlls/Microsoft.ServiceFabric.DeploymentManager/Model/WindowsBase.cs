// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.DeploymentManager.Model
{
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.ClusterManagementCommon;
    using Newtonsoft.Json;

    [JsonObject(IsReference = true)]
    public class WindowsBase
    {
        public string ClusterSPN { get; set; }

        public string ClusterIdentity { get; set; }

        public List<ClientIdentity> ClientIdentities { get; set; }

        public string FabricHostSpn { get; set; }

        // Ensure all overriden implementations of ToInternal cover all newly added properties.
        internal virtual Windows ToInternal()
        {
            return new Windows()
            {
                ClusterSPN = this.ClusterSPN,
                ClusterIdentity = this.ClusterIdentity,
                ClientIdentities = this.ClientIdentities,
                FabricHostSpn = this.FabricHostSpn
            };
        }

        // Ensure all overriden implementations of FromInternal cover all newly added properties.
        internal virtual void FromInternal(Windows windowsIdentities)
        {
            this.ClientIdentities = windowsIdentities.ClientIdentities;
            this.ClusterIdentity = windowsIdentities.ClusterIdentity;
            this.ClusterSPN = windowsIdentities.ClusterSPN;
            this.FabricHostSpn = windowsIdentities.FabricHostSpn;
        }
    }
}