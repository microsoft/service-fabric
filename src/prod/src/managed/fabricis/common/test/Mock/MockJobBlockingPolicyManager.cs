// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Test
{
    using Threading.Tasks;

    /// <summary>
    /// A class mocking <see cref="JobBlockingPolicyManager"/> for unit-testability.
    /// </summary>
    internal class MockJobBlockingPolicyManager : IJobBlockingPolicyManager
    {
        public JobBlockingPolicy Policy { get; private set; }

        public MockJobBlockingPolicyManager()
        {
            Policy = JobBlockingPolicy.BlockNone;
        }

        public Task UpdatePolicyAsync(JobBlockingPolicy newBlockingPolicy)
        {
            Policy = newBlockingPolicy;
            return Task.FromResult(0);
        }

        public void ClearCache()
        {
        }

        public Task<JobBlockingPolicy> GetPolicyAsync()
        {
            return Task.FromResult(Policy);
        }

        public override string ToString()
        {
            return Policy.ToString();
        }
    }
}