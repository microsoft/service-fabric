// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer.FlatNetwork.CNSApi
{
    /// <summary>
    /// Network create operation for CNS
    /// </summary>
    class CreateNetworkRequest
    {
        /// <summary>
        /// Unique identifier of the network
        /// </summary>
        public string NetworkName { get; set; }
    }
}