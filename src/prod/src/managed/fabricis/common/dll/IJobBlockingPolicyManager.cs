// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System.Threading.Tasks;

    /// <summary>
    /// An interface to adjust the job blocking policy of the infrastructure service to respond
    /// to job requests from the fabric controller via management client (MR).
    /// </summary>
    internal interface IJobBlockingPolicyManager
    {
        /// <summary>
        /// Gets the job blocking policy
        /// </summary>
        JobBlockingPolicy Policy { get; }

        /// <summary>
        /// Gets the job blocking policy
        /// </summary>
        Task<JobBlockingPolicy> GetPolicyAsync();

        /// <summary>
        /// Updates the current blocking policy with the supplied value.
        /// </summary>
        /// <param name="newBlockingPolicy">The new job blocking policy to be applied.</param>
        /// <returns>A task that represents the asynchronous operation.</returns>
        Task UpdatePolicyAsync(JobBlockingPolicy newBlockingPolicy);

        /// <summary>
        /// Clears the cache forcing a refresh of the cache the next time data is accessed.
        /// </summary>
        void ClearCache();
    }
}