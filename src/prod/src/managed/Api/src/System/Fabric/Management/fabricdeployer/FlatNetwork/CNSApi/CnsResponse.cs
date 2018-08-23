// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Management.FabricDeployer.FlatNetwork.CNSApi
{
    /// <summary>
    /// Response for CNS operations
    /// </summary>
    class CnsResponse
    {
        /// <summary>
        /// Response message details
        /// </summary>
        public string Message { get; set; }
        /// <summary>
        /// Response return code
        /// </summary>
        public int ReturnCode { get; set; }
    }
}