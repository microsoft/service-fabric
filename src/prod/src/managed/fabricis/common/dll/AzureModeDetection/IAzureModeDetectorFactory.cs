// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Represents an object that can create an <see cref="IAzureModeDetector"/>
    /// </summary>
    internal interface IAzureModeDetectorFactory
    {
        /// <summary>
        /// Creates an instance of <see cref="IAzureModeDetector"/>
        /// </summary>
        IAzureModeDetector CreateAzureModeDetector();
    }
}