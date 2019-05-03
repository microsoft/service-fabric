// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    /// <summary>
    /// Helper class just for ease of live-site debugging
    /// </summary>
    internal sealed class CoordinatorDiagnosticInfo
    {
        public string CoordinatorStartTime { get; set; }

        public string AzureTenantId { get; set; }

        public string AssemblyFileVersion { get; set; }
    }
}