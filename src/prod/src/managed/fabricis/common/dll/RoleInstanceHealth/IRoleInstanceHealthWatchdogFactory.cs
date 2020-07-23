// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Factory interface to create a watchdog.
    /// </summary>
    internal interface IRoleInstanceHealthWatchdogFactory
    {
        /// <summary>
        /// Creates an instance of a health watchdog.
        /// </summary>
        /// <returns>A health watchdog.</returns>
        IRoleInstanceHealthWatchdog Create(string configKeyPrefix);
    }
}