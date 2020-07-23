// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Collections.Generic;

    /// <summary>
    /// Stores a snapshot about the impact of a job. This is used for MR learning mode scenario.
    /// </summary>
    /// <remarks><seealso cref="IJobImpactManager"/></remarks>
    internal interface IJobImpactData
    {
        /// <summary>
        /// The timestamp when the snapshot was captured.
        /// </summary>
        DateTimeOffset Timestamp { get; set; }

        /// <summary>
        /// The Azure infrastructure job Id
        /// </summary>
        string JobId { get; set; }

        /// <summary>
        /// The Update Domain of the Azure infrastructure job.
        /// </summary>
        int UD { get; set; }

        /// <summary>
        /// The job type of the Azure infrastructure job.
        /// </summary>
        JobType JobType { get; set; }

        /// <summary>
        /// The assessed impact in this snapshot.
        /// </summary>
        JobImpactTranslationMode AssessedImpact { get; set; }

        /// <summary>
        /// The nodes that were possibly impacted by the Azure infrastructure job
        /// when the snapshot was captured.
        /// </summary>
        IDictionary<string, INode> AssessedNodes { get; set; }
    }
}