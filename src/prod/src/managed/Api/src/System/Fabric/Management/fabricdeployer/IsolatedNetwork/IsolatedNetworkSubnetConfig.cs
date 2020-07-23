// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    public class IsolatedNetworkSubnetConfig
    {
        public string AddressPrefix { get; set; }

        public string GatewayAddress { get; set; }
        
        public IsolateNetworkPolicyConfig[] Policies { get; set; }
    }
}