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
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides the functionality to manage Service Fabric container networks.</para>
        /// </summary>
        public sealed class NetworkManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricNetworkManagementClient nativeNetworkManagementClient;

            #endregion

            #region Constructor

            internal NetworkManagementClient(FabricClient fabricClient, NativeClient.IFabricNetworkManagementClient nativeNetworkManagementClient)
            {
                this.fabricClient = fabricClient;
                this.nativeNetworkManagementClient = nativeNetworkManagementClient;
            }

            #endregion

            #region API

            #region Create Network Async

            /// <summary>
            ///   <para>Creates the specific Service Fabric container network.</para>
            /// </summary>
            /// <param name="networkName">
            ///   <para>The name of the container network.</para>
            /// </param>
            /// <param name="networkDescription">
            ///   <para>The description of the container network.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            public Task CreateNetworkAsync(string networkName, NetworkDescription networkDescription)
            {
                return this.CreateNetworkAsync(networkName, networkDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///   <para>Creates the specific Service Fabric container network.</para>
            /// </summary>
            /// <param name="networkName">
            ///   <para>The name of the container network.</para>
            /// </param>
            /// <param name="networkDescription">
            ///   <para>The description of the container network.</para>
            /// <param name="timeout">
            /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The CancellationToken that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>          
            public Task CreateNetworkAsync(string networkName, NetworkDescription networkDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("networkName", networkName).NotNullOrWhiteSpace();
                Requires.Argument<NetworkDescription>("networkDescription", networkDescription).NotNull();
                NetworkDescription.Validate(networkDescription);

                return this.CreateNetworkAsyncHelper(networkName, networkDescription, timeout, cancellationToken);
            }

            #endregion

            #region Delete Network Async

            /// <summary>
            ///   <para>Deletes the specific Service Fabric container network.</para>
            /// </summary>
            /// <param name="deleteNetworkDescription">
            ///   <para>The description of the container network to be deleted.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            public Task DeleteNetworkAsync(DeleteNetworkDescription deleteNetworkDescription)
            {
                return this.DeleteNetworkAsync(deleteNetworkDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///   <para>Deletes the specific Service Fabric container network.</para>
            /// </summary>
            /// <param name="deleteNetworkDescription">
            ///   <para>The description of the container network to be deleted.</para>
            /// <param name="timeout">
            /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The CancellationToken that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>          
            public Task DeleteNetworkAsync(DeleteNetworkDescription deleteNetworkDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<DeleteNetworkDescription>("deleteNetworkDescription", deleteNetworkDescription).NotNull();
                DeleteNetworkDescription.Validate(deleteNetworkDescription);

                return this.DeleteNetworkAsyncHelper(deleteNetworkDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Network List Async

            /// <summary>
            /// <para>
            /// Gets the details for all container networks in the cluster. If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkList> GetNetworkListAsync()
            {
                return this.GetNetworkListAsync(new NetworkQueryDescription());
            }

            /// <summary>
            /// <para>
            /// Gets the details for all container networks in the cluster. If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkList> GetNetworkListAsync(NetworkQueryDescription queryDescription)
            {
                return this.GetNetworkListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all container networks in the cluster. If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkList> GetNetworkListAsync(NetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    NetworkQueryDescription.Validate(queryDescription);
                }
                return this.GetNetworkListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Network Application List Async

            /// <summary>
            /// <para>
            /// Gets the details for all applications in the container network. If the applications do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkApplicationList> GetNetworkApplicationListAsync(NetworkApplicationQueryDescription queryDescription)
            {
                return this.GetNetworkApplicationListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications in the container network. If the applications do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkApplicationList> GetNetworkApplicationListAsync(NetworkApplicationQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    NetworkApplicationQueryDescription.Validate(queryDescription);
                }
                return this.GetNetworkApplicationListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Network Node List Async

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the container network. If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkNodeList> GetNetworkNodeListAsync(NetworkNodeQueryDescription queryDescription)
            {
                return this.GetNetworkNodeListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the container network. If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<NetworkNodeList> GetNetworkNodeListAsync(NetworkNodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    NetworkNodeQueryDescription.Validate(queryDescription);
                }
                return this.GetNetworkNodeListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Application Network List Async

            /// <summary>
            /// <para>
            /// Gets the details for all container networks an application is a member of. If the networks do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<ApplicationNetworkList> GetApplicationNetworkListAsync(ApplicationNetworkQueryDescription queryDescription)
            {
                return this.GetApplicationNetworkListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all container networks an application is a member of. If the networks do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<ApplicationNetworkList> GetApplicationNetworkListAsync(ApplicationNetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    ApplicationNetworkQueryDescription.Validate(queryDescription);
                }
                return this.GetApplicationNetworkListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Deployed Network List Async

            /// <summary>
            /// <para>
            /// Gets the details for all container networks deployed on a node. If the networks do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<DeployedNetworkList> GetDeployedNetworkListAsync(DeployedNetworkQueryDescription queryDescription)
            {
                return this.GetDeployedNetworkListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all container networks deployed on a node. If the networks do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<DeployedNetworkList> GetDeployedNetworkListAsync(DeployedNetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    DeployedNetworkQueryDescription.Validate(queryDescription);
                }
                return this.GetDeployedNetworkListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #region Get Deployed Network Code Package List Async

            /// <summary>
            /// <para>
            /// Gets the details for all code packages deployed in a container network on a node. If the code packages do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<DeployedNetworkCodePackageList> GetDeployedNetworkCodePackageListAsync(DeployedNetworkCodePackageQueryDescription queryDescription)
            {
                return this.GetDeployedNetworkCodePackageListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all code packages deployed in a container network on a node. If the code packages do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            public Task<DeployedNetworkCodePackageList> GetDeployedNetworkCodePackageListAsync(DeployedNetworkCodePackageQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    DeployedNetworkCodePackageQueryDescription.Validate(queryDescription);
                }
                return this.GetDeployedNetworkCodePackageListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            #endregion

            #endregion

            #region Helpers & Callbacks

            #region Create Network Async

            private Task CreateNetworkAsyncHelper(string networkName, NetworkDescription networkDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateNetworkBeginWrapper(networkName, networkDescription, timeout, callback),
                    this.CreateNetworkEndWrapper,
                    cancellationToken,
                    "NetworkManager.CreateNetworkAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateNetworkBeginWrapper(string networkName, NetworkDescription networkDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginCreateNetwork(
                        pin.AddObject(networkName),
                        networkDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateNetworkEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeNetworkManagementClient.EndCreateNetwork(context);
            }

            #endregion

            #region Delete Network Async

            private Task DeleteNetworkAsyncHelper(DeleteNetworkDescription deleteNetworkDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteNetworkBeginWrapper(deleteNetworkDescription, timeout, callback),
                    this.DeleteNetworkEndWrapper,
                    cancellationToken,
                    "NetworkManager.DeleteNetworkAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteNetworkBeginWrapper(DeleteNetworkDescription deleteNetworkDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginDeleteNetwork(
                        deleteNetworkDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteNetworkEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeNetworkManagementClient.EndDeleteNetwork(context);
            }

            #endregion

            #region Get Network List Async
            private Task<NetworkList> GetNetworkListAsyncHelper(NetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NetworkList>(
                    (callback) => this.GetNetworkListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetNetworkListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNetworkList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNetworkListAsyncBeginWrapper(NetworkQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetNetworkList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NetworkList GetNetworkListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NetworkList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetNetworkList(context));
            }

            #endregion

            #region Get Network Application List Async
            private Task<NetworkApplicationList> GetNetworkApplicationListAsyncHelper(NetworkApplicationQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NetworkApplicationList>(
                    (callback) => this.GetNetworkApplicationListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetNetworkApplicationListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNetworkApplicationList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNetworkApplicationListAsyncBeginWrapper(NetworkApplicationQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetNetworkApplicationList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NetworkApplicationList GetNetworkApplicationListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NetworkApplicationList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetNetworkApplicationList(context));
            }

            #endregion

            #region Get Network Node List Async
            private Task<NetworkNodeList> GetNetworkNodeListAsyncHelper(NetworkNodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NetworkNodeList>(
                    (callback) => this.GetNetworkNodeListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetNetworkNodeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNetworkNodeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNetworkNodeListAsyncBeginWrapper(NetworkNodeQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetNetworkNodeList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NetworkNodeList GetNetworkNodeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NetworkNodeList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetNetworkNodeList(context));
            }

            #endregion

            #region Get Application Network List Async
            private Task<ApplicationNetworkList> GetApplicationNetworkListAsyncHelper(ApplicationNetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationNetworkList>(
                    (callback) => this.GetApplicationNetworkListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetApplicationNetworkListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationNetworkList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationNetworkListAsyncBeginWrapper(ApplicationNetworkQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetApplicationNetworkList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationNetworkList GetApplicationNetworkListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationNetworkList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetApplicationNetworkList(context));
            }

            #endregion

            #region Get Deployed Network List Async
            private Task<DeployedNetworkList> GetDeployedNetworkListAsyncHelper(DeployedNetworkQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedNetworkList>(
                    (callback) => this.GetDeployedNetworkListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetDeployedNetworkListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedNetworkList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedNetworkListAsyncBeginWrapper(DeployedNetworkQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetDeployedNetworkList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedNetworkList GetDeployedNetworkListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedNetworkList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetDeployedNetworkList(context));
            }

            #endregion

            #region Get Deployed Network Code Package List Async
            private Task<DeployedNetworkCodePackageList> GetDeployedNetworkCodePackageListAsyncHelper(DeployedNetworkCodePackageQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedNetworkCodePackageList>(
                    (callback) => this.GetDeployedNetworkCodePackageListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetDeployedNetworkCodePackageListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedNetworkCodePackageList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedNetworkCodePackageListAsyncBeginWrapper(DeployedNetworkCodePackageQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeNetworkManagementClient.BeginGetDeployedNetworkCodePackageList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedNetworkCodePackageList GetDeployedNetworkCodePackageListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedNetworkCodePackageList.CreateFromNativeListResult(this.nativeNetworkManagementClient.EndGetDeployedNetworkCodePackageList(context));
            }

            #endregion

            #endregion
        }
    }
}
