// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Collections.Generic;
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    /// <summary>
    /// A simple helper class for job-type related functionality.
    /// </summary>
    internal static class JobTypeHelper
    {
        /// <summary>
        /// A 1:1 mapping layer with the intent of mocking out the MR SDK.
        /// </summary>
        private static readonly Dictionary<Type, JobType> jobTypeMap = new Dictionary<Type, JobType>()
        {
            { typeof(PlatformMaintenanceJob), JobType.PlatformMaintenanceJob },
            { typeof(PlatformRepairJob), JobType.PlatformRepairJob },
            { typeof(PlatformUpdateJob), JobType.PlatformUpdateJob },
            { typeof(DeploymentMaintenanceJob), JobType.DeploymentMaintenanceJob },
            { typeof(DeploymentUpdateJob), JobType.DeploymentUpdateJob },
        };

        /// <summary>
        /// Gets the type of the job.
        /// </summary>
        /// <param name="job">The management job.</param>
        /// <returns>A <see cref="JobType"/> enumeration value.</returns>
        /// <exception cref="System.ArgumentNullException">job</exception>
        public static JobType GetJobType(this ManagementJob job)
        {
            if (job == null)
                throw new ArgumentNullException("job");

            JobType jobType;

            if (!jobTypeMap.TryGetValue(job.GetType(), out jobType))
            {
                jobType = JobType.Unknown;
            }

            return jobType;
        }
    }
}