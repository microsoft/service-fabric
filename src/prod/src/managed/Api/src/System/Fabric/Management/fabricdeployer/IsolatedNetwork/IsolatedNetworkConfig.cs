// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    public class IsolatedNetworkConfig
    {
        public string Id { get; set; }

        public string Name { get; set; }

        public string Type { get; set; }
        
        public string ManagementIP { get; set; }

        public string NetworkAdapterName { get; set; }

        public IsolatedNetworkSubnetConfig[] Subnets { get; set; }
    }
}