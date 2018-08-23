// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;

    internal class WorkloadActionGenerator : ActionGeneratorBase
    {
        // Right now, all workloads have same weight.
        private readonly double workloadScriptDefaultWeight = 100;
        private readonly WorkloadActionGeneratorParameters testParameters;
        private WeightedDice<string> workloadDice;

        public WorkloadActionGenerator(WorkloadActionGeneratorParameters parameters, Random random)
            : base(random)
        {
            this.testParameters = parameters;

            this.TraceType = "Chaos.WorkloadActionGenerator";
        }

        internal override IList<StateTransitionAction> GetPendingActions(ClusterStateSnapshot stateInfo, Guid activityId = default(Guid))
        {
            return GetPendingActions(stateInfo.WorkloadList, activityId);
        }

        protected override void GenerateAndEnqueueRandomActions(ClusterStateSnapshot stateSnapShot, Guid activityId = default(Guid))
        {
            this.GenerateAndEnqueueRandomActions(stateSnapShot.WorkloadList, activityId);
        }

        protected void GenerateAndEnqueueRandomActions(WorkloadList workloadInfo, Guid activityId = default(Guid))
        {
            this.UpdateDice(workloadInfo);
            string workloadName = this.workloadDice.NextRoll().Trim();
            ReleaseAssert.AssertIf(string.IsNullOrEmpty(workloadName), "Workload name cannot be null or empty.");

            bool isRunning = workloadInfo.GetWorkLoadState(workloadName);
            workloadInfo.FlipWorkloadState(workloadName);

            StateTransitionAction workloadScriptAction = null;
            if (isRunning)
            {
                workloadScriptAction = WorkloadStateTransitionAction.CreateStopWorkloadAction(workloadName);

                TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: Workload {1} has been chosen for Stop-Workload", activityId, workloadName);
            }
            else
            {
                workloadScriptAction = WorkloadStateTransitionAction.CreateStartWorkloadAction(workloadName);

                TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: Workload {1} has been chosen for Start-Workload", activityId, workloadName);
            }

            this.EnqueueAction(workloadScriptAction);
        }

        private IList<StateTransitionAction> GetPendingActions(WorkloadList workloadInfo, Guid activityId = default(Guid))
        {
            List<StateTransitionAction> pendingActions = new List<StateTransitionAction>();

            const bool isRunning = true;

            foreach (var workloadName in workloadInfo.Workloads)
            {
                if (workloadInfo.GetWorkLoadState(workloadName) == isRunning)
                {
                    pendingActions.Add(WorkloadStateTransitionAction.CreateStopWorkloadAction(workloadName));

                    TestabilityTrace.TraceSource.WriteInfo(this.TraceType, "{0}: Workload {1} has been chosen for Stop-Workload", activityId, workloadName);
                }
            }

            return pendingActions;
        }

        private void UpdateDice(WorkloadList workloadInfo)
        {
            this.workloadDice = new WeightedDice<string>(this.Random);

            if (workloadInfo.Workloads != null)
            {
                foreach (string workload in workloadInfo.Workloads)
                {
                    this.workloadDice.AddNewFace(workload, this.workloadScriptDefaultWeight);
                }
            }
        }
    }
}