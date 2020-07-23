// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Health;

    /// <summary>
    /// Keeps track of all the health policies that are to be applied on a role instance. Runs them sequentially
    /// and returns the final health policy value that the watchdog reports to the health system.
    /// </summary>
    internal interface IRoleInstanceHealthPolicyManager
    {
        /// <summary>
        /// Executes all the health policies on the specified role instance. 
        /// </summary>
        /// <param name="roleInstance">The role instance.</param>
        /// <param name="healthState">The input health state of the role instance.</param>
        /// <returns>The final health state of the role instance after applying all health policies.</returns>
        HealthState Execute(RoleInstance roleInstance, HealthState healthState);
    }
}