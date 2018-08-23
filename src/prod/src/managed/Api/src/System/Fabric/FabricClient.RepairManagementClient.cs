// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Repair;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides methods for managing repair tasks.</para>
        /// <para>This class supports the Service Fabric platform; it is not meant to be called directly from your code.</para>
        /// </summary>
        public sealed class RepairManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricRepairManagementClient2 nativeRepairClient;

            private static readonly RepairScopeIdentifier DefaultScope = new ClusterRepairScopeIdentifier();

            #endregion

            #region Constructor

            internal RepairManagementClient(FabricClient fabricClient, NativeClient.IFabricRepairManagementClient2 nativeRepairClient)
            {
                this.fabricClient = fabricClient;
                this.nativeRepairClient = nativeRepairClient;
            }

            #endregion

            #region API

            /// <summary>
            /// <para>Creates a new repair task.</para>
            /// </summary>
            /// <param name = "repairTask" >
            /// <para> The description of the repair task to be created.</para>
            /// </param>
            /// <returns>
            /// <para>The version number of the newly-created repair task.</para>
            /// </returns>
            public Task<Int64> CreateRepairTaskAsync(RepairTask repairTask)
            {
                return this.CreateRepairTaskAsync(repairTask, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates a new repair task.</para>
            /// </summary>
            /// <param name = "repairTask" >
            /// <para> The description of the repair task to be created.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a<see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name = "cancellationToken" >
            /// <para> The optional cancellation token that the operation is observing.It can be used to send a notification that the operation should be canceled.
            /// Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The version number of the newly-created repair task.</para>
            /// </returns>
            public Task<Int64> CreateRepairTaskAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTask", repairTask).NotNull();

                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    callback => this.CreateRepairTaskAsyncBeginWrapper(repairTask, timeout, callback),
                    this.CreateRepairTaskAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.CreateRepairTaskAsync");
            }

            /// <summary>
            /// <para>Requests the cancellation of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task to be cancelled.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="requestAbort">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> if the repair should be stopped as soon as possible even if it has already started executing. <languageKeyword>False</languageKeyword> if the repair should be cancelled only if execution has not yet started.</para>
            /// </param>
            /// <returns>
            /// <para>The new version number of the repair task.</para>
            /// </returns>
            public Task<Int64> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort)
            {
                return this.CancelRepairTaskAsync(repairTaskId, version, requestAbort, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Requests the cancellation of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task to be cancelled.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="requestAbort">
            /// <para>
            ///     <languageKeyword>True</languageKeyword> if the repair should be stopped as soon as possible even if it has already started executing. <languageKeyword>False</languageKeyword> if the repair should be cancelled only if execution has not yet started.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The new version number of the repair task.</para>
            /// </returns>
            public Task<Int64> CancelRepairTaskAsync(string repairTaskId, Int64 version, bool requestAbort, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTaskId", repairTaskId).NotNullOrEmpty();

                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    callback => this.CancelRepairTaskAsyncBeginWrapper(DefaultScope, repairTaskId, version, requestAbort, timeout, callback),
                    this.CancelRepairTaskAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.CancelRepairTaskAsync");
            }

            /// <summary>
            /// <para>Forces the approval of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task to be approved.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <returns>
            /// <para>The new version number of the repair task.</para>
            /// </returns>
            public Task<Int64> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version)
            {
                return this.ForceApproveRepairTaskAsync(repairTaskId, version, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Forces the approval of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task to be approved.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The new version number of the repair task.</para>
            /// </returns>
            public Task<Int64> ForceApproveRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTaskId", repairTaskId).NotNullOrEmpty();

                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    callback => this.ForceApproveRepairTaskAsyncBeginWrapper(DefaultScope, repairTaskId, version, timeout, callback),
                    this.ForceApproveRepairTaskAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.ForceApproveRepairTaskAsync");
            }

            /// <summary>
            /// <para>Deletes the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the completed repair task to be deleted.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing the asynchronous operation.</para>
            /// </returns>
            public Task DeleteRepairTaskAsync(string repairTaskId, Int64 version)
            {
                return this.DeleteRepairTaskAsync(repairTaskId, version, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the completed repair task to be deleted.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing the asynchronous operation.</para>
            /// </returns>
            public Task DeleteRepairTaskAsync(string repairTaskId, Int64 version, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTaskId", repairTaskId).NotNullOrEmpty();

                return Utility.WrapNativeAsyncInvokeInMTA(
                    callback => this.DeleteRepairTaskAsyncBeginWrapper(DefaultScope, repairTaskId, version, timeout, callback),
                    this.DeleteRepairTaskAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.DeleteRepairTaskAsync");
            }

            /// <summary>
            /// Updates a repair task.
            /// </summary>
            /// <param name="repairTask">The modified repair task.</param>
            /// <returns><para>The new version number of the repair task.</para></returns>
            public Task<Int64> UpdateRepairExecutionStateAsync(RepairTask repairTask)
            {
                return this.UpdateRepairExecutionStateAsync(repairTask, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Updates a repair task.
            /// </summary>
            /// <param name="repairTask">The modified repair task.</param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns><para>The new version number of the repair task.</para></returns>
            public Task<Int64> UpdateRepairExecutionStateAsync(RepairTask repairTask, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTask", repairTask).NotNull();

                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    callback => this.UpdateRepairExecutionStateAsyncBeginWrapper(repairTask, timeout, callback),
                    this.UpdateRepairExecutionStateAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.UpdateRepairExecutionStateAsync");
            }

            /// <summary>
            /// <para>Gets a list of all repair tasks.</para>
            /// </summary>
            /// <returns>
            /// <para>The list of all repair tasks.</para>
            /// </returns>
            public Task<RepairTaskList> GetRepairTaskListAsync()
            {
                return this.GetRepairTaskListAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets a list of all repair tasks.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The list of all repair tasks.</para>
            /// </returns>
            public Task<RepairTaskList> GetRepairTaskListAsync(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.GetRepairTaskListAsync(null, RepairTaskStateFilter.Default, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets a list of repair tasks matching all of the given filters.</para>
            /// </summary>
            /// <param name="taskIdFilter">
            /// <para>The repair task ID prefix to be matched.  If null, no filter is applied to the task ID.</para>
            /// </param>
            /// <param name="stateFilter">
            /// <para>A bitwise combination of state filter values that specify which task states should be included in the list.</para>
            /// </param>
            /// <param name="executorFilter">
            /// <para>The name of the repair executor whose claimed tasks should be included in the list. If null, no filter is applied to the executor name.</para>
            /// </param>
            /// <returns>
            /// <para>The list of repair tasks matching all of the given filters.</para>
            /// </returns>
            public Task<RepairTaskList> GetRepairTaskListAsync(
                string taskIdFilter,
                RepairTaskStateFilter stateFilter,
                string executorFilter)
            {
                return this.GetRepairTaskListAsync(taskIdFilter, stateFilter, executorFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets a list of repair tasks matching all of the given filters.</para>
            /// </summary>
            /// <param name="taskIdFilter">
            /// <para>The repair task ID prefix to be matched.  If null, no filter is applied to the task ID.</para>
            /// </param>
            /// <param name="stateFilter">
            /// <para>A bitwise combination of state filter values that specify which task states should be included in the list.</para>
            /// </param>
            /// <param name="executorFilter">
            /// <para>The name of the repair executor whose claimed tasks should be included in the list. If null, no filter is applied to the executor name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The list of repair tasks matching all of the given filters.</para>
            /// </returns>
            public Task<RepairTaskList> GetRepairTaskListAsync(
                string taskIdFilter,
                RepairTaskStateFilter stateFilter,
                string executorFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                var queryDescription = new RepairTaskQueryDescription(
                    DefaultScope,
                    taskIdFilter,
                    stateFilter,
                    executorFilter);

                return Utility.WrapNativeAsyncInvokeInMTA<RepairTaskList>(
                    callback => this.GetRepairTaskListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetRepairTaskListAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.GetRepairTaskListAsync");
            }

            /// <summary>
            /// <para>Updates the health policy of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task for which the health policy is to be updated.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="performPreparingHealthCheck">
            /// <para>
            /// A nullable boolean indicating if health check is to be performed in the Preparing stage of the repair task.
            /// Specify <c>null</c> for this parameter if the existing value should not be altered. Otherwise, specify the desired new value. 
            /// </para>
            /// </param>
            /// <param name="performRestoringHealthCheck">
            /// <para>
            /// A nullable boolean indicating if health check is to be performed in the Restoring stage of the repair task.
            /// Specify <c>null</c> for this parameter if the existing value should not be altered. Otherwise, specify the desired new value. 
            /// </para>
            /// </param>            
            public Task<Int64> UpdateRepairTaskHealthPolicyAsync(string repairTaskId, Int64 version, bool? performPreparingHealthCheck, bool? performRestoringHealthCheck)
            {
                return this.UpdateRepairTaskHealthPolicyAsync(
                    repairTaskId, 
                    version, 
                    performPreparingHealthCheck, 
                    performRestoringHealthCheck, 
                    DefaultTimeout, 
                    CancellationToken.None);
            }

            /// <summary>
            /// <para>Updates the health policy of the given repair task.</para>
            /// </summary>
            /// <param name="repairTaskId">
            /// <para>The ID of the repair task for which the health policy is to be updated.</para>
            /// </param>
            /// <param name="version">
            /// <para>The current version number of the repair task. If non-zero, then the request will only succeed if this value matches the actual current value of the repair task. If zero, then no version check is performed.</para>
            /// </param>
            /// <param name="performPreparingHealthCheck">
            /// <para>
            /// A nullable boolean indicating if health check is to be performed in the Preparing stage of the repair task.
            /// Specify <c>null</c> for this parameter if the existing value should not be altered. Else, specify the appropriate <c>bool</c> value. 
            /// </para>
            /// </param>
            /// <param name="performRestoringHealthCheck">
            /// <para>
            /// A nullable boolean indicating if health check is to be performed in the Restoring stage of the repair task.
            /// Specify <c>null</c> for this parameter if the existing value should not be altered. Else, specify the appropriate <c>bool</c> value. 
            /// </para>
            /// </param>            
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing the asynchronous operation.</para>
            /// </returns>
            public Task<Int64> UpdateRepairTaskHealthPolicyAsync(
                string repairTaskId, 
                Int64 version, 
                bool? performPreparingHealthCheck, 
                bool? performRestoringHealthCheck, 
                TimeSpan timeout, 
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("repairTaskId", repairTaskId).NotNull();

                var requestDescription = new RepairTaskHealthPolicyUpdateDescription(
                    DefaultScope,
                    repairTaskId,
                    version,
                    performPreparingHealthCheck,
                    performRestoringHealthCheck);

                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    callback => this.UpdateRepairTaskHealthPolicyAsyncBeginWrapper(
                        requestDescription,
                        timeout, 
                        callback),
                    this.UpdateRepairTaskHealthPolicyAsyncEndWrapper,
                    cancellationToken,
                    "RepairManagementClient.UpdateRepairTaskHealthPolicyAsync");
            }

            #endregion

            #region Helpers

            #region CreateRepairTaskAsync

            private NativeCommon.IFabricAsyncOperationContext CreateRepairTaskAsyncBeginWrapper(
                RepairTask repairTask,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeRepairClient.BeginCreateRepairTask(
                        repairTask.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 CreateRepairTaskAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeRepairClient.EndCreateRepairTask(context);
            }

            #endregion

            #region CancelRepairTaskAsync

            private NativeCommon.IFabricAsyncOperationContext CancelRepairTaskAsyncBeginWrapper(
                RepairScopeIdentifier scope,
                string repairTaskId,
                Int64 version,
                bool requestAbort,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var nativeDescription = new NativeTypes.FABRIC_REPAIR_CANCEL_DESCRIPTION()
                    {
                        Scope = scope.ToNative(pin),
                        RepairTaskId = pin.AddBlittable(repairTaskId),
                        Version = version,
                        RequestAbort = NativeTypes.ToBOOLEAN(requestAbort),
                    };

                    return this.nativeRepairClient.BeginCancelRepairTask(
                        pin.AddBlittable(nativeDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 CancelRepairTaskAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeRepairClient.EndCancelRepairTask(context);
            }

            #endregion

            #region ForceApproveRepairTaskAsync

            private NativeCommon.IFabricAsyncOperationContext ForceApproveRepairTaskAsyncBeginWrapper(
                RepairScopeIdentifier scope,
                string repairTaskId,
                Int64 version,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var nativeDescription = new NativeTypes.FABRIC_REPAIR_APPROVE_DESCRIPTION()
                    {
                        Scope = scope.ToNative(pin),
                        RepairTaskId = pin.AddBlittable(repairTaskId),
                        Version = version,
                    };

                    return this.nativeRepairClient.BeginForceApproveRepairTask(
                        pin.AddBlittable(nativeDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 ForceApproveRepairTaskAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeRepairClient.EndForceApproveRepairTask(context);
            }

            #endregion

            #region DeleteRepairTaskAsync

            private NativeCommon.IFabricAsyncOperationContext DeleteRepairTaskAsyncBeginWrapper(
                RepairScopeIdentifier scope,
                string repairTaskId,
                Int64 version,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var nativeDescription = new NativeTypes.FABRIC_REPAIR_DELETE_DESCRIPTION()
                    {
                        Scope = scope.ToNative(pin),
                        RepairTaskId = pin.AddBlittable(repairTaskId),
                        Version = version,
                    };

                    return this.nativeRepairClient.BeginDeleteRepairTask(
                        pin.AddBlittable(nativeDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteRepairTaskAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeRepairClient.EndDeleteRepairTask(context);
            }

            #endregion

            #region UpdateRepairExecutionStateAsync

            private NativeCommon.IFabricAsyncOperationContext UpdateRepairExecutionStateAsyncBeginWrapper(
                RepairTask repairTask,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeRepairClient.BeginUpdateRepairExecutionState(
                        repairTask.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 UpdateRepairExecutionStateAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeRepairClient.EndUpdateRepairExecutionState(context);
            }

            #endregion

            #region GetRepairTaskListAsync

            private NativeCommon.IFabricAsyncOperationContext GetRepairTaskListAsyncBeginWrapper(
                RepairTaskQueryDescription queryDescription,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeRepairClient.BeginGetRepairTaskList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private RepairTaskList GetRepairTaskListAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return RepairTaskList.CreateFromNativeListResult(
                    this.nativeRepairClient.EndGetRepairTaskList(context));
            }

            #endregion

            #region UpdateRepairTaskHealthPolicyAsync

            private NativeCommon.IFabricAsyncOperationContext UpdateRepairTaskHealthPolicyAsyncBeginWrapper(
                RepairTaskHealthPolicyUpdateDescription requestDescription,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeRepairClient.BeginUpdateRepairTaskHealthPolicy(
                        requestDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 UpdateRepairTaskHealthPolicyAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeRepairClient.EndUpdateRepairTaskHealthPolicy(context);
            }

            #endregion

            #endregion
        }
    }
}