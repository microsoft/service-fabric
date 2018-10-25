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
    using System.Threading;
    using System.Threading.Tasks;

    public partial class FabricClient
    {
        /// <summary>
        /// <para>Allows client side creation, deletion, and inspection of service groups inside the cluster, 
        /// just like the <see cref="System.Fabric.FabricClient.ServiceManagementClient" /> for regular services.</para>
        /// </summary>
        public class ServiceGroupManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricServiceGroupManagementClient4 nativeServiceGroupClient;

            #endregion

            #region Constructor

            internal ServiceGroupManagementClient(FabricClient fabricClient, NativeClient.IFabricServiceGroupManagementClient4 nativeServiceGroupClient)
            {
                this.fabricClient = fabricClient;
                this.nativeServiceGroupClient = nativeServiceGroupClient;
            }

            #endregion

            #region APIs

            /// <summary>
            /// Asynchronously creates a service group from the given <see cref="System.Fabric.Description.ServiceGroupDescription" />.
            /// </summary>
            /// <param name="description">The <see cref="System.Fabric.Description.ServiceGroupDescription" /> which describes the group and its members.</param>
            /// <returns>The task representing the asynchronous service group creation operation.</returns>
            public Task CreateServiceGroupAsync(ServiceGroupDescription description)
            {
                return this.CreateServiceGroupAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Asynchronously creates a service group from the given <see cref="System.Fabric.Description.ServiceGroupDescription" /> with the provided timeout 
            /// and <see cref="System.Threading.CancellationToken" />.
            /// </summary>
            /// <param name="description">The <see cref="System.Fabric.Description.ServiceGroupDescription" /> which describes the group and its members.</param>
            /// <param name="timeout">Timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning 
            /// a Timeout Exception.</param>
            /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken" /> object that the operation is observing.  It can be 
            /// used to send a notification that the operation should be canceled.  Note that cancellation is advisory and that the operation may 
            /// still be completed even if it is canceled.</param>
            /// <returns>The task representing the asynchronous service group creation operation.</returns>
            public Task CreateServiceGroupAsync(ServiceGroupDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ServiceGroupDescription>("description", description).NotNull();
                ServiceGroupDescription.Validate(description);

                return this.CreateServiceGroupAsyncHelper(description, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Creates a Service Group from a Service Group Template that is pre-defined in the current Application Manifest.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>Application name for the Service Group</para>
            /// </param>
            /// <param name="serviceName">
            /// <para>Service name for the Service Group</para>
            /// </param>
            /// <param name="serviceTypeName">
            /// <para>Service Type Name for the Service Group</para>
            /// </param>
            /// <param name="initializationData">
            /// <para>Initialization data to pass into the Service Group instance</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group creation operation.</para>
            /// </returns>
            public Task CreateServiceGroupFromTemplateAsync(Uri applicationName, Uri serviceName, string serviceTypeName, byte[] initializationData)
            {
                return this.CreateServiceGroupFromTemplateAsync(
                    new ServiceGroupFromTemplateDescription(
                        applicationName, 
                        serviceName, 
                        serviceTypeName, 
                        ServicePackageActivationMode.SharedProcess, 
                        initializationData));
            }

            /// <summary>
            /// <para>Creates a service group from a Service Group Template that is pre-defined in the current Application Manifest.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>Application name for the Service Group</para>
            /// </param>
            /// <param name="serviceName">
            /// <para>Service name for the Service Group</para>
            /// </param>
            /// <param name="serviceTypeName">
            /// <para>Service Type Name for the Service Group</para>
            /// </param>
            /// <param name="initializationData">
            /// <para>Initialization data to pass into the Service Group instance</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Maximum allowed time for the operation to complete before TimeoutException is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group creation operation.</para>
            /// </returns>
            public Task CreateServiceGroupFromTemplateAsync(Uri applicationName, Uri serviceName, string serviceTypeName, byte[] initializationData, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.CreateServiceGroupFromTemplateAsync(
                    new ServiceGroupFromTemplateDescription(
                        applicationName,
                        serviceName,
                        serviceTypeName,
                        ServicePackageActivationMode.SharedProcess,
                        initializationData), 
                    timeout, 
                    cancellationToken);
            }

            /// <summary>
            /// <para>Creates a service group from a Service Group Template that is pre-defined in the current Application Manifest.</para>
            /// </summary>
            /// <param name="serviceGroupFromTemplateDescription">
            /// <para>Describes the Service Group to be created from Service Group Template specified in Application Manifest.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group creation operation.</para>
            /// </returns>
            public Task CreateServiceGroupFromTemplateAsync(ServiceGroupFromTemplateDescription serviceGroupFromTemplateDescription)
            {
                return this.CreateServiceGroupFromTemplateAsync(
                    serviceGroupFromTemplateDescription,
                    FabricClient.DefaultTimeout, 
                    CancellationToken.None);
            }

            /// <summary>
            /// <para>Creates a service group from a Service Group Template that is pre-defined in the current Application Manifest.</para>
            /// </summary>
            /// <param name="serviceGroupFromTemplateDescription">
            /// <para>Describes the Service Group to be created from Service Group Template specified in Application Manifest.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Maximum allowed time for the operation to complete before TimeoutException is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group creation operation.</para>
            /// </returns>
            public Task CreateServiceGroupFromTemplateAsync(
                ServiceGroupFromTemplateDescription serviceGroupFromTemplateDescription, 
                TimeSpan timeout, 
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                Requires.Argument("serviceGroupFromTemplateDescription", serviceGroupFromTemplateDescription).NotNull();

                return this.CreateServiceGroupFromTemplateAsyncHelper(
                    serviceGroupFromTemplateDescription,
                    timeout, 
                    cancellationToken);
            }

            /// <summary>
            /// Asynchronously updates a service group with the specified description.
            /// </summary>
            /// <param name="name">The URI name of the service group being updated.</param>
            /// <param name="updateDescription">The <see cref="System.Fabric.Description.ServiceGroupUpdateDescription" /> that specifies the updated configuration for the service group.</param>
            /// <returns>The task representing the asynchronous service group update operation.</returns>
            public Task UpdateServiceGroupAsync(Uri name, ServiceGroupUpdateDescription updateDescription)
            {
                return this.UpdateServiceGroupAsync(name, updateDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// Asynchronously updates a service group with specified description.
            /// </summary>
            /// <param name="name">The URI name of the service being updated.</param>
            /// <param name="updateDescription">The <see cref="System.Fabric.Description.ServiceGroupUpdateDescription" /> that specifies the updated configuration for the service.</param>
            /// <param name="timeout">The maximum amount of time the system will allow this API to take before returning <see cref="System.TimeoutException" />.</param>
            /// <param name="cancellationToken">The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</param>
            /// <returns>The task representing the asynchronous service group update operation.</returns>
            public Task UpdateServiceGroupAsync(Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();
                Requires.Argument<ServiceGroupUpdateDescription>("updateDescription", updateDescription).NotNull();

                return this.UpdateServiceGroupAsyncHelper(name, updateDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously deletes the specified service group.</para>
            /// </summary>
            /// <param name="serviceGroupName">
            /// <para>The name of the service group to be deleted.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group delete operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task DeleteServiceGroupAsync(Uri serviceGroupName)
            {
                return this.DeleteServiceGroupAsync(serviceGroupName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously deletes the specified service group with the provided timeout and <see cref="System.Threading.CancellationToken" />.</para>
            /// </summary>
            /// <param name="serviceGroupName">
            /// <para>The name of the service group to be deleted.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a Timeout Exception.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> object that the operation is observing.  It can be used to send a notification that the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous service group delete operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task DeleteServiceGroupAsync(Uri serviceGroupName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceGroupName", serviceGroupName).NotNull();

                return this.DeleteServiceGroupAsyncHelper(serviceGroupName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Asynchronously fetches the <see cref="System.Fabric.Description.ServiceGroupDescription" /> for the specified service group, if it exists.</para>
            /// </summary>
            /// <param name="serviceGroupName">
            /// <para>The name of the service group whose <see cref="System.Fabric.Description.ServiceGroupDescription" /> should be fetched.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<ServiceGroupDescription> GetServiceGroupDescriptionAsync(Uri serviceGroupName)
            {
                return this.GetServiceGroupDescriptionAsync(serviceGroupName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Asynchronously fetches the <see cref="System.Fabric.Description.ServiceGroupDescription" /> for the specified service group, if it exists, with the provided timeout and <see cref="System.Threading.CancellationToken" />.</para>
            /// </summary>
            /// <param name="serviceGroupName">
            /// <para>The name of the service group whose <see cref="System.Fabric.Description.ServiceGroupDescription" /> should be fetched.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>Timespan that defines the maximum amount of time Service Fabric will allow this operation to continue before returning a Timeout Exception.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing.  It can be used to send a notification that the operation should be canceled.  Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>The task representing the asynchronous operation.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public Task<ServiceGroupDescription> GetServiceGroupDescriptionAsync(Uri serviceGroupName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceGroupName", serviceGroupName).NotNull();

                return this.GetServiceGroupDescriptionAsyncHelper(serviceGroupName, timeout, cancellationToken);
            }

            #endregion

            #region Helpers & Callbacks

            #region Create Service Group Async

            private Task CreateServiceGroupAsyncHelper(ServiceGroupDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateServiceGroupBeginWrapper(description, timeout, callback),
                    this.CreateServiceGroupEndWrapper,
                    cancellationToken,
                    "ServiceManager.CreateServiceGroup");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateServiceGroupBeginWrapper(ServiceGroupDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceGroupClient.BeginCreateServiceGroup(
                        InteropHelpers.ServiceGroupDescriptionHelper.ToNative(pin, description),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateServiceGroupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceGroupClient.EndCreateServiceGroup(context);
            }

            #endregion

            #region Create Service From Template Async

            private Task CreateServiceGroupFromTemplateAsyncHelper(
                ServiceGroupFromTemplateDescription serviceGroupFromTemplateDescription,
                TimeSpan timeout, 
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateServiceGroupFromTemplateBeginWrapper(
                        serviceGroupFromTemplateDescription, 
                        timeout, 
                        callback),
                    this.CreateServiceGroupFromTemplateEndWrapper,
                    cancellationToken,
                    "ApplicationManager.CreateServiceGroupFromTemplate");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateServiceGroupFromTemplateBeginWrapper(
                ServiceGroupFromTemplateDescription serviceGroupFromTemplateDescription, 
                TimeSpan timeout, 
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceGroupClient.BeginCreateServiceGroupFromTemplate2(
                        serviceGroupFromTemplateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateServiceGroupFromTemplateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceGroupClient.EndCreateServiceGroupFromTemplate(context);
            }

            #endregion

            #region Update Service Group Async

            private Task UpdateServiceGroupAsyncHelper(Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpdateServiceGroupBeginWrapper(name, updateDescription, timeout, callback),
                    this.UpdateServiceGroupEndWrapper,
                    cancellationToken,
                    "ServiceManager.UpdateServiceGroup");
            }

            private NativeCommon.IFabricAsyncOperationContext UpdateServiceGroupBeginWrapper(Uri name, ServiceGroupUpdateDescription updateDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceGroupClient.BeginUpdateServiceGroup(
                        pin.AddObject(name),
                        InteropHelpers.ServiceGroupUpdateDescriptionHelper.ToNative(pin, updateDescription),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpdateServiceGroupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceGroupClient.EndUpdateServiceGroup(context);
            }

            #endregion

            #region Delete Service Group Async

            private Task DeleteServiceGroupAsyncHelper(Uri serviceGroupName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteServiceGroupBeginWrapper(serviceGroupName, timeout, callback),
                    this.DeleteServiceGroupEndWrapper,
                    cancellationToken,
                    "ServiceManager.DeleteServiceGroup");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteServiceGroupBeginWrapper(Uri serviceGroupName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(serviceGroupName))
                {
                    return this.nativeServiceGroupClient.BeginDeleteServiceGroup(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteServiceGroupEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceGroupClient.EndDeleteServiceGroup(context);
            }

            #endregion

            #region Get ServiceGroup Description Async

            private Task<ServiceGroupDescription> GetServiceGroupDescriptionAsyncHelper(Uri serviceGroupName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceGroupDescription>(
                    (callback) => this.GetServiceGroupDescriptionBeginWrapper(serviceGroupName, timeout, callback),
                    this.GetServiceGroupDescriptionEndWrapper,
                    cancellationToken,
                    "ServiceManager.GetServiceGroupDescription");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceGroupDescriptionBeginWrapper(Uri serviceGroupName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(serviceGroupName))
                {
                    return this.nativeServiceGroupClient.BeginGetServiceGroupDescription(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceGroupDescription GetServiceGroupDescriptionEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ServiceGroupDescription.CreateFromNative(this.nativeServiceGroupClient.EndGetServiceGroupDescription(context));
            }

            #endregion

            #endregion
        }
    }
}