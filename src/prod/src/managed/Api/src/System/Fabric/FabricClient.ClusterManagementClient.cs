// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.ImageStore;
    using System.Fabric.Interop;
    using System.Fabric.Strings;
    using System.Globalization;
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Represents the cluster management client for performing cluster maintenance operations.</para>
        /// </summary>
        /// <remarks>
        /// <para>The <see cref="System.Fabric.FabricClient.ClusterManagementClient" /> provides APIs which help to manage the cluster as a whole. 
        /// These are typically administrative commands which relate to major cluster events such as the loss of nodes and the need to recover services in the case of major failures.</para>
        /// </remarks>
        public sealed class ClusterManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricClusterManagementClient10 nativeClusterClient;
            private NativeClient.IInternalFabricClusterManagementClient internalNativeClient;

            private const double RequestTimeoutFactor = 0.2;
            private const double MinRequestTimeoutInSeconds = 15.0;
            private static readonly TimeSpan MinRequestTimeout = TimeSpan.FromSeconds(MinRequestTimeoutInSeconds);

            #endregion

            #region Constructor

            internal ClusterManagementClient(FabricClient fabricClient, NativeClient.IFabricClusterManagementClient10 nativeClusterClient)
            {
                this.fabricClient = fabricClient;
                this.nativeClusterClient = nativeClusterClient;
                Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    this.internalNativeClient = (NativeClient.IInternalFabricClusterManagementClient)this.nativeClusterClient;
                },
                "ClusterManagementClient.ctor");
            }

            #endregion

            internal FabricClient FabricClient
            {
                get
                {
                    return this.fabricClient;
                }
            }

            internal NativeClient.IFabricClusterManagementClient10 NativeClient
            {
                get
                {
                    return this.nativeClusterClient;
                }
            }

            internal NativeClient.IInternalFabricClusterManagementClient InternalNativeClient
            {
                get
                {
                    return this.internalNativeClient;
                }
            }

            #region API

            /// <summary>
            /// <para>Deactivates a particular node with the specified <see cref="System.Fabric.NodeDeactivationIntent" />.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node to deactivate.</para>
            /// </param>
            /// <param name="deactivationIntent">
            /// <para>The <see cref="System.Fabric.NodeDeactivationIntent" /> for deactivating the node.</para>
            /// </param>
            /// <returns>
            /// <para>A Task that represents the asynchronous acknowledgment of the request.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     When this API completes it implies that the intent to deactivate has been registered by the system. It does not mean that the deactivation is complete. The progress of the operation can be determined by using the <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()" /> API </para>
            /// <para>
            ///     Once the deactivation is in progress, the deactivation intent can be “increased” but not decreased (for example, a node which is was deactivated with the <see cref="System.Fabric.NodeDeactivationIntent.Pause" /> intent can be deactivated further with <see cref="System.Fabric.NodeDeactivationIntent.Restart" />, but not the other way around. Nodes may be reactivated via <see cref="System.Fabric.FabricClient.ClusterManagementClient.ActivateNodeAsync(System.String)" /> any time after they are deactivated. If the deactivation is not complete this will cancel the deactivation. A node which goes down and comes back up while deactivated will still need to be reactivated before services will be placed on that node.</para>
            /// <para>
            ///     Service Fabric ensures that deactivation is a 'safe' process. It performs several safety checks (see <see cref="System.Fabric.SafetyCheckKind" />) to ensure that there is no loss of availability or data </para>
            /// </remarks>
            public Task DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent)
            {
                return this.DeactivateNodeAsync(nodeName, deactivationIntent, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deactivates a particular node with the specified <see cref="System.Fabric.NodeDeactivationIntent" />.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node to deactivate.</para>
            /// </param>
            /// <param name="deactivationIntent">
            /// <para>The <see cref="System.Fabric.NodeDeactivationIntent" /> for deactivating the node.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A Task that represents the asynchronous acknowledgment of the request.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     When this API completes it implies that the intent to deactivate has been registered by the system. It does not mean that the deactivation is complete. The progress of the operation can be determined by using the <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()" /> API </para>
            /// <para>
            ///     Once the deactivation is in progress, the deactivation intent can be “increased” but not decreased (for example, a node which is was deactivated with the <see cref="System.Fabric.NodeDeactivationIntent.Pause" /> intent can be deactivated further with <see cref="System.Fabric.NodeDeactivationIntent.Restart" />, but not the other way around. Nodes may be reactivated via <see cref="System.Fabric.FabricClient.ClusterManagementClient.ActivateNodeAsync(System.String)" /> any time after they are deactivated. If the deactivation is not complete this will cancel the deactivation. A node which goes down and comes back up while deactivated will still need to be reactivated before services will be placed on that node.</para>
            /// <para>
            ///     Service Fabric ensures that deactivation is a 'safe' process. It performs several safety checks (see <see cref="System.Fabric.SafetyCheckKind" />) to ensure that there is no loss of availability or data </para>
            /// </remarks>            
            public Task DeactivateNodeAsync(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.DeactivateNodeAsyncHelper(nodeName, deactivationIntent, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Activates a Service Fabric cluster node which is currently deactivated.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Node to be Activated.</para>
            /// </param>
            /// <returns>
            /// <para>A Task that represents the asynchronous acknowledgment of the request.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     Once activated, the node will again become a viable target for placing new replicas, and any closed replicas remaining on the node will be opened.</para>
            /// <para>
            ///     When this API completes it implies that the intent to activate has been registered by the system. It does not mean that the activation is complete. The progress of the operation can be determined by using the <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()" /> API </para>
            /// </remarks>
            public Task ActivateNodeAsync(string nodeName)
            {
                return this.ActivateNodeAsync(nodeName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Activates a Service Fabric cluster node which is currently deactivated.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The Node to be Activated.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A Task that represents the asynchronous acknowledgment of the request.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     Once activated, the node will again become a viable target for placing new replicas, and any closed replicas remaining on the node will be opened.</para>
            /// <para>
            ///     When this API completes it implies that the intent to activate has been registered by the system. It does not mean that the activation is complete. The progress of the operation can be determined by using the <see cref="System.Fabric.FabricClient.QueryClient.GetNodeListAsync()" /> API </para>
            /// </remarks>
            public Task ActivateNodeAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.ActivateNodeAsyncHelper(nodeName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Indicates that the persisted data of a node is lost (e.g., due to disk failure, or reimage, etc.), and that Service Fabric should treat any services or state on that node as lost and unrecoverable.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node which has been permanently lost.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     After a node goes down, Service Fabric will keep track of replicas of persisted services on that node as they have state on that node.</para>
            /// <para>
            ///     In cases where the administrator knows that the persisted state on a node has been permanently lost the <see cref="System.Fabric.FabricClient.ClusterManagementClient.RemoveNodeStateAsync(string)" /> method should be called ... to notify Service Fabric that the state on the node is gone (or the node can never come back with the state it had).</para>
            /// <para>
            ///     This instructs Service Fabric to stop waiting for that node (and any persisted replicas on that node) to recover.</para>
            /// <para>
            ///     NOTE: This API must be called only after it has been determined that the state on that node has been lost. </para>
            /// <para>
            ///     If this API is called and then the node comes back with its state intact it is Undefined Behavior</para>
            /// </remarks>
            public Task RemoveNodeStateAsync(string nodeName)
            {
                return this.RemoveNodeStateAsync(nodeName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Indicates that a particular node (which is down) has actually been lost, and that Service Fabric should treat any services or state on that node as lost and unrecoverable.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node which has been permanently lost.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     After a node goes down, Service Fabric will keep track of replicas of persisted services on that node as they have state on that node.</para>
            /// <para>
            ///     In cases where the administrator knows that a node (and its state) has been permanently lost the <see cref="System.Fabric.FabricClient.ClusterManagementClient.RemoveNodeStateAsync(string)" /> method should be called.</para>
            /// <para>
            ///     This instructs Service Fabric to stop waiting for that node (and any persisted replicas on that node) to recover.</para>
            /// <para>
            ///     NOTE: This API must be called only after it has been determined that the state on that node has been lost. </para>
            /// <para>
            ///     If this API is called and then the node comes back with its state intact it is Undefined Behavior</para>
            /// </remarks>
            public Task RemoveNodeStateAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrWhiteSpace();

                return this.RemoveNodeStateAsyncHelper(nodeName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover any services (including system services) which are currently stuck in quorum loss.</para>
            /// </summary>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss.</para>
            /// </remarks>
            public Task RecoverPartitionsAsync()
            {
                return this.RecoverPartitionsAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover any services (including system services) which are currently stuck in quorum loss.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>            
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>

            public Task RecoverPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.RecoverPartitionsAsyncHelper(timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover a specific partition which is currently stuck in quorum loss.</para>
            /// </summary>
            /// <param name="partitionId">The partition id to recover</param>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverPartitionAsync(Guid partitionId)
            {
                return this.RecoverPartitionAsync(partitionId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover a specific partition which is currently stuck in quorum loss.</para>
            /// </summary>
            /// <param name="partitionId">The partition id to recover</param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverPartitionAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.RecoverPartitionAsyncHelper(partitionId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover the specified service which is currently stuck in quorum loss.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service to recover.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverServicePartitionsAsync(Uri serviceName)
            {
                return this.RecoverServicePartitionsAsync(serviceName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover the specified service which is currently stuck in quorum loss by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service to recover.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverServicePartitionsAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.RecoverServicePartitionsAsyncHelper(serviceName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover the system services which are currently stuck in quorum loss.</para>
            /// </summary>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverSystemPartitionsAsync()
            {
                return this.RecoverSystemPartitionsAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Indicates to the Service Fabric cluster that it should attempt to recover the system services which are currently stuck in quorum loss.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A task representing acknowledgement of the intent.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/" /> for handling common FabricClient failures.</para>
            /// </exception>
            /// <remarks>
            /// <para>
            ///     This operation should only be performed if it is known that the replicas that are down cannot be recovered. Incorrect use of this API can cause potential data loss. </para>
            /// </remarks>
            public Task RecoverSystemPartitionsAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.RecoverSystemPartitionsAsyncHelper(timeout, cancellationToken);
            }

            /// <summary>
            /// <para> 
            /// Resets a given partition's load
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition Id represented as a Guid </para>
            /// </param>
            /// <returns>
            /// <para>The task associated with this async method. </para>
            /// </returns>
            public Task ResetPartitionLoadAsync(Guid partitionId)
            {
                return this.ResetPartitionLoadAsync(partitionId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para> 
            /// Resets a given partition's load
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition Id represented as a Guid </para>
            /// </param>
            /// <param name="timeout">
            /// <para> The length of time within which the async method must complete in order for the method to not time out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>A cancellation token for this method. </para>
            /// </param>
            /// <returns>
            /// <para>The task associated with this async method. </para>
            /// </returns>
            public Task ResetPartitionLoadAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.ResetPartitionLoadAsyncHelper(partitionId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para> 
            /// Toggles whether the Cluster Resource Balancer will report a health warning when it's unable to place a replica.
            /// </para>
            /// </summary>
            /// <param name="enabled">
            /// <para>A boolean value, which if true causes reporting when a replica is unabled to be placed. </para>
            /// </param>
            /// <returns>
            /// <para>The task associated with this async method. </para>
            /// </returns>
            /// <remarks>
            /// <para>If this method is called twice with the value false, it clears from memory the reports that would potentially have been emitted.
            /// If this method is called with the value true, the Cluster Resource Balancer will report a health warning when it's unable to place a replica.
            /// If such health warnings are blocking a monitored upgrade's health checks the toggle can be switched off. </para>
            /// </remarks>
            public Task ToggleVerboseServicePlacementHealthReportingAsync(bool enabled)
            {
                return this.ToggleVerboseServicePlacementHealthReportingAsync(enabled, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para> 
            /// Toggles whether the Cluster Resource Balancer will report a health warning when it's unable to place a replica.
            /// </para>
            /// </summary>
            /// <param name="enabled">
            /// <para>A boolean value, which if true causes reporting when a replica is unabled to be placed. </para>
            /// </param>
            /// <param name="timeout">
            /// <para> The length of time within which the async method must complete in order for the method to not time out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>A cancellation token for this method. </para>
            /// </param>
            /// <returns>
            /// <para>The task associated with this async method. </para>
            /// </returns>
            /// <remarks>
            /// <para>If this method is called twice with the value false, it clears from memory the reports that would potentially have been emitted.
            /// If this method is called with the value true, the Cluster Resource Balancer will report a health warning when it's unable to place a replica.
            /// If such health warnings are blocking a monitored upgrade's health checks, the toggle can be switched off. </para>
            /// </remarks>
            public Task ToggleVerboseServicePlacementHealthReportingAsync(bool enabled, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.ToggleVerboseServicePlacementHealthReportingAsyncHelper(enabled, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Provisions the Service Fabric.</para>
            /// </summary>
            /// <param name="patchFilePath">
            /// <para>The path to the update patch file.</para>
            /// </param>
            /// <param name="clusterManifestFilePath">
            /// <para>The path to the cluster manifest.</para>
            /// </param>
            /// <returns>
            /// <para>The provisioned Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>A <languageKeyword>null</languageKeyword> value is permitted for either the <paramref name="patchFilePath" /> parameter or the <paramref name="clusterManifestFilePath" /> parameter. A <languageKeyword>null</languageKeyword> value cannot be used for both parameters.</para>
            /// <para>This will upload the patch file and/or cluster manifest file to the image store location. The image store location is specified as a configuration setting in the cluster manifest that was provided when the cluster was created.</para>
            /// <para>Cluster manifest validation will occur within the context of this call.</para>
            /// </remarks>
            public Task ProvisionFabricAsync(string patchFilePath, string clusterManifestFilePath)
            {
                return this.ProvisionFabricAsync(patchFilePath, clusterManifestFilePath, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Provisions the Service Fabric by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="patchFilePath">
            /// <para>The path to the update patch file.</para>
            /// </param>
            /// <param name="clusterManifestFilePath">
            /// <para>The path to the cluster manifest.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The provisioned Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>A <languageKeyword>null</languageKeyword> value is permitted for either the <paramref name="patchFilePath" /> parameter or the <paramref name="clusterManifestFilePath" />
            /// parameter. A <languageKeyword>null</languageKeyword> value cannot be used for both parameters.</para>
            /// <para>This will upload the patch file and/or cluster manifest file to the image store location. The image store location is specified as a configuration setting
            /// in the cluster manifest that was provided when the cluster was created.</para>
            /// <para>Cluster manifest validation will occur within the context of this call.</para>
            /// </remarks>
            public Task ProvisionFabricAsync(string patchFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.ProvisionFabricAsyncHelper(patchFilePath, clusterManifestFilePath, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Unprovisions the Service Fabric.</para>
            /// </summary>
            /// <param name="codeVersion">
            /// <para>The code version to unprovision.</para>
            /// </param>
            /// <param name="configVersion">
            /// <para>The configuration version to unprovision.</para>
            /// </param>
            /// <returns>
            /// <para>The unprovisioned Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>A <languageKeyword>null</languageKeyword> value is permitted for either the <paramref name="codeVersion" /> parameter or the <paramref name="configVersion" /> parameter. A <languageKeyword>null</languageKeyword> value cannot be used for both parameters.</para>
            /// <para>This will delete the patch file and/or cluster manifest file from the image store location. The image store location is specified as a configuration setting in the cluster manifest that was provided when the cluster was created.</para>
            /// </remarks>
            public Task UnprovisionFabricAsync(string codeVersion, string configVersion)
            {
                return this.UnprovisionFabricAsync(codeVersion, configVersion, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Unprovisions the Service Fabric by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="codeVersion">
            /// <para>The code version to unprovision.</para>
            /// </param>
            /// <param name="configVersion">
            /// <para>The configuration version to unprovision.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The unprovisioned Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task UnprovisionFabricAsync(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.UnprovisionFabricAsyncHelper(codeVersion, configVersion, timeout, cancellationToken);
            }
            
            /// <summary>
            /// <para>Upgrades the Service Fabric.</para>
            /// </summary>
            /// <param name="upgradeDescription">
            /// <para>The description of the upgrade.</para>
            /// </param>
            /// <returns>
            /// <para>The upgraded Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task UpgradeFabricAsync(FabricUpgradeDescription upgradeDescription)
            {
                return this.UpgradeFabricAsync(upgradeDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Upgrades the Service Fabric by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="upgradeDescription">
            /// <para>The description of the upgrade.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The upgraded Service Fabric.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task UpgradeFabricAsync(FabricUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<FabricUpgradeDescription>("upgradeDescription", upgradeDescription).NotNull();
                FabricUpgradeDescription.Validate(upgradeDescription);

                return this.UpgradeFabricAsyncHelper(upgradeDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Modifies the upgrade parameters that describe the behavior of the current cluster upgrade.</para>
            /// </summary>
            /// <param name="updateDescription">
            /// <para>Description of the new upgrade parameters to apply.</para>
            /// </param>
            /// <returns>
            /// <para>The current cluster upgrade.</para>
            /// </returns>
            /// <remarks />
            public Task UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription)
            {
                return this.UpdateFabricUpgradeAsync(updateDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Modifies the upgrade parameters that describe the behavior of the current cluster upgrade.</para>
            /// </summary>
            /// <param name="updateDescription">
            /// <para> The new upgrade parameters to apply.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before throwing a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The current cluster upgrade.</para>
            /// </returns>
            public Task UpdateFabricUpgradeAsync(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<FabricUpgradeUpdateDescription>("updateDescription", updateDescription).NotNull();
                FabricUpgradeUpdateDescription.Validate(updateDescription);

                return this.UpdateFabricUpgradeAsyncHelper(updateDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Rolls back the Service Fabric to upgrade the operation.</para>
            /// </summary>
            /// <returns>
            /// <para>The rollback Service Fabric to upgrade the operation.</para>
            /// </returns>
            public Task RollbackFabricUpgradeAsync()
            {
                return this.RollbackFabricUpgradeAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Rolls back the Service Fabric to upgrade the operation.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a timeout exception.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The rollback Service Fabric to upgrade the operation.</para>
            /// </returns>
            public Task RollbackFabricUpgradeAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.RollbackFabricUpgradeAsyncHelper(timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Returns the progress of a Service Fabric upgrade process.</para>
            /// </summary>
            /// <returns>
            /// <para>The progress of a Service Fabric upgrade process.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync()
            {
                return this.GetFabricUpgradeProgressAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Returns the progress of a Service Fabric upgrade process.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The progress of a Service Fabric upgrade process.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetFabricUpgradeProgressAsyncHelper(timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Instructs the Service Fabric to upgrade the next upgrade domain in the cluster if the current upgrade domain has been completed.</para>
            /// </summary>
            /// <param name="upgradeProgress">
            /// <para>The fabric upgrade process object to use.</para>
            /// </param>
            /// <returns>
            /// <para>The upgraded domain in the cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>Similar to <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress)" />.</para>
            /// </remarks>
            public Task MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress upgradeProgress)
            {
                return this.MoveNextFabricUpgradeDomainAsync(upgradeProgress, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Instructs Service Fabric to upgrade the next upgrade domain in the cluster if the current upgrade domain has been completed, by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="upgradeProgress">
            /// <para>The fabric upgrade process object to use.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The upgraded domain in the cluster.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>Similar to <see cref="System.Fabric.FabricClient.ApplicationManagementClient.MoveNextApplicationUpgradeDomainAsync(System.Fabric.ApplicationUpgradeProgress,System.TimeSpan,System.Threading.CancellationToken)" />.</para>
            /// </remarks>
            public Task MoveNextFabricUpgradeDomainAsync(FabricUpgradeProgress upgradeProgress, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<FabricUpgradeProgress>("upgradeProgress", upgradeProgress).NotNull();

                return this.MoveNextFabricUpgradeDomainAsyncHelper(upgradeProgress, timeout, cancellationToken);
            }

            internal Task MoveNextFabricUpgradeDomainAsync(string nextDomain)
            {
                return this.MoveNextFabricUpgradeDomainAsync(nextDomain, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task MoveNextFabricUpgradeDomainAsync(string nextDomain, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nextDomain", nextDomain).NotNull();

                return this.MoveNextFabricUpgradeDomainAsyncHelper(nextDomain, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the XML contents of the current running cluster manifest.</para>
            /// </summary>
            /// <returns>
            /// <para>The cluster manifest contents.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<string> GetClusterManifestAsync()
            {
                return this.GetClusterManifestAsync(new ClusterManifestQueryDescription(), FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the XML contents of the current running cluster manifest.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The cluster manifest contents.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using 
            /// and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<string> GetClusterManifestAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetClusterManifestAsync(new ClusterManifestQueryDescription(), timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the XML contents of a cluster manifest as specified by <paramref name="queryDescription" />.</para>
            /// </summary>
            /// <param name="queryDescription">
            /// <para>Specifies additional parameters to determine which cluster manifest to retrieve.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The cluster manifest contents.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using 
            /// and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<string> GetClusterManifestAsync(ClusterManifestQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ClusterManifestQueryDescription>("queryDescription", queryDescription).NotNull();

                return this.GetClusterManifestAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            internal Task<bool> StartInfrastructureTaskAsync(InfrastructureTaskDescription description)
            {
                return this.StartInfrastructureTaskAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task<bool> StartInfrastructureTaskAsync(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<InfrastructureTaskDescription>("description", description).NotNull();

                return this.StartInfrastructureTaskAsyncHelper(description, timeout, cancellationToken);
            }

            internal Task<bool> FinishInfrastructureTaskAsync(string taskId, long instanceId)
            {
                return this.FinishInfrastructureTaskAsync(taskId, instanceId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task<bool> FinishInfrastructureTaskAsync(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("taskId", taskId).NotNull();

                return this.FinishInfrastructureTaskAsyncHelper(taskId, instanceId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Copies the cluster manifest file and/or Service Fabric code package to the image store.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="clusterManifestPath">
            /// <para>The full path to the cluster manifest file to be copied.</para>
            /// </param>
            /// <param name="clusterManifestPathInImageStore">
            /// <para>The relative path along with the file name of the destination in the image store. This parameter is required when clusterManifestPath is specified. This path is created relative to the root directory in the image store and used as the destination for the cluster manifest copy.</para>
            /// </param>
            /// <param name="codePackagePath">
            /// <para>The full path to the Service Fabric code package to be copied.</para>
            /// </param>
            /// <param name="codePackagePathInImageStore">
            /// <para>The relative path along with the file name of the destination in the image store. This parameter is required when codePackagePathInImageStore is specified. This path is created relative to the root directory in the image store and used as the destination for the code package copy.</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            /// <para>A required file was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            /// <para>A required directory was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            /// <para>A path to an image store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Both source cluster manifest path and source code path cannot be null.</para>
            /// </remarks>
            public void CopyClusterPackage(
                string imageStoreConnectionString, 
                string clusterManifestPath, 
                string clusterManifestPathInImageStore,
                string codePackagePath,
                string codePackagePathInImageStore)
            {
                this.fabricClient.ThrowIfDisposed();
                // Current default timeout
                TimeSpan nativeImageStoreDefaultTimeout = TimeSpan.FromMinutes(30);

                CopyClusterPackage(imageStoreConnectionString, clusterManifestPath, clusterManifestPathInImageStore, codePackagePath, codePackagePathInImageStore, nativeImageStoreDefaultTimeout);
            }

            /// <summary>
            /// <para>Copies the cluster manifest file and/or Service Fabric code package to the image store.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="clusterManifestPath">
            /// <para>The full path to the cluster manifest file to be copied.</para>
            /// </param>
            /// <param name="clusterManifestPathInImageStore">
            /// <para>The relative path along with the file name of the destination in the image store. This parameter is required when clusterManifestPath is specified. This path is created relative to the root directory in the image store and used as the destination for the cluster manifest copy.</para>
            /// </param>
            /// <param name="codePackagePath">
            /// <para>The full path to the Service Fabric code package to be copied.</para>
            /// </param>
            /// <param name="codePackagePathInImageStore">
            /// <para>The relative path along with the file name of the destination in the image store. This parameter is required when codePackagePathInImageStore is specified. This path is created relative to the root directory in the image store and used as the destination for the code package copy.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            /// <para>A required file was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            /// <para>A required directory was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            /// <para>A path to an image store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Both source cluster manifest path and source code path cannot be null.</para>
            /// </remarks>
            public void CopyClusterPackage(
                string imageStoreConnectionString,
                string clusterManifestPath,
                string clusterManifestPathInImageStore,
                string codePackagePath,
                string codePackagePathInImageStore,
                TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();

                if (string.IsNullOrEmpty(clusterManifestPath) && string.IsNullOrEmpty(codePackagePath))
                {
                    throw new ArgumentException(
                        string.Format(CultureInfo.CurrentCulture, StringResources.Error_TwoArgumentsNull, "clusterManifestPath", "codePackagePath"));
                }

                if (!string.IsNullOrEmpty(clusterManifestPath))
                {
                    Requires.Argument<string>("clusterManifestPathInImageStore", clusterManifestPathInImageStore).NotNullOrEmpty();

                    this.fabricClient.imageStoreClient.UploadContent(
                        imageStoreConnectionString,
                        clusterManifestPathInImageStore,
                        clusterManifestPath,
                        timeout,
                        CopyFlag.AtomicCopy,
                        false);
                }

                if (!string.IsNullOrEmpty(codePackagePath))
                {
                    Requires.Argument<string>("codePackagePathInImageStore", codePackagePathInImageStore).NotNullOrEmpty();

                    this.fabricClient.imageStoreClient.UploadContent(
                        imageStoreConnectionString,
                        codePackagePathInImageStore,
                        codePackagePath,                     
                        timeout,
                        CopyFlag.AtomicCopy,
                        false);
                }
            }

            /// <summary>
            /// <para>Deletes the cluster manifest file and/or Service Fabric code package from the image store.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="clusterManifestPathInImageStore">
            /// <para>The relative path of cluster manifest file in the image store specified during <see cref="System.Fabric.FabricClient.ClusterManagementClient.CopyClusterPackage(string,string,string,string,string)"/>.</para>
            /// </param>
            /// <param name="codePackagePathInImageStore">
            /// <para>The relative path of Service Fabric code package in the image store specified during <see cref="System.Fabric.FabricClient.ClusterManagementClient.CopyClusterPackage(string,string,string,string,string)"/>.</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Either clusterManifestPathInImageStore or codePackagePathInImageStore parameter can be <languageKeyword>null</languageKeyword>. However, both of them cannot be <languageKeyword>null</languageKeyword>.</para>
            /// </remarks>
            public void RemoveClusterPackage(string imageStoreConnectionString, string clusterManifestPathInImageStore, string codePackagePathInImageStore)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();

                if (string.IsNullOrEmpty(clusterManifestPathInImageStore) && string.IsNullOrEmpty(codePackagePathInImageStore))
                {
                    throw new ArgumentException(
                        string.Format(CultureInfo.CurrentCulture, StringResources.Error_TwoArgumentsNull, "clusterManifestPathInImageStore", "codePackagePathInImageStore"));
                }

                IImageStore imageStore = fabricClient.GetImageStore(imageStoreConnectionString);
                TimeSpan defaultTimeout = GetImageStoreDefaultTimeout(imageStoreConnectionString);

                if (!string.IsNullOrEmpty(clusterManifestPathInImageStore))
                {
                    imageStore.DeleteContent(clusterManifestPathInImageStore, defaultTimeout);
                }

                if (!string.IsNullOrEmpty(codePackagePathInImageStore))
                {
                    imageStore.DeleteContent(codePackagePathInImageStore,defaultTimeout);
                }
            }

            #region API - UpgradeConfigurationAsync

            /// <summary>
            /// Initiate an Upgrade using a cluster configuration file.
            /// </summary>
            /// <param name="description">Contains:
            /// ClusterConfig, HealthCheckRetryTimeout, HealthCheckWaitDuration,
            /// HealthCheckStableDuration, UpgradeDomainTimeout, UpgradeTimeout,
            /// MaxPercentUnhealthyApplications, MaxPercentUnhealthyNodes, MaxPercentDeltaUnhealthyNodes,
            /// MaxPercentUpgradeDomainDeltaUnhealthyNodes
            /// </param>
            /// <returns>Task</returns>
            public Task UpgradeConfigurationAsync(ConfigurationUpgradeDescription description)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.UpgradeConfigurationAsync(
                    description,
                    FabricClient.DefaultTimeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// Initiate an Upgrade using a cluster configuration file.
            /// </summary>
            /// <param name="description">Contains:
            /// ClusterConfig, HealthCheckRetryTimeout, HealthCheckWaitDuration,
            /// HealthCheckStableDuration, UpgradeDomainTimeout, UpgradeTimeout,
            /// MaxPercentUnhealthyApplications, MaxPercentUnhealthyNodes, MaxPercentDeltaUnhealthyNodes,
            /// MaxPercentUpgradeDomainDeltaUnhealthyNodes</param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>Task</returns>
            public Task UpgradeConfigurationAsync(
                ConfigurationUpgradeDescription description,
                TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.UpgradeConfigurationAsync(
                    description,
                    timeout,
                    CancellationToken.None);
            }

            /// <summary>
            /// Initiate an Upgrade using a cluster configuration file.
            /// </summary>
            /// <param name="description">Contains:
            /// ClusterConfigPath, HealthCheckRetryTimeout, HealthCheckWaitDuration,
            /// HealthCheckStableDuration, UpgradeDomainTimeout, UpgradeTimeout,
            /// MaxPercentUnhealthyApplications, MaxPercentUnhealthyNodes, MaxPercentDeltaUnhealthyNodes,
            /// MaxPercentUpgradeDomainDeltaUnhealthyNodes</param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>Task</returns>
            public Task UpgradeConfigurationAsync(
                ConfigurationUpgradeDescription description,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.UpgradeConfigurationAsync(
                    description,
                    FabricClient.DefaultTimeout,
                    cancellationToken);
            }

            /// <summary>
            /// Initiate an Upgrade using a cluster configuration file.
            /// </summary>
            /// <param name="description">Contains:
            /// ClusterConfig, HealthCheckRetryTimeout, HealthCheckWaitDuration,
            /// HealthCheckStableDuration, UpgradeDomainTimeout, UpgradeTimeout,
            /// MaxPercentUnhealthyApplications, MaxPercentUnhealthyNodes, MaxPercentDeltaUnhealthyNodes,
            /// MaxPercentUpgradeDomainDeltaUnhealthyNodes</param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>Task</returns>
            public Task UpgradeConfigurationAsync(
                ConfigurationUpgradeDescription description,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartClusterConfigurationUpgradeAsyncHelper(
                    description,
                    timeout,
                    cancellationToken);
            }

            #endregion API - UpgradeConfigurationAsync

            #region API - GetClusterConfigurationUpgradeStatusAsync

            /// <summary>
            /// Obtains the status of an upgrade in progress.
            /// </summary>
            /// <returns>FabricOrchestrationUpgradeProgress</returns>
            public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync()
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetClusterConfigurationUpgradeStatusAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Obtains the status of an upgrade in progress.
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>FabricOrchestrationUpgradeProgress</returns>
            public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
                TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetClusterConfigurationUpgradeStatusAsync(timeout, CancellationToken.None);
            }

            /// <summary>
            /// Obtains the status of an upgrade in progress.
            /// </summary>
            /// <param name="cancellationToken"></param>
            /// <returns>FabricOrchestrationUpgradeProgress</returns>
            public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetClusterConfigurationUpgradeStatusAsync(FabricClient.DefaultTimeout, cancellationToken);
            }

            /// <summary>
            /// Obtains the status of an upgrade in progress.
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>FabricOrchestrationUpgradeProgress</returns>
            public Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsync(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetClusterConfigurationUpgradeStatusAsyncHelper(
                    timeout,
                    cancellationToken);
            }
            #endregion API - GetClusterConfigurationUpgradeStatusAsync

            #region Get Cluster Configuration Async

            /// <summary>
            /// <para>Gets the Service Fabric cluster configuration file as a string.</para>
            /// </summary>
            /// <returns>
            /// <para>The Service Fabric cluster configuration file as a string.</para>
            /// </returns>
            public Task<string> GetClusterConfigurationAsync()
            {
                return this.GetClusterConfigurationAsync(null);
            }

            /// <summary>
            /// <para>Gets the Service Fabric cluster configuration file as a string.</para>
            /// </summary>
            /// <param name="apiVersion">Api version.</param>
            /// <returns>
            /// <para>The Service Fabric cluster configuration file as a string.</para>
            /// </returns>
            public Task<string> GetClusterConfigurationAsync(string apiVersion)
            {
                return this.GetClusterConfigurationAsync(apiVersion, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the Service Fabric cluster configuration file as a string, by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Fabric cluster configuration file as a string, by using the specified timeout and cancellation token.</para>
            /// </returns>
            public Task<string> GetClusterConfigurationAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetClusterConfigurationAsync(null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the Service Fabric cluster configuration file as a string, by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="apiVersion">Api verison.</param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Fabric cluster configuration file as a string, by using the specified timeout and cancellation token.</para>
            /// </returns>
            public Task<string> GetClusterConfigurationAsync(string apiVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetClusterConfigurationAsyncHelper(apiVersion, timeout, cancellationToken);
            }

            #endregion

            #region API - GetUpgradesPendingApprovalAsync

            internal Task GetUpgradesPendingApprovalAsync()
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetUpgradesPendingApprovalAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task GetUpgradesPendingApprovalAsync(
                TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetUpgradesPendingApprovalAsync(timeout, CancellationToken.None);
            }

            internal Task GetUpgradesPendingApprovalAsync(
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetUpgradesPendingApprovalAsync(FabricClient.DefaultTimeout, cancellationToken);
            }

            internal Task GetUpgradesPendingApprovalAsync(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.GetUpgradesPendingApprovalAsyncHelper(
                    timeout,
                    cancellationToken);
            }

            #endregion API - GetUpgradesPendingApprovalAsync

            #region API - StartApprovedUpgradesAsync

            internal Task StartApprovedUpgradesAsync()
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartApprovedUpgradesAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task StartApprovedUpgradesAsync(TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartApprovedUpgradesAsync(timeout, CancellationToken.None);
            }

            internal Task StartApprovedUpgradesAsync(CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartApprovedUpgradesAsync(FabricClient.DefaultTimeout, cancellationToken);
            }

            internal Task StartApprovedUpgradesAsync(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                return this.StartApprovedUpgradesAsyncHelper(
                    timeout,
                    cancellationToken);
            }
            #endregion API - StartApprovedUpgradesAsync

            #region API - Get Upgrade Orchestration Service State

            /// <summary>
            /// <para>Gets the Service Fabric Upgrade Orchestration Service state as a string.</para>
            /// </summary>
            /// <returns>
            /// <para>The Service Fabric Upgrade Orchestration Service state as a string.</para>
            /// </returns>
            public Task<string> GetUpgradeOrchestrationServiceStateAsync()
            {
                return this.GetUpgradeOrchestrationServiceStateAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the Service Fabric Upgrade Orchestration Service state as a string, by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Fabric Upgrade Orchestration Service state as a string, by using the specified timeout and cancellation token.</para>
            /// </returns>
            public Task<string> GetUpgradeOrchestrationServiceStateAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetUpgradeOrchestrationServiceStateAsyncHelper(timeout, cancellationToken);
            }
            #endregion

            #region API - Set Upgrade Orchestration Service State

            /// <summary>
            /// <para>Sets the Service Fabric Upgrade Orchestration Service state as a string.</para>
            /// </summary>
            /// <param name="state">state input</param>
            /// <returns>Task</returns>
            public Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsync(string state)
            {
                return this.SetUpgradeOrchestrationServiceStateAsync(state, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Sets the Service Fabric Upgrade Orchestration Service state as a string, by using the specified timeout and cancellation token.</para>
            /// </summary>
            /// <param name="state">state input</param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. 
            /// Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Fabric Upgrade Orchestration Service state, by using the specified timeout and cancellation token.</para>
            /// </returns>
            public Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsync(string state, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.SetUpgradeOrchestrationServiceStateAsyncHelper(state, timeout, cancellationToken);
            }
            #endregion


            #endregion

            #region Helpers

            #region Deactivate Node Async

            private Task DeactivateNodeAsyncHelper(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeactivateNodeBeginWrapper(nodeName, deactivationIntent, timeout, callback),
                    this.DeactivateNodeEndWrapper,
                    cancellationToken,
                    "ClusterManager.DeactivateNodeAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext DeactivateNodeBeginWrapper(string nodeName, NodeDeactivationIntent deactivationIntent, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginDeactivateNode(
                        pin.AddBlittable(nodeName),
                        (NativeTypes.FABRIC_NODE_DEACTIVATION_INTENT)deactivationIntent,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeactivateNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndDeactivateNode(context);
            }

            #endregion

            #region Activate Node Async

            private Task ActivateNodeAsyncHelper(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.ActivateNodeBeginWrapper(nodeName, timeout, callback),
                    this.ActivateNodeEndWrapper,
                    cancellationToken,
                    "ClusterManager.ActivateNodeAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext ActivateNodeBeginWrapper(string nodeName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginActivateNode(
                        pin.AddBlittable(nodeName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void ActivateNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndActivateNode(context);
            }

            #endregion

            #region Remove Node State Async

            private Task RemoveNodeStateAsyncHelper(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RemoveNodeStateBeginWrapper(nodeName, timeout, callback),
                    this.RemoveNodeStateEndWrapper,
                    cancellationToken,
                    "ClusterManager.RemoveNodeState");
            }

            private NativeCommon.IFabricAsyncOperationContext RemoveNodeStateBeginWrapper(string nodeName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                   return this.nativeClusterClient.BeginNodeStateRemoved(
                        pin.AddBlittable(nodeName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RemoveNodeStateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndNodeStateRemoved(context);
            }

            #endregion
            
            #region Recover Partitions Async

            private Task RecoverPartitionsAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RecoverPartitionsBeginWrapper(timeout, callback),
                    this.RecoverPartitionsEndWrapper,
                    cancellationToken,
                    "ClusterManager.RecoverPartitionsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RecoverPartitionsBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginRecoverPartitions(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void RecoverPartitionsEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndRecoverPartitions(context);
            }
            #endregion

            #region Recover Partition Async

            private Task RecoverPartitionAsyncHelper(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RecoverPartitionBeginWrapper(partitionId, timeout, callback),
                    this.RecoverPartitionEndWrapper,
                    cancellationToken,
                    "ClusterManager.RecoverPartitionAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RecoverPartitionBeginWrapper(Guid partitionId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginRecoverPartition(
                    partitionId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void RecoverPartitionEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndRecoverPartition(context);
            }
            #endregion

            #region Recover Service Partitions Async

            private Task RecoverServicePartitionsAsyncHelper(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RecoverServicePartitionsBeginWrapper(serviceName, timeout, callback),
                    this.RecoverServicePartitionsEndWrapper,
                    cancellationToken,
                    "ClusterManager.RecoverServicePartitionsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RecoverServicePartitionsBeginWrapper(Uri serviceName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(serviceName))
                {
                    return this.nativeClusterClient.BeginRecoverServicePartitions(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RecoverServicePartitionsEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndRecoverServicePartitions(context);
            }
            #endregion

            #region Recover System Partitions Async

            private Task RecoverSystemPartitionsAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RecoverSystemPartitionsBeginWrapper(timeout, callback),
                    this.RecoverSystemPartitionsEndWrapper,
                    cancellationToken,
                    "ClusterManager.RecoverSystemPartitionsAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext RecoverSystemPartitionsBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginRecoverSystemPartitions(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void RecoverSystemPartitionsEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndRecoverSystemPartitions(context);
            }
            #endregion

            #region Reset FailoverUnit Load Async

            private Task ResetPartitionLoadAsyncHelper(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.ResetPartitionLoadBeginWrapper(partitionId, timeout, callback),
                    this.ResetPartitionLoadEndWrapper,
                    cancellationToken,
                    "ClusterManager.ResetPartitionLoadAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext ResetPartitionLoadBeginWrapper(Guid partitionId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginResetPartitionLoad(
                    partitionId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void ResetPartitionLoadEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndResetPartitionLoad(context);
            }
            #endregion

            #region Toggle Verbose ServicePlacement HealthReporting Async

            private Task ToggleVerboseServicePlacementHealthReportingAsyncHelper(bool enabled, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.ToggleVerboseServicePlacementHealthReportingBeginWrapper(enabled, timeout, callback),
                    this.ToggleVerboseServicePlacementHealthReportingEndWrapper,
                    cancellationToken,
                    "ClusterManager.ToggleVerboseServicePlacementHealthReportingAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext ToggleVerboseServicePlacementHealthReportingBeginWrapper(bool enabled, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginToggleVerboseServicePlacementHealthReporting(
                    enabled,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void ToggleVerboseServicePlacementHealthReportingEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndToggleVerboseServicePlacementHealthReporting(context);
            }
            #endregion

            #region Provision Fabric

            private Task ProvisionFabricAsyncHelper(string patchFilePath, string clusterManifestFilePath, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.ProvisionFabricBeginWrapper(patchFilePath, clusterManifestFilePath, timeout, callback),
                    this.ProvisionFabricEndWrapper,
                    cancellationToken,
                    "ClusterManager.ProvisionFabricAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext ProvisionFabricBeginWrapper(string patchFilePath, string clusterManifestFilePath, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginProvisionFabric(
                        pin.AddObject(patchFilePath),
                        pin.AddObject(clusterManifestFilePath),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void ProvisionFabricEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndProvisionFabric(context);
            }

            #endregion

            #region Unprovision Fabric Async

            private Task UnprovisionFabricAsyncHelper(string codeVersion, string configVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UnprovisionFabricBeginWrapper(codeVersion, configVersion, timeout, callback),
                    this.UnprovisionFabricEndWrapper,
                    cancellationToken,
                    "ClusterManager.UnprovisionFabric");
            }

            private NativeCommon.IFabricAsyncOperationContext UnprovisionFabricBeginWrapper(string codeVersion, string configVersion, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginUnprovisionFabric(
                        pin.AddObject(codeVersion),
                        pin.AddObject(configVersion),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UnprovisionFabricEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndUnprovisionFabric(context);
            }

            #endregion

            #region Upgrade Fabric Async

            private Task UpgradeFabricAsyncHelper(FabricUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpgradeFabricBeginWrapper(upgradeDescription, timeout, callback),
                    this.UpgradeFabricEndWrapper,
                    cancellationToken,
                    "ClusterManager.UpgradeFabric");
            }

            private NativeCommon.IFabricAsyncOperationContext UpgradeFabricBeginWrapper(FabricUpgradeDescription upgradeDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginUpgradeFabric(
                        upgradeDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpgradeFabricEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndUpgradeFabric(context);
            }

            #endregion

            #region Update Fabric Upgrade Async

            private Task UpdateFabricUpgradeAsyncHelper(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpdateFabricUpgradeBeginWrapper(updateDescription, timeout, callback),
                    this.UpdateFabricUpgradeEndWrapper,
                    cancellationToken,
                    "ClusterManager.UpdateFabricUpgrade");
            }

            private NativeCommon.IFabricAsyncOperationContext UpdateFabricUpgradeBeginWrapper(FabricUpgradeUpdateDescription updateDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginUpdateFabricUpgrade(
                        updateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpdateFabricUpgradeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndUpdateFabricUpgrade(context);
            }

            #endregion

            #region Rollback Fabric Upgrade Async

            private Task RollbackFabricUpgradeAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RollbackFabricUpgradeBeginWrapper(timeout, callback),
                    this.RollbackFabricUpgradeEndWrapper,
                    cancellationToken,
                    "ClusterManager.RollbackFabricUpgrade");
            }

            private NativeCommon.IFabricAsyncOperationContext RollbackFabricUpgradeBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginRollbackFabricUpgrade(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void RollbackFabricUpgradeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndRollbackFabricUpgrade(context);
            }

            #endregion

            #region Get Fabric Upgrade Progress Async

            private Task<FabricUpgradeProgress> GetFabricUpgradeProgressAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<FabricUpgradeProgress>(
                    (callback) => this.GetFabricUpgradeProgressBeginWrapper(timeout, callback),
                    this.GetFabricUpgradeProgressEndWrapper,
                    cancellationToken,
                    "ClusterManager.GetFabricUpgradeProgress");
            }

            private NativeCommon.IFabricAsyncOperationContext GetFabricUpgradeProgressBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginGetFabricUpgradeProgress(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private FabricUpgradeProgress GetFabricUpgradeProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.FabricUpgradeProgressHelper.CreateFromNative(
                    (NativeClient.IFabricUpgradeProgressResult3)this.nativeClusterClient.EndGetFabricUpgradeProgress(context));
            }

            #endregion

            #region Move Next Fabric Upgrade Domain Async

            private Task MoveNextFabricUpgradeDomainAsyncHelper(FabricUpgradeProgress upgradeProgress, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.MoveNextFabricUpgradeDomainAsyncBeginWrapper(upgradeProgress, timeout, callback),
                    this.MoveNextFabricUpgradeDomainAsyncEndWrapper,
                    cancellationToken,
                    "ClusterManager.MoveNextFabricUpgradeDomain");
            }

            private NativeCommon.IFabricAsyncOperationContext MoveNextFabricUpgradeDomainAsyncBeginWrapper(FabricUpgradeProgress upgradeProgress, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeClusterClient.BeginMoveNextFabricUpgradeDomain(
                    upgradeProgress == null ? null : upgradeProgress.InnerProgress,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void MoveNextFabricUpgradeDomainAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndMoveNextFabricUpgradeDomain(context);
            }

            private Task MoveNextFabricUpgradeDomainAsyncHelper(string nextDomain, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.MoveNextFabricUpgradeDomainAsyncBeginWrapper2(nextDomain, timeout, callback),
                    this.MoveNextFabricUpgradeDomainAsyncEndWrapper2,
                    cancellationToken,
                    "ClusterManager.MoveNextFabricUpgradeDomain2");
            }

            private NativeCommon.IFabricAsyncOperationContext MoveNextFabricUpgradeDomainAsyncBeginWrapper2(string nextDomain, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginMoveNextFabricUpgradeDomain2(
                        pin.AddObject(nextDomain),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void MoveNextFabricUpgradeDomainAsyncEndWrapper2(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndMoveNextFabricUpgradeDomain2(context);
            }

            #endregion

            #region Infrastructure Task

            private Task<bool> StartInfrastructureTaskAsyncHelper(InfrastructureTaskDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<bool>(
                    (callback) => this.StartInfrastructureTaskAsyncBeginWrapper(description, timeout, callback),
                    this.StartInfrastructureTaskAsyncEndWrapper,
                    cancellationToken,
                    "ClusterManager.StartInfrastructureTask");
            }

            private NativeCommon.IFabricAsyncOperationContext StartInfrastructureTaskAsyncBeginWrapper(InfrastructureTaskDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeClient.BeginStartInfrastructureTask(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private bool StartInfrastructureTaskAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return (this.internalNativeClient.EndStartInfrastructureTask(context) != 0);
            }

            private Task<bool> FinishInfrastructureTaskAsyncHelper(string taskId, long instanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<bool>(
                    (callback) => this.FinishInfrastructureTaskAsyncBeginWrapper(taskId, instanceId, timeout, callback),
                    this.FinishInfrastructureTaskAsyncEndWrapper,
                    cancellationToken,
                    "ClusterManager.FinishInfrastructureTask");
            }

            private NativeCommon.IFabricAsyncOperationContext FinishInfrastructureTaskAsyncBeginWrapper(string taskId, long instanceId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeClient.BeginFinishInfrastructureTask(
                        pin.AddObject(taskId),
                        (ulong)instanceId,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private bool FinishInfrastructureTaskAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return (this.internalNativeClient.EndFinishInfrastructureTask(context) != 0);
            }

            #endregion

            #region Get Cluster Manifest Async

            private Task<string> GetClusterManifestAsyncHelper(ClusterManifestQueryDescription queryDescription ,TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<string>(
                    (callback) => this.GetClusterManifestAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetClusterManifestAsyncEndWrapper,
                    cancellationToken,
                    "ClusterManager.GetClusterManifest");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterManifestAsyncBeginWrapper(ClusterManifestQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginGetClusterManifest2(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);             
                }
            }

            private string GetClusterManifestAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetClusterManifestHelper.CreateFromNative(this.nativeClusterClient.EndGetClusterManifest2(context));
            }

            #endregion

            #region StartClusterConfigurationUpgrade

            private Task StartClusterConfigurationUpgradeAsyncHelper(
                ConfigurationUpgradeDescription description,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartClusterConfigurationUpgradeAsyncBeginWrapper(
                    description,
                    timeout,
                    callback),
                    this.StartClusterConfigurationUpgradeAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.StartClusterConfigurationUpgradeAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartClusterConfigurationUpgradeAsyncBeginWrapper(
                ConfigurationUpgradeDescription description,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginUpgradeConfiguration(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartClusterConfigurationUpgradeAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndUpgradeConfiguration(context);
            }
            #endregion StartClusterConfigurationUpgrade

            #region GetClusterConfigurationUpgradeStatus
            private Task<FabricOrchestrationUpgradeProgress> GetClusterConfigurationUpgradeStatusAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<FabricOrchestrationUpgradeProgress>(
                    (callback) => this.GetClusterConfigurationUpgradeStatusAsyncBeginWrapper(
                        timeout,
                        callback),
                    this.GetClusterConfigurationUpgradeStatusAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.GetClusterConfigurationUpgradeStatusAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterConfigurationUpgradeStatusAsyncBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginGetClusterConfigurationUpgradeStatus(
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private FabricOrchestrationUpgradeProgress GetClusterConfigurationUpgradeStatusAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.OrchestrationUpgradeProgressHelper.CreateFromNative(
                    (NativeClient.IFabricOrchestrationUpgradeStatusResult)this.nativeClusterClient.EndGetClusterConfigurationUpgradeStatus(context));
            }
            #endregion GetClusterConfigurationUpgradeStatus

            #region GetClusterConfiguration
            private Task<String> GetClusterConfigurationAsyncHelper(
                string apiVersion,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<String>(
                    (callback) => this.GetClusterConfigurationAsyncBeginWrapper(
                        apiVersion,
                        timeout,
                        callback),
                    this.GetClusterConfigurationAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.GetClusterConfigurationAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterConfigurationAsyncBeginWrapper(
                string apiVersion,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginGetClusterConfiguration2(
                        pin.AddBlittable(apiVersion),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private String GetClusterConfigurationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return StringResult.FromNative((NativeCommon.IFabricStringResult)this.nativeClusterClient.EndGetClusterConfiguration(context));
            }
            #endregion GetClusterConfiguration

            #region GetUpgradesPendingApprovalAsync
            private Task GetUpgradesPendingApprovalAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.GetUpgradesPendingApprovalAsyncBeginWrapper(
                        timeout,
                        callback),
                    this.GetUpgradesPendingApprovalAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.GetUpgradesPendingApprovalAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetUpgradesPendingApprovalAsyncBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginGetUpgradesPendingApproval(
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void GetUpgradesPendingApprovalAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndGetUpgradesPendingApproval(context);
            }
            #endregion GetUpgradesPendingApproval

            #region StartApprovedUpgrades
            private Task StartApprovedUpgradesAsyncHelper(
                        TimeSpan timeout,
                        CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.StartApprovedUpgradesAsyncBeginWrapper(
                        timeout,
                        callback),
                    this.StartApprovedUpgradesAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.StartApprovedUpgradesAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext StartApprovedUpgradesAsyncBeginWrapper(
                        TimeSpan timeout,
                        NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginStartApprovedUpgrades(
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void StartApprovedUpgradesAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeClusterClient.EndStartApprovedUpgrades(context);
            }
            #endregion StartUpgrade

            #region GetUpgradeOrchestrationServiceState
            private Task<String> GetUpgradeOrchestrationServiceStateAsyncHelper(
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<String>(
                    (callback) => this.GetUpgradeOrchestrationServiceStateBeginWrapper(
                        timeout,
                        callback),
                    this.GetUpgradeOrchestrationServiceStateAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.GetUpgradeOrchestrationServiceStateAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetUpgradeOrchestrationServiceStateBeginWrapper(
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginGetUpgradeOrchestrationServiceState(
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private String GetUpgradeOrchestrationServiceStateAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return StringResult.FromNative((NativeCommon.IFabricStringResult)this.nativeClusterClient.EndGetUpgradeOrchestrationServiceState(context));
            }
            #endregion GetUpgradeOrchestrationServiceState

            #region SetUpgradeOrchestrationServiceState

            private Task<FabricUpgradeOrchestrationServiceState> SetUpgradeOrchestrationServiceStateAsyncHelper(
                string state,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.SetUpgradeOrchestrationServiceStateAsyncBeginWrapper(
                    state,
                    timeout,
                    callback),
                    this.SetUpgradeOrchestrationServiceStateAsyncEndWrapper,
                    cancellationToken,
                    "UpgradeOrchestrationService.SetUpgradeOrchestrationServiceStateAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext SetUpgradeOrchestrationServiceStateAsyncBeginWrapper(
                string state,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeClusterClient.BeginSetUpgradeOrchestrationServiceState(
                        pin.AddBlittable(state),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private FabricUpgradeOrchestrationServiceState SetUpgradeOrchestrationServiceStateAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.FabricUpgradeOrchestrationServiceStateHelper.CreateFromNative(this.nativeClusterClient.EndSetUpgradeOrchestrationServiceState(context));
            }

            #endregion

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