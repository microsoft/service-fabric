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
        /// <para>Represents the fabric client that can be used to issue queries.</para>
        /// </summary>
        public sealed class QueryClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricQueryClient10 nativeQueryClient;
            private readonly NativeClient.IFabricApplicationManagementClient10 nativeManagementClient;
            private NativeClient.IInternalFabricApplicationManagementClient internalNativeApplicationClient;

            #endregion

            #region Constructor

            internal QueryClient(FabricClient fabricClient, NativeClient.IFabricQueryClient10 nativeQueryClient, NativeClient.IFabricApplicationManagementClient10 nativeManagementClient)
            {
                this.fabricClient = fabricClient;
                this.nativeQueryClient = nativeQueryClient;
                this.nativeManagementClient = nativeManagementClient;

                Utility.WrapNativeSyncInvokeInMTA(() =>
                    {
                        this.internalNativeApplicationClient = (NativeClient.IInternalFabricApplicationManagementClient)this.nativeManagementClient;
                    },
                    "QueryClient.ctor");
            }

            #endregion

            #region API

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the cluster.
            /// If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync()"/>.
            /// </para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync()
            {
                return this.GetNodeListAsync(null);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the cluster or for the specified node. If the nodes do not fit in a page, one page of results is returned as well as a
            /// continuation token which can be used to get the next page.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync(System.Fabric.Description.NodeQueryDescription)"/>.
            /// </para>
            /// </summary>
            /// <param name="nodeNameFilter">
            /// <para>The name of the node to get details for. The node name is a case-sensitive exact match. Gets all nodes in the cluster if the given node name is null.
            /// If the node name does not match any node on the cluster, an empty list is returned.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync(string nodeNameFilter)
            {
                return this.GetNodeListAsync(nodeNameFilter, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the cluster or for the specified node.
            /// If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync(System.Fabric.Description.NodeQueryDescription)"/>.
            /// </para>
            /// </summary>
            /// <param name="nodeNameFilter">
            /// <para>The name of the node to get details for. The node name is a case-sensitive exact match. Gets all nodes if the given node name is null.
            /// If the node name does not match any node on the cluster, an empty list is returned.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// </param>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync(string nodeNameFilter, string continuationToken)
            {
                return this.GetNodeListAsync(nodeNameFilter, continuationToken, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the cluster or for the specified node.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync(System.Fabric.Description.NodeQueryDescription, TimeSpan, System.Threading.CancellationToken)"/>.
            /// </para>
            /// </summary>
            /// <param name="nodeNameFilter">
            /// <para>The name of the node to get details for. The node name is a case-sensitive exact match. Gets all nodes if the given node name is null.
            /// If the node name does not match any node on the cluster, an empty list is returned.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync(string nodeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new NodeQueryDescription();
                queryDescription.NodeNameFilter = nodeNameFilter;

                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all nodes in the cluster or for the specified node.
            /// If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync(System.Fabric.Description.NodeQueryDescription, TimeSpan, System.Threading.CancellationToken)"/>.
            /// </para>
            /// </summary>
            /// <param name="nodeNameFilter">
            /// <para>The name of the node to get details for. The node name is a case-sensitive exact match. Gets all nodes if the given node name is null.
            /// If the node name does not match any node on the cluster, an empty list is returned.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync(string nodeNameFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new NodeQueryDescription();
                queryDescription.NodeNameFilter = nodeNameFilter;
                queryDescription.ContinuationToken = continuationToken;

                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// Gets the details for all nodes in the cluster or for the specified node.
            /// If the nodes do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// For the latest features, use <see cref="QueryClient.GetNodePagedListAsync(System.Fabric.Description.NodeQueryDescription, TimeSpan, System.Threading.CancellationToken)"/>.
            /// </summary>
            /// <param name="nodeNameFilter">
            /// <para>The name of the node to get details for. The node name is a case-sensitive exact match. Gets all nodes if the given node name is null.
            /// If the node name does not match any node on the cluster, an empty list is returned.</para>
            /// </param>
            /// <param name="nodeStatusFilter">
            /// <para>The node status(es) of the nodes to get details for.</para>
            /// </param>
            /// <param name="continuationToken">The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</param>
            /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
            /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodeListAsync(string nodeNameFilter, NodeStatusFilter nodeStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                var queryDescription = new NodeQueryDescription();
                queryDescription.NodeNameFilter = nodeNameFilter;
                queryDescription.NodeStatusFilter = nodeStatusFilter;
                queryDescription.ContinuationToken = continuationToken;

                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// Gets the details for all nodes in the cluster or for the specified node. If the nodes do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodePagedListAsync()
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the details for all nodes in the cluster or for the specified node. If the nodes do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.NodeQueryDescription" />
            ///         object describing which application nodes to return.
            ///     </para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodePagedListAsync(NodeQueryDescription queryDescription)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Gets the details for all nodes in the cluster or for the specified node. If the nodes do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.NodeQueryDescription" />
            ///         object describing which application nodes to return.
            ///     </para>
            /// </param>
            /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
            /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of nodes as <see cref="System.Fabric.Query.NodeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeList> GetNodePagedListAsync(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the details for all the application types provisioned or being provisioned in the system.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of application types as <see cref="System.Fabric.Query.ApplicationTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypeList> GetApplicationTypeListAsync()
            {
                return this.GetApplicationTypeListAsync(null);
            }

            /// <summary>
            /// <para>Gets the details for all the application types provisioned or being provisioned in the system or for the specified application type.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <param name="applicationTypeNameFilter">
            /// <para>The type name of the application to get service types for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of application types as <see cref="System.Fabric.Query.ApplicationTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypeList> GetApplicationTypeListAsync(string applicationTypeNameFilter)
            {
                return this.GetApplicationTypeListAsync(applicationTypeNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the details for all the application types provisioned or being provisioned in the system or for the specified application type.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetApplicationTypePagedListAsync()" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <param name="applicationTypeNameFilter">
            /// <para>The type name of the application to get service types for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of application types as <see cref="System.Fabric.Query.ApplicationTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypeList> GetApplicationTypeListAsync(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetApplicationTypeListAsyncHelper(applicationTypeNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            ///     <para>
            ///         Gets the details for all the application types provisioned or being provisioned in the system.
            ///     </para>
            /// </summary>
            /// <returns>
            ///     <para>
            ///        A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            ///        The value of TResult parameter is an <see cref="System.Fabric.Query.ApplicationTypePagedList" />.
            ///     </para>
            ///     <para>
            ///        If no application types are found matching the filters provided, this query returns no application types rather than an error.
            ///     </para>
            /// </returns>
            /// <remarks>
            ///     <para>
            ///         This is a paged query, meaning that if not all of the application types fit in a page, one page of results is returned as well as a continuation token which
            ///         can be used to get the next page. For example, if there are
            ///         10000 application types but a page only fits the first 3000 application types, 3000 is returned.
            ///         To access the rest of the results, retrieve subsequent pages by using the returned continuation token in the next query.
            ///         A null continuation token is returned if there are no subsequent pages.
            ///     </para>
            ///     <para>
            ///         Each version of a particular application type is one entry in the result.
            ///     </para>
            /// </remarks>
            /// <remarks>
            ///     <para>
            ///         See more details about application types here: <see cref="System.Fabric.Query.ApplicationType" />.
            ///     </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync()
            {
                return this.GetApplicationTypePagedListAsync(new PagedApplicationTypeQueryDescription(), FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///     <para>
            ///         Gets the details for application types provisioned or being provisioned in the system which match filters
            ///         provided by the queryDescription argument.
            ///     </para>
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.PagedApplicationTypeQueryDescription" />
            ///         object describing which application types to return.
            ///     </para>
            /// </param>
            /// <returns>
            ///     <para>
            ///        A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            ///        The value of TResult parameter is an <see cref="System.Fabric.Query.ApplicationTypePagedList" />.
            ///     </para>
            ///     <para>
            ///        If no application types are found matching the filters provided, this query returns no application types rather than an error.
            ///     </para>
            /// </returns>
            /// <remarks>
            ///     <para>
            ///         This is a paged query, meaning that if not all of the application types fit in a page, one page of results is returned as well as a continuation token which
            ///         can be used to get the next page. For example, if there are
            ///         10000 application types but a page only fits the first 3000 application types, 3000 is returned.
            ///         To access the rest of the results, retrieve subsequent pages by using the returned continuation token in the next query.
            ///         A null continuation token is returned if there are no subsequent pages.
            ///     </para>
            ///     <para>
            ///         Each version of a particular application type is one entry in the result.
            ///     </para>
            /// </remarks>
            /// <remarks>
            ///     <para>
            ///         See more details about application types here: <see cref="System.Fabric.Query.ApplicationType" />.
            ///     </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription)
            {
                return this.GetApplicationTypePagedListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///     <para>
            ///         Gets the details for application types provisioned or being provisioned in the system which match filters
            ///         provided by the queryDescription argument.
            ///     </para>
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.PagedApplicationTypeQueryDescription" />
            ///         object describing which application types to return.
            ///     </para>
            ///
            /// </param>
            /// <param name="timeout">
            ///     <para>
            ///         Specifies the duration this operation has to complete before timing out.
            ///     </para>
            /// </param>
            /// <param name="cancellationToken">
            ///     <para>
            ///         Propagates notification that operations should be canceled.
            ///     </para>
            /// </param>
            /// <returns>
            ///     <para>
            ///        A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            ///        The value of TResult parameter is an <see cref="System.Fabric.Query.ApplicationTypePagedList" />.
            ///     </para>
            ///     <para>
            ///        If no application types are found matching the filters provided, this query returns no application types rather than an error.
            ///     </para>
            /// </returns>
            /// <remarks>
            ///     <para>
            ///         This is a paged query, meaning that if not all of the application types fit in a page, one page of results is returned as well as a continuation token which
            ///         can be used to get the next page. For example, if there are
            ///         10000 application types but a page only fits the first 3000 application types, 3000 is returned.
            ///         To access the rest of the results, retrieve subsequent pages by using the returned continuation token in the next query.
            ///         A null continuation token is returned if there are no subsequent pages.
            ///     </para>
            ///     <para>
            ///         Each version of a particular application type is one entry in the result.
            ///     </para>
            /// </remarks>
            /// <remarks>
            ///     <para>
            ///         See more details about application types here: <see cref="System.Fabric.Query.ApplicationType" />.
            ///     </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationTypePagedList> GetApplicationTypePagedListAsync(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                if (queryDescription != null)
                {
                    PagedApplicationTypeQueryDescription.Validate(queryDescription);
                }
                return this.GetApplicationTypePagedListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the list of service types.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The type name of the application to get service types for.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version to get service types for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service types as <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceTypeList> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion)
            {
                return this.GetServiceTypeListAsync(applicationTypeName, applicationTypeVersion, null);
            }

            /// <summary>
            /// <para>Gets the list of service types.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The type name of the application to get service types for.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version to get service types for.</para>
            /// </param>
            /// <param name="serviceTypeNameFilter">
            /// <para>The name of the service type to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service types as <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceTypeList> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter)
            {
                return this.GetServiceTypeListAsync(applicationTypeName, applicationTypeVersion, serviceTypeNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the list of service types.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The type name of the application to get service types for.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version to get service types for.</para>
            /// </param>
            /// <param name="serviceTypeNameFilter">
            /// <para>The name of the service type to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service types as <see cref="System.Fabric.Query.ServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceTypeList> GetServiceTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
                Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();

                return this.GetServiceTypeListAsyncHelper(applicationTypeName, applicationTypeVersion, serviceTypeNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Get service group members types of service group(s).</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The application type name of the service group.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version of the service group.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group member types as <see cref="System.Fabric.Query.ServiceGroupMemberTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(string applicationTypeName, string applicationTypeVersion)
            {
                return this.GetServiceGroupMemberTypeListAsync(applicationTypeName, applicationTypeVersion, null);
            }

            /// <summary>
            /// <para>Get service group members types of service group(s).</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The application type name of the service group.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version of the service group.</para>
            /// </param>
            /// <param name="serviceGroupTypeNameFilter">
            /// <para>The name of the service group type to get.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group member types as <see cref="System.Fabric.Query.ServiceGroupMemberTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter)
            {
                return this.GetServiceGroupMemberTypeListAsync(applicationTypeName, applicationTypeVersion, serviceGroupTypeNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Get service group members types of service group(s).</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The application type name of the service group.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The application type version of the service group.</para>
            /// </param>
            /// <param name="serviceGroupTypeNameFilter">
            /// <para>The name of the service group type to get.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timeout to the operation.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Notifies the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group member types as <see cref="System.Fabric.Query.ServiceGroupMemberTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsync(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
                Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();

                return this.GetServiceGroupMemberTypeListAsyncHelper(applicationTypeName, applicationTypeVersion, serviceGroupTypeNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications created in the system. If the applications do not fit in a page, one page of results is returned as well as a
            /// continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>If the provided application name has no matching applications, it returns a list of 0 entries.</para>
            /// <para>The returned task contains the list of applications as <see cref="System.Fabric.Query.ApplicationList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationListAsync()
            {
                return this.GetApplicationListAsync(null, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications or for a specific application created in the system. If the applications do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationNameFilter">
            /// <para>The name of the application to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of applications as <see cref="System.Fabric.Query.ApplicationList" />.</para>
            /// <para>If the provided application name has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationListAsync(Uri applicationNameFilter)
            {
                return this.GetApplicationListAsync(applicationNameFilter, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications or for a specific application created in the system. If the applications do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationNameFilter">
            /// <para>The name of the application to get details for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of applications as <see cref="System.Fabric.Query.ApplicationList" />.</para>
            /// <para>If the provided application name has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationListAsync(Uri applicationNameFilter, string continuationToken)
            {
                return this.GetApplicationListAsync(applicationNameFilter, continuationToken, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications or for a specific application created in the system. If the applications do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationNameFilter">
            /// <para>The name of the application to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of applications as <see cref="System.Fabric.Query.ApplicationList" />.</para>
            /// <para>If the provided application name has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationListAsync(Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetApplicationListAsync(applicationNameFilter, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all applications or for a specific application created in the system. If the applications do not fit in a page,
            /// one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationNameFilter">
            /// <para>The name of the application to get details for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of applications as <see cref="System.Fabric.Query.ApplicationList" />.</para>
            /// <para>If the provided application name has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationListAsync(Uri applicationNameFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetApplicationListAsyncHelper(
                    new ApplicationQueryDescription()
                    {
                        ApplicationNameFilter = applicationNameFilter,
                        ContinuationToken = continuationToken
                    },
                    timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the details of applications created that match filters specified in query description (if any).
            /// If the applications do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
            /// </summary>
            /// <param name="applicationQueryDescription">
            /// <para>The <see cref="System.Fabric.Description.ApplicationQueryDescription" /> that determines which applications should be queried.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            /// The value of TResult parameter is an <see cref="System.Fabric.Query.ApplicationList" /> that represents the list of applications
            /// that respect the filters in the <see cref="System.Fabric.Description.ApplicationQueryDescription" /> and fit the page.
            /// If the provided query description has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription)
            {
                return this.GetApplicationPagedListAsync(applicationQueryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the details of applications created that match filters specified in query description (if any).
            /// If the applications do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
            /// </summary>
            /// <param name="applicationQueryDescription">
            /// <para>The <see cref="System.Fabric.Description.ApplicationQueryDescription" /> that determines which applications should be queried.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            /// The value of TResult parameter is an <see cref="System.Fabric.Query.ApplicationList" /> that represents the list of applications
            /// that respect the filters in the <see cref="System.Fabric.Description.ApplicationQueryDescription" /> and fit the page.
            /// If the provided query description has no matching applications, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationList> GetApplicationPagedListAsync(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ApplicationQueryDescription>("applicationQueryDescription", applicationQueryDescription).NotNull();
                ApplicationQueryDescription.Validate(applicationQueryDescription);

                return this.GetApplicationListAsyncHelper(applicationQueryDescription, timeout, cancellationToken);
            }

            internal Task<ComposeDeploymentStatusListWrapper> GetComposeDeploymentStatusPagedListAsync(ComposeDeploymentStatusQueryDescriptionWrapper composeDeploymentQueryDescription)
            {
                return this.GetComposeDeploymentStatusPagedListAsync(composeDeploymentQueryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task<ComposeDeploymentStatusListWrapper> GetComposeDeploymentStatusPagedListAsync(ComposeDeploymentStatusQueryDescriptionWrapper composeDeploymentQueryDescription, TimeSpan timeout)
            {
                return this.GetComposeDeploymentStatusPagedListAsync(composeDeploymentQueryDescription, timeout, CancellationToken.None);
            }

            internal Task<ComposeDeploymentStatusListWrapper> GetComposeDeploymentStatusPagedListAsync(ComposeDeploymentStatusQueryDescriptionWrapper composeApplicationStatusQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ComposeDeploymentStatusQueryDescriptionWrapper>("composeApplicationStatusQueryDescription", composeApplicationStatusQueryDescription).NotNull();

                return this.GetComposeDeploymentStatusListAsyncHelper(composeApplicationStatusQueryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the information about all services belonging to the application specified by the application name URI. If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The name of the application to get services for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of services as <see cref="System.Fabric.Query.ServiceList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServiceListAsync(Uri applicationName)
            {
                return this.GetServiceListAsync(applicationName, null, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all services of an application or just the specified service. If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The name of the application to get services for.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The name of the services to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of services as <see cref="System.Fabric.Query.ServiceList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServiceListAsync(Uri applicationName, Uri serviceNameFilter)
            {
                return this.GetServiceListAsync(applicationName, serviceNameFilter, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all services of an application or just the specified service. If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The name of the application to get services for.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The name of the services to get details for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of services as <see cref="System.Fabric.Query.ServiceList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServiceListAsync(Uri applicationName, Uri serviceNameFilter, string continuationToken)
            {
                return this.GetServiceListAsync(applicationName, serviceNameFilter, continuationToken, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all services of an application or just the specified service. If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The name of the application to get services for.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The name of the services to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of services as <see cref="System.Fabric.Query.ServiceList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServiceListAsync(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetServiceListAsync(applicationName, serviceNameFilter, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all services of an application or just the specified service. If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The name of the application to get services for.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The name of the services to get details for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of services as <see cref="System.Fabric.Query.ServiceList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServiceListAsync(Uri applicationName, Uri serviceNameFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetServiceListAsyncHelper(
                    new ServiceQueryDescription(applicationName)
                    {
                        ServiceNameFilter = serviceNameFilter,
                        ContinuationToken = continuationToken
                    },
                    timeout,
                    cancellationToken);
            }

            /// <summary>
            /// <para>Gets the details for all services of an application or just the specified services that match filters specified in query description (if any).
            /// If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
            /// </summary>
            /// <param name="serviceQueryDescription">
            /// <para>The <see cref="System.Fabric.Description.ServiceQueryDescription" /> that determines which services should be queried.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            /// The value of TResult parameter is a <see cref="System.Fabric.Query.ServiceList" /> that represents the list of services that respect the filters in the <see cref="System.Fabric.Query.ServiceList" /> and fit the page.
            /// If the provided query description has no matching services, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription)
            {
                return this.GetServicePagedListAsync(serviceQueryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the details for all services of an application or just the specified services that match filters specified in query description (if any).
            /// If the services do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
            /// </summary>
            /// <param name="serviceQueryDescription">
            /// <para>The <see cref="System.Fabric.Description.ServiceQueryDescription" /> that determines which services should be queried.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
            /// The value of TResult parameter is a <see cref="System.Fabric.Query.ServiceList" /> that represents the list of services that respect the filters in the <see cref="System.Fabric.Query.ServiceList" /> and fit the page.
            /// If the provided query description has no matching services, it returns a list of 0 entries.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://azure.microsoft.com/documentation/articles/service-fabric-errors-and-exceptions/"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceList> GetServicePagedListAsync(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ServiceQueryDescription>("serviceQueryDescription", serviceQueryDescription).NotNull();
                ServiceQueryDescription.Validate(serviceQueryDescription);

                return this.GetServiceListAsyncHelper(serviceQueryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Get service group members of an application.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The application name of the service group.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group members as <see cref="System.Fabric.Query.ServiceGroupMemberList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(Uri applicationName)
            {
                return this.GetServiceGroupMemberListAsync(applicationName, null);
            }

            /// <summary>
            /// <para>Get service group members of an application.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The application name of the service group.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The service name of the service group.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group members as <see cref="System.Fabric.Query.ServiceGroupMemberList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(Uri applicationName, Uri serviceNameFilter)
            {
                return this.GetServiceGroupMemberListAsync(applicationName, serviceNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The application name of the service group.</para>
            /// </param>
            /// <param name="serviceNameFilter">
            /// <para>The service name of the service group.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timeout to the operation.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Notifies the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of service group members as <see cref="System.Fabric.Query.ServiceGroupMemberList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceGroupMemberList> GetServiceGroupMemberListAsync(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetServiceGroupMemberListAsyncHelper(applicationName, serviceNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service to get partitions for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName)
            {
                return this.GetPartitionListAsync(serviceName, null, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName, string continuationToken)
            {
                return this.GetPartitionListAsync(serviceName, null, continuationToken, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service or just the specified partition. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>The partition ID of the partition to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName, Guid? partitionIdFilter)
            {
                return this.GetPartitionListAsync(serviceName, partitionIdFilter, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service or just the specified partition. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>The partition ID of the partition to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetPartitionListAsync(serviceName, partitionIdFilter, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all partitions of a service or just the specified partition. If the partitions do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>The partition ID of the partition to get details for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionListAsync(Uri serviceName, Guid? partitionIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceName", serviceName).NotNull();

                if (!partitionIdFilter.HasValue)
                {
                    partitionIdFilter = Guid.Empty;
                }

                return this.GetPartitionListAsyncHelper(serviceName, partitionIdFilter.Value, continuationToken, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for the specified partition.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID of the partition to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetPartitionAsyncHelper(partitionId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for the specified partition.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID of the partition to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the list of partitions as <see cref="System.Fabric.Query.ServicePartitionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServicePartitionList> GetPartitionAsync(Guid partitionId)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument("partitionId", partitionId).NotNullOrEmpty();
                return this.GetPartitionAsyncHelper(partitionId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the information about the partition load.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID of the partition to get load information for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of a partition as <see cref="System.Fabric.Query.PartitionLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<PartitionLoadInformation> GetPartitionLoadInformationAsync(Guid partitionId)
            {
                return this.GetPartitionLoadInformationAsync(partitionId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the information about the partition load.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID of the partition to get load information for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of a partition as <see cref="System.Fabric.Query.PartitionLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<PartitionLoadInformation> GetPartitionLoadInformationAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetPartitionLoadInformationAsyncHelper(partitionId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match
            ///     <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and
            ///     <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId)
            {
                return this.GetReplicaListAsync(partitionId, 0);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match
            ///     <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, string continuationToken)
            {
                return this.GetReplicaListAsync(partitionId, 0, ServiceReplicaStatusFilter.Default, continuationToken, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition that match the replica or instance filter and the status filter. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceIdFilter">
            /// <para>The replica identifier or instance identifier to get replicas for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter)
            {
                return this.GetReplicaListAsync(partitionId, replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter.Default, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition that match the replica or instance filter. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceIdFilter">
            /// <para>The replica identifier or instance identifier to get replicas for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetReplicaListAsync(partitionId, replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter.Default, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier of the partition to get replicas for.</para>
            /// </param>
            /// <param name="continuationToken">
            /// <para>The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetReplicaListAsync(partitionId, 0, ServiceReplicaStatusFilter.Default, continuationToken, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition that match the replica or instance filter and the status filter. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceIdFilter">
            /// <para>The replica identifier or instance identifier to get replicas for.</para>
            /// </param>
            /// <param name="replicaStatusFilter">
            /// <para>The replica status(es) to get replicas for.</para>
            /// </param>
            /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
            /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter replicaStatusFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetReplicaListAsync(partitionId, replicaIdOrInstanceIdFilter, replicaStatusFilter, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>
            /// Gets the details for all replicas of a partition that match the replica or instance filter and the status filter. If the replicas do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.
            /// </para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition identifier for the partition to get replicas for.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceIdFilter">
            /// <para>The replica identifier or instance identifier to get replicas for.</para>
            /// </param>
            /// <param name="replicaStatusFilter">
            /// <para>Filter results to include only those matching this replica status.</para>
            /// </param>
            /// <param name="continuationToken">The continuation token obtained from a previous query. This value can be passed along to this query to start where the last query left off.
            /// Not passing a continuation token means returned results start from the first page.</param>
            /// <param name="timeout">Specifies the duration this operation has to complete before timing out.</param>
            /// <param name="cancellationToken">Propagates notification that operation should be canceled.</param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information of the partition as <see cref="System.Fabric.Query.ServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,System.Uri)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceReplicaList> GetReplicaListAsync(Guid partitionId, long replicaIdOrInstanceIdFilter, ServiceReplicaStatusFilter replicaStatusFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetReplicaListAsyncHelper(partitionId, replicaIdOrInstanceIdFilter, replicaStatusFilter, continuationToken, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the list of applications deployed on a node.</para>
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.PagedDeployedApplicationQueryDescription" />
            ///         object describing which deployed applications to return.
            ///     </para>
            /// </param>
            /// <returns>
            ///     <para>A task that represents the asynchronous query operation.</para>
            ///     <para>The returned task contains the list of deployed applications as <see cref="System.Fabric.Query.DeployedApplicationPagedList" />.</para>
            ///     <para>
            ///        This query returns empty rather than an error if no results are found matching the filters provided.
            ///     </para>
            /// </returns>
            /// <remarks>
            ///     <para>
            ///         This is a paged query, meaning that if not all of the applications fit in a page, one page of results is returned as well as a continuation token which
            ///         can be used to get the next page. For example, if there are
            ///         10000 applications but a page only fits the first 3000 applications, 3000 is returned.
            ///         To access the rest of the results, retrieve subsequent pages by using the returned continuation token in the next query.
            ///         A null continuation token is returned if there are no subsequent pages.
            ///     </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.
            /// </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsync(PagedDeployedApplicationQueryDescription queryDescription)
            {
                return this.GetDeployedApplicationPagedListAsync(queryDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the list of applications deployed on a node.</para>
            /// </summary>
            /// <param name="queryDescription">
            ///     <para>
            ///         A <see cref="System.Fabric.Description.PagedDeployedApplicationQueryDescription" />
            ///         object describing which deployed applications to return.
            ///     </para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            ///     <para>A task that represents the asynchronous query operation.</para>
            ///     <para>The returned task contains the list of deployed applications as <see cref="System.Fabric.Query.DeployedApplicationPagedList" />.</para>
            ///     <para>
            ///        This query returns empty rather than an error if no application types are found matching the filters provided.
            ///     </para>
            /// </returns>
            /// <remarks>
            ///     <para>
            ///         This is a paged query, meaning that if not all of the applications fit in a page, one page of results is returned as well as a continuation token which
            ///         can be used to get the next page. For example, if there are
            ///         10000 applications but a page only fits the first 3000 applications, 3000 is returned.
            ///         To access the rest of the results, retrieve subsequent pages by using the returned continuation token in the next query.
            ///         A null continuation token is returned if there are no subsequent pages.
            ///     </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsync(
                PagedDeployedApplicationQueryDescription queryDescription,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                Requires.Argument<string>("nodeName", queryDescription.NodeName).NotNullOrEmpty();

                return this.GetDeployedApplicationPagedListAsyncHelper(queryDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the deployed application list.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedApplicationPagedListAsync(PagedDeployedApplicationQueryDescription)" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node to get applications for. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed applications as <see cref="System.Fabric.Query.DeployedApplicationList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedApplicationList> GetDeployedApplicationListAsync(string nodeName)
            {
                return this.GetDeployedApplicationListAsync(nodeName, null);
            }

            /// <summary>
            /// <para>Gets the deployed applications on a node with the specified application name.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedApplicationPagedListAsync(PagedDeployedApplicationQueryDescription)" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node to get applications for. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationNameFilter">
            /// <para>Filter results to include only applications matching this application name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed applications as <see cref="System.Fabric.Query.DeployedApplicationList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedApplicationList> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter)
            {
                return this.GetDeployedApplicationListAsync(nodeName, applicationNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the deployed applications on a node with the specified application name.
            /// For more functionality, please use <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedApplicationPagedListAsync(PagedDeployedApplicationQueryDescription, TimeSpan, CancellationToken)" />.
            /// This method will be deprecated in the future.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node to get applications for. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationNameFilter">
            /// <para>Filter results to include only applications matching this application name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed applications as <see cref="System.Fabric.Query.DeployedApplicationList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedApplicationList> GetDeployedApplicationListAsync(string nodeName, Uri applicationNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();

                return this.GetDeployedApplicationListAsyncHelper(nodeName, applicationNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the deployed service packages for the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service packages as <see cref="System.Fabric.Query.DeployedServicePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName)
            {
                return this.GetDeployedServicePackageListAsync(nodeName, applicationName, null);
            }

            /// <summary>
            /// <para>Gets the deployed service packages for the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service packages as <see cref="System.Fabric.Query.DeployedServicePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter)
            {
                return this.GetDeployedServicePackageListAsync(nodeName, applicationName, serviceManifestNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the deployed service packages for the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service packages as <see cref="System.Fabric.Query.DeployedServicePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServicePackageList> GetDeployedServicePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();
                Requires.Argument<Uri>("applicationName", applicationName).NotNull();

                return this.GetDeployedServicePackageListAsyncHelper(nodeName, applicationName, serviceManifestNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the deployed service types on the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service types as <see cref="System.Fabric.Query.DeployedServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName)
            {
                return this.GetDeployedServiceTypeListAsync(nodeName, applicationName, null, null);
            }

            /// <summary>
            /// <para>Gets the deployed service types on the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only service types matching this service manifest name.</para>
            /// </param>
            /// <param name="serviceTypeNameFilter">
            /// <para>Filter results to include only service types matching this name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service types as <see cref="System.Fabric.Query.DeployedServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter)
            {
                return this.GetDeployedServiceTypeListAsync(nodeName, applicationName, serviceManifestNameFilter, serviceTypeNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the deployed service types on the given node and application.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only service types matching this service manifest name.</para>
            /// </param>
            /// <param name="serviceTypeNameFilter">
            /// <para>Filter results to include only service types matching this name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service types as <see cref="System.Fabric.Query.DeployedServiceTypeList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();

                return this.GetDeployedServiceTypeListAsyncHelper(nodeName, applicationName, serviceManifestNameFilter, serviceTypeNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the list of the deployed code packages.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed code packages as <see cref="System.Fabric.Query.DeployedCodePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName)
            {
                return this.GetDeployedCodePackageListAsync(nodeName, applicationName, null, null);
            }

            /// <summary>
            /// <para>Gets the list of the deployed code packages.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <param name="codePackageNameFilter">
            /// <para>Filter results to include only those matching this code package name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed code packages as <see cref="System.Fabric.Query.DeployedCodePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter)
            {
                return this.GetDeployedCodePackageListAsync(nodeName, applicationName, serviceManifestNameFilter, codePackageNameFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the list of the deployed code packages.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <param name="codePackageNameFilter">
            /// <para>Filter results to include only those matching this code package name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed code packages as <see cref="System.Fabric.Query.DeployedCodePackageList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedCodePackageList> GetDeployedCodePackageListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, string codePackageNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();
                Requires.Argument<Uri>("applicationName", applicationName).NotNull();

                return this.GetDeployedCodePackageListAsyncHelper(nodeName, applicationName, serviceManifestNameFilter, codePackageNameFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the view of replicas from a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service replicas as <see cref="System.Fabric.Query.DeployedServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(string nodeName, Uri applicationName)
            {
                return this.GetDeployedReplicaListAsync(nodeName, applicationName, null, null);
            }


            /// <summary>
            /// <para>Gets the view of replicas from a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>Filter results to only include replicas matching this partition ID.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service replicas as <see cref="System.Fabric.Query.DeployedServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, Guid? partitionIdFilter)
            {
                return this.GetDeployedReplicaListAsync(nodeName, applicationName, null, partitionIdFilter);
            }

            /// <summary>
            /// <para>Gets the view of replicas from a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>Filter results to only include replicas matching this partition ID.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service replicas as <see cref="System.Fabric.Query.DeployedServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and
            ///     <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter)
            {
                return this.GetDeployedReplicaListAsync(nodeName, applicationName, serviceManifestNameFilter, partitionIdFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }



            /// <summary>
            /// <para>Gets the view of replicas from a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="applicationName">
            /// <para>The name of the application.</para>
            /// </param>
            /// <param name="partitionIdFilter">
            /// <para>Filter results to only include replicas matching this partition ID.</para>
            /// </param>
            /// <param name="serviceManifestNameFilter">
            /// <para>Filter results to include only those matching this service manifest name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of deployed service replicas as <see cref="System.Fabric.Query.DeployedServiceReplicaList" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and
            ///     <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaList> GetDeployedReplicaListAsync(string nodeName, Uri applicationName, string serviceManifestNameFilter, Guid? partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();

                if (!partitionIdFilter.HasValue)
                {
                    partitionIdFilter = Guid.Empty;
                }

                return this.GetDeployedReplicaListAsyncHelper(nodeName, applicationName, serviceManifestNameFilter, partitionIdFilter.Value, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Returns details of a replica running on the specified node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The node name from which the results are desired. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition id for which the results are desired.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The identifier for the replica or the instance for which the results are desired.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information as <see cref="System.Fabric.Query.DeployedServiceReplicaDetail" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaListAsync(string,Uri)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaDetail> GetDeployedReplicaDetailAsync(string nodeName, Guid partitionId, long replicaId)
            {
                return this.GetDeployedReplicaDetailAsync(nodeName, partitionId, replicaId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Returns details of a replica running on the specified node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The node name from which the results are desired. This is a case-sensitive exact match. This value should not be null or empty,
            /// and if the node name does not match any node on the cluster, an exception is thrown.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition id for which the results are desired.</para>
            /// </param>
            /// <param name="replicaId">
            /// <para>The identifier for the replica or the instance for which the results are desired.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the replica information as <see cref="System.Fabric.Query.DeployedServiceReplicaDetail" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            ///     Service Fabric is a distributed system where many components have a view of the same entity. </para>
            /// <para>
            ///     In an unstable or transient state, this view may not match <see cref="System.Fabric.FabricClient.QueryClient.GetReplicaListAsync(System.Guid)"/> and <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedReplicaDetailAsync(string,System.Guid,long)"/> </para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricInvalidAddressException">
            ///     <para> For this query, this exception usually means that the given node name does not match any node in the cluster.</para>
            /// </exception>
            public Task<DeployedServiceReplicaDetail> GetDeployedReplicaDetailAsync(string nodeName, Guid partitionId, long replicaId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrEmpty();

                return this.GetDeployedServiceReplicaDetailAsyncHelper(nodeName, partitionId, replicaId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the cluster load information.</para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>See <see cref="System.Fabric.Query.ClusterLoadInformation" />.</para>
            /// <para>The returned task contains the load information of the cluster as <see cref="System.Fabric.Query.ClusterLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ClusterLoadInformation> GetClusterLoadInformationAsync()
            {
                return this.GetClusterLoadInformationAsync(FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the cluster load information.</para>
            /// </summary>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of the cluster as <see cref="System.Fabric.Query.ClusterLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ClusterLoadInformation> GetClusterLoadInformationAsync(TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetClusterLoadInformationAsyncHelper(timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets details for all cluster code versions provisioned in the system.</para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric code versions as <see cref="System.Fabric.Query.ProvisionedFabricCodeVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync()
            {
                return this.GetProvisionedFabricCodeVersionListAsync(null);
            }

            /// <summary>
            /// <para>Gets details for the specific cluster code version provisioned in the system.</para>
            /// </summary>
            /// <param name="codeVersionFilter">
            /// <para>The code version to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric code versions as <see cref="System.Fabric.Query.ProvisionedFabricCodeVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter)
            {
                return this.GetProvisionedFabricCodeVersionListAsync(codeVersionFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets details for the specific cluster code version provisioned in the system.</para>
            /// </summary>
            /// <param name="codeVersionFilter">
            /// <para>Code version to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum time allowed for the operation to complete before TimeoutException is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Reserved for future use.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric code versions as <see cref="System.Fabric.Query.ProvisionedFabricCodeVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsync(string codeVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetProvisionedFabricCodeVersionListAsyncHelper(codeVersionFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets details for all cluster config versions provisioned in the system.</para>
            /// </summary>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric config versions as <see cref="System.Fabric.Query.ProvisionedFabricConfigVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync()
            {
                return this.GetProvisionedFabricConfigVersionListAsync(null);
            }

            /// <summary>
            /// <para>Gets details for a specific cluster config version provisioned in the system.</para>
            /// </summary>
            /// <param name="configVersionFilter">
            /// <para>The config version to get details for.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric config versions as <see cref="System.Fabric.Query.ProvisionedFabricConfigVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter)
            {
                return this.GetProvisionedFabricConfigVersionListAsync(configVersionFilter, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets details for a specific cluster config version provisioned in the system.</para>
            /// </summary>
            /// <param name="configVersionFilter">
            /// <para>The config version to get details for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum time allowed for the operation to complete before TimeoutException is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Reserved for future use.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the list of provisioned Service Fabric config versions as <see cref="System.Fabric.Query.ProvisionedFabricConfigVersionList" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsync(string configVersionFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetProvisionedFabricConfigVersionListAsyncHelper(configVersionFilter, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Get metrics and load information on the node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match.
            /// If the node name does not match any node on the cluster, an empty <see cref="System.Fabric.Query.NodeLoadInformation" /> is returned.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of the node as <see cref="System.Fabric.Query.NodeLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeLoadInformation> GetNodeLoadInformationAsync(string nodeName)
            {
                return this.GetNodeLoadInformationAsync(nodeName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Get metrics and load information on the node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node. This is a case-sensitive exact match.
            /// If the node name does not match any node on the cluster, an empty <see cref="System.Fabric.Query.NodeLoadInformation" /> is returned.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of the node as <see cref="System.Fabric.Query.NodeLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<NodeLoadInformation> GetNodeLoadInformationAsync(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetNodeLoadInformationAsyncHelper(nodeName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Get a list of metric and their load on a replica.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceId">
            /// <para>The replica ID (stateful service) or instance ID (stateless service).</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of the replica as <see cref="System.Fabric.Query.ReplicaLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ReplicaLoadInformation> GetReplicaLoadInformationAsync(Guid partitionId, long replicaIdOrInstanceId)
            {
                return this.GetReplicaLoadInformationAsync(partitionId, replicaIdOrInstanceId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Get a list of metric and their load on a replica.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The partition ID.</para>
            /// </param>
            /// <param name="replicaIdOrInstanceId">
            /// <para>The replica ID (stateful service) or instance ID (stateless service).</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the load information of the replica as <see cref="System.Fabric.Query.ReplicaLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ReplicaLoadInformation> GetReplicaLoadInformationAsync(Guid partitionId, long replicaIdOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.GetReplicaLoadInformationAsyncHelper(partitionId, replicaIdOrInstanceId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Get diagnostics information about services with unplaced replicas.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition ID.</para>
            /// </param>
            /// <param name="onlyQueryPrimaries">
            /// <para>Return only the unplaced replica diagnostics for only the attempted primary replica placements in order to limit output.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the information of an unplaced replica as <see cref="System.Fabric.Query.UnplacedReplicaInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsync(string serviceName, Guid partitionId, bool onlyQueryPrimaries)
            {
                return this.GetUnplacedReplicaInformationAsync(serviceName, partitionId, onlyQueryPrimaries, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Get diagnostics information about services with unplaced replicas.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition ID.</para>
            /// </param>
            /// <param name="onlyQueryPrimaries">
            /// <para>Return only the unplaced replica diagnostics for only the attempted primary replica placements in order to limit output.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the information of an unplaced replica as <see cref="System.Fabric.Query.UnplacedReplicaInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsync(string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetUnplacedReplicaInformationAsyncHelper(serviceName, partitionId, onlyQueryPrimaries, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Retrieves the load information of the specified application instance.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the information of an application as <see cref="System.Fabric.Query.ApplicationLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>This operation has a timeout of 60 seconds.</para>
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationLoadInformation> GetApplicationLoadInformationAsync(string applicationName)
            {
                return this.GetApplicationLoadInformationAsync(applicationName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Retrieves the load information of the specified application instance.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the information of an application as <see cref="System.Fabric.Query.ApplicationLoadInformation" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationLoadInformation> GetApplicationLoadInformationAsync(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetApplicationLoadInformationAsyncHelper(applicationName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the service name for the specified partition.</para>
            /// </summary>
            /// <param name="partitionId">
            /// <para>The id of the partition to get the service name for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation.</para>
            /// <para>The returned task contains the service name as <see cref="System.Fabric.Query.ServiceNameResult" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ServiceNameResult> GetServiceNameAsync(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetServiceNameAsyncHelper(partitionId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the application name for the specified service.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The name of the service to get the application name for.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Specifies the duration this operation has to complete before timing out.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Propagates notification that operations should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A task that represents the asynchronous query operation. </para>
            /// <para>The returned task contains the application name as <see cref="System.Fabric.Query.ApplicationNameResult" />.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            public Task<ApplicationNameResult> GetApplicationNameAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.GetApplicationNameAsyncHelper(serviceName, timeout, cancellationToken);
            }

            #endregion

            #region Helpers

            #region Get Node List Async

            private Task<NodeList> GetNodeListAsyncHelper(NodeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NodeList>(
                    (callback) => this.GetNodeListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetNodeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNodeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNodeListAsyncBeginWrapper(
                NodeQueryDescription queryDescription,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeQueryClient.BeginGetNodeList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NodeList GetNodeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NodeList.CreateFromNativeListResult(this.nativeQueryClient.EndGetNodeList2(context));
            }

            #endregion

            #region Get ApplicationType List Async

            private Task<ApplicationTypeList> GetApplicationTypeListAsyncHelper(string applicationTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationTypeList>(
                    (callback) => this.GetApplicationTypeListAsyncBeginWrapper(applicationTypeNameFilter, timeout, callback),
                    this.GetApplicationTypeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationTypeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationTypeListAsyncBeginWrapper(string applicationTypeNameFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ApplicationTypeQueryDescription queryDesc = new ApplicationTypeQueryDescription(applicationTypeNameFilter);
                    return this.nativeQueryClient.BeginGetApplicationTypeList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationTypeList GetApplicationTypeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationTypeList.CreateFromNativeListResult(this.nativeQueryClient.EndGetApplicationTypeList(context));
            }

            #endregion

            #region Get ApplicationType Paged List Async

            private Task<ApplicationTypePagedList> GetApplicationTypePagedListAsyncHelper(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationTypePagedList>(
                    (callback) => this.GetApplicationTypePagedListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetApplicationTypePagedListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationTypePagedList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationTypePagedListAsyncBeginWrapper(PagedApplicationTypeQueryDescription queryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeQueryClient.BeginGetApplicationTypePagedList(
                        queryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationTypePagedList GetApplicationTypePagedListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationTypePagedList.CreateFromNativeListResult(this.nativeQueryClient.EndGetApplicationTypePagedList(context));
            }

            #endregion

            #region Get ServiceType List Async

            private Task<ServiceTypeList> GetServiceTypeListAsyncHelper(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceTypeList>(
                    (callback) => this.GetServiceTypeListAsyncBeginWrapper(applicationTypeName, applicationTypeVersion, serviceTypeNameFilter, timeout, callback),
                    this.GetServiceTypeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetServiceTypeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceTypeListAsyncBeginWrapper(string applicationTypeName, string applicationTypeVersion, string serviceTypeNameFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ServiceTypeQueryDescription queryDesc = new ServiceTypeQueryDescription(applicationTypeName, applicationTypeVersion, serviceTypeNameFilter);
                    return this.nativeQueryClient.BeginGetServiceTypeList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceTypeList GetServiceTypeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceTypeList.CreateFromNativeListResult(this.nativeQueryClient.EndGetServiceTypeList(context));
            }

            #endregion

            #region Get ServiceGroupType List Async

            private Task<ServiceGroupMemberTypeList> GetServiceGroupMemberTypeListAsyncHelper(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceGroupMemberTypeList>(
                    (callback) => this.GetServiceGroupMemberTypeListAsyncBeginWrapper(applicationTypeName, applicationTypeVersion, serviceGroupTypeNameFilter, timeout, callback),
                    this.GetServiceGroupMemberTypeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetServiceGroupMemberTypeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceGroupMemberTypeListAsyncBeginWrapper(string applicationTypeName, string applicationTypeVersion, string serviceGroupTypeNameFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ServiceGroupMemberTypeQueryDescription queryDesc = new ServiceGroupMemberTypeQueryDescription(applicationTypeName, applicationTypeVersion, serviceGroupTypeNameFilter);
                    return this.nativeQueryClient.BeginGetServiceGroupMemberTypeList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceGroupMemberTypeList GetServiceGroupMemberTypeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceGroupMemberTypeList.CreateFromNativeListResult(this.nativeQueryClient.EndGetServiceGroupMemberTypeList(context));
            }

            #endregion

            #region Get Application List Async

            private Task<ApplicationList> GetApplicationListAsyncHelper(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationList>(
                    (callback) => this.GetApplicationListAsyncBeginWrapper(applicationQueryDescription, timeout, callback),
                    this.GetApplicationListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationListAsyncBeginWrapper(ApplicationQueryDescription applicationQueryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeQueryClient.BeginGetApplicationList(
                        applicationQueryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationList GetApplicationListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationList.CreateFromNativeListResult(this.nativeQueryClient.EndGetApplicationList2(context));
            }

            #endregion

            #region Get Compose Deployment Status List Async

            private Task<ComposeDeploymentStatusListWrapper> GetComposeDeploymentStatusListAsyncHelper(ComposeDeploymentStatusQueryDescriptionWrapper composeDeploymentQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ComposeDeploymentStatusListWrapper>(
                    (callback) => this.GetComposeDeploymentStatusListAsyncBeginWrapper(composeDeploymentQueryDescription, timeout, callback),
                    this.GetComposeDeploymentStatusListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetComposeDeploymentStatusPagedListAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext GetComposeDeploymentStatusListAsyncBeginWrapper(ComposeDeploymentStatusQueryDescriptionWrapper composeDeploymentQueryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callbcak)
            {
                using (var pin = new PinCollection())
                {
                    return this.internalNativeApplicationClient.BeginGetComposeDeploymentStatusList(
                        composeDeploymentQueryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callbcak);
                }
            }

            private ComposeDeploymentStatusListWrapper GetComposeDeploymentStatusListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ComposeDeploymentStatusListWrapper.CreateFromNativeListResult(this.internalNativeApplicationClient.EndGetComposeDeploymentStatusList(context));
            }
            #endregion

            #region Get Service List Async

            private Task<ServiceList> GetServiceListAsyncHelper(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceList>(
                    (callback) => this.GetServiceListAsyncBeginWrapper(serviceQueryDescription, timeout, callback),
                    this.GetServiceListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetServiceList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceListAsyncBeginWrapper(ServiceQueryDescription serviceQueryDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeQueryClient.BeginGetServiceList(
                        serviceQueryDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceList GetServiceListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceList.CreateFromNativeListResult(this.nativeQueryClient.EndGetServiceList2(context));
            }

            #endregion

            #region Get Service Group List Async

            private Task<ServiceGroupMemberList> GetServiceGroupMemberListAsyncHelper(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceGroupMemberList>(
                    (callback) => this.GetServiceGroupMemberListAsyncBeginWrapper(applicationName, serviceNameFilter, timeout, callback),
                    this.GetServiceGroupMemberListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetServiceGroupMemberList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceGroupMemberListAsyncBeginWrapper(Uri applicationName, Uri serviceNameFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ServiceGroupMemberQueryDescription queryDesc = new ServiceGroupMemberQueryDescription(applicationName, serviceNameFilter);
                    return this.nativeQueryClient.BeginGetServiceGroupMemberList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceGroupMemberList GetServiceGroupMemberListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceGroupMemberList.CreateFromNativeListResult(this.nativeQueryClient.EndGetServiceGroupMemberList(context));
            }

            #endregion

            #region Get Partition List Async

            private Task<ServicePartitionList> GetPartitionListAsyncHelper(Uri serviceName, Guid partitionIdFilter, string continuationToken, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServicePartitionList>(
                    (callback) => this.GetPartitionListAsyncBeginWrapper(serviceName, partitionIdFilter, continuationToken, timeout, callback),
                    this.GetPartitionListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetPartitionList");
            }

            private Task<ServicePartitionList> GetPartitionAsyncHelper(Guid partitionIdFilter, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServicePartitionList>(
                    (callback) => this.GetPartitionListAsyncBeginWrapper(partitionIdFilter, timeout, callback),
                    this.GetPartitionListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetPartitionList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionListAsyncBeginWrapper(Guid partitionIdFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    PartitionQueryDescription queryDesc = new PartitionQueryDescription(partitionIdFilter);

                    return this.nativeQueryClient.BeginGetPartitionList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionListAsyncBeginWrapper(Uri serviceName, Guid partitionIdFilter, string continuationToken, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    PartitionQueryDescription queryDesc = new PartitionQueryDescription(serviceName, partitionIdFilter)
                    {
                        ContinuationToken = continuationToken,
                    };

                    return this.nativeQueryClient.BeginGetPartitionList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServicePartitionList GetPartitionListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServicePartitionList.CreateFromNativeListResult(this.nativeQueryClient.EndGetPartitionList2(context));
            }

            #endregion

            #region Get Partition Load Information Async

            private Task<PartitionLoadInformation> GetPartitionLoadInformationAsyncHelper(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<PartitionLoadInformation>(
                    (callback) => this.GetPartitionLoadInformationAsyncBeginWrapper(partitionId, timeout, callback),
                    this.GetPartitionLoadInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetPartitionLoadInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetPartitionLoadInformationAsyncBeginWrapper(Guid partitionId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    PartitionLoadInformationQueryDescription queryDesc = new PartitionLoadInformationQueryDescription(partitionId);
                    return this.nativeQueryClient.BeginGetPartitionLoadInformation(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private PartitionLoadInformation GetPartitionLoadInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return PartitionLoadInformation.CreateFromNative(this.nativeQueryClient.EndGetPartitionLoadInformation(context));
            }

            #endregion

            #region Get Replica List Async

            private Task<ServiceReplicaList> GetReplicaListAsyncHelper(
                Guid partitionId,
                Int64 replicaIdOrInstanceIdFilter,
                ServiceReplicaStatusFilter replicaStatusFilter,
                string continuationToken,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceReplicaList>(
                    (callback) => this.GetReplicaListAsyncBeginWrapper(partitionId, replicaIdOrInstanceIdFilter, replicaStatusFilter, continuationToken, timeout, callback),
                    this.GetReplicaListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetReplicaList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetReplicaListAsyncBeginWrapper(
                Guid partitionId,
                Int64 replicaIdOrInstanceIdFilter,
                ServiceReplicaStatusFilter replicaStatusFilter,
                string continuationToken,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ReplicaQueryDescription queryDesc = new ReplicaQueryDescription(partitionId, replicaIdOrInstanceIdFilter, replicaStatusFilter)
                    {
                        ContinuationToken = continuationToken,
                    };

                    return this.nativeQueryClient.BeginGetReplicaList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceReplicaList GetReplicaListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceReplicaList.CreateFromNativeListResult(this.nativeQueryClient.EndGetReplicaList2(context));
            }

            #endregion

            #region Get DeployedApplication List Async

            private Task<DeployedApplicationList> GetDeployedApplicationListAsyncHelper(
                string nodeName,
                Uri applicationNameFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedApplicationList>(
                    (callback) => this.GetDeployedApplicationListAsyncBeginWrapper(nodeName, applicationNameFilter, timeout, callback),
                    this.GetDeployedApplicationListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedApplicationList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedApplicationListAsyncBeginWrapper(
                string nodeName,
                Uri applicationNameFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    DeployedApplicationQueryDescription queryDesc = new DeployedApplicationQueryDescription(nodeName, applicationNameFilter);
                    return this.nativeQueryClient.BeginGetDeployedApplicationList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedApplicationList GetDeployedApplicationListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedApplicationList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedApplicationList(context));
            }

            #endregion

            #region Get DeployedApplication Paged List Async

            private Task<DeployedApplicationPagedList> GetDeployedApplicationPagedListAsyncHelper(
                PagedDeployedApplicationQueryDescription queryDescription,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedApplicationPagedList>(
                    (callback) => this.GetDeployedApplicationPagedListAsyncBeginWrapper(queryDescription, timeout, callback),
                    this.GetDeployedApplicationPagedListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedApplicationPagedList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedApplicationPagedListAsyncBeginWrapper(
                PagedDeployedApplicationQueryDescription queryDescription,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    PagedDeployedApplicationQueryDescription queryDesc = queryDescription;
                    return this.nativeQueryClient.BeginGetDeployedApplicationPagedList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedApplicationPagedList GetDeployedApplicationPagedListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedApplicationPagedList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedApplicationPagedList(context));
            }

            #endregion

            #region Get DeployedServicePackage List Async

            private Task<DeployedServicePackageList> GetDeployedServicePackageListAsyncHelper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedServicePackageList>(
                    (callback) => this.GetDeployedServicePackageListAsyncBeginWrapper(nodeName, applicationName, serviceManifestNameFilter, timeout, callback),
                    this.GetDeployedServicePackageListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedServiceManifestList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedServicePackageListAsyncBeginWrapper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    DeployedServicePackageQueryDescription queryDesc =
                        new DeployedServicePackageQueryDescription(
                            nodeName,
                            applicationName,
                            serviceManifestNameFilter);

                    return this.nativeQueryClient.BeginGetDeployedServicePackageList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedServicePackageList GetDeployedServicePackageListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedServicePackageList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedServicePackageList(context));
            }

            #endregion

            #region Get DeployedServiceType List Async

            private Task<DeployedServiceTypeList> GetDeployedServiceTypeListAsyncHelper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                string serviceTypeNameFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedServiceTypeList>(
                    (callback) => this.GetDeployedServiceTypeListAsyncBeginWrapper(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        serviceTypeNameFilter,
                        timeout,
                        callback),
                    this.GetDeployedServiceTypeListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedServiceTypeList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedServiceTypeListAsyncBeginWrapper(string nodeName, Uri applicationName, string serviceManifestNameFilter, string serviceTypeNameFilter, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    DeployedServiceTypeQueryDescription queryDesc =
                        new DeployedServiceTypeQueryDescription(
                            nodeName,
                            applicationName,
                            serviceManifestNameFilter,
                            serviceTypeNameFilter);

                    return this.nativeQueryClient.BeginGetDeployedServiceTypeList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedServiceTypeList GetDeployedServiceTypeListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedServiceTypeList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedServiceTypeList(context));
            }

            #endregion

            #region Get DeployedCodePackage List Async

            private Task<DeployedCodePackageList> GetDeployedCodePackageListAsyncHelper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                string codePackageNameFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedCodePackageList>(
                    (callback) => this.GetDeployedCodePackageListAsyncBeginWrapper(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        codePackageNameFilter,
                        timeout,
                        callback),
                    this.GetDeployedCodePackageListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedCodePackageList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedCodePackageListAsyncBeginWrapper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                string codePackageNameFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    DeployedCodePackageQueryDescription queryDesc =
                        new DeployedCodePackageQueryDescription(
                            nodeName,
                            applicationName,
                            serviceManifestNameFilter,
                            codePackageNameFilter);

                    return this.nativeQueryClient.BeginGetDeployedCodePackageList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedCodePackageList GetDeployedCodePackageListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedCodePackageList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedCodePackageList(context));
            }

            #endregion

            #region Get DeployedReplica List Async

            private Task<DeployedServiceReplicaList> GetDeployedReplicaListAsyncHelper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                Guid partitionIdFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedServiceReplicaList>(
                    (callback) => this.GetDeployedReplicaListAsyncBeginWrapper(
                        nodeName,
                        applicationName,
                        serviceManifestNameFilter,
                        partitionIdFilter,
                        timeout,
                        callback),
                    this.GetDeployedReplicaListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedReplicaList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedReplicaListAsyncBeginWrapper(
                string nodeName,
                Uri applicationName,
                string serviceManifestNameFilter,
                Guid partitionIdFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    DeployedServiceReplicaQueryDescription queryDesc =
                        new DeployedServiceReplicaQueryDescription(
                            nodeName,
                            applicationName,
                            serviceManifestNameFilter,
                            partitionIdFilter);

                    return this.nativeQueryClient.BeginGetDeployedReplicaList(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedServiceReplicaList GetDeployedReplicaListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedServiceReplicaList.CreateFromNativeListResult(this.nativeQueryClient.EndGetDeployedReplicaList(context));
            }

            #endregion

            #region Get DeployedServiceReplicaDetail Async

            private Task<DeployedServiceReplicaDetail> GetDeployedServiceReplicaDetailAsyncHelper(
                string nodeName,
                Guid partitionId,
                long replicaId,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<DeployedServiceReplicaDetail>(
                    (callback) => this.GetDeployedServiceReplicaDetailWrapper(
                        nodeName,
                        partitionId,
                        replicaId,
                        timeout,
                        callback),
                    this.GetDeployedServiceReplicaDetailAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetDeployedServiceReplicaDetail");
            }

            private NativeCommon.IFabricAsyncOperationContext GetDeployedServiceReplicaDetailWrapper(
                string nodeName,
                Guid partitionId,
                long replicaId,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var queryDesc = new DeployedServiceReplicaDetailQueryDescription(nodeName, partitionId, replicaId);

                    return this.nativeQueryClient.BeginGetDeployedReplicaDetail(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private DeployedServiceReplicaDetail GetDeployedServiceReplicaDetailAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return DeployedServiceReplicaDetail.CreateFromNative(this.nativeQueryClient.EndGetDeployedReplicaDetail(context));
            }

            #endregion

            #region Get Fabric Code Package Version List Async

            private Task<ProvisionedFabricCodeVersionList> GetProvisionedFabricCodeVersionListAsyncHelper(
                string codeVersionFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ProvisionedFabricCodeVersionList>(
                    (callback) => this.GetFabricCodeVersionListAsyncBeginWrapper(
                        codeVersionFilter,
                        timeout,
                        callback),
                    this.GetFabricCodeVersionListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetFabricCodeVersionList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetFabricCodeVersionListAsyncBeginWrapper(
                string codeVersionFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var nativeDescription = new NativeTypes.FABRIC_PROVISIONED_CODE_VERSION_QUERY_DESCRIPTION();
                    nativeDescription.CodeVersionFilter = pin.AddObject(codeVersionFilter);

                    return this.nativeQueryClient.BeginGetProvisionedFabricCodeVersionList(
                        pin.AddBlittable(nativeDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ProvisionedFabricCodeVersionList GetFabricCodeVersionListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ProvisionedFabricCodeVersionList.CreateFromNativeListResult(this.nativeQueryClient.EndGetProvisionedFabricCodeVersionList(context));
            }

            #endregion

            #region Get Cluster Load Information Async

            private Task<ClusterLoadInformation> GetClusterLoadInformationAsyncHelper(TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ClusterLoadInformation>(
                    (callback) => this.GetClusterLoadInformationAsyncBeginWrapper(timeout, callback),
                    this.GetClusterLoadInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetClusterLoadInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetClusterLoadInformationAsyncBeginWrapper(TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeQueryClient.BeginGetClusterLoadInformation(
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private ClusterLoadInformation GetClusterLoadInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ClusterLoadInformation.CreateFromNative(this.nativeQueryClient.EndGetClusterLoadInformation(context));
            }

            #endregion

            #region Get Fabric Config Package Version List Async

            private Task<ProvisionedFabricConfigVersionList> GetProvisionedFabricConfigVersionListAsyncHelper(
                string configVersionFilter,
                TimeSpan timeout,
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ProvisionedFabricConfigVersionList>(
                    (callback) => this.GetFabricConfigVersionListAsyncBeginWrapper(
                        configVersionFilter,
                        timeout,
                        callback),
                    this.GetFabricConfigVersionListAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetFabricConfigVersionList");
            }

            private NativeCommon.IFabricAsyncOperationContext GetFabricConfigVersionListAsyncBeginWrapper(
                string configVersionFilter,
                TimeSpan timeout,
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var nativeDescription = new NativeTypes.FABRIC_PROVISIONED_CONFIG_VERSION_QUERY_DESCRIPTION();
                    nativeDescription.ConfigVersionFilter = pin.AddObject(configVersionFilter);

                    return this.nativeQueryClient.BeginGetProvisionedFabricConfigVersionList(
                        pin.AddBlittable(nativeDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ProvisionedFabricConfigVersionList GetFabricConfigVersionListAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ProvisionedFabricConfigVersionList.CreateFromNativeListResult(this.nativeQueryClient.EndGetProvisionedFabricConfigVersionList(context));
            }

            #endregion

            #region Get Node Load Information Async

            private Task<NodeLoadInformation> GetNodeLoadInformationAsyncHelper(string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<NodeLoadInformation>(
                    (callback) => this.GetNodeLoadInformationAsyncBeginWrapper(nodeName, timeout, callback),
                    this.GetNodeLoadInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetNodeLoadInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetNodeLoadInformationAsyncBeginWrapper(string nodeName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {

                using (var pin = new PinCollection())
                {
                    var queryDesc = new NodeLoadInformationQueryDescription(nodeName);
                    return this.nativeQueryClient.BeginGetNodeLoadInformation(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private NodeLoadInformation GetNodeLoadInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NodeLoadInformation.CreateFromNative(this.nativeQueryClient.EndGetNodeLoadInformation(context));
            }

            #endregion

            #region Get Application Load Information Async

            private Task<ApplicationLoadInformation> GetApplicationLoadInformationAsyncHelper(string applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationLoadInformation>(
                    (callback) => this.GetApplicationLoadInformationAsyncBeginWrapper(applicationName, timeout, callback),
                    this.GetApplicationLoadInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationLoadInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationLoadInformationAsyncBeginWrapper(string applicationName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {

                using (var pin = new PinCollection())
                {
                    var queryDesc = new ApplicationLoadInformationQueryDescription(applicationName);
                    return this.nativeQueryClient.BeginGetApplicationLoadInformation(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationLoadInformation GetApplicationLoadInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationLoadInformation.CreateFromNative(this.nativeQueryClient.EndGetApplicationLoadInformation(context));
            }

            #endregion

            #region Get Replica Load Information Async

            private Task<ReplicaLoadInformation> GetReplicaLoadInformationAsyncHelper(Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ReplicaLoadInformation>(
                    (callback) => this.GetReplicaLoadInformationAsyncBeginWrapper(partitionId, replicaOrInstanceId, timeout, callback),
                    this.GetReplicaLoadInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetReplicaLoadInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetReplicaLoadInformationAsyncBeginWrapper(Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    ReplicaLoadInformationQueryDescription queryDesc = new ReplicaLoadInformationQueryDescription(partitionId, replicaOrInstanceId);
                    return this.nativeQueryClient.BeginGetReplicaLoadInformation(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ReplicaLoadInformation GetReplicaLoadInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ReplicaLoadInformation.CreateFromNative(this.nativeQueryClient.EndGetReplicaLoadInformation(context));
            }

            #endregion

            #region Get Unplaced Replica Information Async

            private Task<UnplacedReplicaInformation> GetUnplacedReplicaInformationAsyncHelper(string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<UnplacedReplicaInformation>(
                    (callback) => this.GetUnplacedReplicaInformationAsyncBeginWrapper(serviceName, partitionId, onlyQueryPrimaries, timeout, callback),
                    this.GetUnplacedReplicaInformationAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetUnplacedReplicaInformation");
            }

            private NativeCommon.IFabricAsyncOperationContext GetUnplacedReplicaInformationAsyncBeginWrapper(string serviceName, Guid partitionId, bool onlyQueryPrimaries, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    UnplacedReplicaInformationQueryDescription queryDesc = new UnplacedReplicaInformationQueryDescription(serviceName, partitionId, onlyQueryPrimaries);
                    return this.nativeQueryClient.BeginGetUnplacedReplicaInformation(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private UnplacedReplicaInformation GetUnplacedReplicaInformationAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return UnplacedReplicaInformation.CreateFromNative(this.nativeQueryClient.EndGetUnplacedReplicaInformation(context));
            }

            #endregion

            #region Get Application Name Async

            private Task<ApplicationNameResult> GetApplicationNameAsyncHelper(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationNameResult>(
                    (callback) => this.GetApplicationNameAsyncBeginWrapper(serviceName, timeout, callback),
                    this.GetApplicationNameAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetApplicationName");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationNameAsyncBeginWrapper(Uri serviceName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {

                using (var pin = new PinCollection())
                {
                    var queryDesc = new ApplicationNameQueryDescription(serviceName);
                    return this.nativeQueryClient.BeginGetApplicationName(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationNameResult GetApplicationNameAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ApplicationNameResult.CreateFromNativeResult(this.nativeQueryClient.EndGetApplicationName(context));
            }

            #endregion

            #region Get Service Name Async

            private Task<ServiceNameResult> GetServiceNameAsyncHelper(Guid partitionId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceNameResult>(
                    (callback) => this.GetServiceNameAsyncBeginWrapper(partitionId, timeout, callback),
                    this.GetServiceNameAsyncEndWrapper,
                    cancellationToken,
                    "QueryManager.GetServiceName");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceNameAsyncBeginWrapper(Guid partitionId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {

                using (var pin = new PinCollection())
                {
                    var queryDesc = new ServiceNameQueryDescription(partitionId);
                    return this.nativeQueryClient.BeginGetServiceName(
                        queryDesc.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceNameResult GetServiceNameAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceNameResult.CreateFromNativeResult(this.nativeQueryClient.EndGetServiceName(context));
            }

            #endregion

            #endregion
        }
    }
}