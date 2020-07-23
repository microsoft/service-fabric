// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using RD.Fabric.PolicyAgent;
    using Repair;

    /// <summary>
    /// Helper class for creating concrete <see cref="IAction"/> types.
    /// This is only used by the <see cref="Reconciler"/>
    /// </summary>
    internal sealed class ActionFactory
    {
        private readonly CoordinatorEnvironment environment;
        private readonly IPolicyAgentClient policyAgentClient;
        private readonly IRepairManager repairManager;
        private readonly Dictionary<Guid, JobStepResponseEnum> jobResponseMap;
        private readonly RepairActionProvider repairActionProvider;

        public ActionFactory(
            CoordinatorEnvironment environment,
            IPolicyAgentClient policyAgentClient,
            IRepairManager repairManager,
            RepairActionProvider repairActionProvider)
        {
            this.environment = environment.Validate("environment");
            this.policyAgentClient = policyAgentClient.Validate("policyAgentClient");
            this.repairManager = repairManager.Validate("repairManager");
            this.repairActionProvider = repairActionProvider.Validate("repairActionProvider");
            this.jobResponseMap = new Dictionary<Guid, JobStepResponseEnum>();
        }

        public IAction NewMoveToPreparingAction(IRepairTask repairTask, ITenantJob tenantJob, RepairTaskPrepareArgs args, bool surpriseJob = false)
        {
            return new MoveToPreparingAction(environment, repairManager, repairTask, tenantJob, surpriseJob, args);
        }

        public IAction NewMoveToExecutingAction(IRepairTask repairTask, string warning = null)
        {
            return new MoveToExecutingAction(environment, repairManager, repairTask, warning);
        }

        public IAction NewCreateInPreparingAction(ITenantJob tenantJob, RepairTaskPrepareArgs args, bool surpriseJob = false)
        {
            return new CreateInPreparingAction(environment, repairManager, tenantJob, surpriseJob, args);
        }

        public IAction NewClaimedRepairTaskAction(IRepairTask repairTask, IList<string> roleInstanceNames)
        {
            return new ClaimedRepairTaskAction(environment, repairManager, repairTask, roleInstanceNames, repairActionProvider);    
        }

        public IAction NewCancelRepairTaskAction(IRepairTask repairTask)
        {
            return new CancelRepairTaskAction(environment, repairManager, repairTask);
        }

        public IAction NewMoveToRestoringAction(
            IRepairTask repairTask,
            RepairTaskResult repairTaskResult,
            string resultDetails,
            bool surpriseJob = false,
            bool cancelRestoringHealthCheck = false,
            bool processRemovedNodes = false)
        {
            return new MoveToRestoringAction(environment, repairManager, repairTask, repairTaskResult, resultDetails, surpriseJob, cancelRestoringHealthCheck, processRemovedNodes);
        }

        public IAction NewMoveToCompletedAction(
            IRepairTask repairTask,
            RepairTaskResult repairTaskResult,
            string resultDetails,
            bool surpriseJob)
        {
            return new MoveToCompletedAction(environment, repairManager, repairTask, repairTaskResult, resultDetails, surpriseJob);
        }

        public IAction NewRequestRepairAction(IRepairTask repairTask)
        {
            return new RequestRepairAction(environment, policyAgentClient, repairTask, repairActionProvider);
        }

        public IAction NewCancelRestoringHealthCheckAction(IRepairTask repairTask)
        {
            return new CancelRestoringHealthCheckAction(environment, repairManager, repairTask);
        }

        public IAction NewAckJobAction(ITenantJob tenantJob)
        {
            return new AckJobAction(environment, this.MarkJobForAcknowledge, tenantJob);
        }

        public IAction NewCancelJobAction(ITenantJob tenantJob)
        {
            return new CancelJobAction(environment, this.policyAgentClient, tenantJob);
        }

        public IAction NewExecuteJobAction(ITenantJob tenantJob, IRepairTask repairTask)
        {
            return new ExecuteJobAction(environment, this.MarkJobForAcknowledge, repairManager, tenantJob, repairTask);
        }

        private void MarkJobForAcknowledge(Guid jobId)
        {
            this.jobResponseMap[jobId] = JobStepResponseEnum.Acknowledged;
        }

        public IReadOnlyDictionary<Guid, JobStepResponseEnum> GetJobStepResponses()
        {
            return this.jobResponseMap;
        }
    }
}