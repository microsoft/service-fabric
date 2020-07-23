// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;

    internal sealed class DefaultActionPolicyFactory : IActionPolicyFactory
    {
        private readonly CoordinatorEnvironment environment;
        private readonly IJobBlockingPolicyManager jobBlockingPolicyManager;
        private readonly IAllowActionMap allowActionMap;

        public DefaultActionPolicyFactory(
            CoordinatorEnvironment environment, 
            IJobBlockingPolicyManager jobBlockingPolicyManager,
            IAllowActionMap allowActionMap)
        {
            this.environment = environment.Validate("environment");
            this.jobBlockingPolicyManager = jobBlockingPolicyManager.Validate("jobBlockingPolicyManager");
            this.allowActionMap = allowActionMap.Validate("allowActionMap");
        }

        public IList<IActionPolicy> Create()
        {
            return new List<IActionPolicy>
            {
                new PrePostExecuteActionPolicy(environment),
                new JobBlockingActionPolicy(environment, jobBlockingPolicyManager),
                new JobThrottlingActionPolicy(environment),
                new ExternalAllowActionPolicy(environment, allowActionMap),
            };
        }
    }
}