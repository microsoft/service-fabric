// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;

    internal class MappedTenantJob : IMappedTenantJob
    {
        private const int UnknownNodeCount = -1;

        private ActionType blockedActions = ActionType.None;

        public MappedTenantJob(ITenantJob tenantJob)
        {
            TenantJob = tenantJob.Validate("tenantJob");
            Id = tenantJob.Id;
            Actions = new List<IAction>();
            ImpactedNodeCount = UnknownNodeCount;
            MatchedTasks = new List<RepairTaskSummary>();
        }

        public Guid Id { get; private set; }

        public ITenantJob TenantJob { get; set; }

        public bool IsActive { get; set; }

        public int ImpactedNodeCount { get; set; }

        public IList<IAction> Actions { get; set; }

        public ActionType PendingActions
        {
            get
            {
                var pa = Actions.Aggregate(ActionType.None, (current, action) => current | action.ActionType);
                return pa;
            }
        }

        public ActionType AllowedActions
        {
            get
            {
                return this.PendingActions & ~this.blockedActions;
            }
        }

        public IList<RepairTaskSummary> MatchedTasks { get; private set; }

        public void AllowActions(TraceType traceType, ActionType allowedActions)
        {
            this.SetBlockedActions("AllowActions", traceType, this.blockedActions & ~allowedActions);
        }

        public void DenyActions(TraceType traceType, ActionType deniedActions)
        {
            this.SetBlockedActions("DenyActions", traceType, this.blockedActions | deniedActions);
        }

        private void SetBlockedActions(string callerName, TraceType traceType, ActionType blockedActions)
        {
            ActionType oldAllowedActions = this.AllowedActions;
            this.blockedActions = blockedActions;

            if (oldAllowedActions != this.AllowedActions)
            {
                traceType.WriteInfo("{0}: changed allowed actions for job {1} from '{2}' to '{3}'",
                    callerName,
                    this.Id,
                    oldAllowedActions,
                    this.AllowedActions);
            }
        }

        public override string ToString()
        {            
            string text = "Id: {0}, ImpactAction: {1}, Status: {2}, Actions: {3}, PendingActions: {4}, AllowedActions: {5}"
                .ToString(
                    Id,
                    TenantJob.GetImpactAction(),
                    TenantJob.JobStatus,
                    Actions.Count,
                    PendingActions,
                    AllowedActions);
            return text;
        }
    }
}