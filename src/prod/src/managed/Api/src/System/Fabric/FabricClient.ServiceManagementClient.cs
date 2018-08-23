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
        /// <para>Represents the enabling of the services to be managed.</para>
        /// </summary>
        public sealed class ServiceManagementClient
        {
            #region Fields

            private readonly FabricClient fabricClient;
            private readonly NativeClient.IFabricServiceManagementClient6 nativeServiceClient;
            private NativeClient.IInternalFabricServiceManagementClient2 internalNativeClient;

            #endregion

            #region Constructor

            internal ServiceManagementClient(FabricClient fabricClient, NativeClient.IFabricServiceManagementClient6 nativeServiceClient)
            {
                this.fabricClient = fabricClient;
                this.nativeServiceClient = nativeServiceClient;
                Utility.WrapNativeSyncInvokeInMTA(() =>
                {
                    // ReSharper disable once SuspiciousTypeConversion.Global
                    this.internalNativeClient = (NativeClient.IInternalFabricServiceManagementClient2)this.nativeServiceClient;
                },
                "ServiceManagementClient.ctor");
            }

            #endregion

            internal NativeClient.IFabricServiceManagementClient6 NativeServiceClient
            {
                get
                {
                    return this.nativeServiceClient;
                }
            }

            /// <summary>
            /// The event arguments for a <see cref="System.Fabric.FabricClient.ServiceManagementClient.ServiceNotificationFilterMatched" /> event.
            /// </summary>
            public class ServiceNotificationEventArgs : EventArgs
            {
                internal ServiceNotificationEventArgs(ServiceNotification notification)
                {
                    this.Notification = notification;
                } 

                /// <summary>
                /// <para>Gets the notification object.</para>
                /// </summary>
                /// <value>
                /// <para>The notification object.</para>
                /// </value>
                public ServiceNotification Notification { get; internal set; }
            }

            /// <summary>
            /// <para>Raised when a <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> previously registered through <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" /> is matched by a service's endpoint changes in system.</para>
            /// </summary>
            /// <remarks>
            /// The event argument is of type <see cref="System.Fabric.FabricClient.ServiceManagementClient.ServiceNotificationEventArgs" />.
            /// </remarks>
            /// <example>
            /// The following example shows how to register for and process service notifications:
            /// <code language="cs">
            /// namespace ServiceNotificationsExample
            /// {
            ///     class Program
            ///     {
            ///         static void Main(string[] args)
            ///         {
            ///             var client = new FabricClient(new string[] { "[cluster_endpoint]:[client_port]" });
            /// 
            ///             var filter = new ServiceNotificationFilterDescription()
            ///             {
            ///                 Name = new Uri("fabric:/my_application"),
            ///                 MatchNamePrefix = true,
            ///             };
            /// 
            ///             client.ServiceManager.ServiceNotificationFilterMatched += (s, e) => OnNotification(e);
            /// 
            ///             var filterId = client.ServiceManager.RegisterServiceNotificationFilterAsync(filter).Result;
            /// 
            ///             Console.WriteLine(
            ///                 "Registered filter: name={0} id={1}",
            ///                 filter.Name,
            ///                 filterId);
            /// 
            ///             Console.ReadLine();
            /// 
            ///             client.ServiceManager.UnregisterServiceNotificationFilterAsync(filterId).Wait();
            /// 
            ///             Console.WriteLine(
            ///                 "Unregistered filter: name={0} id={1}",
            ///                 filter.Name,
            ///                 filterId);
            ///         }
            /// 
            ///         private static void OnNotification(EventArgs e)
            ///         {
            ///             var castedEventArgs = (FabricClient.ServiceManagementClient.ServiceNotificationEventArgs)e;
            /// 
            ///             var notification = castedEventArgs.Notification;
            /// 
            ///             Console.WriteLine(
            ///                 "[{0}] received notification for service '{1}'",
            ///                 DateTime.UtcNow,
            ///                 notification.ServiceName);
            ///         }
            ///     }
            /// }
            /// </code>
            /// </example>
            public event EventHandler ServiceNotificationFilterMatched;

            internal void OnServiceNotificationFilterMatched(ServiceNotification notification)
            {
                var handler = ServiceNotificationFilterMatched;
                if (handler != null)
                {
                    handler(this, new ServiceNotificationEventArgs(notification));
                }
            }

            #region API

            /// <summary>
            /// <para>Instantiates a service with specified description.</para>
            /// </summary>
            /// <param name="description">
            /// <para>The configuration for the service. A <see cref="System.Fabric.Description.ServiceDescription" /> contains all of the information necessary to create a service.</para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="description"/> is null.</exception>
            /// <remarks>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// </remarks>
            public Task CreateServiceAsync(ServiceDescription description)
            {
                return this.CreateServiceAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }


            /// <summary>
            /// <para>Instantiates a service with specified description. 
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="serviceDescription">
            /// <para>The configuration for the service. A <see cref="System.Fabric.Description.ServiceDescription" /> contains all of the information necessary to create a service.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" />that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="serviceDescription"/> is null.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// </remarks>
            public Task CreateServiceAsync(ServiceDescription serviceDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ServiceDescription>("serviceDescription", serviceDescription).NotNull();
                ServiceDescription.Validate(serviceDescription);

                return this.CreateServiceAsyncHelper(serviceDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Instantiates a service from the template specified in the Application Manifest.</para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The Service Fabric Name of the application under which the service will be created.</para>
            /// </param>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <param name="serviceTypeName">
            /// <para>The name of the service type. This has to be same as the ServiceTypeName specified in the service manifest.</para>
            /// </param>
            /// <param name="initializationData">
            /// <para>The initialization data represents the custom data provided by the creator of the service. Service Fabric does not parse this data. 
            /// This data would be available in every instance or replica in <see cref="System.Fabric.StatefulServiceContext"/> or <see cref="System.Fabric.StatelessServiceContext"/>.            
            /// It cannot be changed after the service is created. 
            /// </para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTemplateNotFound" />: The service template does not exist</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="applicationName"/> or <paramref name="serviceName"/> are null.</exception>            
            /// <exception cref="ArgumentException">When <paramref name="serviceTypeName"/> is null or white-space.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            public Task CreateServiceFromTemplateAsync(Uri applicationName, Uri serviceName, string serviceTypeName, byte[] initializationData)
            {
                return this.CreateServiceFromTemplateAsync(
                    new ServiceFromTemplateDescription(
                        applicationName, 
                        serviceName,
                        string.Empty,
                        serviceTypeName, 
                        ServicePackageActivationMode.SharedProcess, 
                        initializationData));
            }

            /// <summary>
            /// <para>Instantiates a service from the template specified in the Application Manifest.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="applicationName">
            /// <para>The Service Fabric Name of the application under which the service will be created.</para>
            /// </param>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <param name="serviceTypeName">
            /// <para>The name of the service type. This has to be same as the ServiceTypeName specified in the service manifest.</para>
            /// </param>
            /// <param name="initializationData">
            /// <para>The initialization data represents the custom data provided by the creator of the service. Service Fabric does not parse this data. 
            /// This data would be available in every instance or replica in <see cref="System.Fabric.StatefulServiceContext"/> or <see cref="System.Fabric.StatelessServiceContext"/>.            
            /// It cannot be changed after the service is created. 
            /// </para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTemplateNotFound" />: The service template does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="applicationName"/> or <paramref name="serviceName"/> are null.</exception>
            /// <exception cref="ArgumentException">When <paramref name="serviceTypeName"/> is null or white-space.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// </remarks>
            public Task CreateServiceFromTemplateAsync(Uri applicationName, Uri serviceName, string serviceTypeName, byte[] initializationData, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.CreateServiceFromTemplateAsync(
                    new ServiceFromTemplateDescription(
                        applicationName,
                        serviceName,
                        string.Empty,
                        serviceTypeName,
                        ServicePackageActivationMode.SharedProcess,
                        initializationData),
                        timeout,
                        cancellationToken);
            }

            /// <summary>
            /// <para>Instantiates a service from the template specified in the Application Manifest.</para>
            /// </summary>
            /// <param name="serviceFromTemplateDescription">
            /// <para>Describes the Service to be created from service template specified in application manifest.</para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTemplateNotFound" />: The service template does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// </remarks>
            public Task CreateServiceFromTemplateAsync(ServiceFromTemplateDescription serviceFromTemplateDescription)
            {
                return this.CreateServiceFromTemplateAsync(
                    serviceFromTemplateDescription,
                    FabricClient.DefaultTimeout, 
                    CancellationToken.None);
            }

            /// <summary>
            /// <para>Instantiates a service from the template specified in the Application Manifest.</para>
            /// </summary>
            /// <param name="serviceFromTemplateDescription">
            /// <para>Describes a service to be created from service template specified in application manifest.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The instantiated service.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricElementNotFoundException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ServiceTemplateNotFound" />: The service template does not exist.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>The request timed out but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <exception cref="System.OperationCanceledException">
            /// <para>The request was canceled before the timeout expired but may have already been accepted for processing by the system.</para>
            /// </exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly created if it does not already exist.</para>
            /// </remarks>
            public Task CreateServiceFromTemplateAsync(
                ServiceFromTemplateDescription serviceFromTemplateDescription, 
                TimeSpan timeout, 
                CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();

                Requires.Argument("serviceFromTemplateDescription", serviceFromTemplateDescription).NotNull();

                return this.CreateServiceFromTemplateAsyncHelper(
                    serviceFromTemplateDescription, 
                    timeout, 
                    cancellationToken);
            }
            
            /// <summary>
            /// <para>Updates a service with the specified description.</para>
            /// </summary>
            /// <param name="name"><para>The URI name of the service being updated.</para></param>
            /// <param name="updateDescription"><para>The <see cref="System.Fabric.Description.ServiceUpdateDescription" /> that specifies the updated configuration for the service.</para></param>
            /// <returns><para>The updated service.</para></returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state.
            /// Dispose of the<see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="name"/> or <paramref name="updateDescription"/> are null.</exception>
            /// <remarks>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />. </para>
            /// <para>NOTE: To safely increase both the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.MinReplicaSetSize" /> and the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.TargetReplicaSetSize" /> first increase the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.TargetReplicaSetSize" /> and wait for additional replicas to be created and then increase the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.MinReplicaSetSize" /></para></remarks>
            public Task UpdateServiceAsync(Uri name, ServiceUpdateDescription updateDescription)
            {
                return this.UpdateServiceAsync(name, updateDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Updates a service with the specified description.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="name"><para>The URI name of the service being updated.</para></param>
            /// <param name="serviceUpdateDescription"><para>The <see cref="System.Fabric.Description.ServiceUpdateDescription" /> that specifies the updated configuration for the service.</para></param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns><para>The updated service.</para></returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state.
            /// Dispose of the<see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="name"/> or <paramref name="serviceUpdateDescription"/> are null.</exception>
            /// <remarks><para>NOTE: To safely increase both the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.MinReplicaSetSize" /> and the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.TargetReplicaSetSize" /> first increase the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.TargetReplicaSetSize" /> and wait for additional replicas to be created and then increase the <see cref="System.Fabric.Description.StatefulServiceUpdateDescription.MinReplicaSetSize" /></para></remarks>
            public Task UpdateServiceAsync(Uri name, ServiceUpdateDescription serviceUpdateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("name", name).NotNull();
                Requires.Argument<ServiceUpdateDescription>("serviceUpdateDescription", serviceUpdateDescription).NotNull();

                return this.UpdateServiceAsyncHelper(name, serviceUpdateDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the specified service instance.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric name of the service.</para>
            /// </param>
            /// <returns>
            /// <para>The deleted service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="serviceName"/> is null.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly and recursively deleted if the application is Service Fabric managed.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            [Obsolete("This API is deprecated, use overload taking DeleteServiceDescription instead.", false)]
            public Task DeleteServiceAsync(Uri serviceName)
            {
                return this.DeleteServiceAsync(new DeleteServiceDescription(serviceName), FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the specified service instance.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric name of the service.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The deleted service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="serviceName"/> is null.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly and recursively deleted if the application is Service Fabric managed.</para>
            /// </remarks>
            [Obsolete("This API is deprecated, use overload taking DeleteServiceDescription instead.", false)]
            public Task DeleteServiceAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceName", serviceName).NotNull();

                return this.DeleteServiceAsyncHelper(serviceName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Deletes the specified service instance.</para>
            /// </summary>
            /// <param name="deleteServiceDescription">
            /// <para>The description of the service to be deleted.</para>
            /// </param>
            /// <returns>
            /// <para>The deleted service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="deleteServiceDescription"/> is null.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly and recursively deleted if the application is Service Fabric managed.</para>
            /// <para>A forceful deletion call can convert on-going normal deletion to forceful one.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            public Task DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription)
            {
                return this.DeleteServiceAsync(deleteServiceDescription, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Deletes the specified service instance.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="deleteServiceDescription">
            /// <para>The description of the service to be deleted.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The deleted service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="deleteServiceDescription"/> is null.</exception>
            /// <remarks>
            /// <para>Service Fabric name will be implicitly and recursively deleted if the application is Service Fabric managed.</para>
            /// <para>A forceful deletion call can convert on-going normal deletion to forceful one.</para>
            /// </remarks>
            public Task DeleteServiceAsync(DeleteServiceDescription deleteServiceDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<DeleteServiceDescription>("deleteServiceDescription", deleteServiceDescription).NotNull();

                return this.DeleteServiceAsyncHelper2(deleteServiceDescription, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Gets the Service Description for the specified service instance.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Description for the specified service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="serviceName"/> is null.</exception>
            /// <remarks>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricClient.ServiceManagementClient.GetServiceDescriptionAsync(Uri)" /> is the most efficient way of determining whether a name is associated with a service.</para>
            /// </remarks>
            public Task<ServiceDescription> GetServiceDescriptionAsync(Uri serviceName)
            {
                return this.GetServiceDescriptionAsync(serviceName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the Service Description for the specified service instance.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service. </para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The Service Description for the specified service instance.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">When <paramref name="serviceName"/> is null.</exception>
            /// <remarks>
            /// <para>
            ///     <see cref="System.Fabric.FabricClient.ServiceManagementClient.GetServiceDescriptionAsync(Uri)" /> is the most efficient way of determining whether a name is associated with a service.</para>
            /// </remarks>
            public Task<ServiceDescription> GetServiceDescriptionAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceName", serviceName).NotNull();

                return this.GetServiceDescriptionAsyncHelper(serviceName, timeout, cancellationToken);
            }

            #region Resolve Service Overloads

            #region Stateless/Stateful Singleton service
            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>The <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(System.Uri)" /> will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, null, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>A complaint based resolution API.</para>
            /// <para>This method will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available. </para>
            /// <para>PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, ResolvedServicePartition previousResult)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, previousResult, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This method will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, ResolvedServicePartition previousResult, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, previousResult, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" />that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This method will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>
            ///     <paramref name="previousResult" /> argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the <paramref name="previousResult" /> argument or <paramref name="previousResult" /> argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, null, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, previousResult, timeout, cancellationToken);
            }

            #endregion

            #region Stateless/Stateful Int64 service
            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This method will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition. </para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, null, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available. </para>
            /// <para>PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, ResolvedServicePartition previousResult)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, previousResult, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, previousResult, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>Previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>
            ///     <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, long partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, previousResult, timeout, cancellationToken);
            }

            #endregion

            #region Stateless/Stateful string service
            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, null, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, null, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will always return the closest <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, null, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>The PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, ResolvedServicePartition previousResult)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, previousResult, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>The PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, previousResult, timeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Queries the system for the set of endpoints the specified service partition is listening to.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service instance.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="previousResult">
            /// <para>The previous <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition that the user believes is stale.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The set of endpoints the specified service partition is listening to.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>This is a complaint based resolution API.</para>
            /// <para>This will return a <see cref="System.Fabric.ResolvedServicePartition" /> for the specified service partition. When this overload is used, the system will return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> than the “previousResult” argument if it is available.</para>
            /// <para>The PreviousResult argument enables the user to say ”This is the previous list of endpoints for this Service partition. I have tried the endpoints and I believe they are stale. Return me a more up to date version of this set.” In this case, the system will try to return a more up-to-date <see cref="System.Fabric.ResolvedServicePartition" /> in the most efficient way possible. If no newer version can be found, a <see cref="System.Fabric.ResolvedServicePartition" /> with the same version will be returned.ResolveServicePartition can be called without the previousResult argument or previousResult argument set to null. As no prerequisite is specified, the system will return the closest copy of the <see cref="System.Fabric.ResolvedServicePartition" /> for the service partition.</para>
            /// </remarks>
            public Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, string partitionKey, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.ResolveServicePartitionAsync(serviceName, partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, previousResult, timeout, cancellationToken);
            }

            #endregion

            #endregion

            /// <summary>
            /// <para>This API is deprecated, use <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" /> instead.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <param name="callback">
            /// <para>The function that will be called when a notification arrives.</para>
            /// </param>
            /// <returns>
            /// <para>The handler to be raised when the accessibility information of a service partition changes.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">If <paramref name="serviceName"/> or <paramref name="callback"/> are null.</exception>
            /// <remarks>
            /// <para>Notification will include changes on partition’s endpoints or exceptions that occurred while the information was being updated. This overload is used for Singleton partitioned service instances. Returned Int64 is the callback handle identifier for the registration.</para>
            /// <para>Notifications is a mechanism that delivers notifications to user’s code each time there is an service address change or an address resolution error related to a service partition the code has raised interest for. This way instead of resolving every time the current <see cref="System.Fabric.ResolvedServicePartition" /> becomes stale, program registers for updates.</para>
            /// </remarks>
            [ObsoleteAttribute("This API is obsolete. Use RegisterServiceNotificationFilterAsync instead.", false)]
            public long RegisterServicePartitionResolutionChangeHandler(Uri serviceName, ServicePartitionResolutionChangeHandler callback)
            {
                return this.RegisterServicePartitionResolutionChangeHandler(serviceName, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_NONE, null, callback);
            }

            /// <summary>
            /// <para>This API is deprecated, use <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" /> instead.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="callback">
            /// <para>The function that will be called when a notification arrives.</para>
            /// </param>
            /// <returns>
            /// <para>The handler to be raised when the accessibility information of a service partition changes.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">If <paramref name="serviceName"/> or <paramref name="callback"/> are null.</exception>
            /// <remarks>
            /// <para>Notification will include changes on partition’s endpoints or exceptions that occurred while the information was being updated. This overload is used for UniformInt64Range partitioned service instances. Returned Int64 is the callback handle identifier for the registration.</para>
            /// <para>Notifications is a mechanism that delivers notifications to user’s code each time there is an service address change or an address resolution error related to a service partition the code has raised interest for. This way instead of resolving every time the current <see cref="System.Fabric.ResolvedServicePartition" /> becomes stale, program registers for updates.</para>
            /// </remarks>
            [ObsoleteAttribute("This API is obsolete. Use RegisterServiceNotificationFilterAsync instead.", false)]
            public long RegisterServicePartitionResolutionChangeHandler(Uri serviceName, long partitionKey, ServicePartitionResolutionChangeHandler callback)
            {
                return this.RegisterServicePartitionResolutionChangeHandler(serviceName, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_INT64, partitionKey, callback);
            }

            /// <summary>
            /// <para>This API is deprecated, use <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" /> instead.</para>
            /// </summary>
            /// <param name="serviceName">
            /// <para>The Service Fabric Name of the service.</para>
            /// </param>
            /// <param name="partitionKey">
            /// <para>The partition key for the service partition.</para>
            /// </param>
            /// <param name="callback">
            /// <para>The function that will be called when a notification arrives.</para>
            /// </param>
            /// <returns>
            /// <para>The handler to be raised when the accessibility information of a service partition changes.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentNullException">If <paramref name="serviceName"/> or <paramref name="callback"/> are null.</exception>
            /// <exception cref="ArgumentException">If <paramref name="partitionKey"/> is null or empty.</exception>            
            /// <remarks>
            /// <para>Notification will include changes on partition’s endpoints or exceptions that occurred while the information was being updated. This overload is used for Named partitioned service instances. Returned Int64 is the callback handle identifier for the registration.</para>
            /// <para>Notifications is a mechanism that delivers notifications to user’s code each time there is an service address change or an address resolution error related to a service partition the code has raised interest for. This way instead of resolving every time the current <see cref="System.Fabric.ResolvedServicePartition" /> becomes stale, program registers for updates.</para>
            /// </remarks>
            [ObsoleteAttribute("This API is obsolete. Use RegisterServiceNotificationFilterAsync instead.", false)]
            public long RegisterServicePartitionResolutionChangeHandler(Uri serviceName, string partitionKey, ServicePartitionResolutionChangeHandler callback)
            {
                Requires.Argument<string>("partitionKey", partitionKey).NotNullOrEmpty();

                return this.RegisterServicePartitionResolutionChangeHandler(serviceName, NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING, partitionKey, callback);
            }

            /// <summary>
            /// <para>Unregisters a change handler previously registered with <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServicePartitionResolutionChangeHandler(System.Uri,System.Fabric.ServicePartitionResolutionChangeHandler)" />.</para>
            /// </summary>
            /// <param name="callbackHandle">
            /// <para>The callbackHandle identifier that will be removed. This is returned by the <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServicePartitionResolutionChangeHandler(System.Uri,System.Fabric.ServicePartitionResolutionChangeHandler)" /> call.</para>
            /// </param>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            public void UnregisterServicePartitionResolutionChangeHandler(long callbackHandle)
            {
                this.fabricClient.ThrowIfDisposed();
                Utility.WrapNativeSyncInvokeInMTA(() => this.UnregisterServicePartitionResolutionChangeHandlerHelper(callbackHandle), "FabricClient.UnregisterServicePartitionResolutionChangeHandler");
            }

            /// <summary>
            /// <para>Gets the provisioned service manifest document in the specified application type name and application type version.</para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The name of the provisioned application manifest.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The version of the provisioned application manifest.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest referenced in the application manifest.</para>
            /// </param>
            /// <returns>
            /// <para>The provisioned service manifest document.</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <remarks>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            public Task<string> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName)
            {
                return this.GetServiceManifestAsync(applicationTypeName, applicationTypeVersion, serviceManifestName, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Gets the provisioned service manifest document in the specified application type name and application type version.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="applicationTypeName">
            /// <para>The name of the provisioned application manifest.</para>
            /// </param>
            /// <param name="applicationTypeVersion">
            /// <para>The version of the provisioned application manifest.</para>
            /// </param>
            /// <param name="serviceManifestName">
            /// <para>The name of the service manifest referenced in the application manifest.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum amount of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The <see cref="System.Threading.CancellationToken" /> that the operation is observing. It can be used to propagate notification that the operation should be canceled.</para>
            /// </param>
            /// <returns>
            /// <para>The provisioned service manifest document</para>
            /// </returns>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>The <see cref="System.Fabric.FabricClient" /> object is in a closed state. Dispose of the <see cref="System.Fabric.FabricClient" /> object you are using and instantiate a new <see cref="System.Fabric.FabricClient" /> object.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="applicationTypeName"/> or <paramref name="applicationTypeVersion"/> or <paramref name="serviceManifestName"/> are null/empty.</exception>            
            /// <remarks>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            public Task<string> GetServiceManifestAsync(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("applicationTypeName", applicationTypeName).NotNullOrWhiteSpace();
                Requires.Argument<string>("applicationTypeVersion", applicationTypeVersion).NotNullOrWhiteSpace();
                Requires.Argument<string>("serviceManifestName", serviceManifestName).NotNullOrWhiteSpace();

                return this.GetServiceManifestAsyncHelper(applicationTypeName, applicationTypeVersion, serviceManifestName, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Removes a service replica running on a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>This API gives a running replica the chance to cleanup its state and be gracefully shutdown. </para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to data loss for stateful services.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>            
            public Task RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId)
            {
                return this.RemoveReplicaAsync(nodeName, partitionId, replicaOrInstanceId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Removes a service replica running on a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <param name="forceRemove">
            /// <para>Specifies whether the replica should be given a chance to gracefully clean up its state and close</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>This API gives a running replica the chance to cleanup its state and be gracefully shutdown. </para>
            /// <para>If the forceRemove flag is set then no such opportunity is given. Service Fabric will terminate the host for that replica and any persisted state of that replica will be leaked. </para>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to data loss for stateful services.</para>
            /// <para>In addition, the forceRemove flag impacts all other replicas hosted in the same process.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>
            public Task RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove)
            {
                return this.RemoveReplicaAsync(nodeName, partitionId, replicaOrInstanceId, forceRemove, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Removes a service replica running on a node.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>This API gives a running replica the chance to cleanup its state and be gracefully shutdown. </para>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to data loss for stateful services.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>
            public Task RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return this.RemoveReplicaAsync(nodeName, partitionId, replicaOrInstanceId, false, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Removes a service replica running on a node.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <param name="forceRemove">
            /// <para>Specifies whether the replica should be given a chance to gracefully clean up its state and close</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>This API gives a running replica the chance to cleanup its state and be gracefully shutdown. </para>
            /// <para>If the forceRemove flag is set then no such opportunity is given. Service Fabric will terminate the host for that replica and any persisted state of that replica will be leaked. </para>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to data loss for stateful services.</para>
            /// <para>In addition, the forceRemove flag impacts all other replicas hosted in the same process.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>
            public Task RemoveReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrWhiteSpace();

                return this.RemoveReplicaAsyncHelper(nodeName, partitionId, replicaOrInstanceId, forceRemove, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Restarts a service replica of a persisted service running on a node.</para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to availability loss for stateful services.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaOperation"/> is returned if the replica does not belong to a stateful persisted service. Only stateful persisted replicas can be restarted.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>
            public Task RestartReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId)
            {
                return this.RestartReplicaAsync(nodeName, partitionId, replicaOrInstanceId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Restarts a service replica of a persisted service running on a node.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="nodeName">
            /// <para>The name of the node.</para>
            /// </param>
            /// <param name="partitionId">
            /// <para>The partition identifier.</para>
            /// </param>
            /// <param name="replicaOrInstanceId">
            /// <para>The instance identifier.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The timespan that defines the maximum amount of time  will allow this operation to continue before returning a <see cref="System.TimeoutException" />.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>The optional cancellation token that the operation is observing. It can be used to send a notification that the operation should be canceled. Note that cancellation is advisory and that the operation may still be completed even if it is cancelled.</para>
            /// </param>
            /// <returns>
            /// <para>A Task representing the acknowledgment of the request.</para>
            /// </returns>
            /// <remarks>
            /// <para>WARNING: There are no safety checks performed when this API is used. Incorrect use of this API can lead to availability loss for stateful services.</para>
            /// </remarks>
            /// <exception cref="System.Fabric.FabricObjectClosedException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.TimeoutException">
            /// <para>
            ///     See <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="System.Fabric.FabricException">
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.ReplicaDoesNotExist"/> is returned if the replica or instance id is not running on the node.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaStateForReplicaOperation"/> is returned if the replica or instance id cannot be restarted or removed at this time as it is in an invalid state. For example, the replica is already in the process of being closed.</para>
            /// <para>
            ///     <see cref="System.Fabric.FabricErrorCode.InvalidReplicaOperation"/> is returned if the replica does not belong to a stateful persisted service. Only stateful persisted replicas can be restarted.</para>
            /// <para>
            ///     See also <see href="https://docs.microsoft.com/en-us/azure/service-fabric/service-fabric-errors-and-exceptions"/> for handling common FabricClient failures.</para>
            /// </exception>
            /// <exception cref="ArgumentException">If <paramref name="nodeName"/> is null or empty.</exception>
            public Task RestartReplicaAsync(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<string>("nodeName", nodeName).NotNullOrWhiteSpace();

                return this.RestartReplicaAsyncHelper(nodeName, partitionId, replicaOrInstanceId, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Registers a <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" />.</para>
            /// </summary>
            /// <param name="description">
            /// <para>The description that determines which service endpoint change events should be delivered to this client through the <see cref="System.Fabric.FabricClient.ServiceManagementClient.ServiceNotificationFilterMatched" /> event.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the async operation. The task result is an ID corresponding to the registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> that can be used to unregister the same filter through <see cref="System.Fabric.FabricClient.ServiceManagementClient.UnregisterServiceNotificationFilterAsync(Int64)" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            /// The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.
            /// </para>
            /// <para>
            /// There is a cache of service endpoints in the client that gets updated by notifications and this same cache is used to satisfy complaint based resolution requests (see <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(Uri, ResolvedServicePartition)"/>). Applications that both register for notifications and use complaint based resolution on the same client instance typically only need to pass null for the <see cref="System.Fabric.ResolvedServicePartition" /> argument during resolution. This will always return the endpoints in the client cache updated by the latest notification. The notification mechanism itself will keep the client cache updated when service endpoints change, there is no need to convert from a <see cref="System.Fabric.ServiceNotification" /> to a <see cref="System.Fabric.ResolvedServicePartition" /> for the purposes of refreshing the client cache.
            /// </para>
            /// </remarks>
            /// <exception cref="ArgumentNullException">If <paramref name="description"/> is null.</exception>
            public Task<Int64> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription description)
            {
                return this.RegisterServiceNotificationFilterAsync(description, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Registers a <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" />.
            /// Also takes in timeout interval, which is the maximum of time the system will allow this operation to continue before returning <see cref="System.TimeoutException" /> and cancellation-token that the operation is observing. 
            /// </para>
            /// </summary>
            /// <param name="description">
            /// <para>The description that determines which service endpoint change events should be delivered to this client through the <see cref="System.Fabric.FabricClient.ServiceManagementClient.ServiceNotificationFilterMatched" /> event.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum time allowed for processing the request before <see cref="System.TimeoutException" /> is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Reserved for future use.</para>
            /// </param>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the async operation. The task result is an ID corresponding to the registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> that can be used to unregister the same filter through <see cref="System.Fabric.FabricClient.ServiceManagementClient.UnregisterServiceNotificationFilterAsync(Int64)" />.</para>
            /// </returns>
            /// <remarks>
            /// <para>
            /// There is a cache of service endpoints in the client that gets updated by notifications and this same cache is used to satisfy complaint based resolution requests (see <see cref="System.Fabric.FabricClient.ServiceManagementClient.ResolveServicePartitionAsync(Uri, ResolvedServicePartition)"/>). Applications that both register for notifications and use complaint based resolution on the same client instance typically only need to pass null for the <see cref="System.Fabric.ResolvedServicePartition" /> argument during resolution. This will always return the endpoints in the client cache updated by the latest notification. The notification mechanism itself will keep the client cache updated when service endpoints change, there is no need to convert from a <see cref="System.Fabric.ServiceNotification" /> to a <see cref="System.Fabric.ResolvedServicePartition" /> for the purposes of refreshing the client cache.
            /// </para>
            /// </remarks>
            /// <exception cref="ArgumentNullException">If <paramref name="description"/> is null.</exception>
            public Task<Int64> RegisterServiceNotificationFilterAsync(ServiceNotificationFilterDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<ServiceNotificationFilterDescription>("description", description).NotNull();

                return this.RegisterServiceNotificationFilterAsyncHelper(description, timeout, cancellationToken);
            }

            /// <summary>
            /// <para>Unregisters a previously registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" />.</para>
            /// </summary>
            /// <param name="filterId">
            /// <para>The ID of a previously registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> returned from <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" />.</para>
            /// </param>
            /// <remarks>
            /// <para>It's not necessary to unregister individual filters if the client itself will no longer be used since all <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> objects registered by the <see cref="System.Fabric.FabricClient" /> will be automatically unregistered when client is disposed.</para>
            /// <para>The default timeout is one minute for which the system will allow this operation to continue before returning <see cref="System.TimeoutException" />.</para>
            /// </remarks>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the async operation.</para>
            /// </returns>
            public Task UnregisterServiceNotificationFilterAsync(Int64 filterId)
            {
                return this.UnregisterServiceNotificationFilterAsync(filterId, FabricClient.DefaultTimeout, CancellationToken.None);
            }

            /// <summary>
            /// <para>Unregisters a previously registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" />.</para>
            /// </summary>
            /// <param name="filterId">
            /// <para>The ID of a previously registered <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> returned from <see cref="System.Fabric.FabricClient.ServiceManagementClient.RegisterServiceNotificationFilterAsync(System.Fabric.Description.ServiceNotificationFilterDescription)" />.</para>
            /// </param>
            /// <param name="timeout">
            /// <para>The maximum time allowed for processing the request before <see cref="System.TimeoutException" /> is thrown.</para>
            /// </param>
            /// <param name="cancellationToken">
            /// <para>Reserved for future use.</para>
            /// </param>
            /// <remarks>
            /// It's not necessary to unregister individual filters if the client itself will no longer be used since all <see cref="System.Fabric.Description.ServiceNotificationFilterDescription" /> objects registered by the <see cref="System.Fabric.FabricClient" /> will be automatically unregistered when client is disposed.
            /// </remarks>
            /// <returns>
            /// <para>A <see cref="System.Threading.Tasks.Task" /> representing the async operation.</para>
            /// </returns>
            public Task UnregisterServiceNotificationFilterAsync(Int64 filterId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                return this.UnregisterServiceNotificationFilterAsyncHelper(filterId, timeout, cancellationToken);
            }


            #endregion

            #region Helpers & Callbacks

            #region Create Service Async

            private Task CreateServiceAsyncHelper(ServiceDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateServiceBeginWrapper(description, timeout, callback),
                    this.CreateServiceEndWrapper,
                    cancellationToken,
                    "ServiceManager.CreateService");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateServiceBeginWrapper(ServiceDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginCreateService(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateServiceEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndCreateService(context);
            }

            #endregion

            #region Create Service From Template Async

            private Task CreateServiceFromTemplateAsyncHelper(
                ServiceFromTemplateDescription serviceFromTemplateDescription, 
                TimeSpan timeout, 
                CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.CreateServiceFromTemplateBeginWrapper(
                        serviceFromTemplateDescription, 
                        timeout, 
                        callback),
                    this.CreateServiceFromTemplateEndWrapper,
                    cancellationToken,
                    "ApplicationManager.CreateServiceFromTemplate");
            }

            private NativeCommon.IFabricAsyncOperationContext CreateServiceFromTemplateBeginWrapper(
                ServiceFromTemplateDescription serviceFromTemplateDescription, 
                TimeSpan timeout, 
                NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginCreateServiceFromTemplate2(
                        serviceFromTemplateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void CreateServiceFromTemplateEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndCreateServiceFromTemplate(context);
            }

            #endregion

            #region Update Service Async

            private Task UpdateServiceAsyncHelper(Uri name, ServiceUpdateDescription updateDescription, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UpdateServiceBeginWrapper(name, updateDescription, timeout, callback),
                    this.UpdateServiceEndWrapper,
                    cancellationToken,
                    "ServiceManager.UpdateService");
            }

            private NativeCommon.IFabricAsyncOperationContext UpdateServiceBeginWrapper(Uri name, ServiceUpdateDescription updateDescription, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginUpdateService(
                        pin.AddObject(name),
                        updateDescription.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void UpdateServiceEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndUpdateService(context);
            }

            #endregion

            #region Delete Service Async

            private Task DeleteServiceAsyncHelper(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteServiceBeginWrapper(serviceName, timeout, callback),
                    this.DeleteServiceEndWrapper,
                    cancellationToken,
                    "ServiceManager.DeleteService");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteServiceBeginWrapper(Uri serviceName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(serviceName))
                {
                    return this.nativeServiceClient.BeginDeleteService(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void DeleteServiceEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndDeleteService(context);
            }

            #endregion

            #region Forcefully Delete Service Async

            private Task DeleteServiceAsyncHelper2(DeleteServiceDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.DeleteServiceBeginWrapper2(description, timeout, callback),
                    this.DeleteServiceEndWrapper2,
                    cancellationToken,
                    "ServiceManager.DeleteService2");
            }

            private NativeCommon.IFabricAsyncOperationContext DeleteServiceBeginWrapper2(DeleteServiceDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                 using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginDeleteService2(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
             }

            private void DeleteServiceEndWrapper2(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndDeleteService2(context);
            }

            #endregion

            #region Get Service Description Async

            private Task<ServiceDescription> GetServiceDescriptionAsyncHelper(Uri serviceName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ServiceDescription>(
                    (callback) => this.GetServiceDescriptionBeginWrapper(serviceName, timeout, callback),
                    this.GetServiceDescriptionEndWrapper,
                    cancellationToken,
                    "ServiceManager.GetServiceDescription");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceDescriptionBeginWrapper(Uri serviceName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pinName = PinBlittable.Create(serviceName))
                {
                    return this.nativeServiceClient.BeginGetServiceDescription(
                        pinName.AddrOfPinnedObject(),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ServiceDescription GetServiceDescriptionEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return InteropHelpers.ServiceDescriptionHelper.CreateFromNative(this.nativeServiceClient.EndGetServiceDescription(context));
            }

            #endregion

            #region Resolve Service Partition Async

            private Task<ResolvedServicePartition> ResolveServicePartitionAsync(Uri serviceName, object partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceName", serviceName).NotNull();

                if (keyType == NativeTypes.FABRIC_PARTITION_KEY_TYPE.FABRIC_PARTITION_KEY_TYPE_STRING)
                {
                    Requires.Argument<string>("partitionKey", (string)partitionKey).NotNull();
                }

                return this.ResolveServicePartitionAsyncHelper(serviceName, partitionKey, keyType, previousResult, timeout, cancellationToken);
            }

            private Task<ResolvedServicePartition> ResolveServicePartitionAsyncHelper(Uri serviceName, object partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType, ResolvedServicePartition previousResult, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<ResolvedServicePartition>(
                    (callback) => this.ResolveServicePartitionBeginWrapper(serviceName, partitionKey, keyType, previousResult, timeout, callback),
                    this.ResolveServicePartitionEndWrapper,
                    cancellationToken,
                    "ApplicationManager.ResolveService");
            }

            private NativeCommon.IFabricAsyncOperationContext ResolveServicePartitionBeginWrapper(Uri serviceName, object partitionKey, NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType, ResolvedServicePartition previousResult, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                partitionKey = FabricClient.NormalizePartitionKey(partitionKey);

                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginResolveServicePartition(
                        pin.AddObject(serviceName),
                        keyType,
                        pin.AddBlittable(partitionKey),
                        (previousResult == null) ? null : previousResult.NativeResult,
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private ResolvedServicePartition ResolveServicePartitionEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return ResolvedServicePartition.FromNative(this.nativeServiceClient.EndResolveServicePartition(context));
            }

            #endregion

            #region ServicePartitionResolutionChangeHandler
            private long RegisterServicePartitionResolutionChangeHandler(Uri serviceName, NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType, object partitionKey, ServicePartitionResolutionChangeHandler callback)
            {
                this.fabricClient.ThrowIfDisposed();
                Requires.Argument<Uri>("serviceName", serviceName).NotNull();
                Requires.Argument<ServicePartitionResolutionChangeHandler>("callback", callback).NotNull();

                return Utility.WrapNativeSyncInvokeInMTA(() => this.RegisterServicePartitionResolutionChangeHandlerHelper(serviceName, keyType, partitionKey, callback), "FabricClient.RegisterServicePartitionResolutionChangeHandler");
            }

            private long RegisterServicePartitionResolutionChangeHandlerHelper(Uri serviceName, NativeTypes.FABRIC_PARTITION_KEY_TYPE keyType, object partitionKey, ServicePartitionResolutionChangeHandler callback)
            {
                partitionKey = FabricClient.NormalizePartitionKey(partitionKey);

                using (var pin = new PinCollection())
                {
                    NativeClient.IFabricServicePartitionResolutionChangeHandler handler = InteropHelpers.ComFabricServicePartitionResolutionChangeHandler.ToNative(this.fabricClient, callback);
                    long callbackHandle = this.nativeServiceClient.RegisterServicePartitionResolutionChangeHandler(
                        pin.AddObject(serviceName),
                        keyType,
                        pin.AddBlittable(partitionKey),
                        handler);

                    return callbackHandle;
                }
            }

            private void UnregisterServicePartitionResolutionChangeHandlerHelper(long callbackHandle)
            {
                this.nativeServiceClient.UnregisterServicePartitionResolutionChangeHandler(callbackHandle);
            }

            #endregion

            #region Get Service manifest Async

            private Task<string> GetServiceManifestAsyncHelper(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<string>(
                    (callback) => this.GetServiceManifestAsyncBeginWrapper(applicationTypeName, applicationTypeVersion, serviceManifestName, timeout, callback),
                    this.GetServiceManifestAsyncEndWrapper,
                    cancellationToken,
                    "ServiceManager.GetServiceManifest");
            }

            private NativeCommon.IFabricAsyncOperationContext GetServiceManifestAsyncBeginWrapper(string applicationTypeName, string applicationTypeVersion, string serviceManifestName, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginGetServiceManifest(
                        pin.AddBlittable(applicationTypeName),
                        pin.AddBlittable(applicationTypeVersion),
                        pin.AddBlittable(serviceManifestName),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private string GetServiceManifestAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return NativeTypes.FromNativeString(this.nativeServiceClient.EndGetServiceManifest(context));
            }

            #endregion

            #region Remove Replica Async

            private Task RemoveReplicaAsyncHelper(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RemoveReplicaAsyncBeginWrapper(nodeName, partitionId, replicaOrInstanceId, forceRemove, timeout, callback),
                    this.RemoveReplicaAsyncEndWrapper,
                    cancellationToken,
                    "ServiceManager.RemoveReplica");
            }

            private NativeCommon.IFabricAsyncOperationContext RemoveReplicaAsyncBeginWrapper(string nodeName, Guid partitionId, long replicaOrInstanceId, bool forceRemove, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var desc = new NativeTypes.FABRIC_REMOVE_REPLICA_DESCRIPTION();
                    desc.NodeName = pin.AddBlittable(nodeName);
                    desc.PartitionId = partitionId;
                    desc.ReplicaOrInstanceId = replicaOrInstanceId;

                    var reserved = new NativeTypes.FABRIC_REMOVE_REPLICA_DESCRIPTION_EX1();
                    reserved.ForceRemove = NativeTypes.ToBOOLEAN(forceRemove);
                    reserved.Reserved = IntPtr.Zero;

                    desc.Reserved = pin.AddBlittable(reserved);

                    return this.nativeServiceClient.BeginRemoveReplica(
                        pin.AddBlittable(desc),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RemoveReplicaAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndRemoveReplica(context);
            }

            #endregion

            #region Restart Replica Async

            private Task RestartReplicaAsyncHelper(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.RestartReplicaAsyncBeginWrapper(nodeName, partitionId, replicaOrInstanceId, timeout, callback),
                    this.RestartReplicaAsyncEndWrapper,
                    cancellationToken,
                    "ServiceManager.RestartReplica");
            }

            private NativeCommon.IFabricAsyncOperationContext RestartReplicaAsyncBeginWrapper(string nodeName, Guid partitionId, long replicaOrInstanceId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    var desc = new NativeTypes.FABRIC_RESTART_REPLICA_DESCRIPTION();
                    desc.NodeName = pin.AddBlittable(nodeName);
                    desc.PartitionId = partitionId;
                    desc.ReplicaOrInstanceId = replicaOrInstanceId;

                    return this.nativeServiceClient.BeginRestartReplica(
                        pin.AddBlittable(desc),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private void RestartReplicaAsyncEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndRestartReplica(context);
            }

            #endregion

            #region Register/Unregister Service Notification Filter

            private Task<Int64> RegisterServiceNotificationFilterAsyncHelper(ServiceNotificationFilterDescription description, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA<Int64>(
                    (callback) => this.RegisterServiceNotificationFilterBeginWrapper(description, timeout, callback),
                    this.RegisterServiceNotificationFilterEndWrapper,
                    cancellationToken,
                    "ServiceManager.RegisterServiceNotificationFilter");
            }

            private NativeCommon.IFabricAsyncOperationContext RegisterServiceNotificationFilterBeginWrapper(ServiceNotificationFilterDescription description, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                using (var pin = new PinCollection())
                {
                    return this.nativeServiceClient.BeginRegisterServiceNotificationFilter(
                        description.ToNative(pin),
                        Utility.ToMilliseconds(timeout, "timeout"),
                        callback);
                }
            }

            private Int64 RegisterServiceNotificationFilterEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                return this.nativeServiceClient.EndRegisterServiceNotificationFilter(context);
            }

            private Task UnregisterServiceNotificationFilterAsyncHelper(Int64 filterId, TimeSpan timeout, CancellationToken cancellationToken)
            {
                return Utility.WrapNativeAsyncInvokeInMTA(
                    (callback) => this.UnregisterServiceNotificationFilterBeginWrapper(filterId, timeout, callback),
                    this.UnregisterServiceNotificationFilterEndWrapper,
                    cancellationToken,
                    "ServiceManager.UnregisterServiceNotificationFilter");
            }

            private NativeCommon.IFabricAsyncOperationContext UnregisterServiceNotificationFilterBeginWrapper(Int64 filterId, TimeSpan timeout, NativeCommon.IFabricAsyncOperationCallback callback)
            {
                return this.nativeServiceClient.BeginUnregisterServiceNotificationFilter(
                    filterId,
                    Utility.ToMilliseconds(timeout, "timeout"),
                    callback);
            }

            private void UnregisterServiceNotificationFilterEndWrapper(NativeCommon.IFabricAsyncOperationContext context)
            {
                this.nativeServiceClient.EndUnregisterServiceNotificationFilter(context);
            }

            #endregion

            #endregion
        }
    }
}