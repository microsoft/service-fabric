// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Threading.Tasks;

    /// <summary>
    /// Helper class for applying policies and executing actions.
    /// </summary>
    internal static class ActionHelper
    {
        public static void CreateMappedWorkItems(
            IList<ITenantJob> tenantJobs,
            IList<IRepairTask> repairTasks,
            CoordinatorContext coordinatorContext)
        {
            tenantJobs.Validate("tenantJobs");
            repairTasks.Validate("repairTasks");
            coordinatorContext.Validate("coordinatorContext");

            foreach (var job in tenantJobs)
            {
                coordinatorContext.MappedTenantJobs.Add(job.Id, new MappedTenantJob(job));
            }

            foreach (var repairTask in repairTasks)
            {
                coordinatorContext.MappedRepairTasks.Add(repairTask.TaskId, new MappedRepairTask(repairTask));
            }
        }

        public static async Task ApplyPoliciesAsync(
            Guid activityId,
            IList<IActionPolicy> actionPolicies,
            CoordinatorContext coordinatorContext)
        {
            actionPolicies.Validate("actionPolicies");
            coordinatorContext.Validate("coordinatorContext");

            foreach (var policy in actionPolicies)
            {
                // if a policy throws an exception, we don't continue
                await policy.ApplyAsync(activityId, coordinatorContext).ConfigureAwait(false);
            }
        }

        public static IList<IAction> GetActions(
            Guid activityId,
            CoordinatorContext coordinatorContext)
        {
            coordinatorContext.Validate("coordinatorContext");

            IList<IAction> actions = new List<IAction>();

            foreach (var mappedTenantJob in coordinatorContext.MappedTenantJobs.Values)
            {
                AddActions(mappedTenantJob, actions);
            }

            foreach (var mappedRepairTask in coordinatorContext.MappedRepairTasks.Values)
            {
                AddActions(mappedRepairTask, actions);
            }

            return actions;
        }

        private static void AddActions(IActionableWorkItem workItem, IList<IAction> actions)
        {
            foreach (var action in workItem.Actions)
            {
                if ((workItem.AllowedActions & action.ActionType) == action.ActionType)
                {
                    actions.Add(action);
                }
            }
        }

        public static async Task ExecuteActionsAsync(
            Guid activityId,
            TraceType traceType,
            IList<IAction> actions)
        {
            traceType.Validate("traceType");
            actions.Validate("actions");

            var exceptions = new List<Exception>();

            foreach (var action in actions)
            {
                try
                {
                    traceType.WriteInfo("{0} Start: {1}", activityId, action);
                    await action.ExecuteAsync(activityId).ConfigureAwait(false);
                    traceType.WriteInfo("{0} Finish: {1}", activityId, action);
                }
                catch (Exception ex)
                {
                    traceType.WriteWarning("{0} Error: {1}{2}Exception: {3}", activityId, action, Environment.NewLine, ex);
                    exceptions.Add(ex);
                }
            }

            if (exceptions.Count > 0)
            {
                throw new AggregateException(exceptions);
            }
        }

        public static void ResetActionPolicies(IList<IActionPolicy> actionPolicies)
        {
            actionPolicies.Validate("actionPolicies");

            foreach (var actionPolicy in actionPolicies)
            {
                actionPolicy.Reset();
            }
        }
    }
}