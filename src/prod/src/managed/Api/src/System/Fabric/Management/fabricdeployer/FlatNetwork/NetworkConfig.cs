// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.FabricDeployer
{
    public class NetworkConfig
    {
        public string Name { get; set; }
        public string Id { get; set; }
        public string Driver { get; set; }
        public bool CheckDuplicate { get; set; }
        public IpamConfig Ipam { get; set; }
        public dynamic Options { get; set; }
    }
}