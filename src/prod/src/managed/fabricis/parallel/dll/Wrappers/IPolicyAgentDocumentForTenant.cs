// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;

    /// <summary>
    /// Wrapper for <see cref="PolicyAgentDocumentForTenant"/> mainly for unit-testability.
    /// Wrapper interfaces are created for policy agent types that an IBonded object that 
    /// has to be deserialized.
    /// </summary>
    /// <remarks>Using List instead of IList since it is simpler for Json serialization of concrete types.</remarks>
    internal interface IPolicyAgentDocumentForTenant
    {
        /// <summary>
        /// The incarnation of the document.
        /// </summary>
        string Incarnation { get; }

        /// <summary>
        /// The incarnation of the jobs structure of the document. If a job is newly added,
        /// or if one is approved, alerted etc. then this incarnation number changes. The
        /// <see cref="Incarnation"/> also changes if this changes.
        /// </summary>
        int JobDocumentIncarnation { get; }

        /// <summary>
        /// The list of tenant jobs.
        /// </summary>
        List<ITenantJob> Jobs { get; }

        /// <summary>
        /// This is populated for platform update jobs.
        /// This indicates that Azure FC is waiting (blocked) on certain sets of jobs and 
        /// at least one job from each set have to be approved for FC to be unblocked.
        /// </summary>
        /// <example>
        /// If FC sends out 5 platform update jobs: A, B, C, D, E
        /// Then this may be populated as { A, B, C }, { D, E}
        /// So, we need to approve at least job from each set. Say, A and E. 
        /// Then the platform update can progress.
        /// </example>
        List<List<string>> JobSetsRequiringApproval { get; }
         
        /// <summary>
        /// The incarnation of the role instance info of the document. Each time the
        /// role instance info is updated by FC, this incarnation number changes. The
        /// <see cref="Incarnation"/> also changes if this changes.
        /// </summary>
        ulong RoleInstanceHealthInfoIncarnation { get; }

        /// <summary>
        /// Timestamp associated with the role instance health info.
        /// </summary>
        string RoleInstanceHealthInfoTimestamp { get; }

        /// <summary>
        /// The role instance health information of all role instances of this tenant.
        /// </summary>
        List<RoleInstanceHealthInfo> RoleInstanceHealthInfos { get; }
    }
}