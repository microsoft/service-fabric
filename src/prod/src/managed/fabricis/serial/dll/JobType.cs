// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// Represents a 1:1 mapping with the concrete classes of type ManagementJob in the Windows Azure management protocol (MR) SDK.
    /// This exists with the intent of mocking out the MR SDK via <see cref="IManagementClient"/>.
    /// </summary>
    internal enum JobType
    {
        Unknown,

        /// <summary>
        /// Platform initiated update job.
        /// </summary>
        /// <example>Root HE (hosting environment)</example>
        PlatformUpdateJob,

        /// <summary>
        /// Platform initiated repair job.
        /// </summary>
        /// <example>Host reboot</example>
        PlatformRepairJob,

        /// <summary>
        /// Platform initiated maintenance job.
        /// </summary>
        /// <example>Service healing</example>
        PlatformMaintenanceJob,

        /// <summary>
        /// Deployment update job.
        /// </summary>
        /// <example>Cspkg/cscfg update or guest OS update</example>
        DeploymentUpdateJob,

        /// <summary>
        /// Deployment maintenance job.
        /// </summary>
        /// <example>Tenant initiated repair or tenant initiated service healing</example>
        DeploymentMaintenanceJob,
    }
}