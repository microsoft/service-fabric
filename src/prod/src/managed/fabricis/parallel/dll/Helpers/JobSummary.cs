// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;

    /// <summary>
    /// Class just for display purposes of a job. E.g. in a powershell window
    /// </summary>
    internal sealed class JobSummary
    {
        public JobSummary()
        {
            RoleInstancesToBeImpacted = new List<string>();
            CurrentlyImpactedRoleInstances = new List<ImpactedRoleInstance>();
        }

        public Guid Id { get; set; }

        /// <summary>
        /// <see cref="ImpactActionEnum"/>
        /// </summary>
        public string ImpactAction { get; set; }

        /// <summary>
        /// <see cref="JobStatusEnum"/>
        /// </summary>
        public string JobStatus { get; set; }

        /// <summary>
        /// <see cref="ImpactStepEnum"/>
        /// </summary>
        public string ImpactStep { get; set; }

        /// <summary>
        /// <see cref="AcknowledgementStatusEnum"/>
        /// </summary>
        public string AcknowledgementStatus { get; set; }

        public string DeadlineForResponse { get; set; }

        /// <summary>
        /// <see cref="ImpactActionStatus"/>
        /// </summary>
        public string ActionStatus { get; set; }

        public uint? CurrentUD { get; set; }

        public string ContextId { get; set; }

        /// <summary>
        /// Role instances to be impacted across all UDs for the job.
        /// </summary>
        public IList<string> RoleInstancesToBeImpacted { get; set; }

        /// <summary>
        /// Role instances impacted for this particular job step (confined to a single UD)
        /// </summary>        
        public IList<ImpactedRoleInstance> CurrentlyImpactedRoleInstances { get; set; }

        public bool IsActive;

        public string PendingActions { get; set; }

        public string AllowedActions { get; set; }

        public int? ImpactedNodeCount;

        public IList<RepairTaskSummary> RepairTasks { get; set;}

        public string CorrelationId { get; set; }
    }
}