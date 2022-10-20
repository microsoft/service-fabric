// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Threading.Tasks;

    /// <summary>
    /// Represents an object that can detect the Azure MR mode
    /// </summary>
    internal interface IAzureModeDetector
    {
        /// <summary>
        /// Determines the current Azure MR mode
        /// </summary>
        Task<AzureMode> DetectModeAsync();
    }
}