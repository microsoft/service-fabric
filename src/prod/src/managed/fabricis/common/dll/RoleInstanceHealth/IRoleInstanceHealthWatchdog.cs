// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;

    /// <summary>
    /// Interface for a health watchdog that reports role instance health to the Service Fabric health store.
    /// </summary>
    internal interface IRoleInstanceHealthWatchdog
    {
        /// <summary>
        /// Reports health of role instances to the Service Fabric health store.
        /// </summary>
        /// <param name="roleInstances">
        /// The role instances to report health on.
        /// </param>
        /// <param name="validityPeriod">
        /// Serves as an indication to the watchdog as to how long the reported health is valid for.
        /// </param>
        /// <remarks>Consider ReportHealthAsync in future.</remarks>
        void ReportHealth(IList<RoleInstance> roleInstances, TimeSpan validityPeriod);
    }
}