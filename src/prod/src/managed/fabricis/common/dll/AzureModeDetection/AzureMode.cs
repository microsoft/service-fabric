// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Describes the Azure MR mode that is configured for the tenant
    /// </summary>
    internal enum AzureMode
    {
        /// <summary>
        /// Mode could not be determined
        /// </summary>
        Unknown,

        /// <summary>
        /// Serial job mode (Tenant.PolicyAgent.JobParallelismEnabled=false)
        /// </summary>
        Serial,

        /// <summary>
        /// Parallel job mode (Tenant.PolicyAgent.JobParallelismEnabled=true)
        /// </summary>
        Parallel,
    }
}