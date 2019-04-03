// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Linq;

    internal class MappedRepairTask : IMappedRepairTask
    {
        private ActionType blockedActions = ActionType.None;

        public string Id { get; private set; }

        /// <summary>
        /// The repair task that this object wraps.
        /// </summary>
        public IRepairTask RepairTask { get; set; }

        public ITenantJob TenantJob { get; set; }

        public IList<IAction> Actions { get; set; }

        public MappedRepairTask(IRepairTask repairTask)
        {
            RepairTask = repairTask.Validate("repairTask");
            Id = repairTask.TaskId;
            Actions = new List<IAction>();
        }

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
                traceType.WriteInfo("{0}: changed allowed actions for repair task {1} from '{2}' to '{3}'",
                    callerName,
                    this.Id,
                    oldAllowedActions,
                    this.AllowedActions);
            }
        }

        public override string ToString()
        {
            string text = "Id: {0}, State: {1}, Actions: {2}, PendingActions: {3}, AllowedActions: {4}"
                .ToString(
                    Id,
                    RepairTask.State,
                    Actions.Count,
                    PendingActions,
                    AllowedActions);
            return text;
        }
    }
}