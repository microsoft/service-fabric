// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;
    using RD.Fabric.PolicyAgent;
    using Threading;
    using Threading.Tasks;

    internal sealed class RequestRepairAction : Action
    {
        private readonly IPolicyAgentClient policyAgentClient;
        private readonly IRepairTask repairTask;
        private readonly RepairActionProvider repairActionProvider;

        public RequestRepairAction(CoordinatorEnvironment environment, IPolicyAgentClient policyAgentClient, IRepairTask repairTask, RepairActionProvider repairActionProvider)
            : base(environment, ActionType.RequestRepair)
        {
            this.policyAgentClient = policyAgentClient.Validate("policyAgentClient");
            this.repairTask = repairTask.Validate("repairTask");
            this.repairActionProvider = repairActionProvider.Validate("repairActionProvider");
        }

        public override async Task ExecuteAsync(Guid activityId)
        {
            RepairActionTypeEnum? action = repairActionProvider.GetRepairAction(repairTask.Action);
            if (action == null)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring unknown action {1}", repairTask.TaskId, repairTask.Action);
                return;
            }

            // Ignore requests for anything other than a single node
            IList<string> targetNodeNames = repairTask.GetTargetNodeNames();
            if (targetNodeNames == null || targetNodeNames.Count != 1)
            {
                this.Environment.DefaultTraceType.WriteInfo("Repair task {0}: ignoring bad target description {1}", repairTask.TaskId, repairTask.Target);
                return;
            }

            // Map Service Fabric node name to Azure role instance Id
            string targetRoleInstance = targetNodeNames[0].TranslateNodeNameToRoleInstance();

            // TODO cancel task if its action/target is bad

            await policyAgentClient.RequestRepairAsync(
                activityId,
                new RepairRequest
                {
                    Action = action.Value,
                    ContextId = repairTask.GetTenantJobContext(),
                    RoleInstanceId = targetRoleInstance,
                },
                CancellationToken.None).ConfigureAwait(false);            
        }

        public override string ToString()
        {
            var targetNodeName = repairTask.GetTargetNodeNames() != null
                ? repairTask.GetTargetNodeNames().FirstOrDefault()
                : null;

            return "{0}, RepairTask: {1}, Node: {2}".ToString(base.ToString(), repairTask, targetNodeName);
        }
    }
}