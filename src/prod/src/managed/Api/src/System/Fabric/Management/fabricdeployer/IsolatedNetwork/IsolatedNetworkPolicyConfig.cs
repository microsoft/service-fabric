// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    public class IsolateNetworkPolicyConfig
    {
        public string Type { get; set; }
        
        public int VSID { get; set; }
    }
}