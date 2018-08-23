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
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Provides the functionality to manage Service Fabric applications.</para>
        /// </summary>
        public sealed class ApplicationManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricApplicationManagementClient10 nativeManagementClient;
            private NativeClient.IInternalFabricApplicationManagementClient internalNativeApplicationClient;

            #endregion

            #region Constructor

            internal ApplicationManagementClient(FabricClient fabricClient, NativeClient.IFabricApplicationManagementClient10 nativeManagementClient)
            {
                this.fabricClient = fabricClient;
                this.nativeManagementClient = nativeManagementClient;
                Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    this.internalNativeApplicationClient = (NativeClient.IInternalFabricApplicationManagementClient)this.nativeManagementClient;
                },
                "ApplicationManagementClient.ctor");
            }

            #endregion

            #region API

            /// <summary>
            ///   <para>Creates and instantiates the specific Service Fabric application.</para>
            /// </summary>
            /// <param name="applicationDescription">
            ///   <para>The description of the application.</para>
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
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationAlreadyExists" />: The application has already been created.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeNotFound" />: The requested application type has not been provisioned yet.</para>
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
            ///     The application capacity parameters specified are incorrect. Refer to
            ///     <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/>,
            ///     <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/> and
            ///     <see cref="System.Fabric.Description.ApplicationDescription.Metrics"/> for correct specification of application capacity parameters.
            ///   </para>
            /// </exception>
            public Task CreateApplicationAsync(ApplicationDescription applicationDescription)
            {
                return this.CreateApplicationAsync(applicationDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///   <para>Creates and instantiates the specific Service Fabric application.</para>
            /// </summary>
            /// <param name="applicationDescription">
            ///   <para>The description of the application.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The CancellationToken that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
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
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the ImageStore.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationAlreadyExists" />: The application has already been created.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeNotFound" />: The requested application type has not been provisioned yet.</para>
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
            ///     The application capacity parameters specified are incorrect. Refer to
            ///     <see cref="System.Fabric.Description.ApplicationDescription.MinimumNodes"/>,
            ///     <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/> and
            ///     <see cref="System.Fabric.Description.ApplicationDescription.Metrics"/> for correct specification of application capacity parameters.
            ///   </para>
            /// </exception>
            public Task CreateApplicationAsync(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ApplicationDescription>("applicationDescription", applicationDescription).NotNull();
                ApplicationDescription.Validate(applicationDescription);

                return this.CreateApplicationAsyncHelper(applicationDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// Updates a Service Fabric application.
            /// </summary>
            /// <param name="applicationUpdateDescription">Application update description.</param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.
            ///     </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.
            ///     </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.ApplicationUpdateInProgress" />: Another application update is already in progress.
            ///     </para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            ///   <para>
            ///     The application update parameters specified are incorrect. Refer to
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.MinimumNodes"/>,
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/> and
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.Metrics"/> for correct specification of application capacity parameters.
            ///   </para>
            ///   <para>
            ///     It is possible that parameters in <see cref="System.Fabric.Description.ApplicationUpdateDescription"/> are valid, but when combined with existing
            ///     application capacity parameters they produce an invalid combination. For example, setting <see cref="System.Fabric.Description.ApplicationUpdateDescription.MinimumNodes"/> to
            ///     a value that is higher than the one that was specified in <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/> when application was created.
            ///   </para>
            /// </exception>
            public Task UpdateApplicationAsync(ApplicationUpdateDescription applicationUpdateDescription)
            {
                return this.UpdateApplicationAsync(applicationUpdateDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Updates a Service Fabric application.
            /// </summary>
            /// <param name="applicationUpdateDescription">Application update description.</param>
            /// <param name="timeout">Defines the maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</param>
            /// <param name="cancellationToken">The CancellationToken that the operation is observing.
            /// It can be used to propagate notification that the operation should be canceled.</param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.
            ///     </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.
            ///     </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            ///     <para>
            ///         <see cref="System.Fabric.FabricErrorCode.ApplicationUpdateInProgress" />: Another application update is already in progress.
            ///     </para>
            /// </exception>
            /// <exception cref="System.ArgumentException">
            ///   <para>
            ///     The application update parameters specified are incorrect. Refer to
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.MinimumNodes"/>,
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.MaximumNodes"/> and
            ///     <see cref="System.Fabric.Description.ApplicationUpdateDescription.Metrics"/> for correct specification of application capacity parameters.
            ///   </para>
            ///   <para>
            ///     It is possible that parameters in <see cref="System.Fabric.Description.ApplicationUpdateDescription"/> are valid, but when combined with existing
            ///     application capacity parameters they produce an invalid combination. For example, setting <see cref="System.Fabric.Description.ApplicationUpdateDescription.MinimumNodes"/> to
            ///     a value that is higher than the one that was specified in <see cref="System.Fabric.Description.ApplicationDescription.MaximumNodes"/> when application was created.
            ///   </para>
            /// </exception>
            public Task UpdateApplicationAsync(ApplicationUpdateDescription applicationUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                Requires.Argument<ApplicationUpdateDescription>("applicationUpdateDescription", applicationUpdateDescription).NotNull();
                ApplicationUpdateDescription.Validate(applicationUpdateDescription);

                return this.UpdateApplicationAsyncHelper(applicationUpdateDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the application instance from the cluster and deletes all services belonging to the application.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
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
            [Obsolete("This api is deprecated, use overload taking DeleteApplicationDescription instead.", false)]
            public Task DeleteApplicationAsync(Uri applicationName)
            {
                return this.DeleteApplicationAsync(new DeleteApplicationDescription(applicationName), FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the application instance from the cluster and deletes all services belonging to the application.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
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
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
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
            [Obsolete("This api is deprecated, use overload taking DeleteApplicationDescription instead.", false)]
            public Task DeleteApplicationAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("applicationName", applicationName).NotNull();

                return this.DeleteApplicationAsyncHelper(applicationName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the application instance from the cluster and deletes all services belonging to the application.</para>
            /// </summary>
            /// <param name="deleteApplicationDescription">
            /// <para>The description of the application to be deleted.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
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
            /// <para>A forceful deletion call can convert on-going normal deletion to forceful one.</para> 
            /// </remarks>
            public Task DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription)
            {
                return this.DeleteApplicationAsync(deleteApplicationDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the application instance from the cluster and deletes all services belonging to the application.</para>
            /// </summary>
            /// <param name="deleteApplicationDescription">
            /// <para>The description of the application to be deleted.</para>
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
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
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
            /// <para>A forceful deletion call can convert on-going normal deletion to forceful one.</para> 
            /// </remarks>
            public Task DeleteApplicationAsync(DeleteApplicationDescription deleteApplicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<DeleteApplicationDescription>("deleteApplicationDescription", deleteApplicationDescription).NotNull();

                return this.DeleteApplicationAsyncHelper2(deleteApplicationDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Provisions or registers a Service Fabric application type with the cluster.</para>
            /// </summary>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the application package in the image store specified during <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, TimeSpan)"/>.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: There are errors in the manifests being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeProvisionInProgress" />: Another version of the application type is currently being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeAlreadyExists" />: The application type has already been provisioned</para>
            /// </exception>
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
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is mandatory before an application instance can be created.</para>
            /// </remarks>
            public Task ProvisionApplicationAsync(string applicationPackagePathInImageStore)
            {
                return this.ProvisionApplicationAsync(applicationPackagePathInImageStore, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Provision or register an application type with the cluster.</para>
            /// </summary>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the application package in the image store specified during <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, System.TimeSpan)"/>.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
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
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: There are errors in the manifests being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeProvisionInProgress" />: Another version of the application type is currently being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeAlreadyExists" />: The application type has already been provisioned</para>
            /// </exception>
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
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is mandatory before an application instance can be created.</para>
            /// </remarks>
            public Task ProvisionApplicationAsync(string applicationPackagePathInImageStore, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ProvisionApplicationAsync(new ProvisionApplicationTypeDescription(applicationPackagePathInImageStore), timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Provision or register an application type with the cluster.</para>
            /// </summary>
            /// <param name="description">
            /// <para>The description of the provision request.
            /// To provision a package previously copied to the image store, use <see cref="System.Fabric.Description.ProvisionApplicationTypeDescription"/>.
            /// To provision a package from an external store, use <see cref="System.Fabric.Description.ExternalStoreProvisionApplicationTypeDescription"/>.
            /// </para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: There are errors in the manifests being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeProvisionInProgress" />: Another version of the application type is currently being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeAlreadyExists" />: The application type has already been provisioned</para>
            /// </exception>
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
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>The provision operation is mandatory before an application instance can be created.</para>
            /// </remarks>
            public Task ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description)
            {
                return this.ProvisionApplicationAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Provision or register an application type with the cluster.</para>
            /// </summary>
            /// <param name="description">
            /// <para>Description of the provision request.
            /// To provision a package previously copied to the image store, use <see cref="System.Fabric.Description.ProvisionApplicationTypeDescription"/>.
            /// To provision a package from an external store, use <see cref="System.Fabric.Description.ExternalStoreProvisionApplicationTypeDescription"/>.
            /// </para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
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
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: There are errors in the manifests being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeProvisionInProgress" />: Another version of the application type is currently being provisioned. </para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementAlreadyExistsException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeAlreadyExists" />: The application type has already been provisioned</para>
            /// </exception>
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
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>The provision operation is mandatory before an application instance can be created.</para>
            /// </remarks>
            public Task ProvisionApplicationAsync(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ProvisionApplicationTypeDescriptionBase>("description", description).NotNull();

                return this.ProvisionApplicationAsyncHelper(description, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Unregisters and removes a Service Fabric application type from the cluster.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The name of the application type.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The version of the application type.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeInUse" />: The application type is being used by one or more applications. </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This method can only be called if all application instance of the application type has been deleted. Once the application type is unregistered, no new application instance can be created for this particular application type.</para>
            /// </remarks>
            public Task UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion)
            {
                return this.UnprovisionApplicationAsync(applicationTypeName, applicationTypeVersion, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Unregisters and removes a Service Fabric application type from the cluster.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The name of the application type.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The version of the application type.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
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
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeInUse" />: The application type is being used by one or more applications. </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This method can only be called if all application instance of the application type has been deleted. Once the application type is unregistered, no new application instance can be created for this particular application type.</para>
            /// </remarks>
            public Task UnprovisionApplicationAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.UnprovisionApplicationAsync(new UnprovisionApplicationTypeDescription(applicationTypeName, applicationTypeVersion), timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Unregisters and removes a Service Fabric application type from the cluster.</para>
            /// </summary>
            /// <param name="description">
            /// <para>Describes parameters for the unprovision operation.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeInUse" />: The application type is being used by one or more applications. </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This method can only be called if all application instance of the application type has been deleted. Once the application type is unregistered, no new application instance can be created for this particular application type.</para>
            /// </remarks>
            public Task UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description)
            {
                return this.UnprovisionApplicationAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Unregisters and removes a Service Fabric application type from the cluster.</para>
            /// </summary>
            /// <param name="description">
            /// <para>Describes parameters for the unprovision operation.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
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
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationTypeInUse" />: The application type is being used by one or more applications. </para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>This method can only be called if all application instance of the application type has been deleted. Once the application type is unregistered, no new application instance can be created for this particular application type.</para>
            /// </remarks>
            public Task UnprovisionApplicationAsync(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<UnprovisionApplicationTypeDescription>("description", description).NotNull();

                return this.UnprovisionApplicationAsyncHelper(description, timeout, cancellationToken);
            }

            /// <summary>
            ///   <para>Performs upgrade on an application instance.</para>
            /// </summary>
            /// <param name="upgradeDescription">
            ///   <para>The description of the upgrade policy and the application to be upgrade.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            ///   <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: The upgrade is invalid with respect to the provisioned manifests. </para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeValidationError" />: The application type does not exist. </para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeInProgress" />: The application is already being upgraded to the requested version.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            ///   <para>There was an error accessing a file on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            ///   <para>A required file was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            ///   <para>A required directory was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            ///   <para>A path to an image store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.IO.IOException">
            ///   <para>  
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            ///   <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            ///   <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public Task UpgradeApplicationAsync(ApplicationUpgradeDescription upgradeDescription)
            {
                return this.UpgradeApplicationAsync(upgradeDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            ///   <para>Performs upgrade on an application instance.</para>
            /// </summary>
            /// <param name="upgradeDescription">
            ///   <para>The description of the upgrade policy and the application to be upgraded.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The token that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            ///   <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageBuilderValidationError" />: The upgrade is invalid with respect to the provisioned manifests. </para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeValidationError" />: The application type does not exist. </para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidNameUri" />: The application name is not a valid Naming URI.</para>
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.CorruptedImageStoreObjectFound" />: A corrupted file was encountered on the image store.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricTransientException">
            ///   <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationUpgradeInProgress" />: The application is already being upgraded to the requested version.</para>
            /// </exception>
            /// <exception cref="System.UnauthorizedAccessException">
            ///   <para>There was an error accessing a file on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            ///   <para>A required file was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            ///   <para>A required directory was not found on the image store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            ///   <para>A path to an image store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.IO.IOException">
            ///   <para>  
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            ///   <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            ///   <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public Task UpgradeApplicationAsync(ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ApplicationUpgradeDescription>("upgradeDescription", upgradeDescription).NotNull();
                ApplicationUpgradeDescription.Validate(upgradeDescription);

                return this.UpgradeApplicationAsyncHelper(upgradeDescription, timeout, cancellationToken);
            }
            
            /// <summary>
            /// <para>Modifies the upgrade parameters of a pending application upgrade.</para>
            /// </summary>
            /// <param name="updateDescription">
            /// <para>Description of parameters to modify. Unspecified parameters are left unmodified and will retain their current value in the upgrade.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotUpgrading " />: There is no pending application upgrade to modify.</para>
            /// </exception>
            public Task UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription updateDescription)
            {
                return this.UpdateApplicationUpgradeAsync(updateDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Modifies the upgrade parameters of a pending application upgrade.</para>
            /// </summary>
            /// <param name="updateDescription">
            /// <para>Description of parameters to modify. Unspecified parameters are left unmodified and will retain their current value in the upgrade.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The token that the operation is observing. It can be used to propagate a notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotUpgrading " />: There is no pending application upgrade to modify.</para>
            /// </exception>
            public Task UpdateApplicationUpgradeAsync(ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ApplicationUpgradeUpdateDescription>("updateDescription", updateDescription).NotNull();
                ApplicationUpgradeUpdateDescription.Validate(updateDescription);

                return this.UpdateApplicationUpgradeAsyncHelper(updateDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Starts rolling back the current application upgrade.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>Name of the application</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotUpgrading " />: There is no pending upgrade for the specified application to rollback.</para>
            /// </exception>
            public Task RollbackApplicationUpgradeAsync(Uri applicationName)
            {
                return this.RollbackApplicationUpgradeAsync(applicationName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Starts rolling back the current application upgrade</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>Name of the application</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            public Task RollbackApplicationUpgradeAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("applicationName", applicationName).NotNull();

                return this.RollbackApplicationUpgradeAsyncHelper(applicationName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Retrieves the upgrade progress of the specified application instance.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified application instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsync(Uri applicationName)
            {
                return this.GetApplicationUpgradeProgressAsync(applicationName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Retrieves the upgrade progress of the specified application instance.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The URI of the application instance name.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the upgrade progress of the specified application instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsync(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("applicationName", applicationName).NotNull();

                return this.GetApplicationUpgradeProgressAsyncHelper(applicationName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Instructs the Service Fabric to upgrade the application instance in the next upgrade domain.</para>
            /// </summary>
            /// <param name="upgradeProgress">
            /// <para>–The Upgrade progress of the application instance of interest. This provides information about the next upgrade domain to be upgraded.</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Service Fabric would only move to the next upgrade domain if it has completed the upgrade domain it is currently updating. In other words, <see cref="System.Fabric.ApplicationUpgradeProgress.UpgradeState" /> property should be Pending before calling this method.</para>
            /// </remarks>
            public Task MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress upgradeProgress)
            {
                return this.MoveNextApplicationUpgradeDomainAsync(upgradeProgress, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Instructs the upgrade to continue with the application instance in the next upgrade domain.</para>
            /// </summary>
            /// <param name="upgradeProgress">
            /// <para>The upgrade progress of the application instance of interest. This provides information about the next upgrade domain to be upgraded.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
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
            ///     <see cref="System.Fabric.FabricErrorCode.ApplicationNotFound" />: The application does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Service Fabric would only move to the next upgrade domain if it has completed the upgrade domain it is currently updating. In other words, <see cref="System.Fabric.ApplicationUpgradeProgress.UpgradeState" /> property should be Pending before calling this method.</para>
            /// </remarks>
            public Task MoveNextApplicationUpgradeDomainAsync(ApplicationUpgradeProgress upgradeProgress, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ApplicationUpgradeProgress>("upgradeProgress", upgradeProgress).NotNull();

                return this.MoveNextApplicationUpgradeDomainAsyncHelper(upgradeProgress, timeout, cancellationToken);
            }

            /// <summary>
            ///   <para>Downloads packages associated with service manifest to image cache on specified node. </para>
            /// </summary>
            /// <param name="applicationTypeName">
            ///   <para>ApplicationTypeName associated with service manifest to be downloaded</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            ///   <para>Version of ApplicationType </para>
            /// </param>
            /// <param name="serviceManifestName">
            ///   <para>Name of service manifest whose packages need to be downloaded</para>
            /// </param>
            /// <param name="sharingPolicies">
            ///   <para>Sharing policy representing packages that need to be copied to shared folders</para>
            /// </param>
            /// <param name="nodeName">
            ///   <para>Name of the node where packages need to be downloaded.</para>
            /// </param>
            /// <param name="timeout">
            ///   <para>The maximum amount of time the system will allow this operation to continue before returning T:System.TimeoutException</para>
            /// </param>
            /// <param name="cancellationToken">
            ///   <para>The <see cref="System.Threading.CancellationToken" />that the operation is observing. It can be used to propagate notification that the operation should be canceled</para>
            /// </param>
            /// <returns>
            ///   <para>A <see cref="System.Threading.Tasks.Task" /> representing the operation.</para>
            /// </returns>
            public Task DeployServicePackageToNode(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, PackageSharingPolicyList sharingPolicies, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNull();
                Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNull();
                Requires.Argument<string>("serviceManifestName", serviceManifestName).NotNull();
                Requires.Argument<string>("nodeName", nodeName).NotNull();

                return this.DeployServicePackageToNodeAsyncHelper(applicationTypeName, applicationTypeVersion, serviceManifestName, sharingPolicies, nodeName, timeout, cancellationToken);
            }

            internal Task MoveNextApplicationUpgradeDomainAsync(Uri appName, string nextUpgradeDomain)
            {
                return this.MoveNextApplicationUpgradeDomainAsync(appName, nextUpgradeDomain, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            internal Task MoveNextApplicationUpgradeDomainAsync(Uri appName, string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("appName", appName).NotNull();
                Requires.Argument<string>("nextUpgradeDomain", nextUpgradeDomain).NotNull();

                return this.MoveNextApplicationUpgradeDomainAsyncHelper(appName, nextUpgradeDomain, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the contents of a provisioned Application Manifest stored in the cluster.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The type name as specified in the Application Manifest.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The type version as specified in the Application Manifest.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the raw XML string contents of the Application Manifest.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricErrorCode.ApplicationTypeNotFound">
            /// <para>The requested application type and version has not been provisioned.</para>
            /// </exception>
            public Task<string> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion)
            {
                return this.GetApplicationManifestAsync(applicationTypeName, applicationTypeVersion, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the contents of a provisioned Application Manifest stored in the cluster.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The type name as specified in the Application Manifest.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The type version as specified in the Application Manifest.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> whose result is the raw XML string contents of the Application Manifest.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricErrorCode.ApplicationTypeNotFound">
            /// <para>The requested application type and version has not been provisioned.</para>
            /// </exception>
            public Task<string> GetApplicationManifestAsync(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
                Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();

                return this.GetApplicationManifestAsyncHelper(applicationTypeName, applicationTypeVersion, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Uploads an application package to the Image Store in preparation for provisioning a new application type.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="applicationPackagePath">
            /// <para>The full path to the source application package.</para>
            /// </param>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the destination in the Image Store. This path is created relative to the root directory in the image store and used as the destination for the application package copy.</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            /// <para>A required file was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            /// <para>A required directory was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            /// <para>A path to an Image Store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>The timeout of the operation will default to 30 minutes for native image store and there is no timeout capacity for XStore and file share. Can also consider specifying proper timeout value in the overloading function <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(System.String, System.String, System.String ,System.TimeSpan)"/></para>
            /// </remarks>
            public void CopyApplicationPackage(string imageStoreConnectionString, string applicationPackagePath, string applicationPackagePathInImageStore)
            {
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                var defaultTimeout = GetImageStoreDefaultTimeout(imageStoreConnectionString);
                this.CopyApplicationPackage(imageStoreConnectionString, applicationPackagePath, applicationPackagePathInImageStore, defaultTimeout);
            }

            /// <summary>
            /// <para>Uploads an application package to the Image Store in preparation for provisioning a new application type.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="applicationPackagePath">
            /// <para>The full path to the source application package.</para>
            /// </param>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the destination in the Image Store. This path is created relative to the root directory in the image store and used as the destination for the application package copy.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timeout of copying application package operation</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            /// <para>A required file was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            /// <para>A required directory was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            /// <para>A path to an Image Store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public void CopyApplicationPackage(string imageStoreConnectionString, string applicationPackagePath, string applicationPackagePathInImageStore, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                Requires.Argument<string>("applicationPackagePath", applicationPackagePath).NotNullOrEmpty();
                Requires.Argument<string>("applicationPackagePathInImageStore", applicationPackagePathInImageStore).NotNullOrEmpty();

                this.fabricClient.imageStoreClient.UploadContent(
                    imageStoreConnectionString,
                    applicationPackagePathInImageStore,
                    applicationPackagePath,
                    timeout,
                    CopyFlag.AtomicCopy,
                    false);
            }

            /// <summary>
            /// <para>Uploads an application package to the Image Store in preparation for provisioning a new application type.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="applicationPackagePath">
            /// <para>The full path to the source application package.</para>
            /// </param>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the destination in the Image Store. This path is created relative to the root directory in the image store and used as the destination for the application package copy.</para>
            /// </param>
            /// <param name="progressHandler">
            /// <para>The progress handler to retrieve real time progress information</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timeout of copying application package operation</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.FileNotFoundException">
            /// <para>A required file was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.DirectoryNotFoundException">
            /// <para>A required directory was not found on the Image Store.</para>
            /// </exception>
            /// <exception cref="System.IO.PathTooLongException">
            /// <para>A path to an Image Store file/directory was too long.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para><see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the Image Store.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            public void CopyApplicationPackage(string imageStoreConnectionString, string applicationPackagePath, string applicationPackagePathInImageStore, IImageStoreProgressHandler progressHandler, TimeSpan timeout)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                Requires.Argument<string>("applicationPackagePath", applicationPackagePath).NotNullOrEmpty();
                Requires.Argument<string>("applicationPackagePathInImageStore", applicationPackagePathInImageStore).NotNullOrEmpty();

                this.fabricClient.imageStoreClient.UploadContent(
                    imageStoreConnectionString,
                    applicationPackagePathInImageStore,
                    applicationPackagePath,
                    progressHandler,
                    timeout,
                    CopyFlag.AtomicCopy,
                    false);
            }

            /// <summary>
            /// <para>Deletes an application package from the Image Store.</para>
            /// </summary>
            /// <param name="imageStoreConnectionString">
            /// <para>The connection string for the image store, which should match the "ImageStoreConnectionString" setting value found in the cluster manifest of the target cluster. In an on-premise cluster, the value is chosen during initial deployment by the cluster administrator. In an Azure cluster created through the Azure Resource Manager, this value is "fabric:ImageStore". The image store connection string value can be checked by looking at the cluster manifest contents returned by <see cref="System.Fabric.FabricClient.ClusterManagementClient.GetClusterManifestAsync()"/>. 
            /// </para>
            /// </param>
            /// <param name="applicationPackagePathInImageStore">
            /// <para>The relative path for the application package in the image store specified during <see cref="System.Fabric.FabricClient.ApplicationManagementClient.CopyApplicationPackage(string, string, string, TimeSpan)"/>.</para>
            /// </param>
            /// <exception cref="System.UnauthorizedAccessException">
            /// <para>There was an error accessing a file on the ImageStore.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para> <see cref="System.Fabric.FabricErrorCode.ImageStoreIOException" />: There was an IO error communicating with the image store. </para>
            /// <para> <see cref="System.Fabric.FabricErrorCode.ImageBuilderReservedDirectoryError" />: There was an error while trying to delete reserved folders in the image store. </para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            public void RemoveApplicationPackage(string imageStoreConnectionString, string applicationPackagePathInImageStore)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("imageStoreConnectionString", imageStoreConnectionString).NotNullOrEmpty();
                Requires.Argument<string>("applicationPackagePathInImageStore", applicationPackagePathInImageStore).NotNullOrEmpty();

                applicationPackagePathInImageStore = applicationPackagePathInImageStore.TrimEnd('\\');
                if (string.Compare(applicationPackagePathInImageStore, Constants.StoreFolder, true) == 0 ||
                    string.Compare(applicationPackagePathInImageStore, Constants.WindowsFabricStoreFolder, true) == 0)
                {
                    throw new FabricImageBuilderReservedDirectoryException(string.Format(StringResources.Error_InvalidReservedImageStoreOperation, applicationPackagePathInImageStore));
                }

                IImageStore imageStore = fabricClient.GetImageStore(imageStoreConnectionString);
                imageStore.DeleteContent(applicationPackagePathInImageStore, GetImageStoreDefaultTimeout(imageStoreConnectionString));
            }

            #endregion

            #region Helpers & Callbacks

            #region Create Application Async

            private Task CreateApplicationAsyncHelper(ApplicationDescription applicationDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateApplicationBeginWrapper(applicationDescription, timeout, callback),
                    this.CreateApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.CreateApplicationAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateApplicationBeginWrapper(ApplicationDescription applicationDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginCreateApplication(
                        applicationDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndCreateApplication(context);
            }

            #endregion

            #region UpdateApplicationAsync

            private Task UpdateApplicationAsyncHelper(ApplicationUpdateDescription applicationUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpdateApplicationBeginWrapper(applicationUpdateDescription, timeout, callback),
                    this.UpdateApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.UpdateApplicationAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext UpdateApplicationBeginWrapper(ApplicationUpdateDescription applicationUpdateDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginUpdateApplication(
                        applicationUpdateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpdateApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndUpdateApplication(context);
            }

            #endregion

            #region Delete Application Async

            private Task DeleteApplicationAsyncHelper(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteApplicationBeginWrapper(applicationName, timeout, callback),
                    this.DeleteApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.DeleteApplication");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteApplicationBeginWrapper(Uri appName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginDeleteApplication(
                        pin.AddObject(appName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndDeleteApplication(context);
            }

            #endregion

            #region Forcefully Delete Application Async

            private Task DeleteApplicationAsyncHelper2(DeleteApplicationDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteApplicationBeginWrapper2(description, timeout, callback),
                    this.DeleteApplicationEndWrapper2,
                    cancellationToken,
                    "ApplicationManager.DeleteApplication2");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteApplicationBeginWrapper2(DeleteApplicationDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginDeleteApplication2(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteApplicationEndWrapper2(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndDeleteApplication2(context);
            }

            #endregion

            #region Provision Application Async

            private Task ProvisionApplicationAsyncHelper(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.ProvisionApplicationBeginWrapper(description, timeout, callback),
                    this.ProvisionApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.ProvisionApplicationAsync");
            }

            private NativeCommon.IFabricAsyncOperationContext ProvisionApplicationBeginWrapper(ProvisionApplicationTypeDescriptionBase description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginProvisionApplicationType3(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void ProvisionApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndProvisionApplicationType3(context);
            }

            #endregion

            #region Unprovision Application Async

            private Task UnprovisionApplicationAsyncHelper(UnprovisionApplicationTypeDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UnprovisionApplicationBeginWrapper(description, timeout, callback),
                    this.UnprovisionApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.UnprovisionApplication");
            }

            private NativeCommon.IFabricAsyncOperationContext UnprovisionApplicationBeginWrapper(UnprovisionApplicationTypeDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginUnprovisionApplicationType2(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UnprovisionApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndUnprovisionApplicationType(context);
            }

            #endregion

            #region Upgrade Application Async

            private Task UpgradeApplicationAsyncHelper(ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpgradeApplicationBeginWrapper(upgradeDescription, timeout, callback),
                    this.UpgradeApplicationEndWrapper,
                    cancellationToken,
                    "ApplicationManager.UpgradeApplication");
            }

            private NativeCommon.IFabricAsyncOperationContext UpgradeApplicationBeginWrapper(ApplicationUpgradeDescription upgradeDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginUpgradeApplication(
                        upgradeDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpgradeApplicationEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndUpgradeApplication(context);
            }

            #endregion

            #region Update Application Upgrade Async

            private Task UpdateApplicationUpgradeAsyncHelper(ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpdateApplicationUpgradeBeginWrapper(updateDescription, timeout, callback),
                    this.UpdateApplicationUpgradeEndWrapper,
                    cancellationToken,
                    "ApplicationManager.UpdateApplicationUpgrade");
            }

            private NativeCommon.IFabricAsyncOperationContext UpdateApplicationUpgradeBeginWrapper(ApplicationUpgradeUpdateDescription updateDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginUpdateApplicationUpgrade(
                        updateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpdateApplicationUpgradeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndUpdateApplicationUpgrade(context);
            }

            #endregion

            #region Rollback Application Upgrade Async

            private Task RollbackApplicationUpgradeAsyncHelper(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RollbackApplicationUpgradeBeginWrapper(applicationName, timeout, callback),
                    this.RollbackApplicationUpgradeEndWrapper,
                    cancellationToken,
                    "ApplicationManager.RollbackApplicationUpgrade");
            }

            private NativeCommon.IFabricAsyncOperationContext RollbackApplicationUpgradeBeginWrapper(Uri applicationName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(applicationName))
                {
                    return this.nativeManagementClient.BeginRollbackApplicationUpgrade(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RollbackApplicationUpgradeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndRollbackApplicationUpgrade(context);
            }

            #endregion

            #region Get Application Upgrade Progress Async

            private Task<ApplicationUpgradeProgress> GetApplicationUpgradeProgressAsyncHelper(Uri applicationName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ApplicationUpgradeProgress>(
                    (callback) => this.GetApplicationUpgradeProgressBeginWrapper(applicationName, timeout, callback),
                    this.GetApplicationUpgradeProgressEndWrapper,
                    cancellationToken,
                    "ApplicationManager.GetApplicationUpgradeProgress");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationUpgradeProgressBeginWrapper(Uri applicationName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(applicationName))
                {
                    return this.nativeManagementClient.BeginGetApplicationUpgradeProgress(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ApplicationUpgradeProgress GetApplicationUpgradeProgressEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                // This cast must happen in an MTA thread
                return InteropHelpers.ApplicationUpgradeProgressHelper.CreateFromNative(
                    (NativeClient.IFabricApplicationUpgradeProgressResult3)this.nativeManagementClient.EndGetApplicationUpgradeProgress(context));
            }

            #endregion

            #region Get Application Manifest Async
            
            private Task<string> GetApplicationManifestAsyncHelper(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<string>(
                    (callback) => this.GetApplicationManifestAsyncBeginWrapper(applicationTypeName, applicationTypeVersion, timeout, callback),
                    this.GetApplicationManifestAsyncEndWrapper,
                    cancellationToken,
                    "ApplicationManager.GetApplicationManifest");
            }

            private NativeCommon.IFabricAsyncOperationContext GetApplicationManifestAsyncBeginWrapper(string applicationTypeName, string applicationTypeVersion, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginGetApplicationManifest(
                        pin.AddBlittable(applicationTypeName),
                        pin.AddBlittable(applicationTypeVersion),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private string GetApplicationManifestAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.GetApplicationManifestHelper.CreateFromNative(this.nativeManagementClient.EndGetApplicationManifest(context));
            }
           
            #endregion

            #region Move Next Application Upgrade Domain Async

            private Task MoveNextApplicationUpgradeDomainAsyncHelper(ApplicationUpgradeProgress upgradeProgress, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.MoveNextApplicationUpgradeDomainAsyncBeginWrapper(upgradeProgress, timeout, callback),
                    this.MoveNextApplicationUpgradeDomainAsyncEndWrapper,
                    cancellationToken,
                    "ApplicationManager.MoveNextApplicationUpgradeDomain");
            }

            private NativeCommon.IFabricAsyncOperationContext MoveNextApplicationUpgradeDomainAsyncBeginWrapper(ApplicationUpgradeProgress upgradeProgress, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeManagementClient.BeginMoveNextApplicationUpgradeDomain(
                    (upgradeProgress == null) ? null : upgradeProgress.InnerProgress,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void MoveNextApplicationUpgradeDomainAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndMoveNextApplicationUpgradeDomain(context);
            }

            private Task MoveNextApplicationUpgradeDomainAsyncHelper(Uri appName, string nextUpgradeDomain, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.MoveNextApplicationUpgradeDomainAsyncBeginWrapper2(appName, nextUpgradeDomain, timeout, callback),
                    this.MoveNextApplicationUpgradeDomainAsyncEndWrapper2,
                    cancellationToken,
                    "ApplicationManager.MoveNextApplicationUpgradeDomain2");
            }

            private NativeCommon.IFabricAsyncOperationContext MoveNextApplicationUpgradeDomainAsyncBeginWrapper2(Uri appName, string nextUpgradeDomain, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeManagementClient.BeginMoveNextApplicationUpgradeDomain2(
                        pin.AddObject(appName),
                        pin.AddBlittable(nextUpgradeDomain),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void MoveNextApplicationUpgradeDomainAsyncEndWrapper2(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndMoveNextApplicationUpgradeDomain2(context);
            }

            #endregion

            #endregion

            #region DeployServicePackageToNode

            private Task DeployServicePackageToNodeAsyncHelper(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, PackageSharingPolicyList sharingPolicies, string nodeName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeployServicePackageToNodeBeginWrapper(applicationTypeName, applicationTypeVersion, serviceManifestName, sharingPolicies, nodeName, timeout, callback),
                    this.DeployServicePackageToNodeEndWrapper,
                    cancellationToken,
                    "ApplicationManager.DeployServicePackageToNode");
            }

            private NativeCommon.IFabricAsyncOperationContext DeployServicePackageToNodeBeginWrapper(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, PackageSharingPolicyList sharingPolicies, string nodeName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    IntPtr NodeName = pin.AddObject(nodeName);
                    IntPtr ApplicationTypeName = pin.AddObject(applicationTypeName);
                    IntPtr ApplicationTypeVersion = pin.AddObject(applicationTypeVersion);
                    IntPtr ServiceManifestName = pin.AddObject(serviceManifestName);
                    IntPtr nativeSharingPolicies = IntPtr.Zero;
                    if (sharingPolicies != null)
                    {
                        nativeSharingPolicies = sharingPolicies.ToNative(pin);
                    }

                    return this.nativeManagementClient.BeginDeployServicePackageToNode(
                        ApplicationTypeName,
                        ApplicationTypeVersion,
                        ServiceManifestName,
                        nativeSharingPolicies,
                        NodeName,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeployServicePackageToNodeEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeManagementClient.EndDeployServicePackageToNode(context);
            }
            #endregion

        }
    }
}