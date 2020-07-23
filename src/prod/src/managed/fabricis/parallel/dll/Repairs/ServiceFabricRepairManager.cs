// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Collections.Generic;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal class ServiceFabricRepairManager : IRepairManager
    {
        private readonly FabricClient fabricClient = new FabricClient();
        private readonly FabricClient.RepairManagementClient repairManager;

        private readonly IActivityLogger activityLogger;
        private readonly TraceType traceType;

        public ServiceFabricRepairManager(CoordinatorEnvironment environment, IActivityLogger activityLogger)
        {
            this.activityLogger = activityLogger.Validate("activityLogger");
            this.traceType = environment.Validate("environment").CreateTraceType("RMClient");

            this.repairManager = this.fabricClient.RepairManager;
        }

        public IRepairTask NewRepairTask(string taskId, string action)
        {
            return new ServiceFabricRepairTask(new ClusterRepairTask(taskId, action));
        }

        public async Task RemoveNodeStateAsync(Guid activityId, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
        {
            nodeName.Validate("nodeName");

            var startTime = DateTimeOffset.UtcNow;

            try
            {
                await this.fabricClient.ClusterManager.RemoveNodeStateAsync(nodeName, timeout, cancellationToken);
                traceType.WriteInfo("RemoveNodeState succeeded for {0}", nodeName);

                activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, nodeName);
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("RemoveNodeState failed for {0}: {1}", nodeName, ex);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, nodeName);
                throw;
            }
        }

        public async Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            var startTime = DateTimeOffset.UtcNow;

            try
            {
                // TODO, using the overload without timeout and cancellation token for now since there is some max timeout limit
                // being exercised somewhere. if timeout provided is more than that, repair task creation fails
                long result = await repairManager.CreateRepairTaskAsync(Unwrap(repairTask)).ConfigureAwait(false);                
                traceType.WriteInfo(
                    "Created repair task. Result: {0}, repair task: {1}",                    
                    result,
                    repairTask.ToJson());

                activityLogger.LogChangeState(activityId, repairTask, RepairTaskState.Invalid, repairTask.State, TimeSpan.Zero, repairTask.ToJson(), repairTask.TaskId);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return result;
            }
            catch (Exception ex)
            {
                traceType.WriteError("Unable to create repair task. Errors: {0}", ex.GetMessage());
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }
        }

        public async Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            var startTime = DateTimeOffset.UtcNow;

            try
            {
                var commitVersion = await repairManager.UpdateRepairExecutionStateAsync(Unwrap(repairTask)).ConfigureAwait(false);

                traceType.WriteInfo(
                    "Successfully updated repair execution state. Commit version: {0}, repair task: {1}",
                    commitVersion,
                    repairTask.ToJson());

                activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteError("Unable to update repair execution state: Error: {0}", ex);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }
        }

        public async Task<IList<IRepairTask>> GetRepairTaskListAsync(
            Guid activityId, 
            string taskIdFilter = null, 
            RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default,
            string executorFilter = null)
        {
            var startTime = DateTimeOffset.UtcNow;

            try
            {
                // TODO, using the overload without timeout and cancellation token for now since there is some max timeout limit
                // being exercised somewhere. if timeout provided is more than that, repair task creation fails
                RepairTaskList repairTaskList = await repairManager.GetRepairTaskListAsync(taskIdFilter, stateFilter, executorFilter).ConfigureAwait(false);

                var repairTasks = new List<IRepairTask>();
                foreach (var repairTask in repairTaskList)
                {
                    repairTasks.Add(new ServiceFabricRepairTask(repairTask));
                }

                activityLogger.LogOperation(activityId, startTime);

                return repairTasks;
            }
            catch (Exception ex)
            {
                traceType.WriteError("Unable to get repair task list. Errors: {0}", ex.GetMessage());
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex);
                throw;
            }
        }

        public async Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            var startTime = DateTimeOffset.UtcNow;
            var oldState = repairTask.State;
            var duration = repairTask.GetTimeInCurrentState(startTime);

            try
            {
                var commitVersion = await repairManager.CancelRepairTaskAsync(repairTask.TaskId, repairTask.Version, false).ConfigureAwait(false);

                traceType.WriteInfo(
                    "Canceled repair task: {0}",                    
                    repairTask.ToJson());

                activityLogger.LogChangeState(activityId, repairTask, oldState, RepairTaskState.Invalid, duration, repairTask.ToString(), repairTask.TaskId);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning(
                    "Unable to cancel repair task. Continuing. Repair task: {0}{1}Errors: {2}",
                    repairTask.ToJson(), Environment.NewLine, ex);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }
        }

        public async Task<long> UpdateRepairTaskHealthPolicyAsync(
            Guid activityId, 
            IRepairTask repairTask, 
            bool? performPreparingHealthCheck,
            bool? performRestoringHealthCheck)
        {
            repairTask.Validate("repairTask");

            var startTime = DateTimeOffset.UtcNow;
            var preparingOldState = repairTask.PerformPreparingHealthCheck;
            var restoringOldState = repairTask.PerformRestoringHealthCheck;

            var duration = repairTask.GetTimeInCurrentState(startTime);

            try
            {
                var commitVersion = await repairManager.UpdateRepairTaskHealthPolicyAsync(
                    repairTask.TaskId, 
                    repairTask.Version, 
                    performPreparingHealthCheck, 
                    performRestoringHealthCheck).ConfigureAwait(false);

                traceType.WriteInfo(
                    "Updated repair task health policy: {0}",                    
                    repairTask.ToJson());

                if (performPreparingHealthCheck.HasValue && preparingOldState != performPreparingHealthCheck.Value)
                {
                    activityLogger.LogChangeState(activityId, repairTask, preparingOldState, performPreparingHealthCheck.Value, duration, repairTask.ToString(), repairTask.TaskId);
                }
                if (performRestoringHealthCheck.HasValue && restoringOldState != performRestoringHealthCheck.Value)
                {
                    activityLogger.LogChangeState(activityId, repairTask, restoringOldState, performRestoringHealthCheck.Value, duration, repairTask.ToString(), repairTask.TaskId);
                }

                activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning(
                    "Unable to update repair task health policy. Continuing. Repair task: {0}{1}Errors: {2}",
                    repairTask.ToJson(), Environment.NewLine, ex);
                activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }            
        }

        private static RepairTask Unwrap(IRepairTask repairTask)
        {
            RepairTask unwrappedRepairTask = ((ServiceFabricRepairTask)repairTask).Unwrap();
            return unwrappedRepairTask;
        }
    }
}