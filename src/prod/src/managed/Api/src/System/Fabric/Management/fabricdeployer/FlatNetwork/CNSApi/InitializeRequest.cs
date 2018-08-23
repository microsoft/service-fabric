// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer.FlatNetwork.CNSApi
{
    /// <summary>
    /// Initialize network environment request
    /// </summary>
    class InitializeRequest
    {
        /// <summary>
        /// Network location, should be 'Azure' or 'StandAlone'
        /// </summary>
        public string Location { get; set; }
        /// <summary>
        /// Workload type on nodes, should be 'Underlay' or 'Overlay'
        /// </summary>
        public string NetworkType { get; set; }
    }
}