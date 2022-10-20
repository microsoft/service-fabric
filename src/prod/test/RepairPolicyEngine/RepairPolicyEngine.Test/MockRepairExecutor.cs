// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Fabric;
using System.Fabric.Repair;
using System.Fabric.Test;
using System.Threading.Tasks;
using Microsoft.VisualStudio.TestTools.UnitTesting;

namespace RepairPolicyEngine.Test
{
    class MockRepairExecutor
    {
        private static readonly TimeSpan RepairManagerProcessingTime = TimeSpan.FromSeconds(1);
        
        public string Name { get; private set; }
        public TimeSpan ActionTime { get; private set; }
        public bool IsEnabled { get; private set; }
        public TimeSpan ProbationToHealthyPostRepair { get; private set; }

        private readonly FabricClient.RepairManagementClient _repairManager;
        public FabricClient.RepairManagementClient RepairManager
        {
            get { return _repairManager; }
        }

        public MockRepairExecutor(string name, TimeSpan actionTime, bool isEnabled, TimeSpan probationToHealthyPostRepair, FabricClient.RepairManagementClient repairManager)
        {
            Name = name;
            _repairManager = repairManager;
            IsEnabled = isEnabled;
            ActionTime = actionTime;
            ProbationToHealthyPostRepair = probationToHealthyPostRepair;
        }

        public static RepairTask GetAnyCreatedTask(string nodeName, FabricClient.RepairManagementClient repairManager)
        {
            var repairTaskList = repairManager.GetRepairTaskListAsync(null, RepairTaskStateFilter.Created, null).Result;
            var query = from repairTask in repairTaskList
                        where repairTask.Target is NodeRepairTargetDescription &&
                              ((NodeRepairTargetDescription)repairTask.Target).Nodes[0] == nodeName
                        orderby repairTask.CreatedTimestamp descending
                        select repairTask;
            if (query.Any())
            {
                Assert.IsTrue(query.Count() == 1, "There should be only 1 repair task created");
                return query.First();
            }

            return null;
        }

        public static void DeleteTask(RepairTask task, FabricClient.RepairManagementClient repairManager)
        {
            var t = repairManager.DeleteRepairTaskAsync(task.TaskId, 0);
            t.Wait();
        }

        public static void CancelAll(IEnumerable<MockRepairExecutor> executors, FabricClient.RepairManagementClient repairManager)
        {
            var repairTaskList = repairManager.GetRepairTaskListAsync(null, RepairTaskStateFilter.All, null).Result;

            var query = from repairTask in repairTaskList
                        where executors.Any(e => e.Name == repairTask.Action) && // ignore tasks with action not supported by our mock executors
                              repairTask.State != RepairTaskState.Completed && // ignore Completed tasks
                              (repairTask.State == RepairTaskState.Created ||
                              repairTask.Flags.HasFlag(RepairTaskFlags.CancelRequested))
                        select repairTask;

            if (!query.Any()) return;

            LogHelper.Log(
                "Cancelling the repair tasks with Created state or CancelRequested flags. Number of tasks = {0}",
                query.Count());

            Parallel.ForEach(query, task =>
                {
                    var executor = executors.Single(e => e.Name == task.Action);
                    executor.Cancel(task);
                });
        }

        public RepairTask GetCreatedTask(string nodeName)
        {
            var repairTaskList = _repairManager.GetRepairTaskListAsync(null, RepairTaskStateFilter.Created, null).Result;
            var query = from repairTask in repairTaskList
                        where repairTask.Action == Name && 
                              repairTask.Target is NodeRepairTargetDescription &&
                              ((NodeRepairTargetDescription)repairTask.Target).Nodes[0] == nodeName
                        select repairTask;
            if (query.Any())
            {
                Assert.IsTrue(query.Count() == 1, "There should be only 1 repair task created");
                return query.First();
            }

            return null;
        }

        public void Process(ref RepairTask task, bool isStuck = false)
        {
            Approve(ref task);

            // Executing
            Update(task, RepairTaskState.Executing, RepairTaskResult.Succeeded);
            task = GetRepairTask(task.TaskId);
            Assert.IsNotNull(task, "Repair task must be not null at Executing state");

            // Restoring
            Update(task, RepairTaskState.Restoring, RepairTaskResult.Succeeded);
            task = GetRepairTask(task.TaskId);
            Assert.IsNotNull(task, "Repair task must be not null at Restoring state");

            // Completed
            Thread.Sleep(RepairManagerProcessingTime);
            task = GetRepairTask(task.TaskId);
            Assert.IsTrue(task.State == RepairTaskState.Completed, "Repair task must be set to Completed by Repair Manager");
            LogHelper.Log("[{0}]Repair task is completed by RepairManager", Name);
        }

        public void Approve(ref RepairTask task)
        {
            Validate(task);
            Assert.IsTrue(task.State == RepairTaskState.Created, "Processing must start from Created state");

            // Claim
            Update(task, RepairTaskState.Claimed, RepairTaskResult.Succeeded);
            task = GetRepairTask(task.TaskId);
            Assert.IsNotNull(task, "Repair task must be not null at Claimed state");

            // Preparing
            Update(task, RepairTaskState.Preparing, RepairTaskResult.Succeeded);
            task = GetRepairTask(task.TaskId);
            Assert.IsNotNull(task, "Repair task must be not null at Preparing state");

            // Approved by RM
            Thread.Sleep(RepairManagerProcessingTime);
            task = GetRepairTask(task.TaskId);
            Assert.IsTrue(task.State == RepairTaskState.Approved, "Repair task must be set to Approved by Repair Manager");
            LogHelper.Log("[{0}]Repair task is approved by RepairManager", Name);
        }

        public void Complete(RepairTask task)
        {
            Validate(task);
            Update(task, RepairTaskState.Completed, RepairTaskResult.Succeeded);
        }

        public bool IsExecutable(RepairTask scheduledTask)
        {
            return Name.Equals(scheduledTask.Action);
        }

        private void Validate(RepairTask task)
        {
            Assert.IsNotNull(task, "Repair task must be not null");
            Assert.IsTrue(IsExecutable(task), "Repair task must be executable by this Executor");
        }

        private void Cancel(RepairTask task)
        {
            LogHelper.Log("[{0}]Cancel repair task from state {1} with target {2}.",
                Name,
                task.State,
                task.Target is NodeRepairTargetDescription ?
                    (object)((NodeRepairTargetDescription)task.Target).Nodes[0] :
                    task.Target);

            long commitVersion;
            switch (task.State)
            {
                case RepairTaskState.Created:
                case RepairTaskState.Claimed:
                    commitVersion = RepairManager.CancelRepairTaskAsync(task.TaskId, 0, true).Result;
                    LogHelper.Log("Cancel Commit version = {0}", commitVersion);
                    break;
                case RepairTaskState.Preparing:
                    Assert.IsTrue(task.Flags.HasFlag(RepairTaskFlags.CancelRequested), "Repair task must have CancelRequested flag");
                    commitVersion = RepairManager.CancelRepairTaskAsync(task.TaskId, 0, true).Result;
                    LogHelper.Log("Cancel Commit version = {0}", commitVersion);
                    break;
                case RepairTaskState.Approved:
                case RepairTaskState.Executing:
                    Assert.IsTrue(task.Flags.HasFlag(RepairTaskFlags.CancelRequested), "Repair task must have CancelRequested flag");
                    Update(task, RepairTaskState.Restoring, RepairTaskResult.Cancelled);
                    break;
                case RepairTaskState.Restoring:
                    LogHelper.Log("Repair task state is Restoring");
                    break;
                case RepairTaskState.Completed:
                    LogHelper.Log("Repair task is already completed");
                    return;
                default:
                    throw new ArgumentOutOfRangeException();
            }

            // Wait for Repair Manager to move to Completed
            Thread.Sleep(RepairManagerProcessingTime);
            task = GetRepairTask(task.TaskId);
            Assert.IsTrue(task.State == RepairTaskState.Completed && task.ResultStatus == RepairTaskResult.Cancelled, "Repair task must be cancelled by RepairManager");
        }

        private void Update(RepairTask task, RepairTaskState state, RepairTaskResult result)
        {
            LogHelper.Log("[{0}]Repair task from state {1} to state {2} with result {3}.", Name, task.State.ToString(), state.ToString(), result.ToString());
            task.State = state;
            task.Executor = Name;
            if (state == RepairTaskState.Preparing)
            {
                task.Impact = new NodeRepairImpactDescription(); //zero impact
            }
            if (state == RepairTaskState.Restoring || state == RepairTaskState.Completed)
            {
                task.ResultStatus = result;
            }
            task.ResultDetails = "Hooray!";
            long committedVersion = _repairManager.UpdateRepairExecutionStateAsync(task).Result;
            LogHelper.Log("...Done. Commit version: {0}.", committedVersion);
        }

        private RepairTask GetRepairTask(string repairTaskId)
        {
            var query = _repairManager.GetRepairTaskListAsync(repairTaskId, RepairTaskStateFilter.All, null).Result;
            if (query.Any())
            {
                Assert.IsTrue(query.Count == 1);
                return query.First();
            }
            return null;
        }

    }
}