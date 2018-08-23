// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Repair
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a repair task, which includes information about what kind of repair was requested, what its progress 
    /// is, and what its final result was.</para>
    /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
    /// </summary>
    public class RepairTask
    {
        internal RepairTask(RepairScopeIdentifier scope)
        {
            Requires.Argument("scope", scope).NotNull();
            this.Scope = scope;
        }

        internal RepairTask(RepairScopeIdentifier scope, string taskId, string action)
        {
            Requires.Argument("scope", scope).NotNull();
            Requires.Argument("taskId", taskId).NotNullOrEmpty();
            Requires.Argument("action", action).NotNullOrEmpty();

            this.Scope = scope;
            this.TaskId = taskId;
            this.State = RepairTaskState.Created;
            this.Action = action;
        }

        /// <summary>
        /// <para>Gets an object describing the scope of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.Repair.RepairScopeIdentifier" /> object describing the scope of the repair task.</para>
        /// </value>
        /// <remarks>
        /// <para>The repair task scope determines the resource against which access checks are performed when the repair task 
        /// is created, modified, deleted, or retrieved.  Entities impacted by a repair must be contained within the scope of 
        /// the repair task.  For example, repairs which impact nodes require that the repair task be created under the Cluster scope.</para>
        /// </remarks>
        public RepairScopeIdentifier Scope { get; private set; }

        /// <summary>
        /// <para>Gets the identifier of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>The identifier of the repair task.</para>
        /// </value>
        public string TaskId { get; private set; }

        /// <summary>
        /// <para>Gets or sets the version of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the repair task.</para>
        /// </value>
        /// <remarks>
        /// <para>When creating a new repair task, the version must be set to zero.  When updating a repair task via the 
        /// UpdateRepairExecutionStateAsync method, the version is used for optimistic concurrency checks.  If the version is 
        /// set to zero, the update will not check for write conflicts.  If the version is set to a non-zero value, then the 
        /// update will only succeed if the actual current version of the repair task matches this value.</para>
        /// </remarks>
        public Int64 Version { get; set; }

        /// <summary>
        /// <para>Gets or sets a description of the purpose or other informational details of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>A description of the purpose or other informational details of the repair task.</para>
        /// </value>
        public string Description { get; set; }

        /// <summary>
        /// <para>Gets or sets the workflow state of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>The workflow state of the repair task.</para>
        /// </value>
        public RepairTaskState State { get; set; }

        /// <summary>
        /// <para>Gets the flags that give additional details about the status of the repair task.</para>
        /// </summary>
        /// <value>
        /// <para>The flags that give additional details about the status of the repair task.</para>
        /// </value>
        public RepairTaskFlags Flags { get; private set; }

        /// <summary>
        /// <para>Gets the requested repair action.</para>
        /// </summary>
        /// <value>
        /// <para>The requested repair action.</para>
        /// </value>
        public string Action { get; private set; }

        /// <summary>
        /// <para>Gets or sets an object describing which entities the requested repair action is targeting.</para>
        /// </summary>
        /// <value>
        /// <para>An object describing which entities the requested repair action is targeting.</para>
        /// </value>
        /// <remarks>
        /// <para>Target may be null if the repair action does not require information about specific targets.</para>
        /// </remarks>
        public RepairTargetDescription Target { get; set; }

        /// <summary>
        /// <para>Gets or sets the name of the repair executor.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the repair executor.</para>
        /// </value>
        public string Executor { get; set; }

        /// <summary>
        /// <para>Gets or sets a data string that the repair executor can use to store its internal state.</para>
        /// </summary>
        /// <value>
        /// <para>A data string that the repair executor can use to store its internal state.</para>
        /// </value>
        public string ExecutorData { get; set; }

        /// <summary>
        /// <para>Gets an object that describes the impact of the repair.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.Repair.RepairImpactDescription" /> object that describes the impact of the repair.</para>
        /// </value>
        /// <remarks>
        /// <para>Impact must be specified by the repair executor upon transitioning to the Preparing state. The impact object 
        /// determines what actions the system will take to prepare for the impact of the repair, prior to approving execution 
        /// of the repair.</para>
        /// </remarks>
        public RepairImpactDescription Impact { get; set; }

        /// <summary>
        /// <para>Gets or sets a value describing the overall result of the repair task execution.</para>
        /// </summary>
        /// <value>
        /// <para>A <see cref="System.Fabric.Repair.RepairTaskResult" /> value describing the overall result of the repair 
        /// task execution.</para>
        /// </value>
        public RepairTaskResult ResultStatus { get; set; }

        /// <summary>
        /// <para>Gets or sets a value providing additional details about the result of the repair task execution.</para>
        /// </summary>
        /// <value>
        /// <para>A value providing additional details about the result of the repair task execution.</para>
        /// </value>
        /// <remarks>
        /// <para>This value should be an HRESULT.</para>
        /// </remarks>
        public int ResultCode { get; set; }

        /// <summary>
        /// <para>Gets or sets a string providing additional details about the result of the repair task execution.</para>
        /// </summary>
        /// <value>
        /// <para>A string providing additional details about the result of the repair task execution.</para>
        /// </value>
        public string ResultDetails { get; set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Created state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Created state.</para>
        /// </value>
        public DateTime? CreatedTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Claimed state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Claimed state.</para>
        /// </value>
        public DateTime? ClaimedTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Preparing state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Preparing state.</para>
        /// </value>
        public DateTime? PreparingTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Approved state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Approved state.</para>
        /// </value>
        public DateTime? ApprovedTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Executing state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Executing state.</para>
        /// </value>
        public DateTime? ExecutingTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Restoring state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Restoring state.</para>
        /// </value>
        public DateTime? RestoringTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task entered the Completed state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task entered the Completed state.</para>
        /// </value>
        public DateTime? CompletedTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task started the health check in the Preparing state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task started the health check in the Preparing state.</para>
        /// </value>
        public DateTime? PreparingHealthCheckStartTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task completed the health check in the Preparing state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task completed the health check in the Preparing state.</para>
        /// </value>
        public DateTime? PreparingHealthCheckEndTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task started the health check in the Restoring state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task started the health check in the Restoring state.</para>
        /// </value>
        public DateTime? RestoringHealthCheckStartTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the time when the repair task completed the health check in the Restoring state.</para>
        /// </summary>
        /// <value>
        /// <para>The time when the repair task completed the health check in the Restoring state.</para>
        /// </value>
        public DateTime? RestoringHealthCheckEndTimestamp { get; private set; }

        /// <summary>
        /// <para>Gets the workflow state of the health check when the repair task is in the Preparing state.</para>
        /// </summary>
        /// <value>
        /// <para>The workflow state of the health check when the repair task is in the Preparing state.</para>
        /// </value>
        public RepairTaskHealthCheckState PreparingHealthCheckState { get; private set; }

        /// <summary>
        /// <para>Gets the workflow state of the health check when the repair task is in the Restoring state.</para>
        /// </summary>
        /// <value>
        /// <para>The workflow state of the health check when the repair task is in the Restoring state.</para>
        /// </value>
        public RepairTaskHealthCheckState RestoringHealthCheckState { get; private set; }

        /// <summary>
        /// <para>Gets or sets a value to determine if health checks have to be performed when the repair task enters the Preparing state.</para>
        /// </summary>
        /// <value>
        /// <para>A value to determine if health checks have to be performed when the repair task enters the Preparing state.</para>
        /// </value>
        public bool PerformPreparingHealthCheck { get; set; }

        /// <summary>
        /// <para>Gets or sets a value to determine if health checks have to be performed when the repair task enters the Restoring state.</para>
        /// </summary>
        /// <value>
        /// <para>A value to determine if health checks have to be performed when the repair task enters the Restoring state.</para>
        /// </value>
        public bool PerformRestoringHealthCheck { get; set; }
        
        internal IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeExecutorState = new NativeTypes.FABRIC_REPAIR_EXECUTOR_STATE()
            {
                Executor = pinCollection.AddBlittable(this.Executor),
                ExecutorData = pinCollection.AddBlittable(this.ExecutorData),
            };

            var nativeResultDescription = new NativeTypes.FABRIC_REPAIR_RESULT_DESCRIPTION()
            {
                ResultStatus = (NativeTypes.FABRIC_REPAIR_TASK_RESULT)this.ResultStatus,
                ResultCode = this.ResultCode,
                ResultDetails = pinCollection.AddBlittable(this.ResultDetails),
            };

            var ex1 = new NativeTypes.FABRIC_REPAIR_TASK_EX1
            {
                PerformPreparingHealthCheck = NativeTypes.ToBOOLEAN(this.PerformPreparingHealthCheck),
                PerformRestoringHealthCheck = NativeTypes.ToBOOLEAN(this.PerformRestoringHealthCheck),
                PreparingHealthCheckState = (NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE)this.PreparingHealthCheckState,
                RestoringHealthCheckState = (NativeTypes.FABRIC_REPAIR_TASK_HEALTH_CHECK_STATE)this.RestoringHealthCheckState,
            };

            var nativeTask = new NativeTypes.FABRIC_REPAIR_TASK()
            {
                TaskId = pinCollection.AddBlittable(this.TaskId),
                Version = this.Version,
                Description = pinCollection.AddBlittable(this.Description),

                State = (NativeTypes.FABRIC_REPAIR_TASK_STATE)this.State,
                Flags = (UInt32)this.Flags,

                Action = pinCollection.AddBlittable(this.Action),

                ExecutorState = pinCollection.AddBlittable(nativeExecutorState),
                Result = pinCollection.AddBlittable(nativeResultDescription),
                Reserved = pinCollection.AddBlittable(ex1),
            };

            nativeTask.Scope = this.Scope.ToNative(pinCollection);

            if (this.Target != null)
            {
                nativeTask.Target = this.Target.ToNative(pinCollection);
            }

            if (this.Impact != null)
            {
                nativeTask.Impact = this.Impact.ToNative(pinCollection);
            }

            return pinCollection.AddBlittable(nativeTask);
        }

        private static RepairTask CreateFromScope(RepairScopeIdentifier scope)
        {
            switch (scope.Kind)
            {
                case RepairScopeIdentifierKind.Cluster:
                    return new ClusterRepairTask();
                default:
                    return new RepairTask(scope);
            }
        }

        internal static unsafe RepairTask FromNative(NativeTypes.FABRIC_REPAIR_TASK* casted)
        {
            RepairScopeIdentifier scope = RepairScopeIdentifier.CreateFromNative(casted->Scope);
            RepairTask managed = CreateFromScope(scope);

            managed.TaskId = NativeTypes.FromNativeString(casted->TaskId);
            managed.Version = casted->Version;
            managed.Description = NativeTypes.FromNativeString(casted->Description);
            managed.State = (RepairTaskState)casted->State;
            managed.Flags = (RepairTaskFlags)casted->Flags;
            managed.Action = NativeTypes.FromNativeString(casted->Action);

            managed.Target = RepairTargetDescription.CreateFromNative(casted->Target);
            managed.Impact = RepairImpactDescription.CreateFromNative(casted->Impact);

            if (casted->ExecutorState != IntPtr.Zero)
            {
                var nativeExecutorState = (NativeTypes.FABRIC_REPAIR_EXECUTOR_STATE*)casted->ExecutorState;

                managed.Executor = NativeTypes.FromNativeString(nativeExecutorState->Executor);
                managed.ExecutorData = NativeTypes.FromNativeString(nativeExecutorState->ExecutorData);
            }

            if (casted->Result != IntPtr.Zero)
            {
                var nativeResult = (NativeTypes.FABRIC_REPAIR_RESULT_DESCRIPTION*)casted->Result;

                managed.ResultStatus = (RepairTaskResult)nativeResult->ResultStatus;
                managed.ResultCode = nativeResult->ResultCode;
                managed.ResultDetails = NativeTypes.FromNativeString(nativeResult->ResultDetails);
            }

            if (casted->History != IntPtr.Zero)
            {
                var nativeHistory = (NativeTypes.FABRIC_REPAIR_TASK_HISTORY*)casted->History;

                managed.CreatedTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->CreatedUtcTimestamp);
                managed.ClaimedTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->ClaimedUtcTimestamp);
                managed.PreparingTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->PreparingUtcTimestamp);
                managed.ApprovedTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->ApprovedUtcTimestamp);
                managed.ExecutingTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->ExecutingUtcTimestamp);
                managed.RestoringTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->RestoringUtcTimestamp);
                managed.CompletedTimestamp = NativeTypes.FromNullableNativeFILETIME(nativeHistory->CompletedUtcTimestamp);

                if (nativeHistory->Reserved != IntPtr.Zero)
                {
                    var ex1 = (NativeTypes.FABRIC_REPAIR_TASK_HISTORY_EX1*)nativeHistory->Reserved;

                    managed.PreparingHealthCheckStartTimestamp =
                        NativeTypes.FromNullableNativeFILETIME(ex1->PreparingHealthCheckStartUtcTimestamp);
                    managed.PreparingHealthCheckEndTimestamp =
                        NativeTypes.FromNullableNativeFILETIME(ex1->PreparingHealthCheckEndUtcTimestamp);
                    managed.RestoringHealthCheckStartTimestamp =
                        NativeTypes.FromNullableNativeFILETIME(ex1->RestoringHealthCheckStartUtcTimestamp);
                    managed.RestoringHealthCheckEndTimestamp =
                        NativeTypes.FromNullableNativeFILETIME(ex1->RestoringHealthCheckEndUtcTimestamp);
                }
            }

            if (casted->Reserved != IntPtr.Zero)
            {
                var ex1 = (NativeTypes.FABRIC_REPAIR_TASK_EX1*)casted->Reserved;

                managed.PreparingHealthCheckState = (RepairTaskHealthCheckState)ex1->PreparingHealthCheckState;
                managed.RestoringHealthCheckState = (RepairTaskHealthCheckState)ex1->RestoringHealthCheckState;
                managed.PerformPreparingHealthCheck = NativeTypes.FromBOOLEAN(ex1->PerformPreparingHealthCheck);
                managed.PerformRestoringHealthCheck = NativeTypes.FromBOOLEAN(ex1->PerformRestoringHealthCheck);
            }

            return managed;
        }
    }
}