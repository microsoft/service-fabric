// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Autopilot
{
    using Collections.Generic;
    using Repair;
    using Threading;
    using Threading.Tasks;

    internal sealed class ServiceFabricRepairManager : IRepairManager
    {
        private sealed class ServiceFabricRepairTask : IRepairTask
        {
            private readonly RepairTask repairTask;

            public ServiceFabricRepairTask(RepairTask repairTask)
            {
                this.repairTask = repairTask.Validate("repairTask");
            }

            public RepairScopeIdentifier Scope { get { return repairTask.Scope; } }

            public string TaskId { get { return repairTask.TaskId; } }

            public long Version { get { return repairTask.Version; } set { repairTask.Version = value; } }

            public string Description { get { return repairTask.Description; } set { repairTask.Description = value; } }

            public RepairTaskState State { get { return repairTask.State; } set { repairTask.State = value; } }

            public RepairTaskFlags Flags { get { return repairTask.Flags; } }

            public string Action { get { return repairTask.Action; } }

            public RepairTargetDescription Target { get { return repairTask.Target; } set { repairTask.Target = value; } }

            public string Executor { get { return repairTask.Executor; } set { repairTask.Executor = value; } }

            public string ExecutorData { get { return repairTask.ExecutorData; } set { repairTask.ExecutorData = value; } }

            public RepairImpactDescription Impact { get { return repairTask.Impact; } set { repairTask.Impact = value; } }

            public RepairTaskResult ResultStatus { get { return repairTask.ResultStatus; } set { repairTask.ResultStatus = value; } }

            public int ResultCode { get { return repairTask.ResultCode; } set { repairTask.ResultCode = value; } }

            public string ResultDetails { get { return repairTask.ResultDetails; } set { repairTask.ResultDetails = value; } }

            public DateTime? CreatedTimestamp { get { return repairTask.CreatedTimestamp; } }

            public DateTime? ClaimedTimestamp { get { return repairTask.ClaimedTimestamp; } }

            public DateTime? PreparingTimestamp { get { return repairTask.PreparingTimestamp; } }

            public DateTime? ApprovedTimestamp { get { return repairTask.ApprovedTimestamp; } }

            public DateTime? ExecutingTimestamp { get { return repairTask.ExecutingTimestamp; } }

            public DateTime? RestoringTimestamp { get { return repairTask.RestoringTimestamp; } }

            public DateTime? CompletedTimestamp { get { return repairTask.CompletedTimestamp; } }

            public DateTime? PreparingHealthCheckStartTimestamp { get { return repairTask.PreparingHealthCheckStartTimestamp; } }

            public DateTime? PreparingHealthCheckEndTimestamp { get { return repairTask.PreparingHealthCheckEndTimestamp; } }

            public DateTime? RestoringHealthCheckStartTimestamp { get { return repairTask.RestoringHealthCheckStartTimestamp; } }

            public DateTime? RestoringHealthCheckEndTimestamp { get { return repairTask.RestoringHealthCheckEndTimestamp; } }

            public RepairTaskHealthCheckState PreparingHealthCheckState { get { return repairTask.PreparingHealthCheckState; } }

            public RepairTaskHealthCheckState RestoringHealthCheckState { get { return repairTask.RestoringHealthCheckState; } }

            public bool PerformPreparingHealthCheck { get { return repairTask.PerformPreparingHealthCheck; } set { repairTask.PerformPreparingHealthCheck = value; } }

            public bool PerformRestoringHealthCheck { get { return repairTask.PerformRestoringHealthCheck; } set { repairTask.PerformRestoringHealthCheck = value; } }

            /// <summary>
            /// Get's the underlying 'real' repair task that this class wraps.
            /// </summary>
            /// <remarks>
            /// Making it a method instead of a property to avoid <see cref="CoordinatorHelper.ToJson"/> method showing an inner repair task property also
            /// while deserializing an <see cref="IRepairTask"/> object.
            /// </remarks>
            public RepairTask Unwrap()
            {
                return repairTask;
            }

            public override string ToString()
            {
                return "{0}, State: {1}".ToString(TaskId, State);
            }
        }

        private readonly FabricClient fabricClient = new FabricClient();
        private readonly FabricClient.RepairManagementClient repairManager;

        //private readonly IActivityLogger activityLogger;
        private readonly TraceType traceType;

        public ServiceFabricRepairManager(CoordinatorEnvironment environment)
        {
            //this.activityLogger = activityLogger.Validate("activityLogger");
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

            //var startTime = DateTimeOffset.UtcNow;

            try
            {
                await this.fabricClient.ClusterManager.RemoveNodeStateAsync(nodeName, timeout, cancellationToken);
                traceType.WriteInfo("RemoveNodeState succeeded for {0}", nodeName);

                //activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, nodeName);
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("RemoveNodeState failed for {0}: {1}", ex);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, nodeName);
                throw;
            }
        }

        public async Task<long> CreateRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            //var startTime = DateTimeOffset.UtcNow;

            try
            {
                // TODO, using the overload without timeout and cancellation token for now since there is some max timeout limit
                // being exercised somewhere. if timeout provided is more than that, repair task creation fails
                long result = await repairManager.CreateRepairTaskAsync(Unwrap(repairTask)).ConfigureAwait(false);
                traceType.WriteInfo(
                    "Created repair task. Result: {0}, repair task: {1}",
                    result,
                    repairTask.ToJson());

                //activityLogger.LogChangeState(activityId, repairTask, RepairTaskState.Invalid, repairTask.State, TimeSpan.Zero, repairTask.ToJson(), repairTask.TaskId);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return result;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("Unable to create repair task. Errors: {0}", ex.GetMessage());
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }
        }

        public async Task<long> UpdateRepairExecutionStateAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            //var startTime = DateTimeOffset.UtcNow;

            try
            {
                var commitVersion = await repairManager.UpdateRepairExecutionStateAsync(Unwrap(repairTask)).ConfigureAwait(false);

                traceType.WriteInfo(
                    "Successfully updated repair execution state. Commit version: {0}, repair task: {1}",
                    commitVersion,
                    repairTask.ToJson());

                //activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("Unable to update repair execution state: Error: {0}", ex);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
                throw;
            }
        }

        public async Task<IList<IRepairTask>> GetRepairTaskListAsync(
            Guid activityId,
            string taskIdFilter = null,
            RepairTaskStateFilter stateFilter = RepairTaskStateFilter.Default,
            string executorFilter = null)
        {
            //var startTime = DateTimeOffset.UtcNow;

            try
            {
                // TODO, using the overload without timeout and cancellation token for now since there is some max timeout limit
                // being exercised somewhere. if timeout provided is more than that, repair task creation fails
                RepairTaskList repairTaskList = await repairManager.GetRepairTaskListAsync(taskIdFilter, stateFilter, executorFilter).ConfigureAwait(false);

                var repairTasks = new List<IRepairTask>(repairTaskList.Count);
                foreach (var repairTask in repairTaskList)
                {
                    repairTasks.Add(new ServiceFabricRepairTask(repairTask));
                }

                //activityLogger.LogOperation(activityId, startTime);

                return repairTasks;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning("Unable to get repair task list. Errors: {0}", ex.GetMessage());
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex);
                throw;
            }
        }

        public async Task<long> CancelRepairTaskAsync(Guid activityId, IRepairTask repairTask)
        {
            repairTask.Validate("repairTask");

            //var startTime = DateTimeOffset.UtcNow;
            //var oldState = repairTask.State;
            //var duration = repairTask.GetTimeInCurrentState(startTime);

            try
            {
                var commitVersion = await repairManager.CancelRepairTaskAsync(repairTask.TaskId, repairTask.Version, false).ConfigureAwait(false);

                traceType.WriteInfo(
                    "Canceled repair task: {0}",
                    repairTask.ToJson());

                //activityLogger.LogChangeState(activityId, repairTask, oldState, RepairTaskState.Invalid, duration, repairTask.ToString(), repairTask.TaskId);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning(
                    "Unable to cancel repair task. Continuing. Repair task: {0}{1}Errors: {2}",
                    repairTask.ToJson(), Environment.NewLine, ex);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
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

            //var startTime = DateTimeOffset.UtcNow;
            //var preparingOldState = repairTask.PerformPreparingHealthCheck;
            //var restoringOldState = repairTask.PerformRestoringHealthCheck;

            //var duration = repairTask.GetTimeInCurrentState(startTime);

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

                //if (performPreparingHealthCheck.HasValue && preparingOldState != performPreparingHealthCheck.Value)
                //{
                //    activityLogger.LogChangeState(activityId, repairTask, preparingOldState, performPreparingHealthCheck.Value, duration, repairTask.ToString(), repairTask.TaskId);
                //}
                //if (performRestoringHealthCheck.HasValue && restoringOldState != performRestoringHealthCheck.Value)
                //{
                //    activityLogger.LogChangeState(activityId, repairTask, restoringOldState, performRestoringHealthCheck.Value, duration, repairTask.ToString(), repairTask.TaskId);
                //}

                //activityLogger.LogOperation(activityId, startTime, OperationResult.Success, null, repairTask.ToJson());

                return commitVersion;
            }
            catch (Exception ex)
            {
                traceType.WriteWarning(
                    "Unable to update repair task health policy. Continuing. Repair task: {0}{1}Errors: {2}",
                    repairTask.ToJson(), Environment.NewLine, ex);
                //activityLogger.LogOperation(activityId, startTime, OperationResult.Failure, ex, repairTask.ToJson());
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