// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    /// <summary>
    /// The job blocking policy. This allows an operator (human) of the Service Fabric cluster to
    /// adjust what kind of jobs he/she wants to process from Fabric Controller (FC). 
    /// </summary>
    public enum JobBlockingPolicy
    {
        /// <summary>
        /// Invalid state. This is the initial state till the initial blocking policy can be retrieved from
        /// the persisted store.
        /// </summary>
        Invalid,

        /// <summary>
        /// Don't block any external job.
        /// </summary>
        BlockNone,

        /// <summary>
        /// New update jobs are blocked. The current update job, if any is allowed to complete.
        /// </summary>
        BlockNewUpdateJob,

        /// <summary>
        /// New maintenance jobs are blocked. The current maintenance job, if any is allowed to complete.
        /// </summary>
        BlockNewMaintenanceJob,

        /// <summary>
        /// New Jobs are blocked. The current job, if any is allowed to complete.
        /// </summary>
        BlockAllNewJobs,

        /// <summary>
        /// All new jobs are blocked and all JobState transitions are stopped - current job if any doesn't make progress.
        /// </summary>
        BlockAllJobs,

        /// <summary>
        /// New update jobs are blocked if they will result in any node deactivations.
        /// </summary>
        BlockNewImpactfulUpdateJobs,

        /// <summary>
        /// New tenant update jobs are blocked if they will result in any node deactivations.
        /// </summary>
        BlockNewImpactfulTenantUpdateJobs,

        /// <summary>
        /// New Platform update jobs are blocked if they will result in any node deactivations.
        /// </summary>
        BlockNewImpactfulPlatformUpdateJobs,

    }
}