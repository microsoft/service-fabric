// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    internal class WorkloadStateTransitionAction : StateTransitionAction
    {
        private WorkloadStateTransitionAction(string workloadName, StateTransitionActionType actionType)
            : base(actionType, Guid.Empty, Guid.Empty)
        {
            this.WorkloadName = workloadName;
        }

        public string WorkloadName { get; private set; }

        public static WorkloadStateTransitionAction CreateStartWorkloadAction(string workloadName)
        {
            return new WorkloadStateTransitionAction(workloadName, StateTransitionActionType.StartWorkload);
        }

        public static WorkloadStateTransitionAction CreateStopWorkloadAction(string workloadName)
        {
            return new WorkloadStateTransitionAction(workloadName, StateTransitionActionType.StopWorkload);
        }

        public override string ToString()
        {
            return string.Format("{0}, WorkloadName: {1}",
                base.ToString(),
                this.WorkloadName);
        }
    }
}