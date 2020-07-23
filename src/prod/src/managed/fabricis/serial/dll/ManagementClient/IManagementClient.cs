// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;

    /// <summary>
    /// Represents a client for interacting with the Windows Azure management protocol.
    /// </summary>
    /// <remarks>Abstraction for testability</remarks>
    internal interface IManagementClient
    {
        /// <summary>
        /// Fetches the current job notification from the management protocol.
        /// </summary>
        /// <returns>An object with details about the active job, or null if there is no active job.</returns>
        IManagementNotificationContext GetCurrentNotification();

        IList<RoleInstance> GetRoleInstances();

        /// <summary>
        /// Requests a maintenance action to Azure through the management protocol (MR).
        /// </summary>
        /// <param name="roleInstanceName">Name of the role instance.</param>
        /// <param name="action">The action.</param>
        /// <param name="reason">The reason.</param>
        /// <param name="contextId">An optional context string that will be attached to the MR job resulting from this request.</param>
        void RequestMaintenance(string roleInstanceName, InfrastructureServiceMaintenanceAction action, string reason, string contextId);

        IList<IManagementJobWrapper> GetAllJobs();

        ulong? GetJobListIncarnation();

        ulong? GetServiceHealthIncarnation();
    }
}
