// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Chaos.DataStructures;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides methods for issuing and controlling test commands.</para>
        /// </summary>
        public sealed class TestManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricTestManagementClient4 nativeTestManagementClient;
            private readonly NativeClient.IFabricTestManagementClientInternal2 nativeTestManagementClientInternal;

            private const double RequestTimeoutFactor = 0.2;
            private const double MinRequestTimeoutInSeconds = 15.0;
            private static readonly TimeSpan MinRequestTimeout = TimeSpan.FromSeconds(MinRequestTimeoutInSeconds);

            private const double DefaultOperationTimeoutInSeconds = 300.0;
            internal static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromSeconds(DefaultOperationTimeoutInSeconds);

            private FabricTestContext testContext = null;

            private const string TraceType = "TestManagementClient";

            #endregion

            #region Constructor

            internal TestManagementClient(
                FabricClient fabricClient,
                NativeClient.IFabricTestManagementClient4 nativeTestManagementClient,
                NativeClient.IFabricTestManagementClientInternal2 nativeTestManagementClientInternal
                )
            {
                this.fabricClient = fabricClient;
                this.nativeTestManagementClient = nativeTestManagementClient;
                this.nativeTestManagementClientInternal = nativeTestManagementClientInternal;
                this.testContext = new FabricTestContext(this.fabricClient);
            }

            #endregion

            internal FabricTestContext TestContext
            {
                get
                {
                    return this.testContext;
                }
            }

            #region API

            #region API - InvokeDataLoss

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>.
            /// PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Note:  Once this API has been called, it cannot be reversed. Calling CancelTestCommandAsync() will only stop execution and clean up internal system state.
            /// It will not restore data if the command has progressed far enough to cause data loss.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API.</param>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures.
            /// FabricErrorCode.PartitionNotFound - If the specified partition selected does not exist.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionDataLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode)
            {
                return this.StartPartitionDataLossAsync(
                    operationId,
                    partitionSelector,
                    dataLossMode,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of inflight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// <para>
            /// Note:  Once this API has been called, it cannot be reversed.  Calling CancelTestCommandAsync() will only stop execution and clean up internal system state.
            /// It will not restore data if the command has progressed far enough to cause data loss.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionDataLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                CancellationToken cancellationToken)
            {
                return this.StartPartitionDataLossAsync(
                    operationId,
                    partitionSelector,
                    dataLossMode,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// <para>
            /// Note:  Once this API has been called, it cannot be reversed.  Calling CancelTestCommandAsync() will only stop execution and clean up internal system state.
            /// It will not restore data if the command has progressed far enough to cause data loss.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="operationTimeout">The overall timeout for the operation</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionDataLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan operationTimeout)
            {
                return this.StartPartitionDataLossAsync(
                    operationId,
                    partitionSelector,
                    dataLossMode,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// <para>
            /// Note:  Once this API has been called, it cannot be reversed.  Calling CancelTestCommandAsync() will only stop execution and clean up internal system state.
            /// It will not restore data if the command has progressed far enough to cause data loss.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="operationTimeout">The overall timeout for the operation</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionDataLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartPartitionDataLossAsyncHelper(
                                operationId,
                                partitionSelector,
                                dataLossMode,
                                operationTimeout,
                                cancellationToken);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>InvokeDataLossResult which gives information about the Partition that was selected for data loss.</returns>
            [Obsolete("This api is deprecated, use StartPartitionDataLossAsync instead.  StartPartitionDataLossAsync requires the FaultAnalysisService")]
            public Task<InvokeDataLossResult> InvokeDataLossAsync(
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode)
            {
                return this.InvokeDataLossAsync(
                    partitionSelector,
                    dataLossMode,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>InvokeDataLossResult which gives information about the Partition that was selected for data loss.</returns>
            [Obsolete("This api is deprecated, use StartPartitionDataLossAsync instead.  StartPartitionDataLossAsync requires the FaultAnalysisService")]
            public Task<InvokeDataLossResult> InvokeDataLossAsync(
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                CancellationToken cancellationToken)
            {
                return this.InvokeDataLossAsync(
                    partitionSelector,
                    dataLossMode,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="operationTimeout">The overall timeout for the operation</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>InvokeDataLossResult which gives information about the Partition that was selected for data loss.</returns>
            [Obsolete("This api is deprecated, use StartPartitionDataLossAsync instead.  StartPartitionDataLossAsync requires the FaultAnalysisService")]
            public Task<InvokeDataLossResult> InvokeDataLossAsync(
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan operationTimeout)
            {
                return this.InvokeDataLossAsync(
                    partitionSelector,
                    dataLossMode,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will induce data loss for the specified partition. It will trigger a call to the OnDataLoss API of the partition.
            /// </summary>
            /// <remarks>
            /// <para>
            /// Actual data loss will depend on the specified <see cref="System.Fabric.DataLossMode"/>
            /// PartialDataLoss - PartialDataLoss - Only a quorum of replicas are removed and OnDataLoss is triggered for the partition but actual data loss depends on presence of in-flight replication.
            /// FullDataLoss - All replicas are removed hence all data is lost and OnDataLoss is triggered.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector">The <see cref="System.Fabric.PartitionSelector"/> to specify which partition data loss needs to be induced for.</param>
            /// <param name="dataLossMode">Specifies the <see cref="System.Fabric.DataLossMode"/> i.e. the options for inducing data loss.</param>
            /// <param name="operationTimeout">The overall timeout for the operation</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>InvokeDataLossResult which gives information about the Partition that was selected for data loss.</returns>
            [Obsolete("This api is deprecated, use StartPartitionDataLossAsync instead.  StartPartitionDataLossAsync requires the FaultAnalysisService")]
            public Task<InvokeDataLossResult> InvokeDataLossAsync(
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.InvokeDataLossUsingActionAsync(
                                partitionSelector,
                                dataLossMode,
                                operationTimeout,
                                cancellationToken);
            }

            internal Task<NodeList> GetNodeListInternalAsync(string nodeNameFilter, NodeStatusFilter nodeStatusFilter, string continuationToken, bool excludeStoppedNodeInfo, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListInternalAsyncHelper(nodeNameFilter, nodeStatusFilter, continuationToken, excludeStoppedNodeInfo, timeout, cancellationToken);
            }

            private Task<NodeList> GetNodeListInternalAsyncHelper(string nodeNameFilter, NodeStatusFilter nodeStatusFilter, string continuationToken, bool excludeStoppedNodeInfo, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NodeList>(
                    (callback) => this.GetNodeListInternalAsyncBeginWrapper(nodeNameFilter, nodeStatusFilter, continuationToken, excludeStoppedNodeInfo, timeout, callback),
                    this.GetNodeListInternalAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNodeListInternal");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNodeListInternalAsyncBeginWrapper(string nodeNameFilter, NodeStatusFilter nodeStatusFilter, string continuationToken, bool excludeStoppedNodeInfo, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    NodeQueryDescription queryDescription = new NodeQueryDescription();
                    queryDescription.NodeNameFilter = nodeNameFilter;
                    queryDescription.NodeStatusFilter = nodeStatusFilter;

                    if (!string.IsNullOrWhiteSpace(continuationToken))
                    {
                        queryDescription.ContinuationToken = continuationToken;
                    }

                    return this.nativeTestManagementClientInternal.BeginGetNodeListInternal(
                        queryDescription.ToNative(pin),
                        NativeTypes.ToBOOLEAN(excludeStoppedNodeInfo),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NodeList GetNodeListInternalAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NodeList.CreateFromNativeListResult(this.nativeTestManagementClientInternal.EndGetNodeList2Internal(context));

            }

            internal async Task<InvokeDataLossResult> InvokeDataLossUsingActionAsync(
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                InvokeDataLossAction invokeDataLoss = new InvokeDataLossAction(partitionSelector, dataLossMode)
                {
                    ActionTimeout = timeout,
                    RequestTimeout = GetRequestTimeout(timeout)
                };

                await this.testContext.ActionExecutor.RunAsync(invokeDataLoss, cancellationToken);

                return invokeDataLoss.Result;
            }

            #endregion API - InvokeDataLoss

            #region API - UnreliableTransport
            internal Task AddUnreliableTransportAsync(
                string nodeName,
                string behaviorName,
                UnreliableTransportBehavior behavior,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                return this.AddUnreliableTransportBehaviorAsync(nodeName, behaviorName, behavior, operationTimeout, cancellationToken);
            }

            internal Task RemoveUnreliableTransportAsync(
                string nodeName,
                string behaviorName,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                return this.RemoveUnreliableTransportBehaviorAsync(nodeName, behaviorName, operationTimeout, cancellationToken);
            }

            internal Task<IList<string>> GetTransportBehavioursAsync(
                string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var internalClusterClient = this.fabricClient.ClusterManager.InternalNativeClient;
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.BeginGetTransportBehaviorAsyncWrapper(internalClusterClient, nodeName, timeout, callback),
                    (context) => this.EndGetTransportBehaviorAsyncWrapper(internalClusterClient, context),
                    cancellationToken,
                    "ClusterManager.GetTransportBehavioursAsync");
            }
            #endregion API - UnreliableTransport

            #region API - InvokeQuorumLoss

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="operationId"> A user-provided identifier.  This identifier can also be passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionQuorumLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration)
            {
                return this.StartPartitionQuorumLossAsync(
                    operationId,
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="operationId"> A user-provided identifier.  This identifier can also be passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="cancellationToken">The cancellation token for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionQuorumLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration,
                CancellationToken cancellationToken)
            {
                return this.StartPartitionQuorumLossAsync(
                    operationId,
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="operationId"> A user-provided identifier.  This identifier can also be passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="operationTimeout">Overall timeout for the entire operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionQuorumLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration,
                TimeSpan operationTimeout)
            {
                return this.StartPartitionQuorumLossAsync(
                    operationId,
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="operationId"> A user-provided identifier.  This identifier can also be passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumlossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            ///
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="quorumlossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="operationTimeout">Overall timeout for the entire operation.</param>
            /// <param name="cancellationToken">The cancellation token for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionQuorumLossAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumlossMode,
                TimeSpan quorumlossDuration,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartPartitionQuorumLossAsyncHelper(
                                operationId,
                                partitionSelector,
                                quorumlossMode,
                                quorumlossDuration,
                                operationTimeout,
                                cancellationToken);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>InvokeQuorumLossResult <see cref="System.Fabric.Result.InvokeQuorumLossResult"/></returns>
            [Obsolete("This api is deprecated, use StartPartitionQuorumLossAsync instead.  StartPartitionQuorumLossAsync requires the FaultAnalysisService")]
            public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration)
            {
                return this.InvokeQuorumLossAsync(
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="cancellationToken">The cancellation token for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>InvokeQuorumLossResult <see cref="System.Fabric.Result.InvokeQuorumLossResult"/></returns>
            [Obsolete("This api is deprecated, use StartPartitionQuorumLossAsync instead.  StartPartitionQuorumLossAsync requires the FaultAnalysisService")]
            public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration,
                CancellationToken cancellationToken)
            {
                return this.InvokeQuorumLossAsync(
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumLossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="quorumLossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="operationTimeout">Overall timeout for the entire operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>InvokeQuorumLossResult <see cref="System.Fabric.Result.InvokeQuorumLossResult"/></returns>
            [Obsolete("This api is deprecated, use StartPartitionQuorumLossAsync instead.  StartPartitionQuorumLossAsync requires the FaultAnalysisService")]
            public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration,
                TimeSpan operationTimeout)
            {
                return this.InvokeQuorumLossAsync(
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>Induces quorum loss for a given stateful service partition. </summary>
            /// <param name="partitionSelector">Partition which the quorum loss will be invoked. <see cref="System.Fabric.PartitionSelector"/></param>
            /// <param name="quorumlossMode">PartialQuorumLoss or FullQuorumLoss.</param>
            /// <remarks>
            /// <para>
            /// FullQuorumLoss - All replicas for the target partition will be downed.
            /// PartialQuorumLoss - A quorum of replicas for the target partition will be downed..
            /// </para>
            /// <para>
            /// quorumLossMode indicates the number of replicas that will be faulted in order to cause quorum loss. The partition will remain in quorum loss for quorumLossDuration.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Calling this API with a system service as the target is not advised.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="quorumlossDuration">Amount of time for which the partition will be kept in quorum loss.</param>
            /// <param name="operationTimeout">Overall timeout for the entire operation.</param>
            /// <param name="cancellationToken">The cancellation token for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.OperationCanceledException" >Async operation is canceled.</exception>
            /// <exception cref= "InvalidOperationException" > Partition specified is not a part of a stateful Persisted Service.</exception>
            /// <returns>InvokeQuorumLossResult <see cref="System.Fabric.Result.InvokeQuorumLossResult"/></returns>
            [Obsolete("This api is deprecated, use StartPartitionQuorumLossAsync instead.  StartPartitionQuorumLossAsync requires the FaultAnalysisService")]
            public Task<InvokeQuorumLossResult> InvokeQuorumLossAsync(
                PartitionSelector partitionSelector,
                QuorumLossMode quorumlossMode,
                TimeSpan quorumlossDuration,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.InvokeQuorumLossUsingActionAsync(
                                partitionSelector,
                                quorumlossMode,
                                quorumlossDuration,
                                operationTimeout,
                                cancellationToken);
            }

            internal async Task<InvokeQuorumLossResult> InvokeQuorumLossUsingActionAsync(
                PartitionSelector partitionSelector,
                QuorumLossMode quorumlossMode,
                TimeSpan quorumlossDuration,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                InvokeQuorumLossAction invokeQuorumLoss = new InvokeQuorumLossAction(partitionSelector, quorumlossMode, quorumlossDuration);
                invokeQuorumLoss.ActionTimeout = timeout;
                invokeQuorumLoss.RequestTimeout = GetRequestTimeout(timeout);

                await this.testContext.ActionExecutor.RunAsync(invokeQuorumLoss, cancellationToken);

                return invokeQuorumLoss.Result;
            }

            #endregion API - InvokeQuorumLoss

            #region API - RestartPartition

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the
            /// <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API may be called on both stateful and stateless services.
            /// If the call is on a stateless service, RestartPartitionMode must be RestartPartitionMode.AllReplicasOrInstances.  Other modes will result in ArgumentException inside the returned Result object
            /// when GetPartitionRestartProgressAsync() is called.  See GetPartitionRestartProgressAsync().
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.ArgumentException"  >The input was invalid.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionRestartAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode)
            {
                return this.StartPartitionRestartAsync(
                    operationId,
                    partitionSelector,
                    restartPartitionMode,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API may be called on both stateful and stateless services.
            /// If the call is on a stateless service, RestartPartitionMode must be RestartPartitionMode.AllReplicasOrInstances.  Other modes will result in ArgumentException inside the returned Result object
            /// when GetPartitionRestartProgressAsync() is called.  See GetPartitionRestartProgressAsync().
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.ArgumentException"  >The input was invalid.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionRestartAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                CancellationToken cancellationToken)
            {
                return this.StartPartitionRestartAsync(
                    operationId,
                    partitionSelector,
                    restartPartitionMode,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API may be called on both stateful and stateless services.
            /// If the call is on a stateless service, RestartPartitionMode must be RestartPartitionMode.AllReplicasOrInstances.  Other modes will result in ArgumentException inside the returned Result object
            /// when GetPartitionRestartProgressAsync() is called.  See GetPartitionRestartProgressAsync().
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.ArgumentException"  >The input was invalid.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionRestartAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan operationTimeout)
            {
                return this.StartPartitionRestartAsync(
                    operationId,
                    partitionSelector,
                    restartPartitionMode,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API may be called on both stateful and stateless services.
            /// If the call is on a stateless service, RestartPartitionMode must be RestartPartitionMode.AllReplicasOrInstances.  Other modes will result in ArgumentException inside the returned Result object
            /// when GetPartitionRestartProgressAsync() is called.  See GetPartitionRestartProgressAsync().
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationId"> A GUID that identifies a call of this API; this is passed into the corresponding GetProgress API</param>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.ArgumentException"  >The input was invalid.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>A task.</returns>
            public Task StartPartitionRestartAsync(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartPartitionRestartAsyncHelper(
                                operationId,
                                partitionSelector,
                                restartPartitionMode,
                                operationTimeout,
                                cancellationToken);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service with <see cref="System.Fabric.RestartPartitionMode"/> set to OnlyActiveSecondaries.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RestartPartitionResult which gives information about the actual selected partition.</returns>
            [Obsolete("This api is deprecated, use StartPartitionRestartAsync instead.  StartPartitionRestartAsync requires the FaultAnalysisService")]
            public Task<RestartPartitionResult> RestartPartitionAsync(
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode)
            {
                return this.RestartPartitionAsync(
                    partitionSelector,
                    restartPartitionMode,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service with <see cref="System.Fabric.RestartPartitionMode"/> set to OnlyActiveSecondaries.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>RestartPartitionResult which gives information about the actual selected partition</returns>
            [Obsolete("This api is deprecated, use StartPartitionRestartAsync instead.  StartPartitionRestartAsync requires the FaultAnalysisService")]
            public Task<RestartPartitionResult> RestartPartitionAsync(
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                CancellationToken cancellationToken)
            {
                return this.RestartPartitionAsync(
                    partitionSelector,
                    restartPartitionMode,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service with <see cref="System.Fabric.RestartPartitionMode"/> set to OnlyActiveSecondaries.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>RestartPartitionResult which gives information about the actual selected partition.</returns>
            [Obsolete("This api is deprecated, use StartPartitionRestartAsync instead.  StartPartitionRestartAsync requires the FaultAnalysisService")]
            public Task<RestartPartitionResult> RestartPartitionAsync(
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan operationTimeout)
            {
                return this.RestartPartitionAsync(
                    partitionSelector,
                    restartPartitionMode,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// This API will restart some or all the replicas of a partition at the same time (ensures all the replicas are down concurrently) depending on the <see cref="System.Fabric.RestartPartitionMode"/>.
            /// </summary>
            /// <remarks>
            /// <para>
            /// This API is useful to test the recovery time of a partition after a full or partial restart and also to test failover.
            /// </para>
            /// <para>
            /// This API should only be called with a stateful service as the target.
            /// </para>
            /// <para>
            /// Important note:  this API should not be aborted while running.  Aborting this API while it is running may leave state behind.
            /// If this API is aborted while running, CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// </remarks>
            /// <param name="partitionSelector"><see cref="System.Fabric.PartitionSelector"/> that specifies the partition which needs to be restarted.</param>
            /// <param name="restartPartitionMode">The <see cref="System.Fabric.RestartPartitionMode"/> which can be AllReplicasOrInstances or OnlyActiveSecondaries based on which the replicas to be restarted
            /// are selected.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.InvalidOperationException" >If the API is called for a partition belonging to a stateless service with <see cref="System.Fabric.RestartPartitionMode"/> set to OnlyActiveSecondaries.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>RestartPartitionResult which gives information about the actual selected partition.</returns>
            [Obsolete("This api is deprecated, use StartPartitionRestartAsync instead.  StartPartitionRestartAsync requires the FaultAnalysisService")]
            public Task<RestartPartitionResult> RestartPartitionAsync(
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.RestartPartitionUsingActionAsync(
                                partitionSelector,
                                restartPartitionMode,
                                operationTimeout,
                                cancellationToken);
            }

            internal async Task<RestartPartitionResult> RestartPartitionUsingActionAsync(
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                RestartPartitionAction restartPartitionAction = new RestartPartitionAction(partitionSelector, restartPartitionMode);
                restartPartitionAction.ActionTimeout = timeout;
                restartPartitionAction.RequestTimeout = GetRequestTimeout(timeout);

                await this.testContext.ActionExecutor.RunAsync(restartPartitionAction, cancellationToken);

                return restartPartitionAction.Result;
            }

            #endregion API - RestartPartition

            #region API - GetInvokeDataLossProgress

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionDataLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionDataLossAsync().</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionDataLossProgress.State == Faulted, examine PartitionDataLossProgress.Result.Exception to determine why.
            /// PartitionDataLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionDataLossProgress object, containing TestCommandProgressState and PartitionDataLossResult.</returns>
            public Task<PartitionDataLossProgress> GetPartitionDataLossProgressAsync(Guid operationId)
            {
                return this.GetPartitionDataLossProgressAsync(operationId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionDataLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionDataLossAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionDataLossProgress.State == Faulted, examine PartitionDataLossProgress.Result.Exception to determine why.
            /// PartitionDataLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionDataLossProgress object, containing TestCommandProgressState and PartitionDataLossResult.</returns>
            public Task<PartitionDataLossProgress> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout)
            {
                return this.GetPartitionDataLossProgressAsync(operationId, timeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionDataLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionDataLossAsync().</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionDataLossProgress.State == Faulted, examine PartitionDataLossProgress.Result.Exception to determine why.
            /// PartitionDataLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionDataLossProgress object, containing TestCommandProgressState and PartitionDataLossResult.</returns>
            public Task<PartitionDataLossProgress> GetPartitionDataLossProgressAsync(Guid operationId, CancellationToken cancellationToken)
            {
                return this.GetPartitionDataLossProgressAsync(operationId, FabricClient.DefaultTimeout, cancellationToken);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionDataLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionDataLossAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionDataLossProgress.State == Faulted, examine PartitionDataLossProgress.Result.Exception to determine why.
            /// PartitionDataLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionDataLossProgress object, containing TestCommandProgressState and PartitionDataLossResult.</returns>
            public Task<PartitionDataLossProgress> GetPartitionDataLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetPartitionDataLossProgressAsyncHelper(operationId, timeout, cancellationToken);
            }

            #endregion API - GetInvokeDataLossProgress

            #region API - GetInvokeQuorumLossProgress

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionQuorumLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionQuorumLossAsync().</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>
            /// If the returned PartitionQuorumLossProgress.State == Faulted, examine PartitionQuorumLossProgress.Result.Exception to determine why.
            /// PartitionQuorumLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            ///     - FabricOnlyValidForStatefulPersistentServicesException - this operation is not valid for stateful in-memory services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionQuorumLossProgress object, containing TestCommandProgressState and PartitionQuorumLossResult.</returns>
            public Task<PartitionQuorumLossProgress> GetPartitionQuorumLossProgressAsync(Guid operationId)
            {
                return this.GetPartitionQuorumLossProgressAsync(operationId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionQuorumLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionQuorumLossAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>
            /// If the returned PartitionQuorumLossProgress.State == Faulted, examine PartitionQuorumLossProgress.Result.Exception to determine why.
            /// PartitionQuorumLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            ///     - FabricOnlyValidForStatefulPersistentServicesException - this operation is not valid for stateful in-memory services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionQuorumLossProgress object, containing TestCommandProgressState and PartitionQuorumLossResult.</returns>
            public Task<PartitionQuorumLossProgress> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout)
            {
                return this.GetPartitionQuorumLossProgressAsync(operationId, timeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionQuorumLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionQuorumLossAsync().</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>
            /// If the returned PartitionQuorumLossProgress.State == Faulted, examine PartitionQuorumLossProgress.Result.Exception to determine why.
            /// PartitionQuorumLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            ///     - FabricOnlyValidForStatefulPersistentServicesException - this operation is not valid for stateful in-memory services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionQuorumLossProgress object, containing TestCommandProgressState and PartitionQuorumLossResult.</returns>
            public Task<PartitionQuorumLossProgress> GetPartitionQuorumLossProgressAsync(Guid operationId, CancellationToken cancellationToken)
            {
                return this.GetPartitionQuorumLossProgressAsync(operationId, FabricClient.DefaultTimeout, cancellationToken);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionQuorumLossAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionQuorumLossAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>
            /// If the returned PartitionQuorumLossProgress.State == Faulted, examine PartitionQuorumLossProgress.Result.Exception to determine why.
            /// PartitionQuorumLossProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            ///     - FabricInvalidForStatelessServicesException - this operation is not valid for stateless services.
            ///     - FabricOnlyValidForStatefulPersistentServicesException - this operation is not valid for stateful in-memory services.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionQuorumLossProgress object, containing TestCommandProgressState and PartitionQuorumLossResult.</returns>
            public Task<PartitionQuorumLossProgress> GetPartitionQuorumLossProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetPartitionQuorumLossProgressAsyncHelper(operationId, timeout, cancellationToken);
            }

            #endregion API - GetInvokeQuorumLossProgress

            #region API - GetRestartPartitionProgress

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionRestartAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionRestartAsync().</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionRestartProgress.State == Faulted, examine PartitionRestartProgress.Result.Exception to determine why.
            /// PartitionRestartProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionRestartProgress object, containing TestCommandProgressState and PartitionRestartResult.</returns>
            public Task<PartitionRestartProgress> GetPartitionRestartProgressAsync(Guid operationId)
            {
                return this.GetPartitionRestartProgressAsync(operationId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionRestartAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionRestartAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionRestartProgress.State == Faulted, examine PartitionRestartProgress.Result.Exception to determine why.
            /// PartitionRestartProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionRestartProgress object, containing TestCommandProgressState and PartitionRestartResult.</returns>
            public Task<PartitionRestartProgress> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout)
            {
                return this.GetPartitionRestartProgressAsync(operationId, timeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionRestartAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionRestartAsync().</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionRestartProgress.State == Faulted, examine PartitionRestartProgress.Result.Exception to determine why.
            /// PartitionRestartProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionRestartProgress object, containing TestCommandProgressState and PartitionRestartResult.</returns>
            public Task<PartitionRestartProgress> GetPartitionRestartProgressAsync(Guid operationId, CancellationToken cancellationToken)
            {
                return this.GetPartitionRestartProgressAsync(operationId, FabricClient.DefaultTimeout, cancellationToken);
            }

            /// <summary>
            /// Gets the progress of a test command started using StartPartitionRestartAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was starting using StartPartitionRestartAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>
            /// <para>The FaultAnalysisService must be enabled to use this API.</para>
            /// <para>If the returned PartitionRestartProgress.State == Faulted, examine PartitionRestartProgress.Result.Exception to determine why.
            /// PartitionRestartProgress.Result.Exception values:
            ///   - ArgumentException - the input was invalid.
            ///   - FabricException, with an ErrorCode property of:
            ///     - PartitionNotFound - the specified partition was not found, or is not a partition that belongs to the specified service.
            /// </para>
            /// </remarks>
            /// <returns>A PartitionRestartProgress object, containing TestCommandProgressState and PartitionRestartResult.</returns>
            public Task<PartitionRestartProgress> GetPartitionRestartProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetPartitionRestartProgressAsyncHelper(operationId, timeout, cancellationToken);
            }

            #endregion API - GetRestartPartitionProgress

            #region API - GetTestCommandStatusList

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="operationTimeout">A timeout for the API call.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    TestCommandStateFilter.All,
                    TestCommandTypeFilter.All,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    TestCommandStateFilter.All,
                    TestCommandTypeFilter.All,
                    DefaultOperationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="operationTimeout">A timeout for the API call.</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                TimeSpan operationTimeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    TestCommandStateFilter.All,
                    TestCommandTypeFilter.All,
                    operationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="stateFilter">This parameter can be used to filter by TestCommandState</param>
            /// <param name="operationTimeout">A timeout for the API call.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                TestCommandStateFilter stateFilter,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    stateFilter,
                    TestCommandTypeFilter.All,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="typeFilter">This parameter can be used to filter by TestCommandType</param>
            /// <param name="operationTimeout">A timeout for the API call.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                TestCommandTypeFilter typeFilter,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    TestCommandStateFilter.All,
                    typeFilter,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Gets the status of test commands.
            /// </summary>
            /// <param name="stateFilter">This parameter can be used to filter by TestCommandState</param>
            /// <param name="typeFilter">This parameter can be used to filter by TestCommandType</param>
            /// <param name="operationTimeout">A timeout for the API call.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A TestCommandStatusList, which is an IList of TestCommandStatus objects</returns>
            public Task<TestCommandStatusList> GetTestCommandStatusListAsync(
                TestCommandStateFilter stateFilter,
                TestCommandTypeFilter typeFilter,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetTestCommandStatusListAsyncHelper(
                    stateFilter,
                    typeFilter,
                    operationTimeout,
                    cancellationToken);
            }

            #endregion

            #region API - CancelTestCommand

            /// <summary>
            /// Cancels a test command.
            /// </summary>
            /// <param name="operationId">Indicates the operationId of the test command to cancel.</param>
            /// <param name="force">Indicates whether to gracefully rollback and clean up internal system state modified by executing the test command.  See Remarks.</param>
            /// <returns>A Task.</returns>
            /// <remarks>
            /// <para>
            /// If force is false, then the specified test command will be gracefully stopped and cleaned up.  If force is true, the command will be aborted, and some internal state
            /// may be left behind.  Specifying force as true should be used with care.  Calling CancelTestCommandAsync() with force set to true is not allowed until CancelTestCommandAsync() has
            /// been called on the same test command with force set to false first, or unless the test command already has a TestCommandProgressState of TestCommandProgressState.RollingBack.
            /// Clarification: TestCommandProgressState.RollingBack means that the system will/is cleaning up internal system state caused by executing the command.  It will not restore data if the
            /// test command was to cause data loss.  For example, if you call StartPartitionDataLossAsync() then call CancelTestCommandAsync() the system will only clean up internal state from running the command.
            /// It will not restore the target partition's data, if the command progressed far enough to cause data loss.
            ///
            ///
            /// </para>
            ///
            /// <para>
            /// Important note:  if this API is invoked with force==true, internal state may be left behind.  CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            public Task CancelTestCommandAsync(
                Guid operationId,
                bool force)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.CancelTestCommandAsync(operationId, force, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Cancels a test command.
            /// </summary>
            /// <param name="operationId">Indicates the operationId of the test command to cancel.</param>
            /// <param name="force">Indicates whether to gracefully rollback and clean up internal system state modified by executing the test command.  See Remarks.</param>
            /// <param name="timeout">The timeout to use for the API call.</param>
            /// <returns>A Task.</returns>
            /// <remarks>
            /// <para>
            /// If force is false, then the specified test command will be gracefully stopped and cleaned up.  If force is true, the command will be aborted, and some internal state
            /// may be left behind.  Specifying force as true should be used with care.  Calling CancelTestCommandAsync() with force set to true is not allowed until CancelTestCommandAsync() has
            /// been called on the same test command with force set to false first, or unless the test command already has a TestCommandProgressState of TestCommandProgressState.RollingBack.
            /// Clarification: TestCommandProgressState.RollingBack means that the system will/is cleaning up internal system state caused by executing the command.  It will not restore data if the
            /// test command was to cause data loss.  For example, if you call StartPartitionDataLossAsync() then call CancelTestCommandAsync() the system will only clean up internal state from running the command.
            /// It will not restore the target partition's data, if the command progressed far enough to cause data loss.
            ///
            ///
            ///
            /// </para>
            ///
            /// <para>
            /// Important note:  if this API is invoked with force==true, internal state may be left behind.  CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            public Task CancelTestCommandAsync(
                Guid operationId,
                bool force,
                TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.CancelTestCommandAsync(operationId, force, timeout, CancellationToken.None);
            }


            /// <summary>
            /// Cancels a test command.
            /// </summary>
            /// <param name="operationId">Indicates the operationId of the test command to cancel.</param>
            /// <param name="force">Indicates whether to gracefully rollback and clean up internal system state modified by executing the test command.  See Remarks.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <returns>A Task.</returns>
            /// <remarks>
            /// <para>
            /// If force is false, then the specified test command will be gracefully stopped and cleaned up.  If force is true, the command will be aborted, and some internal state
            /// may be left behind.  Specifying force as true should be used with care.  Calling CancelTestCommandAsync() with force set to true is not allowed until CancelTestCommandAsync() has
            /// been called on the same test command with force set to false first, or unless the test command already has a TestCommandProgressState of TestCommandProgressState.RollingBack.
            /// Clarification: TestCommandProgressState.RollingBack means that the system will/is cleaning up internal system state caused by executing the command.  It will not restore data if the
            /// test command was to cause data loss.  For example, if you call StartPartitionDataLossAsync() then call CancelTestCommandAsync() the system will only clean up internal state from running the command.
            /// It will not restore the target partition's data, if the command progressed far enough to cause data loss.
            ///
            ///
            ///
            /// </para>
            ///
            /// <para>
            /// Important note:  if this API is invoked with force==true, internal state may be left behind.  CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            public Task CancelTestCommandAsync(
                Guid operationId,
                bool force,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.CancelTestCommandAsync(operationId, force, FabricClient.DefaultTimeout, cancellationToken);
            }

            /// <summary>
            /// Cancels a test command.
            /// </summary>
            /// <param name="operationId">Indicates the operationId of the test command to cancel.</param>
            /// <param name="force">Indicates whether to gracefully rollback and clean up internal system state modified by executing the test command.  See Remarks.</param>
            /// <param name="timeout">The timeout to use for the API call.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <returns>A Task.</returns>
            /// <remarks>
            /// <para>
            /// If force is false, then the specified test command will be gracefully stopped and cleaned up.  If force is true, the command will be aborted, and some internal state
            /// may be left behind.  Specifying force as true should be used with care.  Calling CancelTestCommandAsync() with force set to true is not allowed until CancelTestCommandAsync() has
            /// been called on the same test command with force set to false first, or unless the test command already has a TestCommandProgressState of TestCommandProgressState.RollingBack.
            /// Clarification: TestCommandProgressState.RollingBack means that the system will/is cleaning up internal system state caused by executing the command.  It will not restore data if the
            /// test command was to cause data loss.  For example, if you call StartPartitionDataLossAsync() then call CancelTestCommandAsync() the system will only clean up internal state from running the command.
            /// It will not restore the target partition's data, if the command progressed far enough to cause data loss.
            ///
            ///
            ///
            /// </para>
            ///
            /// <para>
            /// Important note:  if this API is invoked with force==true, internal state may be left behind.  CleanTestStateAsync() should be invoked to remove state that may have been left behind.
            /// </para>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            public Task CancelTestCommandAsync(
                Guid operationId,
                bool force,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.CancelTestCommandAsyncHelper(
                    operationId,
                    force,
                    timeout,
                    cancellationToken);
            }

            #endregion

            /// <summary>
            /// Starts or stops a cluster node.  A cluster node is a process, not the OS instance itself.  To start a node, pass in an object of type NodeStartDescription into
            /// the description parameter.  To stop a node, pass in an object of type NodeStopDescription.  After this API returns, call GetNodeTransitionProgressAsync()
            /// to get progress on the operation.
            /// </summary>
            /// <param name="description">An object which describes what type of node transition to perform.  The transition can be to start or stop a node.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellationToken</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A task</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the errorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            public Task StartNodeTransitionAsync(
                NodeTransitionDescription description,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartNodeTransitionAsyncHelper(
                     description,
                     operationTimeout,
                     token);
            }

            /// <summary>
            /// Gets the progress of a command started using StartNodeTransitionAsync().
            /// </summary>
            /// <param name="operationId">The operationId passed in when the test command was started using StartNodeTransitionAsync().</param>
            /// <param name="timeout">Timeout.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes</param>
            /// <remarks>The FaultAnalysisService must be enabled to use this API.</remarks>
            /// <returns>A PartitionRestartProgress object, containing TestCommandProgressState and PartitionRestartResult.</returns>
            public Task<NodeTransitionProgress> GetNodeTransitionProgressAsync(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeTransitionProgressAsyncHelper(operationId, timeout, cancellationToken);
            }

            #region API - CleanTestState
            /// <summary>
            /// Cleans up all the test state in the cluster.
            /// </summary>
            /// <remarks>
            /// Cleans up all the test state in the cluster which has been set for fault operations; like StopNode, InvokeDataLoss, RestartPartition and InvokeQuorumLoss
            /// This API should be called if any of these operations fail or if the test driver process dies or an operation is canceled while in flight to ensure that
            /// the cluster is back into the normal state. Normally all the fault operations clean up their state at the end of the execution of the API so CleanTestState only
            /// needs to be called if the API operation is interrupted.
            /// </remarks>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <returns>Task</returns>
            public Task CleanTestStateAsync()
            {
                return this.CleanTestStateAsync(DefaultOperationTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Cleans up all the test state in the cluster.
            /// </summary>
            /// <remarks>
            /// Cleans up all the test state in the cluster which has been set for fault operations, InvokeDataLoss, RestartPartition and InvokeQuorumLoss
            /// This API should be called if any of these operations fail or if the test driver process dies or an operation is canceled while in flight to ensure that
            /// the cluster is back into the normal state. Normally all the fault operations clean up their state at the end of the execution of the API so CleanTestState only
            /// needs to be called if the API operation is interrupted .
            /// </remarks>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <returns>Task</returns>
            public Task CleanTestStateAsync(
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                CleanTestStateAction cleanTestState = new CleanTestStateAction
                {
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                return this.testContext.ActionExecutor.RunAsync(cleanTestState, token);
            }
            #endregion APIs - CleanTestState

            #region API - Validate

            /// <summary>
            /// This API will validate the availability and health of all services in the specified application.
            /// </summary>
            /// <param name="applicationName">Name of the application whose services need to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the services to stabilize else fail the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateApplicationAsync(
                Uri applicationName,
                TimeSpan maximumStabilizationTimeout)
            {
                return this.ValidateApplicationAsync(applicationName, maximumStabilizationTimeout, CancellationToken.None);
            }

            /// <summary>
            /// This API will validate the availability and health of all services in the specified application.
            /// </summary>
            /// <param name="applicationName">Name of the application whose services need to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the services to stabilize else fail the operation.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateApplicationAsync(
                Uri applicationName,
                TimeSpan maximumStabilizationTimeout,
                CancellationToken token)
            {
                return this.ValidateApplicationAsync(applicationName, maximumStabilizationTimeout, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// This API will validate the availability and health of all services in the specified application.
            /// </summary>
            /// <param name="applicationName">Name of the application whose services need to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the services to stabilize else fail the operation.</param>
            /// <param name="operationTimeout">Amount of time to wait for an operation to complete else fail the operation.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateApplicationAsync(
                Uri applicationName,
                TimeSpan maximumStabilizationTimeout,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                var requestTimeout = GetRequestTimeout(operationTimeout);

                ValidateApplicationServicesAction validateApplicationAction
                    = new ValidateApplicationServicesAction(applicationName, maximumStabilizationTimeout);
                validateApplicationAction.ActionTimeout = operationTimeout;
                validateApplicationAction.RequestTimeout = requestTimeout;

                return this.testContext.ActionExecutor.RunAsync(validateApplicationAction, token);
            }

            /// <summary>
            /// This API will validate the availability and health of the specified service.
            /// </summary>
            /// <param name="serviceName">Name of the service that needs to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the service to stabilize else fail the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateServiceAsync(
                Uri serviceName,
                TimeSpan maximumStabilizationTimeout)
            {
                return this.ValidateServiceAsync(serviceName, maximumStabilizationTimeout, CancellationToken.None);
            }

            /// <summary>
            /// This API will validate the availability and health of the specified service.
            /// </summary>
            /// <param name="serviceName">Name of the service that needs to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the service to stabilize else fail the operation.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateServiceAsync(
                Uri serviceName,
                TimeSpan maximumStabilizationTimeout,
                CancellationToken token)
            {
                return this.ValidateServiceAsync(serviceName, maximumStabilizationTimeout, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// This API will validate the availability and health of the specified service.
            /// </summary>
            /// <param name="serviceName">Name of the service that needs to be validated.</param>
            /// <param name="maximumStabilizationTimeout">Max amount of time to wait for the service to stabilize else fail the operation.</param>
            /// <param name="operationTimeout">Amount of time to wait for an operation to complete else fail the operation.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricValidationException" >If any service does not stabilize within the specified timeout.</exception>
            /// <returns>Task</returns>
            public Task ValidateServiceAsync(
                Uri serviceName,
                TimeSpan maximumStabilizationTimeout,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                var requestTimeout = GetRequestTimeout(operationTimeout);

                ValidateServiceAction validateServiceAction
                    = new ValidateServiceAction(serviceName, maximumStabilizationTimeout);

                validateServiceAction.ActionTimeout = operationTimeout;
                validateServiceAction.RequestTimeout = requestTimeout;

                return this.testContext.ActionExecutor.RunAsync(validateServiceAction, token);
            }

            #endregion API - Validate

            #region API - UnreliableTransportBehavior
            internal Task AddUnreliableTransportBehaviorAsync(
                string nodeName,
                string behaviorName,
                UnreliableTransportBehavior behavior)
            {
                return this.AddUnreliableTransportBehaviorAsync(nodeName, behaviorName, behavior, GetRequestTimeout(DefaultOperationTimeout), CancellationToken.None);
            }

            internal Task AddUnreliableTransportBehaviorAsync(
                string nodeName,
                string behaviorName,
                UnreliableTransportBehavior behavior,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.AddUnreliableTransportBehaviorAsyncHelper(nodeName, behaviorName, behavior, timeout, cancellationToken);
            }

            internal Task RemoveUnreliableTransportBehaviorAsync(
                string nodeName,
                string behaviorName)
            {
                return this.RemoveUnreliableTransportBehaviorAsync(nodeName, behaviorName, GetRequestTimeout(DefaultOperationTimeout), CancellationToken.None);
            }

            internal Task RemoveUnreliableTransportBehaviorAsync(
                string nodeName,
                string behaviorName,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.RemoveUnreliableTransportBehaviorAsyncHelper(nodeName, behaviorName, timeout, cancellationToken);
            }

            #endregion API - UnreliableTransportBehavior

            #region API - UnreliableLeaseBehavior
            internal Task AddUnreliableLeaseBehaviorAsync(
                string nodeName,
                string behaviorString,
                string alias)
            {
                return this.AddUnreliableLeaseBehaviorAsync(nodeName, behaviorString, alias, GetRequestTimeout(DefaultOperationTimeout), CancellationToken.None);
            }

            internal Task AddUnreliableLeaseBehaviorAsync(
                string nodeName,
                string behaviorString,
                string alias,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.AddUnreliableLeaseBehaviorAsyncHelper(nodeName, behaviorString, alias, timeout, cancellationToken);
            }

            internal Task RemoveUnreliableLeaseBehaviorAsync(
                string nodeName,
                string alias)
            {
                return this.RemoveUnreliableLeaseBehaviorAsync(nodeName, alias, GetRequestTimeout(DefaultOperationTimeout), CancellationToken.None);
            }

            internal Task RemoveUnreliableLeaseBehaviorAsync(
                string nodeName,
                string alias,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.RemoveUnreliableLeaseBehaviorAsyncHelper(nodeName, alias, timeout, cancellationToken);
            }

            #endregion API - UnreliableTransportBehavior

            #region API -  Chaos

            #region GetChaos

            /// <summary>
            /// Gets a description of the state of Chaos.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationTimeout"> The overall timeout for the operation.</param>
            /// <param name="cancellationToken"> Cancellation token.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <returns>A description of the state of Chaos.</returns>
            public Task<ChaosDescription> GetChaosAsync(
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetChaosAsyncHelper(operationTimeout, cancellationToken);
            }

            #endregion

            #region StartChaos

            /// <summary>
            /// This API will start Chaos with the supplied parameter values.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="chaosParameters"> Contains various parameters for controlling Chaos; e.g., time to run, maximum number of concurrent faults, etc.</param>
            /// <param name="cancellationToken"> Cancellation token.</param>
            /// <param name="operationTimeout"> The overall timeout for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricChaosAlreadyRunningException" >This exception is thrown when StartChaos API is invoked while Chaos is already running in the cluster.</exception>
            /// <returns>A description of the state of Chaos.</returns>
            public Task StartChaosAsync(
                ChaosParameters chaosParameters,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.StartChaosAsyncHelper(
                    chaosParameters,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will start Chaos with the supplied parameter values.
            /// </summary>
            /// <param name="chaosParameters"> <see cref="System.Fabric.Chaos.DataStructures.ChaosParameters"/> contains various parameters for controlling Chaos; e.g., time to run, maximum number of concurrent fautls, etc. </param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricChaosAlreadyRunningException" >This exception is thrown when StartChaos API is invoked while Chaos is already running in the cluster.</exception>
            /// <returns>A task.</returns>
            public Task StartChaosAsync(ChaosParameters chaosParameters)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.StartChaosAsyncHelper(
                    chaosParameters,
                    DefaultOperationTimeout,
                    CancellationToken.None);
            }

            #endregion StartChaos

            #region StopChaos

            /// <summary>
            /// This API will stop Chaos.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="operationTimeout"> The overall timeout for the operation.</param>
            /// <param name="cancellationToken"> Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <returns>A task.</returns>
            public Task StopChaosAsync(
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.StopChaosAsyncHelper(
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// This API will stop Chaos.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <returns>A task.</returns>
            public Task StopChaosAsync()
            {
                this.fabricClient.ThrowIfDisposed();
                return this.StopChaosAsyncHelper(
                    DefaultOperationTimeout,
                    CancellationToken.None);
            }

            #endregion StopChaos

            #region GetChaosEvents

            /// <summary>
            /// Retrieves a history of Chaos events. The events to be returned can be filtered based on time of occurrence. When no filter is defined, all events will be returned.
            /// </summary>
            /// <param name="filter">Filter for the lsit of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>
            /// to be included in the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegment"/>.</param>
            /// <param name="maxResults">Maximum number of ChaosEvents in the history to be returned.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <returns>A segment of the history of Chaos events.</returns>
            public Task<ChaosEventsSegment> GetChaosEventsAsync(
                ChaosEventsSegmentFilter filter,
                long maxResults,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                Requires.Argument<ChaosEventsSegmentFilter>("ChaosEventsSegmentFilter", filter).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosEventsAsync(
                    filter,
                    null,
                    maxResults,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Retrieves a segment of the history of Chaos events.
            /// </summary>
            /// <param name="continuationToken">Continuation token for the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>s in the <see cref="System.Fabric.Chaos.DataStructures.ChaosEventsSegment"/>.</param>
            /// <param name="maxResults">Maximum number of ChaosEvents in the history to be returned.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes.</param>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <returns>A segment of the history of Chaos events.</returns>
            public Task<ChaosEventsSegment> GetChaosEventsAsync(
                string continuationToken,
                long maxResults,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                Requires.Argument<string>("continuationToken", continuationToken).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosEventsAsync(
                    new ChaosEventsSegmentFilter(),
                    continuationToken,
                    maxResults,
                    operationTimeout,
                    cancellationToken);
            }

            internal Task<ChaosEventsSegment> GetChaosEventsAsync(
                ChaosEventsSegmentFilter filter,
                string continuationToken,
                long maxResults,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                return this.GetChaosEventsAsyncHelper(
                    filter,
                    continuationToken,
                    maxResults,
                    operationTimeout,
                    cancellationToken);
            }

            #endregion

            #region GetChaosSchedule

            /// <summary>
            /// Gets the description of the Chaos schedule.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="cancellationToken"> Cancellation token.</param>
            /// <param name="operationTimeout"> The overall timeout for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <returns>A description of the Chaos schedule.</returns>
            public Task<ChaosScheduleDescription> GetChaosScheduleAsync(
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetChaosScheduleAsyncHelper(operationTimeout, cancellationToken);
            }

            #endregion

            #region SetChaosSchedule

            /// <summary>
            /// This API will set a description of the Chaos Schedule.
            /// </summary>
            /// <remarks>
            /// <para>
            /// The FaultAnalysisService must be enabled to use this API.
            /// </para>
            /// </remarks>
            /// <param name="schedule"> Chaos Schedule description to be set.</param>
            /// <param name="cancellationToken"> Cancellation token.</param>
            /// <param name="operationTimeout"> The overall timeout for the operation.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <returns>A task.</returns>
            public Task SetChaosScheduleAsync(
                ChaosScheduleDescription schedule,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                Requires.Argument<ChaosScheduleDescription>("ChaosScheduleDescription", schedule).NotNull();

                this.fabricClient.ThrowIfDisposed();

                return this.SetChaosScheduleAsyncHelper(schedule, operationTimeout, cancellationToken);
            }

            #endregion

            #region GetChaosReport

            /// <summary>
            /// Retrieves the report of Chaos runs.
            /// </summary>
            /// <param name="filter">Filter for the <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>s to be included in the report.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <returns>Report of Chaos runs.</returns>
            public Task<ChaosReport> GetChaosReportAsync(
                ChaosReportFilter filter)
            {
                Requires.Argument<ChaosReportFilter>("ChaosReportFilter", filter).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosReportAsync(
                    filter,
                    null,
                    DefaultOperationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// Retrieves the report of Chaos runs.
            /// </summary>
            /// <param name="filter">Filter for the <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>s
            /// to be included in the <see cref="System.Fabric.Chaos.DataStructures.ChaosReport"/>.</param>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <returns>Report of Chaos runs.</returns>
            public Task<ChaosReport> GetChaosReportAsync(
                ChaosReportFilter filter,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                Requires.Argument<ChaosReportFilter>("ChaosReportFilter", filter).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosReportAsync(
                    filter,
                    null,
                    operationTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Retrieves the report of Chaos runs.
            /// </summary>
            /// <param name="continuationToken">Continuation token for the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>s in the
            /// <see cref="System.Fabric.Chaos.DataStructures.ChaosReport"/>.</param>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <returns>Report of Chaos runs.</returns>
            public Task<ChaosReport> GetChaosReportAsync(
                string continuationToken)
            {
                Requires.Argument<string>("continuationToken", continuationToken).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosReportAsync(
                    new ChaosReportFilter(),
                    continuationToken,
                    DefaultOperationTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// Retrieves the report of Chaos runs.
            /// </summary>
            /// <param name="continuationToken">Continuation token for the list of <see cref="System.Fabric.Chaos.DataStructures.ChaosEvent"/>s in the <see cref="System.Fabric.Chaos.DataStructures.ChaosReport"/>.</param>
            /// <exception cref= "System.Fabric.FabricException" >These are Service Fabric exceptions and the following error codes should be inspected.
            /// FabricErrorCode.NotReady - if this API is called before starting Chaos.</exception>
            /// <param name="operationTimeout">The overall timeout for the operation.</param>
            /// <param name="cancellationToken">This token can be signalled to abort this operation before it finishes.</param>
            /// <returns>Report of Chaos runs.</returns>
            public Task<ChaosReport> GetChaosReportAsync(
                string continuationToken,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                Requires.Argument<string>("continuationToken", continuationToken).NotNull();

                this.fabricClient.ThrowIfDisposed();
                return this.GetChaosReportAsync(
                    new ChaosReportFilter(),
                    continuationToken,
                    operationTimeout,
                    cancellationToken);
            }

            internal Task<ChaosReport> GetChaosReportAsync(
                ChaosReportFilter filter,
                string continuationToken,
                TimeSpan operationTimeout,
                CancellationToken cancellationToken)
            {
                return this.GetChaosReportAsyncHelper(
                    filter,
                    continuationToken,
                    operationTimeout,
                    cancellationToken);
            }

            #endregion

            #endregion API - Chaos

            #endregion

            #region Helpers

            #region Helpers - InvokeDataLoss
            private Task StartPartitionDataLossAsyncHelper(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartPartitionDataLossAsyncBeginWrapper(
                        operationId,
                        partitionSelector,
                        dataLossMode,
                        timeout,
                        callback),
                    this.StartPartitionDataLossAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.InvokeDataLossAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartPartitionDataLossAsyncBeginWrapper(
                Guid operationId,
                PartitionSelector partitionSelector,
                DataLossMode dataLossMode,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var invokeDataLossDescription = new InvokeDataLossDescription(
                    operationId,
                    partitionSelector,
                    dataLossMode);

                using (var pin = new PinCollection())
                {
                    IntPtr datalossdesc = invokeDataLossDescription.ToNative(pin);

                    InvokeDataLossDescription desc = InvokeDataLossDescription.CreateFromNative(datalossdesc);

                    return this.nativeTestManagementClient.BeginStartPartitionDataLoss(
                        invokeDataLossDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartPartitionDataLossAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStartPartitionDataLoss(context);
            }

            #endregion Helpers - InvokeDataLoss

            #region Helpers - InvokeQuorumLoss
            private Task StartPartitionQuorumLossAsyncHelper(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumlossMode,
                TimeSpan quorumlossDuration,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartPartitionQuorumLossAsyncBeginWrapper(
                        operationId,
                        partitionSelector,
                        quorumlossMode,
                        quorumlossDuration,
                        timeout,
                        callback),
                    this.StartPartitionQuorumLossAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.InvokeQuorumLossAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartPartitionQuorumLossAsyncBeginWrapper(
                Guid operationId,
                PartitionSelector partitionSelector,
                QuorumLossMode quorumLossMode,
                TimeSpan quorumLossDuration,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var invokeQuorumLossDescription = new InvokeQuorumLossDescription(
                    operationId,
                    partitionSelector,
                    quorumLossMode,
                    quorumLossDuration);

                using (var pin = new PinCollection())
                {
                    IntPtr quorumlossdesc = invokeQuorumLossDescription.ToNative(pin);

                    InvokeQuorumLossDescription desc = InvokeQuorumLossDescription.CreateFromNative(quorumlossdesc);

                    return this.nativeTestManagementClient.BeginStartPartitionQuorumLoss(
                        invokeQuorumLossDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartPartitionQuorumLossAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStartPartitionQuorumLoss(context);
            }

            #endregion Helpers - InvokeDataLoss

            #region Helpers - RestartPartition
            private Task StartPartitionRestartAsyncHelper(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartPartitionRestartAsyncBeginWrapper(
                        operationId,
                        partitionSelector,
                        restartPartitionMode,
                        timeout,
                        callback),
                    this.StartPartitionRestartAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.RestartPartitionAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartPartitionRestartAsyncBeginWrapper(
                Guid operationId,
                PartitionSelector partitionSelector,
                RestartPartitionMode restartPartitionMode,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var restartPartitionDescription = new RestartPartitionDescription(
                    operationId,
                    partitionSelector,
                    restartPartitionMode);

                using (var pin = new PinCollection())
                {
                    IntPtr restartPartitionDesc = restartPartitionDescription.ToNative(pin);

                    RestartPartitionDescription desc = RestartPartitionDescription.CreateFromNative(restartPartitionDesc);

                    return this.nativeTestManagementClient.BeginStartPartitionRestart(
                        restartPartitionDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartPartitionRestartAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStartPartitionRestart(context);
            }

            #endregion Helpers - RestartPartition

            #region Helpers - GetInvokeDataLossProgress
            private Task<PartitionDataLossProgress> GetPartitionDataLossProgressAsyncHelper(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PartitionDataLossProgress>(
                    (callback) => this.GetPartitionDataLossProgressBeginWrapper(operationId, timeout, callback),
                    this.GetPartitionDataLossProgressEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetInvokeDataLossProgress");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionDataLossProgressBeginWrapper(Guid operationId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetPartitionDataLossProgress(
                    operationId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private PartitionDataLossProgress GetPartitionDataLossProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.InvokeDataLossProgressHelper.CreateFromNative(
                    (NativeClient.IFabricPartitionDataLossProgressResult)this.nativeTestManagementClient.EndGetPartitionDataLossProgress(context));
            }

            #endregion Helpers - GetInvokeDataLossProgress

            #region Helpers - GetInvokeQuorumLossProgress

            private Task<PartitionQuorumLossProgress> GetPartitionQuorumLossProgressAsyncHelper(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PartitionQuorumLossProgress>(
                    (callback) => this.GetPartitionQuorumLossProgressBeginWrapper(operationId, timeout, callback),
                    this.GetPartitionQuorumLossProgressEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetInvokeQuorumLossProgress");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionQuorumLossProgressBeginWrapper(Guid operationId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetPartitionQuorumLossProgress(
                    operationId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private PartitionQuorumLossProgress GetPartitionQuorumLossProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.InvokeQuorumLossProgressHelper.CreateFromNative(
                    (NativeClient.IFabricPartitionQuorumLossProgressResult)this.nativeTestManagementClient.EndGetPartitionQuorumLossProgress(context));
            }

            #endregion Helpers - GetInvokeQuorumLossProgress

            #region Helpers - GetRestartPartitionProgress

            private Task<PartitionRestartProgress> GetPartitionRestartProgressAsyncHelper(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PartitionRestartProgress>(
                    (callback) => this.GetPartitionRestartProgressBeginWrapper(operationId, timeout, callback),
                    this.GetPartitionRestartProgressEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetRestartPartitionProgress");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionRestartProgressBeginWrapper(Guid operationId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetPartitionRestartProgress(
                    operationId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private PartitionRestartProgress GetPartitionRestartProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.RestartPartitionProgressHelper.CreateFromNative(
                    (NativeClient.IFabricPartitionRestartProgressResult)this.nativeTestManagementClient.EndGetPartitionRestartProgress(context));
            }

            #endregion Helpers - GetRestartPartitionProgress

            #region Helpers - GetTestCommandStatusList

            private Task<TestCommandStatusList> GetTestCommandStatusListAsyncHelper(
                TestCommandStateFilter stateFilter,
                TestCommandTypeFilter typeFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<TestCommandStatusList>(
                    (callback) => this.GetTestCommandStatusListAsyncBeginWrapper(
                        stateFilter,
                        typeFilter,
                        timeout,
                        callback),
                    this.GetTestCommandStatusListAsyncEndWrapper,
                    cancellationToken,
                    "GetTestCommandStatusListAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetTestCommandStatusListAsyncBeginWrapper(
                TestCommandStateFilter stateFilter,
                TestCommandTypeFilter typeFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    TestCommandListDescription description = new TestCommandListDescription(stateFilter, typeFilter);
                    return this.nativeTestManagementClient.BeginGetTestCommandStatusList(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private TestCommandStatusList GetTestCommandStatusListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetTestCommandListHelper.CreateFromNative(
                    (NativeClient.IFabricTestCommandStatusResult)this.nativeTestManagementClient.EndGetTestCommandStatusList(context));
            }

            #endregion

            #region Helpers - CancelTestCommand
            private Task CancelTestCommandAsyncHelper(
                Guid operationId,
                bool force,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CancelTestCommandAsyncBeginWrapper(
                        operationId,
                        force,
                        timeout,
                        callback),
                    this.CancelTestCommandAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.CancelTestCommandAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext CancelTestCommandAsyncBeginWrapper(
                Guid operationId,
                bool force,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    CancelTestCommandDescription description = new CancelTestCommandDescription(operationId, force);
                    return this.nativeTestManagementClient.BeginCancelTestCommand(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CancelTestCommandAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndCancelTestCommand(context);
            }
            #endregion

            #region Helpers - GetChaosScheduleAsync

            private Task<ChaosScheduleDescription> GetChaosScheduleAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ChaosScheduleDescription>(
                    (callback) => this.GetChaosScheduleAsyncBeginWrapper(
                        timeout,
                        callback),
                    this.GetChaosScheduleAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetChaosScheduleAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetChaosScheduleAsyncBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetChaosSchedule(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private ChaosScheduleDescription GetChaosScheduleAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetChaosScheduleHelper.CreateFromNative(
                    (NativeClient.IFabricChaosScheduleDescriptionResult)this.nativeTestManagementClient.EndGetChaosSchedule(context));
            }

            #endregion

            #region Helpers - GetChaosAsync

            private Task<ChaosDescription> GetChaosAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ChaosDescription>(
                    (callback) => this.GetChaosAsyncBeginWrapper(
                        timeout,
                        callback),
                    this.GetChaosAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetChaosAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetChaosAsyncBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetChaos(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private ChaosDescription GetChaosAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetChaosHelper.CreateFromNative(
                    (NativeClient.IFabricChaosDescriptionResult)this.nativeTestManagementClient.EndGetChaos(context));
            }

            #endregion

            #region Helpers - StartChaosAsync

            private Task StartChaosAsyncHelper(
                ChaosParameters chaosParameters,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartChaosBeginWrapper(chaosParameters, timeout, callback),
                    this.StartChaosEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StartChaos");
            }

            private NativeCommon.IFabricAsyncOperationContext StartChaosBeginWrapper(
                ChaosParameters chaosTestScenarioParameters,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var startChaosDescription = new StartChaosDescription(
                    chaosTestScenarioParameters);

                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClient.BeginStartChaos(
                        startChaosDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartChaosEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStartChaos(context);
            }

            #endregion

            #region Helpers - StopChaosAsync

            private Task StopChaosAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StopChaosBeginWrapper(timeout, callback),
                    this.StopChaosEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StopChaos");
            }

            private NativeCommon.IFabricAsyncOperationContext StopChaosBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginStopChaos(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void StopChaosEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStopChaos(context);
            }

            #endregion

            #region Helpers - GetChaosReport

            private Task<ChaosReport> GetChaosReportAsyncHelper(
                ChaosReportFilter filter,
                string continuationToken,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ChaosReport>(
                    (callback) => this.GetChaosReportAsyncBeginWrapper(
                        filter,
                        continuationToken,
                        timeout,
                        callback),
                    this.GetChaosReportAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetChaosReportAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetChaosReportAsyncBeginWrapper(
                ChaosReportFilter filter,
                string continuationToken,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var getChaosReportDescription = new GetChaosReportDescription(
                    filter,
                    continuationToken);

                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClient.BeginGetChaosReport(
                        getChaosReportDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ChaosReport GetChaosReportAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetChaosReportHelper.CreateFromNative(
                    (NativeClient.IFabricChaosReportResult)this.nativeTestManagementClient.EndGetChaosReport(context));
            }

            #endregion

            #region Helpers - GetChaosEvents

            private Task<ChaosEventsSegment> GetChaosEventsAsyncHelper(
                ChaosEventsSegmentFilter filter,
                string continuationToken,
                long maxResults,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ChaosEventsSegment>(
                    (callback) => this.GetChaosEventsAsyncBeginWrapper(
                        filter,
                        continuationToken,
                        maxResults,
                        timeout,
                        callback),
                    this.GetChaosEventsAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetChaosEventsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetChaosEventsAsyncBeginWrapper(
                ChaosEventsSegmentFilter filter,
                string continuationToken,
                long maxResults,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var chaosEventsDescription = new ChaosEventsDescription(
                    filter,
                    continuationToken,
                    maxResults);

                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClient.BeginGetChaosEvents(
                        chaosEventsDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ChaosEventsSegment GetChaosEventsAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetChaosEventsHelper.CreateFromNative(
                    (NativeClient.IFabricChaosEventsSegmentResult)this.nativeTestManagementClient.EndGetChaosEvents(context));
            }

            #endregion

            #region Helpers - SetChaosScheduleAsync

             private Task SetChaosScheduleAsyncHelper(
                ChaosScheduleDescription schedule,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.SetChaosScheduleAsyncBeginWrapper(
                        schedule,
                        timeout,
                        callback),
                    this.SetChaosScheduleAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.SetChaosScheduleAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext SetChaosScheduleAsyncBeginWrapper(
                ChaosScheduleDescription schedule,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                var setChaosScheduleDescription = new SetChaosScheduleDescription(schedule);

                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClient.BeginSetChaosSchedule(
                        setChaosScheduleDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void SetChaosScheduleAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndSetChaosSchedule(context);
            }

            #endregion

            private Task StartNodeTransitionAsyncHelper(
                NodeTransitionDescription description,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartNodeTransitionAsyncBeginWrapper(
                        description,
                        timeout,
                        callback),
                    this.StartNodeTransitionAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StartNodeTransitionAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartNodeTransitionAsyncBeginWrapper(
                NodeTransitionDescription description,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClient.BeginStartNodeTransition(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartNodeTransitionAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClient.EndStartNodeTransition(context);
            }

            private Task<NodeTransitionProgress> GetNodeTransitionProgressAsyncHelper(Guid operationId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NodeTransitionProgress>(
                    (callback) => this.GetNodeTransitionProgressBeginWrapper(operationId, timeout, callback),
                    this.GetNodeTransitionProgressEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.GetNodeTransitionProgressAsyncHelper");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNodeTransitionProgressBeginWrapper(Guid operationId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeTestManagementClient.BeginGetNodeTransitionProgress(
                    operationId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private NodeTransitionProgress GetNodeTransitionProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.NodeTransitionProgressHelper.CreateFromNative(
                    (NativeClient.IFabricNodeTransitionProgressResult)this.nativeTestManagementClient.EndGetNodeTransitionProgress(context));
            }

            #region Helpers - UnreliableTransportBehavior

            private Task AddUnreliableTransportBehaviorAsyncHelper(
                string nodeName,
                string behaviorName,
                UnreliableTransportBehavior behavior,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                var internalClusterClient = this.fabricClient.ClusterManager.InternalNativeClient;
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.AddUnreliableTransportBehaviorAsyncBeginWrapper(internalClusterClient, nodeName, behaviorName, behavior, timeout, callback),
                    (context) => this.AddUnreliableTransportBehaviorAsyncEndWrapper(internalClusterClient, context),
                    cancellationToken,
                    "ClusterManager.AddUnreliableTransportBehavior");
            }

            private Task RemoveUnreliableTransportBehaviorAsyncHelper(
                string nodeName,
                string behaviorName,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                var internalClusterClient = this.fabricClient.ClusterManager.InternalNativeClient;
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RemoveUnreliableTransportBehaviorAsyncBeginWrapper(internalClusterClient, nodeName, behaviorName, timeout, callback),
                    (context) => this.RemoveUnreliableTransportBehaviorAsyncEndWrapper(internalClusterClient, context),
                    cancellationToken,
                    "ClusterManager.RemoveUnreliableTransportBehavior");
            }

            private NativeCommon.IFabricAsyncOperationContext BeginGetTransportBehaviorAsyncWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                string nodeName,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {

                    return clusterManagementClient.BeginGetTransportBehaviorList(
                        pin.AddObject(nodeName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private IList<string> EndGetTransportBehaviorAsyncWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                NativeCommon.IFabricAsyncOperationContext context)
            {
                return StringCollectionResult.FromNative(clusterManagementClient.EndGetTransportBehaviorList(context));
            }

            private NativeCommon.IFabricAsyncOperationContext AddUnreliableTransportBehaviorAsyncBeginWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                string nodeName,
                string behaviorName,
                UnreliableTransportBehavior behavior,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var unreliableTransportBehavior = new NativeTypes.FABRIC_UNRELIABLETRANSPORT_BEHAVIOR();

                    unreliableTransportBehavior.Destination = pin.AddObject(behavior.Destination);
                    unreliableTransportBehavior.Action = pin.AddObject(behavior.ActionName);
                    unreliableTransportBehavior.ProbabilityToApply = behavior.ProbabilityToApply;
                    unreliableTransportBehavior.DelayInSeconds = behavior.Delay.TotalSeconds;
                    unreliableTransportBehavior.DelaySpanInSeconds = behavior.DelaySpan.TotalSeconds;
                    unreliableTransportBehavior.Priority = behavior.Priority;
                    unreliableTransportBehavior.ApplyCount = behavior.ApplyCount;

                    if (behavior.Filters.Count > 0)
                    {
                        var nativeArray = new NativeTypes.FABRIC_STRING_PAIR[behavior.Filters.Count];
                        for (int i = 0; i < behavior.Filters.Count; ++i)
                        {
                            nativeArray[i].Name = pin.AddObject(behavior.Filters.GetKey(i));
                            nativeArray[i].Value = pin.AddObject(behavior.Filters.Get(i));
                            nativeArray[i].Reserved = IntPtr.Zero;
                        }

                        var nativeList = new NativeTypes.FABRIC_STRING_PAIR_LIST();
                        nativeList.Count = nativeArray.Length;
                        nativeList.Items = pin.AddBlittable(nativeArray);

                        unreliableTransportBehavior.Filters = pin.AddBlittable(nativeList);
                    }
                    else
                    {
                        unreliableTransportBehavior.Filters = IntPtr.Zero;
                    }

                    return clusterManagementClient.BeginAddUnreliableTransportBehavior(
                        pin.AddObject(nodeName),
                        pin.AddObject(behaviorName),
                        pin.AddBlittable(unreliableTransportBehavior),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void AddUnreliableTransportBehaviorAsyncEndWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                NativeCommon.IFabricAsyncOperationContext context)
            {
                clusterManagementClient.EndAddUnreliableTransportBehavior(context);
            }

            private NativeCommon.IFabricAsyncOperationContext RemoveUnreliableTransportBehaviorAsyncBeginWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                string nodeName,
                string behaviorName,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return clusterManagementClient.BeginRemoveUnreliableTransportBehavior(
                        pin.AddObject(nodeName),
                        pin.AddObject(behaviorName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RemoveUnreliableTransportBehaviorAsyncEndWrapper(
                NativeClient.IInternalFabricClusterManagementClient clusterManagementClient,
                NativeCommon.IFabricAsyncOperationContext context)
            {
                clusterManagementClient.EndRemoveUnreliableTransportBehavior(context);
            }

            #endregion Helpers - UnreliableTransportBehavior

            #region Helpers - UnreliableLeaseBehavior
            private Task AddUnreliableLeaseBehaviorAsyncHelper(
                string nodeName,
                string behaviorString,
                string alias,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.AddUnreliableLeaseBehaviorAsyncBeginWrapper(
                        nodeName,
                        behaviorString,
                        alias,
                        timeout,
                        callback),
                    this.AddUnreliableLeaseBehaviorAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.AddUnreliableLeaseBehavior");
            }

            private Task RemoveUnreliableLeaseBehaviorAsyncHelper(
                string nodeName,
                string alias,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {

                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RemoveUnreliableLeaseBehaviorAsyncBeginWrapper(
                        nodeName,
                        alias,
                        timeout,
                        callback),
                    this.RemoveUnreliableLeaseBehaviorAsyncEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.RemoveUnreliableLeaseBehavior");
            }
            private NativeCommon.IFabricAsyncOperationContext AddUnreliableLeaseBehaviorAsyncBeginWrapper(
              string nodeName,
              string behaviorString,
              string alias,
              TimeSpan timeout,
              NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClientInternal.BeginAddUnreliableLeaseBehavior(
                        pin.AddObject(nodeName),
                        pin.AddObject(behaviorString),
                        pin.AddObject(alias),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void AddUnreliableLeaseBehaviorAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClientInternal.EndAddUnreliableLeaseBehavior(context);
            }

            private NativeCommon.IFabricAsyncOperationContext RemoveUnreliableLeaseBehaviorAsyncBeginWrapper(
                string nodeName,
                string alias,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeTestManagementClientInternal.BeginRemoveUnreliableLeaseBehavior(
                        pin.AddObject(nodeName),
                        pin.AddObject(alias),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RemoveUnreliableLeaseBehaviorAsyncEndWrapper(
                NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeTestManagementClientInternal.EndRemoveUnreliableLeaseBehavior(context);
            }

            #endregion Helpers - UnreliableLeaseBehavior

            #endregion

            #region Utilities
            private static TimeSpan GetRequestTimeout(TimeSpan operationTimeout)
            {
                if (operationTimeout.TotalSeconds <= MinRequestTimeoutInSeconds)
                {
                    return operationTimeout;
                }

                TimeSpan requestTimeout = TimeSpan.FromSeconds(RequestTimeoutFactor * operationTimeout.TotalSeconds);
                return requestTimeout < MinRequestTimeout ? MinRequestTimeout : requestTimeout;
            }
            #endregion Utilities
        }
    }
}