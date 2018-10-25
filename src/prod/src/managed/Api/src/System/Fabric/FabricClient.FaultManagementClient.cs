// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Fabric.Query;
    using System.Fabric.Result;
    using System.Numerics;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides functionality to introduce faults in a Service Fabric cluster.</para>
        /// </summary>
        public sealed class FaultManagementClient
        {
            #region Fields
            private const double RequestTimeoutFactor = 0.2;
            private const double MinRequestTimeoutInSeconds = 30.0;
            private const double DefaultOperationTimeoutInSeconds = 300.0;
            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricFaultManagementClient nativeFaultClient;
            private readonly NativeClient.IFabricFaultManagementClientInternal nativeFaultManagementClientInternal;

            private static readonly TimeSpan MinRequestTimeout = TimeSpan.FromSeconds(MinRequestTimeoutInSeconds);
            internal static readonly TimeSpan DefaultOperationTimeout = TimeSpan.FromSeconds(DefaultOperationTimeoutInSeconds);

            private FabricTestContext testContext = null;

            #endregion

            #region Constructors

            internal FaultManagementClient(
                FabricClient fabricClient, 
                NativeClient.IFabricFaultManagementClient nativeFaultClient,
                NativeClient.IFabricFaultManagementClientInternal nativeFaultClientInternal)
            {
                this.fabricClient = fabricClient;
                this.nativeFaultClient = nativeFaultClient;
                this.nativeFaultManagementClientInternal = nativeFaultClientInternal;
                this.testContext = new FabricTestContext(this.fabricClient);
            }

            internal FabricTestContext TestContext
            {
                get { return this.testContext; }
            }

            #endregion

            #region API

            #region APIs - RestartNode

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to restart.</param>
            /// <param name="nodeInstance"><para>The node instance ID of the node to restart.
            /// If not specified, or is set to 0, the value is ignored.
            /// If the instance is set to -1, the system will internally determine this value.
            /// If the instance has a positive value, it is compared with the active instance ID.
            /// If the instances do not match, the process is not restarted and an error occurs.
            /// A stale message can cause this error.
            /// </para>
            /// </param>
            /// <param name="completionMode">If set to <see cref="System.Fabric.CompletionMode.Verify"/>, the system will check that the node restarted,
            /// and the API will not return until it has and NodeStatus is Up.
            /// If set to <see cref="System.Fabric.CompletionMode.DoNotVerify"/>, the API returns once the node restart has been initiated.</param>
            /// <param name="token">The CancellationToken that this operation is observing. It is used to notify the operation that it should be canceled.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  If the ErrorCode is NodeNotFound, nodeName is invalid.
            /// If the ErrorCode is InstanceIdMismatch, the <paramref name="nodeInstance"/> provided does not match the currently running instance.</exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.RestartNodeAsync(nodeName, nodeInstance, false, completionMode, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to restart.</param>
            /// <param name="nodeInstance"><para>The node instance ID of the node to restart.
            /// If not specified, or is set to 0, the value is ignored.
            /// If the instance is set to -1, the system will internally determine this value.
            /// If the instance has a positive value, it is compared with the active node ID.
            /// If the IDs do not match, the process is not restarted and an error occurs.
            /// A stale message can cause this error.
            /// </para>
            /// </param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the ErrorCode is NodeNotFound, nodeName or nodeInstance is invalid.
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode)
            {
                return this.RestartNodeAsync(nodeName, nodeInstance, false, completionMode, DefaultOperationTimeout, default(CancellationToken));
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to restart.</param>
            /// <param name="nodeInstance"><para>The node instance ID of the node to restart.
            /// If not specified, or is set to 0, the value is ignored.
            /// If the instance is set to -1, the system will internally determine this value.
            /// If the instance has a positive value, it is compared with the active node ID.
            /// If the IDs do not match, the process is not restarted and an error occurs.
            /// A stale message can cause this error.
            /// </para>
            /// </param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is NodeNotFound, nodeName is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.RestartNodeAsync(nodeName, nodeInstance, false, CompletionMode.DoNotVerify, operationTimeout, token);
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to restart.</param>
            /// <param name="nodeInstance"><para>The node instance ID of the node to restart.
            /// If not specified, or is set to 0, the value is ignored.
            /// If the instance is set to -1, the system will internally determine this value.
            /// If the instance has a positive value, it is compared with the active node ID.
            /// If the IDs do not match, the process is not restarted and an error occurs.
            /// A stale message can cause this error.
            /// </para>
            /// </param>
            /// <param name="createFabricDump"> If set to true, the system will create the process dump for Fabric.exe on this node.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is NodeNotFound, nodeName is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public async Task<RestartNodeResult> RestartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                bool createFabricDump,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                RestartNodeAction restartNodeAction = new RestartNodeAction(nodeName, nodeInstance);
                restartNodeAction.CompletionMode = completionMode;
                restartNodeAction.ActionTimeout = operationTimeout;
                restartNodeAction.RequestTimeout = GetRequestTimeout(operationTimeout);
                restartNodeAction.CreateFabricDump = createFabricDump;

                await this.testContext.ActionExecutor.RunAsync(restartNodeAction, token).ConfigureAwait(false);
                return restartNodeAction.Result;
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="replicaSelector">This parameter is used to choose a specific replica.  This replica's corresponding node will be restarted.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node, and the replica selected.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.
            ///   If the ErrorCode is ReplicaDoesNotExist, the selected replica was not found.
            ///   If the ErrorCode is PartitionNotFound, the specified partition does not exist.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.RestartNodeAsync(replicaSelector, false, completionMode, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="replicaSelector">This parameter is used to choose a specific replica.  This replica's corresponding node will be restarted.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <returns>A task with information representing the target node, and the replica selected.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.
            ///   If the ErrorCode is ReplicaDoesNotExist, the selected replica was not found.
            ///   If the ErrorCode is PartitionNotFound, the specified partition does not exist.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode)
            {
                return this.RestartNodeAsync(replicaSelector, false, completionMode, DefaultOperationTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="replicaSelector">This parameter is used to choose a specific replica.
            /// The node where the replica is deployed will be restarted.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node, and the replica selected.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.
            ///   If the ErrorCode is ReplicaDoesNotExist, the selected replica was not found.
            ///   If the ErrorCode is PartitionNotFound, the specified partition does not exist.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>
            /// This API simulates Service Fabric node failures in the cluster,
            /// which tests the fail-over recovery paths of your service.
            /// </remarks>
            public Task<RestartNodeResult> RestartNodeAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.RestartNodeAsync(replicaSelector, false, completionMode, operationTimeout, token);
            }

            /// <summary>
            /// Restarts a cluster node by restarting the Fabric.exe process that hosts the node.
            /// </summary>
            /// <param name="replicaSelector">This parameter is used to choose a specific replica.  This replica's corresponding node will be restarted.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node restarted, and the API will not return until it has and NodeStatus is Up.  If set to DoNotVerify, the API returns once the node restart has been initiated.</param>
            /// <param name="createFabricDump"> If set to true, the system will create the process dump for Fabric.exe on this node.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node, and the replica selected.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.
            ///   If the ErrorCode is ReplicaDoesNotExist, the selected replica was not found.
            ///   If the ErrorCode is PartitionNotFound, the specified partition does not exist.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.
            /// If the createFabricDump parameter is set, on restart the process is crashed and
            /// the crash dump is placed in the Crash Dumps folder which the DCA can be configured to upload.</remarks>
            public async Task<RestartNodeResult> RestartNodeAsync(
                ReplicaSelector replicaSelector,
                bool createFabricDump,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                RestartNodeAction restartNodeAction = new RestartNodeAction(replicaSelector);
                restartNodeAction.CompletionMode = completionMode;
                restartNodeAction.ActionTimeout = operationTimeout;
                restartNodeAction.RequestTimeout = GetRequestTimeout(operationTimeout);
                restartNodeAction.CreateFabricDump = createFabricDump;

                await this.testContext.ActionExecutor.RunAsync(restartNodeAction, token).ConfigureAwait(false);
                return restartNodeAction.Result;
            }

            // This is called by the RestartNodeAction to actually restart the node
            internal Task<RestartNodeResult> RestartNodeUsingNodeNameAsync(
                string nodeName,
                BigInteger nodeInstance,
                bool shouldCreateFabricDump,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.RestartNodeUsingNodeNameAsyncHelper(nodeName, nodeInstance, shouldCreateFabricDump, timeout, cancellationToken);
            }

            #endregion

            #region APIs - StartNode

            /// <summary>
            /// Starts a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to start.</param>
            /// <param name="nodeInstance">The node instance ID of the node, before it was stopped. If this is not specified, or is set to 0, this is ignored. If this is set to -1, the system will internally determine this value.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node started, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node start has been initiated.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName or nodeInstance is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.  
            ///   If the ErrorCode is NodeHasNotStoppedYet, there is a currently pending stop operation on this node.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StartNodeResult> StartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode)
            {
                return this.StartNodeAsync(nodeName, nodeInstance, completionMode, CancellationToken.None);
            }

            /// <summary>
            /// Starts a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to start.</param>
            /// <param name="nodeInstance">The node instance ID of the node, before it was stopped.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node started, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node start has been initiated.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName or nodeInstance is invalid.  
            ///   If the errorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.  
            ///   If the ErrorCode is NodeHasNotStoppedYet, there is a currently pending stop operation on this node.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StartNodeResult> StartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.StartNodeAsync(nodeName, nodeInstance, string.Empty, 0, completionMode, token);
            }

            /// <summary>
            /// Starts a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to start.</param>
            /// <param name="nodeInstance">The node instance ID of the node, before it was stopped.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="ipAddressOrFQDN">The IP address or fully-qualified domain name (FQDN) of the target node.  If this parameter is specified, 'ClusterConnectionPort" also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="clusterConnectionPort">The cluster connection port of the target node.  If this parameter is specified, 'ipAddressOrFQDN' also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node started, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node start has been initiated.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName or nodeInstance is invalid.  
            ///   If the errorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.  
            ///   If the ErrorCode is NodeHasNotStoppedYet, there is a currently pending stop operation on this node.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StartNodeResult> StartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                string ipAddressOrFQDN,
                int clusterConnectionPort,
                CompletionMode completionMode)
            {
                return this.StartNodeAsync(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, completionMode, CancellationToken.None);
            }

            /// <summary>
            /// Starts a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to start.</param>
            /// <param name="nodeInstance">The node instance ID of the node, before it was stopped.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="ipAddressOrFQDN">The IP address or fully-qualified domain name (FQDN) of the target node.  If this parameter is specified, <paramref name="clusterConnectionPort"/> also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="clusterConnectionPort">The cluster connection port of the target node.  If this parameter is specified, <paramref name="ipAddressOrFQDN"/> also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node started, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node start has been initiated.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName or nodeInstance is invalid.  
            ///   If the errorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.  
            ///   If the ErrorCode is NodeHasNotStoppedYet, there is a currently pending stop operation on this node.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StartNodeResult> StartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                string ipAddressOrFQDN,
                int clusterConnectionPort,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.StartNodeAsync(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, completionMode, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Starts a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to start.</param>
            /// <param name="nodeInstance">The node instance ID of the node, before it was stopped.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="ipAddressOrFQDN">The IP address or fully-qualified domain name (FQDN) of the target node.  If this parameter is specified, <paramref name="clusterConnectionPort"/> also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="clusterConnectionPort">The cluster connection port of the target node.  If this parameter is specified, <paramref name="ipAddressOrFQDN"/> also must be specified.  If neither is specified, the system internally determines these.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node started, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node start has been initiated.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellationToken</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName or nodeInstance is invalid.  
            ///   If the errorCode is InstanceIdMismatch, the nodeInstance provided does not match the instance of the node that was stopped.  
            ///   If the ErrorCode is NodeHasNotStoppedYet, there is a currently pending stop operation on this node.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public async Task<StartNodeResult> StartNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                string ipAddressOrFQDN,
                int clusterConnectionPort,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                StartNodeAction startNodeAction = new StartNodeAction(nodeName, nodeInstance);
                startNodeAction.CompletionMode = completionMode;
                startNodeAction.IPAddressOrFQDN = ipAddressOrFQDN;
                startNodeAction.ClusterConnectionPort = clusterConnectionPort;
                startNodeAction.ActionTimeout = operationTimeout;
                startNodeAction.RequestTimeout = GetRequestTimeout(operationTimeout);

                await this.testContext.ActionExecutor.RunAsync(startNodeAction, token).ConfigureAwait(false);
                return startNodeAction.Result;
            }

            // This is called by the StartNodeAction to actually restart the node
            internal Task<StartNodeResult> StartNodeUsingNodeNameAsync(
                string nodeName,
                BigInteger nodeInstance,
                string ipAddressOrFQDN,
                int clusterConnectionPort,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.StartNodeUsingNodeNameAsyncHelper(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, timeout, cancellationToken);
            }

            #endregion APIs - StartNode

            #region APIs - StopNode

            /// <summary>
            /// Stops a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to stop.</param>
            /// <param name="nodeInstance">The node instance ID of the node to stop.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node stopped, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node stop has been initiated.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StopNodeResult> StopNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode)
            {
                return this.StopNodeAsync(nodeName, nodeInstance, completionMode, DefaultOperationTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Stops a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to stop.</param>
            /// <param name="nodeInstance">The node instance ID of the node to stop.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node stopped, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node stop has been initiated.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node.</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public Task<StopNodeResult> StopNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.StopNodeAsync(nodeName, nodeInstance, completionMode, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Stops a cluster node.
            /// </summary>
            /// <param name="nodeName">The node name of the node to stop.</param>
            /// <param name="nodeInstance">The node instance ID of the node to stop.  If this is not specified, or is set to 0, this is ignored.  If this is set to -1, the system will internally determine this value.</param>
            /// <param name="completionMode">If set to Verify, the system will check that the node stopped, and the API will not return until it has.  If set to DoNotVerify, the API returns once the node stop has been initiated.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token that is monitored for any request to cancel the operation.</param>
            /// <returns>A task with information representing the target node</returns>
            /// <exception cref="System.Fabric.FabricException">The <see cref="System.Fabric.FabricException.ErrorCode"/> property will indicate the reason.  
            ///   If the ErrorCode is InvalidArgument, nodeName is invalid.  
            ///   If the ErrorCode is InstanceIdMismatch, the nodeInstance provided does not match the currently running instance.
            /// </exception>
            /// <exception cref="System.TimeoutException">The operation timed out.</exception>
            /// <exception cref="System.ArgumentNullException">An argument with a value of null was passed in.</exception>
            /// <remarks>A cluster node is a process, not an virtual or physical machine.</remarks>
            [Obsolete("This api is deprecated, use StartNodeTransitionAsync instead.")]
            public async Task<StopNodeResult> StopNodeAsync(
                string nodeName,
                BigInteger nodeInstance,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                StopNodeAction stopNodeAction = new StopNodeAction(nodeName, nodeInstance)
                                                    {
                                                        CompletionMode = completionMode,
                                                        ActionTimeout = operationTimeout,
                                                        RequestTimeout = GetRequestTimeout(operationTimeout)
                                                    };

                await this.testContext.ActionExecutor.RunAsync(stopNodeAction, token).ConfigureAwait(false);
                return stopNodeAction.Result;
            }

            // This is called by the StopNodeAction to actually restart the node
            internal Task<StopNodeResult> StopNodeUsingNodeNameAsync(
                string nodeName,
                BigInteger nodeInstance,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.StopNodeUsingNodeNameAsyncHelper(nodeName, nodeInstance, timeout, cancellationToken);
            }
         
            internal Task StopNodeInternalAsync(
                string nodeName,
                BigInteger nodeInstance,
                int stopDurationInSeconds,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return this.StopNodeInternalAsyncHelper(nodeName, nodeInstance, stopDurationInSeconds, timeout, cancellationToken);
            }

            #endregion APIs - StopNode

            #region APIs - RestartDeployedCodePackage

            /// <summary>
            /// This API call restarts the code package which hosts the replica specified by the <see cref="System.Fabric.ReplicaSelector"/> and 
            /// belongs to the specified application name.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="applicationName">The name of the application to which the code package belong.s</param>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which identifies the replica whose host code package needs to be restarted.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found.</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted and replica selected.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                Uri applicationName,
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return await this.RestartDeployedCodePackageAsync(applicationName, replicaSelector, completionMode, DefaultOperationTimeout, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API call restarts the code package which hosts the replica specified by the <see cref="System.Fabric.ReplicaSelector"/> and 
            /// belongs to the specified application name.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="applicationName">The name of the application to which the code package belongs</param>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which identifies the replica whose host code package needs to be restarted.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted and replica selected.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                Uri applicationName,
                ReplicaSelector replicaSelector,
                CompletionMode completionMode)
            {
                return await this.RestartDeployedCodePackageAsync(applicationName, replicaSelector, completionMode, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API call restarts the code package which hosts the replica specified by the <see cref="System.Fabric.ReplicaSelector"/> and 
            /// belongs to the specified application name.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="applicationName">The name of the application to which the code package belongs</param>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which identifies the replica whose host code package needs to be restarted.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.n</param>
            /// <param name="operationTimeout">The overall timeout for the operation including the timeout
            /// to wait for code package to restart if <see cref="System.Fabric.CompletionMode"/> is Verify</param>
            /// <param name="token">Cancellation token.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted and replica selected.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                Uri applicationName,
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                var restartDeployedCodePackageAction = new RestartDeployedCodePackageAction(applicationName, replicaSelector)
                                                           {
                                                               CompletionMode = completionMode,
                                                               ActionTimeout = operationTimeout,
                                                               RequestTimeout = GetRequestTimeout(operationTimeout)
                                                           };

                await this.testContext.ActionExecutor.RunAsync(restartDeployedCodePackageAction, token).ConfigureAwait(false);
                return restartDeployedCodePackageAction.Result;
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted</param>
            /// <param name="applicationName">The name of the application to which the code package belongs</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined</param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return await this.RestartDeployedCodePackageAsync(nodeName, applicationName, serviceManifestName, codePackageName, codePackageInstanceId, completionMode, DefaultOperationTimeout, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted</param>
            /// <param name="applicationName">The name of the application to which the code package belongs</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined</param>
            /// <param name="servicePackageActivationId">
            /// <para>
            /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package which contains the code package. 
            /// You can get the ServicePackageActivationId of a deployed service package by using 
            /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
            /// </para>
            /// <para>
            /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service was 
            /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
            /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
            /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
            /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
            /// </para>
            /// </param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return this.RestartDeployedCodePackageAsync(
                    nodeName, 
                    applicationName, 
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName, 
                    codePackageInstanceId, 
                    completionMode, 
                    DefaultOperationTimeout, 
                    token);
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted.</param>
            /// <param name="applicationName">The name of the application to which the code package belongs.</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined.</param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode)
            {
                return await this.RestartDeployedCodePackageAsync(nodeName, applicationName, serviceManifestName, codePackageName, codePackageInstanceId, completionMode, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted.</param>
            /// <param name="applicationName">The name of the application to which the code package belongs.</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined.</param>
            /// <param name="servicePackageActivationId">
            /// <para>
            /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package which contains the code package. 
            /// You can get the ServicePackageActivationId of a deployed service package by using 
            /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
            /// </para>
            /// <para>
            /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service was
            /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
            /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
            /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
            /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
            /// </para>
            /// </param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode)
            {
                return this.RestartDeployedCodePackageAsync(
                    nodeName, 
                    applicationName, 
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName, 
                    codePackageInstanceId, 
                    completionMode, 
                    CancellationToken.None);
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted.</param>
            /// <param name="applicationName">The name of the application to which the code package belongs.</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined.</param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <param name="operationTimeout">The overall timeout for the operation including the timeout to wait for code package to restart if <see cref="System.Fabric.CompletionMode"/> is Verify</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.RestartDeployedCodePackageAsync(
                    nodeName, 
                    applicationName, 
                    serviceManifestName,
                    string.Empty, // Default ServicePackageActivationId.
                    codePackageName, 
                    codePackageInstanceId, 
                    completionMode,
                    operationTimeout,
                    token);
            }

            /// <summary>
            /// This API call restarts the code package as specified by the input parameters.
            /// </summary>
            /// <remarks>
            /// The <see cref="System.Fabric.CompletionMode"/> options are
            /// DoNotVerify - Return after triggering the restart of the code package
            /// Verify - Return after the restart completes i.e. the code package has come back up again.
            /// </remarks>
            /// <param name="nodeName">The node on which the code package is hosted.</param>
            /// <param name="applicationName">The name of the application to which the code package belongs.</param>
            /// <param name="serviceManifestName">The name of the service manifest where the code package is defined.</param>
            /// <param name="servicePackageActivationId">
            /// <para>
            /// The <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package which contains the code package. 
            /// You can get the ServicePackageActivationId of a deployed service package by using 
            /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/> query. 
            /// </para>
            /// <para>
            /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service was 
            /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it was not specified, in
            /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
            /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
            /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
            /// </para>
            /// </param>
            /// <param name="codePackageName">The name of the code package to be restarted</param>
            /// <param name="codePackageInstanceId">The code package instance id for the running code package which if specified and does not match then the restart is not processed
            /// If the value is 0 then the comparison is skipped.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the code package completes or not.</param>
            /// <param name="operationTimeout">The overall timeout for the operation including the timeout to wait for code package to restart if <see cref="System.Fabric.CompletionMode"/> is Verify</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.CodePackageNotFound - If the selected code package was not found
            /// FabricErrorCode.InstanceIdMismatch - If the specified instance id did not match</exception>
            /// <exception cref= "System.InvalidOperationException" >The code package was not in a valid running state.</exception>
            /// <returns>RestartDeployedCodePackageResult which gives information about the actual code package restarted. SelectedReplica is None in this overload.</returns>
            public async Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                var restartDeployedCodePackageAction = new RestartDeployedCodePackageAction(
                    nodeName, 
                    applicationName, 
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName, 
                    codePackageInstanceId)
                {
                    CompletionMode = completionMode,
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                await this.testContext.ActionExecutor.RunAsync(restartDeployedCodePackageAction, token).ConfigureAwait(false);
                return restartDeployedCodePackageAction.Result;
            }

            // Used by action to actually restart code package
            internal Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageUsingNodeNameAsync(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.RestartDeployedCodePackageUsingNodeNameAsyncHelper(
                    nodeName, 
                    applicationName, 
                    serviceManifestName,
                    servicePackageActivationId,
                    codePackageName, 
                    codePackageInstanceId, 
                    operationTimeout, 
                    token);
            }

            #endregion APIs - RestartDeployedCodePackage

            #region APIs - Replica

            #region APIs - RemoveReplica

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be removed.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the remove of the replica is complete or not 
            /// DoNotVerify - Return after triggering the remove of the replica
            /// Verify - Return after the remove completes i.e. the replica is out of the FM view</param>
            /// <param name="forceRemove">Will forcefully remove the replica</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                bool forceRemove,
                CancellationToken token)
            {
                return await this.RemoveReplicaAsync(replicaSelector, completionMode, forceRemove, DefaultOperationTimeout, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be removed.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the remove of the replica is complete or not 
            /// DoNotVerify - Return after triggering the remove of the replica
            /// Verify - Return after the remove completes i.e. the replica is out of the FM vie.w</param>
            /// <param name="forceRemove">Will forcefully remove the replica</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                bool forceRemove)
            {
                return await this.RemoveReplicaAsync(replicaSelector, completionMode, forceRemove, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be removed.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the remove of the replica is complete or not 
            /// DoNotVerify - Return after triggering the remove of the replica
            /// Verify - Return after the remove completes i.e. the replica is out of the FM view.</param>
            /// <param name="forceRemove">Will forcefully remove the replica.</param>
            /// <param name="operationTimeout">The overall timeout for the operation including the timeout to wait for replica to be removed if <see cref="System.Fabric.CompletionMode"/> is Verify</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                bool forceRemove,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                RemoveReplicaAction removeReplicaAction = new RemoveReplicaAction(replicaSelector)
                                                              {
                                                                  CompletionMode = completionMode,
                                                                  ForceRemove = forceRemove,
                                                                  ActionTimeout = operationTimeout,
                                                                  RequestTimeout = GetRequestTimeout(operationTimeout)
                                                              };

                await this.testContext.ActionExecutor.RunAsync(removeReplicaAction, token).ConfigureAwait(false);
                return removeReplicaAction.Result;
            }

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="nodeName">Node name where replica is to be moved <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be removed </param>
            /// <param name="replicaId">Replica Id that needs to be removed </param>
            /// <param name="forceRemove">Will forcefully remove the replica</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the remove of the replica is complete or not 
            /// DoNotVerify - Return after triggering the remove of the replica
            /// Verify - Return after the remove completes i.e. the replica is out of the FM view.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode,
                bool forceRemove,
                CancellationToken token)
            {
                return await this.RemoveReplicaAsync(nodeName, partitionId, replicaId, completionMode, forceRemove, DefaultOperationTimeout.TotalSeconds, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in 
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="nodeName">Node name where replica is to be moved <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be removed </param>
            /// <param name="replicaId">Replica Id that needs to be removed </param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="forceRemove">Will forcefully remove the replica</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode,
                bool forceRemove)
            {
                return await this.RemoveReplicaAsync(nodeName, partitionId, replicaId, completionMode, forceRemove, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will remove the replica (equivalent of ReportFault - Permanent) specified by the passed in 
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="nodeName">Node name where replica is to be moved <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be removed </param>
            /// <param name="replicaId">Replica Id that needs to be removed </param>
            /// <param name="forceRemove">Will forcefully remove the replica.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="operationTimeoutSec">The overall timeout in seconds for the operation, including the timeout to wait for replica to be removed if 
            /// <see cref="System.Fabric.CompletionMode"/> is Verify</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RemoveReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RemoveReplicaResult> RemoveReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode,
                bool forceRemove,
                double operationTimeoutSec,
                CancellationToken token)
            {
                TimeSpan operationTimeout = TimeSpan.FromSeconds(operationTimeoutSec);

                RemoveReplicaAction removeReplicaAction = new RemoveReplicaAction()
                                                              {
                                                                  NodeName = nodeName,
                                                                  PartitionId = partitionId,
                                                                  ReplicaId = replicaId,
                                                                  CompletionMode = completionMode,
                                                                  ForceRemove = forceRemove,
                                                                  ActionTimeout = operationTimeout,
                                                                  RequestTimeout = GetRequestTimeout(operationTimeout)
                                                              };

                await this.testContext.ActionExecutor.RunAsync(removeReplicaAction, token).ConfigureAwait(false);
                return removeReplicaAction.Result;
            }

            #endregion APIs - RemoveReplica

            #region APIs - RestartReplica

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in ReplicaSelector.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be restarted. This API can only be called for persisted service replicas.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return await this.RestartReplicaAsync(replicaSelector, completionMode, DefaultOperationTimeout, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in 
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be restarted. This API can only be called for persisted service replicas.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode)
            {
                return await this.RestartReplicaAsync(replicaSelector, completionMode, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in 
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="replicaSelector">The <see cref="System.Fabric.ReplicaSelector"/> which indicates the replica to be restarted. This API can only be called for persisted service replicas.</param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="operationTimeout">The overall timeout for the operation including the timeout to wait for replica to be restarted if 
            /// <see cref="System.Fabric.CompletionMode"/> is Verify.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                ReplicaSelector replicaSelector,
                CompletionMode completionMode,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                RestartReplicaAction restartReplicaAction = new RestartReplicaAction(replicaSelector)
                                                                {
                                                                    CompletionMode = completionMode,
                                                                    ActionTimeout = operationTimeout,
                                                                    RequestTimeout = GetRequestTimeout(operationTimeout)
                                                                };

                await this.testContext.ActionExecutor.RunAsync(restartReplicaAction, token).ConfigureAwait(false);
                return restartReplicaAction.Result;
            }

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in ReplicaSelector.
            /// </summary>
            /// <param name="nodeName">Node name where replica needs to be restarted <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be restarted </param>
            /// <param name="replicaId">Replica Id that needs to be restarted </param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode,
                CancellationToken token)
            {
                return await this.RestartReplicaAsync(nodeName, partitionId, replicaId, completionMode, DefaultOperationTimeout.TotalSeconds, token).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in 
            /// <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="nodeName">Node name where replica needs to be restarted <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be restarted </param>
            /// <param name="replicaId">Replica Id that needs to be restarted </param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist.</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode)
            {
                return await this.RestartReplicaAsync(nodeName, partitionId, replicaId, completionMode, CancellationToken.None).ConfigureAwait(false);
            }

            /// <summary>
            /// This API will restart the replica (equivalent of ReportFault - Temporary) specified by the passed in <see cref="System.Fabric.ReplicaSelector"/>.
            /// </summary>
            /// <param name="nodeName">Node name where replica needs to be restarted <see cref="System.Fabric.ReplicaSelector"/> </param>
            /// <param name="partitionId">Partition Id where the replica needs to be restarted </param>
            /// <param name="replicaId">Replica Id that needs to be restarted </param>
            /// <param name="completionMode">The <see cref="System.Fabric.CompletionMode"/> that specifies whether to wait until the restart of the replica is complete or not 
            /// DoNotVerify - Return after triggering the restart of the replica
            /// Verify - Return after the remove completes</param>
            /// <param name="operationTimeoutSec">The overall timeout in seconds for the operation, including the timeout to wait for replica to be restarted if 
            /// <see cref="System.Fabric.CompletionMode"/> is Verify.</param>
            /// <param name="token">Cancellation token</param>
            /// <exception cref= "System.TimeoutException" >Action took more than its allocated time.</exception>
            /// <exception cref= "System.ArgumentNullException" >Any of the required arguments are null.</exception>
            /// <exception cref= "System.Fabric.FabricException" >These are the fabric failures
            /// FabricErrorCode.ReplicaDoesNotExist - If the Selected replica was not found
            /// FabricErrorCode.PartitionNotFound - if the specified partition selected does not exist</exception>
            /// <returns>RestartReplicaResult which gives information about the actual selected replica.</returns>
            public async Task<RestartReplicaResult> RestartReplicaAsync(
                string nodeName,
                Guid partitionId,
                long replicaId,
                CompletionMode completionMode,
                double operationTimeoutSec,
                CancellationToken token)
            {
                TimeSpan operationTimeout = TimeSpan.FromSeconds(operationTimeoutSec);

                RestartReplicaAction restartReplicaAction = new RestartReplicaAction()
                                                                {
                                                                    NodeName = nodeName,
                                                                    PartitionId = partitionId,
                                                                    ReplicaId = replicaId,
                                                                    CompletionMode = completionMode,
                                                                    ActionTimeout = operationTimeout,
                                                                    RequestTimeout = GetRequestTimeout(operationTimeout)
                                                                };

                await this.testContext.ActionExecutor.RunAsync(restartReplicaAction, token).ConfigureAwait(false);
                return restartReplicaAction.Result;
            }

            #endregion APIs - RestartReplica

            #region APIs - MovePrimary

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints)
            {
                return this.MovePrimaryAsync(string.Empty, partitionSelector, ignoreConstraints);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector)
            {
                return this.MovePrimaryAsync(string.Empty, partitionSelector, false);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                CancellationToken token)
            {
                return this.MovePrimaryAsync(string.Empty, partitionSelector, ignoreConstraints, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector,
                CancellationToken token)
            {
                return this.MovePrimaryAsync(string.Empty, partitionSelector, false, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.</remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition. </param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public async Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return await this.MovePrimaryAsync(string.Empty, partitionSelector, ignoreConstraints, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location.
            /// This overload uses random node selected from current node list, where primary replica does
            /// not exist at the time of API call for moving primary replica.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.</remarks>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                PartitionSelector partitionSelector,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.MovePrimaryAsync(string.Empty, partitionSelector, false, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition. </param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// API uses the primary replica of the selected partition to move to new node location.
            ///
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints)
            {
                return this.MovePrimaryAsync(nodeName, partitionSelector, ignoreConstraints, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition. </param>
            /// API uses the primary replica of the selected partition to move to new node location.
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector)
            {
                return this.MovePrimaryAsync(nodeName, partitionSelector, false, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition. </param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// API uses the primary replica of the selected partition to move to new node location.
            ///
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                CancellationToken token)
            {
                return this.MovePrimaryAsync(nodeName, partitionSelector, ignoreConstraints, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition. </param>
            /// API uses the primary replica of the selected partition to move to new node location.
            ///
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector,
                CancellationToken token)
            {
                return this.MovePrimaryAsync(nodeName, partitionSelector, false, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.
            /// API uses the primary replica of the selected partition to move to new node location.
            /// </param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public async Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                MovePrimaryAction movePrimaryAction = new MovePrimaryAction(nodeName, partitionSelector, ignoreConstraints)
                {
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                await this.testContext.ActionExecutor.RunAsync(movePrimaryAction, token).ConfigureAwait(false);
                return movePrimaryAction.Result;
            }

            /// <summary>
            /// Moves selected primary replica to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the primary replica of the selected partition to move to new node location specified by nodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="nodeName">Node name where primary replica to be moved</param>
            /// <param name="partitionSelector">Move primary will be called on this Selected Partition.
            /// API uses the primary replica of the selected partition to move to new node location.
            /// </param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.NotReady - If Primary replica is not ready for movement
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move primary result</returns>
            public async Task<MovePrimaryResult> MovePrimaryAsync(
                string nodeName,
                PartitionSelector partitionSelector,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                MovePrimaryAction movePrimaryAction = new MovePrimaryAction(nodeName, partitionSelector, false)
                {
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                await this.testContext.ActionExecutor.RunAsync(movePrimaryAction, token).ConfigureAwait(false);
                return movePrimaryAction.Result;
            }

            internal Task<MovePrimaryResult> MovePrimaryUsingNodeNameAsync(
                string nodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.MovePrimaryUsingNodeNameAsyncHelper(nodeName, serviceName, partitionId, ignoreConstraints, operationTimeout, token);
            }

            #endregion APIs - MovePrimary

            #region APIs - MoveSecondary

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by current secondary node. This API overload randomly selects current secondary
            /// node for random secondary replica of the selected partition and new secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition. </param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, ignoreConstraints, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by current secondary node. This API overload randomly selects current secondary
            /// node for random secondary replica of the selected partition and new secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.
            /// </param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, false, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// This API overload randomly selects current secondary
            /// node for random secondary replica of the selected partition and new secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the replica being moved is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, ignoreConstraints, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// This API overload randomly selects current secondary
            /// node for random secondary replica of the selected partition and new secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the replica being moved is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, false, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName. This API overload randomly selects new
            /// secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the replica being moved is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, ignoreConstraints, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName. This API overload randomly selects new
            /// secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>

            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                PartitionSelector partitionSelector)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, false, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName. This API overload randomly selects new
            /// secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
               string currentNodeName,
               PartitionSelector partitionSelector,
               bool ignoreConstraints,
               CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, ignoreConstraints, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName. This API overload randomly selects new
            /// secondary node for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.
            /// </param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
               string currentNodeName,
               PartitionSelector partitionSelector,
               CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, false, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica specified by currentNodeName and
            /// moves it to new node location specified by newNodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints)
            {
                return await this.MoveSecondaryAsync(currentNodeName, newNodeName, partitionSelector, ignoreConstraints, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica specified by currentNodeName and
            /// moves it to new node location specified by newNodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.
            /// </param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector)
            {
                return await this.MoveSecondaryAsync(currentNodeName, newNodeName, partitionSelector, false, CancellationToken.None);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica specified by currentNodeName and
            /// moves it to new node location specified by newNodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, newNodeName, partitionSelector, ignoreConstraints, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica specified by currentNodeName and
            /// moves it to new node location specified by newNodeName.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, newNodeName, partitionSelector, false, DefaultOperationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the selected secondary replica specified by currentNodeName.
            /// This selected replica will be moved to the randomly selected new node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, ignoreConstraints, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>API uses the selected secondary replica specified by currentNodeName.
            /// This selected replica will be moved to the randomly selected new node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - If active Secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// FabricErrorCode.ConstraintNotSatisfied - If the constraints for the new location of the replica would prohibit the move
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                PartitionSelector partitionSelector,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(currentNodeName, string.Empty, partitionSelector, false, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the randomly selected secondary replica for specified partition selector.
            /// This API overload randomly selects new secondary node location for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - Active secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, ignoreConstraints, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the randomly selected secondary replica for specified partition selector.
            /// This API overload randomly selects new secondary node location for replica movement
            /// This selected replica will be moved to new node location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - Active secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                PartitionSelector partitionSelector,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return await this.MoveSecondaryAsync(string.Empty, string.Empty, partitionSelector, false, operationTimeout, token);
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName location. This selected replica will be moved to newNodeName location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="ignoreConstraints">Whether or not to ignore constraints when attempting to execute the move.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - Active secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                MoveSecondaryAction moveSecondaryAction = new MoveSecondaryAction(currentNodeName, newNodeName, partitionSelector, ignoreConstraints)
                {
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                await this.testContext.ActionExecutor.RunAsync(moveSecondaryAction, token).ConfigureAwait(false);
                return moveSecondaryAction.Result;
            }

            /// <summary>
            /// Moves selected secondary replica from current node to new node in the cluster.
            /// </summary>
            /// <remarks>
            /// API uses the selected secondary replica inside partition selector structure
            /// specified by currentNodeName location. This selected replica will be moved to newNodeName location from current node location.
            /// This API is safe i.e. it will not cause quorum or data loss by itself unless additional faults or failures happen at the same time.
            /// </remarks>
            /// <param name="currentNodeName">node name where selected replica for move is currently present</param>
            /// <param name="newNodeName">node name where selected replica to be moved</param>
            /// <param name="partitionSelector">Move Secondary will be called on this Selected Partition.</param>
            /// <param name="operationTimeout">The timeout for this API call.</param>
            /// <param name="token">The cancellation token</param>
            /// <exception cref="System.TimeoutException">Retry is exhausted.</exception>
            /// <exception cref="System.InvalidOperationException">Invalid operation
            /// - If action called on stateless service.
            /// - If no active secondary replica exists
            /// - If not enough nodes available for action
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// FabricErrorCode.AlreadyPrimaryReplica - If Primary replica for selected partition already exist on new node
            /// FabricErrorCode.AlreadySecondaryReplica - Active secondary replica for selected partition already exist on new node
            /// FabricErrorCode.InvalidReplicaStateForReplicaOperation - If the target replica is not a secondary
            /// </exception>
            /// <returns>A task with move secondary result</returns>
            public async Task<MoveSecondaryResult> MoveSecondaryAsync(
                string currentNodeName,
                string newNodeName,
                PartitionSelector partitionSelector,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                MoveSecondaryAction moveSecondaryAction = new MoveSecondaryAction(currentNodeName, newNodeName, partitionSelector, false)
                {
                    ActionTimeout = operationTimeout,
                    RequestTimeout = GetRequestTimeout(operationTimeout)
                };

                await this.testContext.ActionExecutor.RunAsync(moveSecondaryAction, token).ConfigureAwait(false);
                return moveSecondaryAction.Result;
            }

            internal Task<MoveSecondaryResult> MoveSecondaryUsingNodeNameAsync(
                string currentNodeName,
                string newNodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return this.MoveSecondaryUsingNodeNameAsyncHelper(currentNodeName, newNodeName, serviceName, partitionId, ignoreConstraints, operationTimeout, token);
            }

            #endregion APIs - MoveSecondary

            #endregion APIs - Replica

            #endregion API

            #region Helper
            #region Helpers - RestartNode

            private Task<RestartNodeResult> RestartNodeUsingNodeNameAsyncHelper(string nodeName, BigInteger nodeInstance, bool shouldCreateFabricDump, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<RestartNodeResult>(
                    (callback) => this.RestartNodeUsingNodeNameBeginWrapper(nodeName, nodeInstance, shouldCreateFabricDump, timeout, callback),
                    this.RestartNodeEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.RestartNode");
            }

            private NativeCommon.IFabricAsyncOperationContext RestartNodeUsingNodeNameBeginWrapper(string nodeName, BigInteger nodeInstance, bool shouldCreateFabricDump, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var descusingnn = new RestartNodeDescriptionUsingNodeName(
                        nodeName,
                        nodeInstance,
                        shouldCreateFabricDump,
                        CompletionMode.DoNotVerify);

                    var description = new RestartNodeDescription2(
                        RestartNodeDescriptionKind.UsingNodeName,
                        descusingnn);

                    return this.nativeFaultClient.BeginRestartNode(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private RestartNodeResult RestartNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var result = (NativeClient.IFabricRestartNodeResult)this.nativeFaultClient.EndRestartNode(context);

                var nodeResult = InteropHelpers.RestartNodeHelper.CreateFromNative(result);

                return nodeResult;
            }

            #endregion Helpers - RestartNode

            #region Helpers - StartNode

            private Task<StartNodeResult> StartNodeUsingNodeNameAsyncHelper(string nodeName, BigInteger nodeInstance, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<StartNodeResult>(
                    (callback) => this.StartNodeUsingNodeNameBeginWrapper(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, timeout, callback),
                    this.StartNodeEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StartNode");
            }

            private NativeCommon.IFabricAsyncOperationContext StartNodeUsingNodeNameBeginWrapper(string nodeName, BigInteger nodeInstance, string ipAddressOrFQDN, int clusterConnectionPort, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var description = new StartNodeDescription2(
                        StartNodeDescriptionKind.UsingNodeName,
                        new StartNodeDescriptionUsingNodeName(nodeName, nodeInstance, ipAddressOrFQDN, clusterConnectionPort, CompletionMode.DoNotVerify));

                    return this.nativeFaultClient.BeginStartNode(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private StartNodeResult StartNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var gottenResult = (NativeClient.IFabricStartNodeResult)this.nativeFaultClient.EndStartNode(context);

                var startNodeResult = InteropHelpers.StartNodeHelper.CreateFromNative(gottenResult);

                return startNodeResult;
            }

            #endregion Helpers - StartNode

            #region Helpers - StopNode

            private Task<StopNodeResult> StopNodeUsingNodeNameAsyncHelper(string nodeName, BigInteger nodeInstance, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<StopNodeResult>(
                    (callback) => this.StopNodeUsingNodeNameBeginWrapper(nodeName, nodeInstance, timeout, callback),
                    this.StopNodeEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StopNode");
            }

            private NativeCommon.IFabricAsyncOperationContext StopNodeUsingNodeNameBeginWrapper(string nodeName, BigInteger nodeInstance, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var description = new StopNodeDescription2(
                        StopNodeDescriptionKind.UsingNodeName,
                        new StopNodeDescriptionUsingNodeName(nodeName, nodeInstance, CompletionMode.DoNotVerify));

                    return this.nativeFaultClient.BeginStopNode(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private StopNodeResult StopNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var gottenResult = (NativeClient.IFabricStopNodeResult)this.nativeFaultClient.EndStopNode(context);

                var stopNodeResult = InteropHelpers.StopNodeHelper.CreateFromNative(gottenResult);

                return stopNodeResult;
            }

            private Task StopNodeInternalAsyncHelper(string nodeName, BigInteger nodeInstance, int stopDurationInSeconds, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StopNodeInternalBeginWrapper(nodeName, nodeInstance, stopDurationInSeconds, timeout, callback),
                    this.StopNodeInternalEndWrapper,
                    cancellationToken,
                    "FaultAnalysisService.StopNodeInternal");
            }

            private NativeCommon.IFabricAsyncOperationContext StopNodeInternalBeginWrapper(string nodeName, BigInteger nodeInstance, int stopDurationInSeconds, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var description = new StopNodeDescriptionInternal(nodeName, nodeInstance, stopDurationInSeconds);
                    return this.nativeFaultManagementClientInternal.BeginStopNodeInternal(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StopNodeInternalEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeFaultManagementClientInternal.EndStopNodeInternal(context);                
            }

            #endregion Helpers - StopNode

            #region Helpers - RestartDeployedCodePackage
            private Task<RestartDeployedCodePackageResult> RestartDeployedCodePackageUsingNodeNameAsyncHelper(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<RestartDeployedCodePackageResult>(
                    (callback) => this.RestartDeployedCodePackageUsingNodeNameBeginWrapper(
                        nodeName, 
                        applicationName, 
                        serviceManifestName,
                        servicePackageActivationId,
                        codePackageName, 
                        codePackageInstanceId, 
                        operationTimeout, 
                        callback),
                    this.RestartDeployedCodePackageEndWrapper,
                    token,
                    "FaultAnalysisService.StopNodeRestartDeployedCodePackage");
            }

            private NativeCommon.IFabricAsyncOperationContext RestartDeployedCodePackageUsingNodeNameBeginWrapper(
                string nodeName,
                Uri applicationName,
                string serviceManifestName,
                string servicePackageActivationId,
                string codePackageName,
                long codePackageInstanceId,
                TimeSpan operationTimeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var description = new RestartDeployedCodePackageDescription2(
                        RestartDeployedCodePackageDescriptionKind.UsingNodeName,
                        new RestartDeployedCodePackageDescriptionUsingNodeName(
                            nodeName, 
                            applicationName, 
                            serviceManifestName,
                            servicePackageActivationId,
                            codePackageName, 
                            codePackageInstanceId));

                    return this.nativeFaultClient.BeginRestartDeployedCodePackage(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(operationTimeout, "timeout"),
                        callback);
                }
            }

            private RestartDeployedCodePackageResult RestartDeployedCodePackageEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var gottenResult = (NativeClient.IFabricRestartDeployedCodePackageResult)this.nativeFaultClient.EndRestartDeployedCodePackage(context);

                var restartDeployedCodePackageResult = InteropHelpers.RestartDeployedCodePackageHelper.CreateFromNative(gottenResult);

                return restartDeployedCodePackageResult;
            }
            #endregion Helpers - RestartDeployedCodePackage

            #region Helpers - MovePrimary

            private Task<MovePrimaryResult> MovePrimaryUsingNodeNameAsyncHelper(
                string nodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<MovePrimaryResult>(
                    (callback) => this.MovePrimaryUsingNodeNameBeginWrapper(nodeName, serviceName, partitionId, ignoreConstraints, operationTimeout, callback),
                    this.MovePrimaryUsingNodeNameEndWrapper,
                    token,
                    "FaultAnalysisService.MovePrimary");
            }

            private NativeCommon.IFabricAsyncOperationContext MovePrimaryUsingNodeNameBeginWrapper(
                string nodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var d = new MovePrimaryDescriptionUsingNodeName(
                        nodeName,
                        serviceName,
                        partitionId,
                        ignoreConstraints);

                    var description = new MovePrimaryDescription2(
                        MovePrimaryDescriptionKind.UsingNodeName,
                        d);

                    return this.nativeFaultClient.BeginMovePrimary(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(operationTimeout, "timeout"),
                        callback);
                }
            }

            private MovePrimaryResult MovePrimaryUsingNodeNameEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var gottenResult = (NativeClient.IFabricMovePrimaryResult)this.nativeFaultClient.EndMovePrimary(context);

                var movePrimaryResult = InteropHelpers.MovePrimaryHelper.CreateFromNative(gottenResult);

                return movePrimaryResult;
            }

            #endregion Helpers - MovePrimary

            #region Helpers - MoveSecondary

            private Task<MoveSecondaryResult> MoveSecondaryUsingNodeNameAsyncHelper(
                string currentNodeName,
                string newNodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                CancellationToken token)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<MoveSecondaryResult>(
                    (callback) => this.MoveSecondaryUsingNodeNameBeginWrapper(currentNodeName, newNodeName, serviceName, partitionId, ignoreConstraints, operationTimeout, callback),
                    this.MoveSecondaryUsingNodeNameEndWrapper,
                    token,
                    "FaultAnalysisService.RemoveReplica");
            }

            private NativeCommon.IFabricAsyncOperationContext MoveSecondaryUsingNodeNameBeginWrapper(
                string currentNodeName,
                string newNodeName,
                Uri serviceName,
                Guid partitionId,
                bool ignoreConstraints,
                TimeSpan operationTimeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var description = new MoveSecondaryDescription2(
                        MoveSecondaryDescriptionKind.UsingNodeName,
                        new MoveSecondaryDescriptionUsingNodeName(currentNodeName, newNodeName, serviceName, partitionId, ignoreConstraints));

                    return this.nativeFaultClient.BeginMoveSecondary(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(operationTimeout, "timeout"),
                        callback);
                }
            }

            private MoveSecondaryResult MoveSecondaryUsingNodeNameEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                var gottenResult = (NativeClient.IFabricMoveSecondaryResult)this.nativeFaultClient.EndMoveSecondary(context);

                var moveSecondaryResult = InteropHelpers.MoveSecondaryHelper.CreateFromNative(gottenResult);

                return moveSecondaryResult;
            }

            #endregion Helpers - MoveSecondary
            #endregion API

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

            #endregion
        }
    }
}