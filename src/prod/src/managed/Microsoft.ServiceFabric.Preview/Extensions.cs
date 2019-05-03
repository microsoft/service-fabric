// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Preview.Client
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Preview.Client.Description;
    using Microsoft.ServiceFabric.Preview.Client.Query;

    /// <summary>
    /// Extension methods for FabricClient.
    /// </summary>
    public static class Extensions
    {
        /// <summary>
        ///   <para>Creates and instantiates the Service Fabric compose deployment described by the compose deployment description.</para>
        /// </summary>
        /// <param name="composeDeploymentDescription">
        /// <para>The <see cref="ComposeDeploymentDescription"/> that describes the compose deployment to be created.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        ///   <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
        ///   object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: The create application request is not valid with respect to the provisioned manifests for the requested application type.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
        ///   <para><see cref="System.Fabric.FabricErrorCode.ComposeDeploymentAlreadyExists" />: The compose deployment has already been created.</para>
        ///   <para><see cref="System.Fabric.FabricErrorCode.ApplicationAlreadyExists" />: The application has already been created so that compose deployment can not be created using the same name. </para>
        /// </exception>
        /// <exception cref="System.UnauthorizedAccessException">
        ///   <para>There was an error accessing a file on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.FileNotFoundException">
        ///   <para>A required file was not found .</para>
        /// </exception>
        /// <exception cref="System.IO.PathTooLongException">
        ///   <para>A path to an Image Store file/directory was too long.</para>
        /// </exception>
        /// <exception cref="System.IO.IOException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        ///   <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        ///   <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        ///   <para>
        ///     The parameters specified via the <see cref="ComposeDeploymentDescription"/> are incorrect.
        ///   </para>
        /// </exception>
        public static Task CreateComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client, 
            ComposeDeploymentDescription composeDeploymentDescription)
        {
            return client.CreateComposeDeploymentAsync(composeDeploymentDescription.ToWrapper());
        }

        /// <summary>
        ///   <para>Creates and instantiates the Service Fabric compose deployment described by the compose deployment description.</para>
        /// </summary>
        /// <param name="composeDeploymentDescription">
        /// <para>The <see cref="ComposeDeploymentDescription"/> that describes the compose deployment to be created.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
        /// </param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        ///   <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
        ///   object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: The create application request is not valid with respect to the provisioned manifests for the requested application type.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
        ///   <para><see cref="System.Fabric.FabricErrorCode.ComposeDeploymentAlreadyExists" />: The compose deployment has already been created.</para>
        ///   <para><see cref="System.Fabric.FabricErrorCode.ApplicationAlreadyExists" />: The application has already been created so that compose deployment can not be created using the same name. </para>
        /// </exception>
        /// <exception cref="System.UnauthorizedAccessException">
        ///   <para>There was an error accessing a file on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.FileNotFoundException">
        ///   <para>A required file was not found on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.DirectoryNotFoundException">
        ///   <para>A required directory was not found on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.PathTooLongException">
        ///   <para>A path to an Image Store file/directory was too long.</para>
        /// </exception>
        /// <exception cref="System.IO.IOException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        ///   <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        ///   <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        ///   <para>
        ///     The parameters specified via the <see cref="ComposeDeploymentDescription"/> are incorrect.
        ///   </para>
        /// </exception>
        public static Task CreateComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client, 
            ComposeDeploymentDescription composeDeploymentDescription,
            TimeSpan timeout)
        {
            return client.CreateComposeDeploymentAsync(composeDeploymentDescription.ToWrapper(), timeout);
        }

        /// <summary>
        ///   <para>Creates and instantiates the Service Fabric compose deployment described by the compose deployment description.</para>
        /// </summary>
        /// <param name="composeDeploymentDescription">
        /// <para>The <see cref="ComposeDeploymentDescription"/> that describes the compose deployment to be created.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The CancellationToken that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
        /// </param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        ///   <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> 
        ///   object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: The create application request is not valid with respect to the provisioned manifests for the requested application type.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
        ///   <para><see cref="System.Fabric.FabricErrorCode.ComposeDeploymentAlreadyExists" />: The compose deployment has already been created.</para>
        ///   <para><see cref="System.Fabric.FabricErrorCode.ApplicationAlreadyExists" />: The application has already been created so that compose deployment can not be created using the same name. </para>
        /// </exception>
        /// <exception cref="System.UnauthorizedAccessException">
        ///   <para>There was an error accessing a file on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.FileNotFoundException">
        ///   <para>A required file was not found on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.DirectoryNotFoundException">
        ///   <para>A required directory was not found on the Image Store.</para>
        /// </exception>
        /// <exception cref="System.IO.PathTooLongException">
        ///   <para>A path to an Image Store file/directory was too long.</para>
        /// </exception>
        /// <exception cref="System.IO.IOException">
        ///   <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        ///   <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        ///   <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.ArgumentException">
        ///   <para>
        ///     The parameters specified via the <see cref="ComposeDeploymentDescription"/> are incorrect.
        ///   </para>
        /// </exception>
        public static Task CreateComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client, 
            ComposeDeploymentDescription composeDeploymentDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return client.CreateComposeDeploymentAsync(composeDeploymentDescription.ToWrapper(), timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Gets the status of compose deployments created that match filters specified in query description (if any).
        /// If the compose deployments do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
        /// </summary>
        /// <param name="composeDeploymentQueryDescription">
        /// <para>The <see cref="Description.ComposeDeploymentStatusQueryDescription" /> that determines which compose deployments should be queried.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
        /// The value of TResult parameter is an <see cref="Microsoft.ServiceFabric.Preview.Client.Query.ComposeDeploymentStatusList" /> that represents the list of compose deployments that respect the filters in the <see cref="Description.ComposeDeploymentStatusQueryDescription" /> and fit the page.
        /// If the provided query description has no matching compose deployments, it will return a list of 0 entries.</para>
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
        public static Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(
            this FabricClient.QueryClient client,
            ComposeDeploymentStatusQueryDescription composeDeploymentQueryDescription)
        {
            return Task<ComposeDeploymentStatusList>.Run(() => { return ComposeDeploymentStatusListConverter(client.GetComposeDeploymentStatusPagedListAsync(composeDeploymentQueryDescription.ToWrapper()).Result); });
        }

        /// <summary>
        /// <para>Gets the status of compose deployments created that match filters specified in query description (if any).
        /// If the compose deployments do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
        /// </summary>
        /// <param name="composeDeploymentQueryDescription">
        /// <para>The <see cref="Description.ComposeDeploymentStatusQueryDescription" /> that determines which compose deployments should be queried.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
        /// The value of TResult parameter is an <see cref="Microsoft.ServiceFabric.Preview.Client.Query.ComposeDeploymentStatusList" /> that represents the list of compose deployments that respect the filters in the <see cref="Description.ComposeDeploymentStatusQueryDescription" /> and fit the page.
        /// If the provided query description has no matching compose deployments, it will return a list of 0 entries.</para>
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
        public static Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(
            this FabricClient.QueryClient client,
            ComposeDeploymentStatusQueryDescription composeDeploymentQueryDescription,
            TimeSpan timeout)
        {
            return Task<ComposeDeploymentStatusList>.Run(() => { return ComposeDeploymentStatusListConverter(client.GetComposeDeploymentStatusPagedListAsync(composeDeploymentQueryDescription.ToWrapper(), timeout).Result); });
        }

        /// <summary>
        /// <para>Gets the status of compose deployments created that match filters specified in query description (if any).
        /// If the compose deployments do not fit in a page, one page of results is returned as well as a continuation token which can be used to get the next page.</para>
        /// </summary>
        /// <param name="composeDeploymentQueryDescription">
        /// <para>The <see cref="Description.ComposeDeploymentStatusQueryDescription" /> that determines which compose deployments should be queried.</para>
        /// </param>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The CancellationToken that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> that represents the asynchronous query operation.
        /// The value of TResult parameter is an <see cref="Microsoft.ServiceFabric.Preview.Client.Query.ComposeDeploymentStatusList" /> that represents the list of compose deployments that respect the filters in the <see cref="Description.ComposeDeploymentStatusQueryDescription" /> and fit the page.
        /// If the provided query description has no matching compose deployments, it will return a list of 0 entries.</para>
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
        public static Task<ComposeDeploymentStatusList> GetComposeDeploymentStatusPagedListAsync(
            this FabricClient.QueryClient client,
            ComposeDeploymentStatusQueryDescription composeDeploymentQueryDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return Task<ComposeDeploymentStatusList>.Run(() => { return ComposeDeploymentStatusListConverter(client.GetComposeDeploymentStatusPagedListAsync(composeDeploymentQueryDescription.ToWrapper(), timeout, cancellationToken).Result); });
        }

        private static ComposeDeploymentStatusList ComposeDeploymentStatusListConverter(System.Fabric.Query.ComposeDeploymentStatusListWrapper wrapper)
        {
            IList<ComposeDeploymentStatusResultItem> list = new List<ComposeDeploymentStatusResultItem>();
            foreach(var wrapperItem in wrapper)
            {
                var item = new ComposeDeploymentStatusResultItem()
                {
                    DeploymentName = wrapperItem.DeploymentName,
                    ApplicationName = wrapperItem.ApplicationName,
                    StatusDetails = wrapperItem.StatusDetails
                };

                switch(wrapperItem.ComposeDeploymentStatus)
                {
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Provisioning:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Provisioning;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Creating:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Creating;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Ready:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Ready;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Unprovisioning:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Unprovisioning;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Deleting:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Deleting;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Failed:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Failed;
                        break;
                    case System.Fabric.Query.ComposeDeploymentStatusWrapper.Upgrading:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Upgrading;
                        break;
                    default:
                        item.ComposeDeploymentStatus = ComposeDeploymentStatus.Invalid;
                        break;
                }

                list.Add(item);
            }

            return new ComposeDeploymentStatusList(list)
            {
                ContinuationToken = wrapper.ContinuationToken
            };
        }

        /// <summary>
        /// <para>Deletes the compose deployment instance from the cluster.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.DeleteComposeDeploymentDescription"/> that determines which compose deployment should be deleted.</para>
        /// </param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeInProgress" />: The application is being upgraded. </para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        /// <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <remarks>
        /// <para>All compose deployment state will be lost and cannot be recovered after the compose deployment is deleted.</para>
        /// </remarks>
        public static Task DeleteComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client,
            DeleteComposeDeploymentDescription description)
        {
            return client.DeleteComposeDeploymentWrapperAsync(description.ToWrapper());
        }

        /// <summary>
        /// <para>Deletes the compose deployment from the cluster.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.DeleteComposeDeploymentDescription"/> that determines which compose deployment should be deleted.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning System.TimeoutException.</para>
        /// </param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeInProgress" />: The application is being upgraded. </para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        /// <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <remarks>
        /// <para>All application state will be lost and cannot be recovered after the application is deleted.</para>
        /// </remarks>
        public static Task DeleteComposeDeploymentAsync(
                this FabricClient.ComposeDeploymentClient client,
                DeleteComposeDeploymentDescription description,
                TimeSpan timeout)
        {
            return client.DeleteComposeDeploymentWrapperAsync(description.ToWrapper(), timeout);
        }

        /// <summary>
        /// <para>Deletes the compose deployment from the cluster.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.DeleteComposeDeploymentDescription"/> that determines which compose deployment should be deleted.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning System.TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
        /// </param>
        /// <returns>
        ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricTransientException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeInProgress" />: The application is being upgraded. </para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        /// <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <remarks>
        /// <para>All application state will be lost and cannot be recovered after the application is deleted.</para>
        /// </remarks>
        public static Task DeleteComposeDeploymentAsync(
                this FabricClient.ComposeDeploymentClient client,
                DeleteComposeDeploymentDescription description,
                TimeSpan timeout,
                CancellationToken cancellationToken)
        {
            return client.DeleteComposeDeploymentWrapperAsync(description.ToWrapper(), timeout, cancellationToken);
        }
    
        /// <summary>
        /// <para>Starts the upgrade for the compose deployment identified by the deployment name, in the cluster.</para>
        /// </summary>
        public static Task UpgradeComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client, 
            ComposeDeploymentUpgradeDescription upgradeDescription)
        {
            return client.UpgradeComposeDeploymentAsync(upgradeDescription.ToWrapper());
        }    

        /// <summary>
        /// <para>Starts the upgrade for the compose deployment identified by the deployment name, in the cluster.</para>
        /// </summary>
        public static Task UpgradeComposeDeploymentAsync(
            this FabricClient.ComposeDeploymentClient client, 
            ComposeDeploymentUpgradeDescription upgradeDescription,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return client.UpgradeComposeDeploymentAsync(upgradeDescription.ToWrapper(), timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Retrieves the upgrade progress of the specified compose deployment.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="deploymentName">
        /// <para>The name of the deployment.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified compose deployment.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        /// <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        public static Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(
            this FabricClient.ComposeDeploymentClient client,
            string deploymentName)
        {
            return client.GetComposeDeploymentUpgradeProgressAsync(deploymentName, FabricClient.DefaultTimeout, CancellationToken.None);
        }

        /// <summary>
        /// <para>Retrieves the upgrade progress of the specified compose deployment.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="deploymentName">
        /// <para>The name of the deployment.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified compose deployment.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricObjectClosedException">
        /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        ///     <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.TimeoutException">
        /// <para>The request timed out but may have already been accepted for processing by the system.</para>
        /// </exception>
        /// <exception cref="System.OperationCanceledException">
        /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
        /// </exception>
        internal static Task<ComposeDeploymentUpgradeProgress> GetComposeDeploymentUpgradeProgressAsync(
            this FabricClient.ComposeDeploymentClient client,
            string deploymentName,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            return client.GetComposeDeploymentUpgradeProgressAsync(deploymentName, timeout, cancellationToken);
        }

        /// <summary>
        /// <para>Starts rolling back the current compose deployment upgrade.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.ComposeDeploymentRollbackDescription"/> that determines which compose deployment should be rolled back.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotUpgrading" />: There is no pending upgrade for the specified compose deployment to rollback.
        /// </para>
        /// </exception>
        public static Task RollbackComposeDeploymentUpgradeAsync(
            this FabricClient.ComposeDeploymentClient client,
            ComposeDeploymentRollbackDescription description)
        {
            return client.RollbackComposeDeploymentUpgradeAsync(description.ToWrapper());
        }

        /// <summary>
        /// <para>Starts rolling back the current compose deployment upgrade.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.ComposeDeploymentRollbackDescription"/> that determines which compose deployment should be rolled back.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning System.TimeoutException.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotUpgrading" />: There is no pending upgrade for the specified compose deployment to rollback.
        /// </para>
        /// </exception>
        public static Task RollbackComposeDeploymentUpgradeAsync(
                this FabricClient.ComposeDeploymentClient client,
                ComposeDeploymentRollbackDescription description,
                TimeSpan timeout)
        {
            return client.RollbackComposeDeploymentUpgradeAsync(description.ToWrapper(), timeout);
        }

        /// <summary>
        /// <para>Starts rolling back the current compose deployment upgrade.</para>
        /// </summary>
        /// <param name="client"><see cref="System.Fabric.FabricClient"/> object.</param>
        /// <param name="description">
        /// <para>The <see cref="Description.ComposeDeploymentRollbackDescription"/> that determines which compose deployment should be rolled back.</para>
        /// </param>
        /// <param name="timeout">
        /// <para>Defines the maximum amount of time the system will allow this operation to continue before returning System.TimeoutException.</para>
        /// </param>
        /// <param name="cancellationToken">
        /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
        /// </param>
        /// <returns>
        /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
        /// </returns>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotFound" />: The compose deployment does not exist.
        /// </para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        /// <para>
        /// <see cref="System.Fabric.FabricErrorCode.ComposeDeploymentNotUpgrading" />: There is no pending upgrade for the specified compose deployment to rollback.
        /// </para>
        /// </exception>
        public static Task RollbackComposeDeploymentUpgradeAsync(
                this FabricClient.ComposeDeploymentClient client,
                ComposeDeploymentRollbackDescription description,
                TimeSpan timeout,
                CancellationToken cancellationToken)
        {
            return client.RollbackComposeDeploymentUpgradeAsync(description.ToWrapper(), timeout, cancellationToken);
        }
    }
}