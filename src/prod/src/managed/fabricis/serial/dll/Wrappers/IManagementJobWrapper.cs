// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Microsoft.WindowsAzure.ServiceRuntime.Management;

    /// <summary>
    /// Represents (or wraps) a job that provides an abstraction over the MR SDK for testability purposes.
    /// </summary>
    internal interface IManagementJobWrapper
    {
        /// <summary>
        /// Gets the job Id.
        /// </summary>
        string Id { get; }

        /// <summary>
        /// Gets the status of the job.
        /// </summary>
        ManagementJobStatus Status { get; }

        /// <summary>
        /// Gets the type of the job as an enumeration.
        /// </summary>
        JobType Type { get; }

        /// <summary>
        /// Detailed status about the job.
        /// </summary>
        ManagementJobDetailedStatus DetailedStatus { get; }

        /// <summary>
        /// Context Id of the job.
        /// </summary>
        string ContextId { get; }

        /// <summary>
        /// Requests the resumption of a job.
        /// Please check the concrete implementation for constraints on job types that can be resumed etc.        
        /// </summary>
        void RequestResume();

        /// <summary>
        /// Requests the suspension of a job.
        /// Please check the concrete implementation for constraints on job types that can be suspended etc.        
        /// </summary>        
        void RequestSuspend();
    }
}