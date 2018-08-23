// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;
    using System.Runtime.InteropServices;

    /// <summary>
    /// <para>Represents the activation which contains information about a running code package in a Service Fabric application.</para>
    /// <para>The <see cref="System.Fabric.FabricRuntime.GetActivationContext"/> and <see cref="System.Fabric.FabricRuntime.GetActivationContextAsync"/> methods can be used to get an instance of the activation context.</para>
    /// </summary>
    public class CodePackageActivationContext : ICodePackageActivationContext3
    {
        private readonly NativeRuntime.IFabricCodePackageActivationContext6 nativeActivationContext;
        private long codePackageChangeHandlerId;
        private long configPackageChangeHandlerId;
        private long dataPackageChangeHandlerId;

        private volatile bool disposed;

        internal CodePackageActivationContext(NativeRuntime.IFabricCodePackageActivationContext6 nativeActivationContext)
        {
            Requires.Argument("nativeActivationContext", nativeActivationContext).NotNull();

            var notificationBroker = new PackageNotificationBroker
            {
                OwnerWref = new WeakReference(this)
            };

            this.nativeActivationContext = nativeActivationContext;
            this.codePackageChangeHandlerId = this.nativeActivationContext.RegisterCodePackageChangeHandler(notificationBroker);
            this.configPackageChangeHandlerId = this.nativeActivationContext.RegisterConfigurationPackageChangeHandler(notificationBroker);
            this.dataPackageChangeHandlerId = this.nativeActivationContext.RegisterDataPackageChangeHandler(notificationBroker);
        }

        /// <summary>
        /// <para>
        /// Performs cleanup operations on unmanaged resources held by the current object before the object is destroyed.
        /// </para>
        /// </summary>
        ~CodePackageActivationContext()
        {
            this.Dispose(false);
        }

        // older obsolete events that return description instead of the package object

        /// <summary>
        /// Raised when a new code package is added to the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="CodePackageAddedEvent"/> instead.</remarks>
        [Obsolete("Use CodePackageAddedEvent event.")]
        public event EventHandler<PackageAddedEventArgs<CodePackageDescription>> CodePackageAdded;

        /// <summary>
        /// Raised when the code package is removed from the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="CodePackageRemovedEvent"/> instead.</remarks>
        [Obsolete("Use CodePackageRemovedEvent event.")]
        public event EventHandler<PackageRemovedEventArgs<CodePackageDescription>> CodePackageRemoved;

        /// <summary>
        /// Raised when an existing code package is modified in the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="CodePackageModifiedEvent"/> instead.</remarks>
        [Obsolete("Use CodePackageModifiedEvent event.")]
        public event EventHandler<PackageModifiedEventArgs<CodePackageDescription>> CodePackageModified;

        /// <summary>
        /// Raised when a new configuration package is added to the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="ConfigurationPackageAddedEvent"/> instead.</remarks>
        [Obsolete("Use ConfigurationPackageAddedEvent event.")]
        public event EventHandler<PackageAddedEventArgs<ConfigurationPackageDescription>> ConfigurationPackageAdded;

        /// <summary>
        /// Raised when a configuration package is removed from the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="ConfigurationPackageRemovedEvent"/> instead.</remarks>
        [Obsolete("Use ConfigurationPackageRemovedEvent event.")]
        public event EventHandler<PackageRemovedEventArgs<ConfigurationPackageDescription>> ConfigurationPackageRemoved;

        /// <summary>
        /// Raised when a configuration package is modified in the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="ConfigurationPackageModifiedEvent"/> instead.</remarks>
        [Obsolete("Use ConfigurationPackageModifiedEvent event.")]
        public event EventHandler<PackageModifiedEventArgs<ConfigurationPackageDescription>> ConfigurationPackageModified;

        /// <summary>
        /// Raised when a data package is added to the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="DataPackageAddedEvent"/> instead.</remarks>
        [Obsolete("Use DataPackageAddedEvent event.")]
        public event EventHandler<PackageAddedEventArgs<DataPackageDescription>> DataPackageAdded;

        /// <summary>
        /// Raised when a data package is removed from the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="DataPackageRemovedEvent"/> instead.</remarks>
        [Obsolete("Use DataPackageRemovedEvent event.")]
        public event EventHandler<PackageRemovedEventArgs<DataPackageDescription>> DataPackageRemoved;

        /// <summary>
        /// Raised when a data package is modified in the service manifest.
        /// </summary>
        /// <remarks>Obsolete. Use <see cref="DataPackageModifiedEvent"/> instead.</remarks>
        [Obsolete("Use DataPackageModifiedEvent event.")]
        public event EventHandler<PackageModifiedEventArgs<DataPackageDescription>> DataPackageModified;

        // new change events, that return the package object

        /// <summary>
        /// Raised during an application upgrade when a new code package is added to the service manifest.
        /// </summary>
        public event EventHandler<PackageAddedEventArgs<CodePackage>> CodePackageAddedEvent;

        /// <summary>
        /// Raised during an application upgrade when a code package is removed from the service manifest.
        /// </summary>
        public event EventHandler<PackageRemovedEventArgs<CodePackage>> CodePackageRemovedEvent;

        /// <summary>
        /// Raised during an application upgrade when an existing code package is modified in the service manifest.
        /// </summary>
        public event EventHandler<PackageModifiedEventArgs<CodePackage>> CodePackageModifiedEvent;

        /// <summary>
        /// Raised during an application upgrade when a new configuration package is added to the service manifest.
        /// </summary>
        public event EventHandler<PackageAddedEventArgs<ConfigurationPackage>> ConfigurationPackageAddedEvent;

        /// <summary>
        /// Raised during an application upgrade when a configuration package is removed from the service manifest.
        /// </summary>
        public event EventHandler<PackageRemovedEventArgs<ConfigurationPackage>> ConfigurationPackageRemovedEvent;

        /// <summary>
        /// Raised during an application upgrade when a configuration package is modified in the service manifest.
        /// </summary>
        public event EventHandler<PackageModifiedEventArgs<ConfigurationPackage>> ConfigurationPackageModifiedEvent;

        /// <summary>
        /// Raised during an application upgrade when a data package is added to the service manifest.
        /// </summary>
        public event EventHandler<PackageAddedEventArgs<DataPackage>> DataPackageAddedEvent;

        /// <summary>
        /// Raised during an application upgrade when a data package is removed from the service manifest.
        /// </summary>
        public event EventHandler<PackageRemovedEventArgs<DataPackage>> DataPackageRemovedEvent;

        /// <summary>
        /// Raised during an application upgrade when a data package is modified in the service manifest.
        /// </summary>
        public event EventHandler<PackageModifiedEventArgs<DataPackage>> DataPackageModifiedEvent;

        /// <summary>
        /// <para>Gets the path to the Work directory that the application can use to store data. For example: the state of the replicas.</para>
        /// </summary>
        /// <value>
        /// <para>The path to the Work directory.</para>
        /// </value>        
        public string WorkDirectory
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the path to the log directory that the application can use.</para>
        /// </summary>
        /// <value>
        /// <para>The path to the application logs directory.</para>
        /// </value>
        public string LogDirectory
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the path to the Temp directory that the Application can use for temporary files.</para>
        /// </summary>
        /// <value>
        /// <para>The path to the Temp directory.</para>
        /// </value>
        public string TempDirectory
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the ID that represents the service package name qualified with Application package name.</para>
        /// </summary>
        /// <value>
        /// <para>The context ID.</para>
        /// </value>
        public string ContextId
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the name of the fabric activated code package.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the fabric activated code package.</para>
        /// </value>
        public string CodePackageName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the version of the fabric activated code package</para>
        /// </summary>
        /// <value>
        /// <para>The version of the fabric activated code package.</para>
        /// </value>
        public string CodePackageVersion
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the name of the application.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application.</para>
        /// </value>
        public string ApplicationName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>Gets the name of the application type.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the application type.</para>
        /// </value>
        public string ApplicationTypeName
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </summary>
        /// <value>
        /// <para>The address at which the service should start the communication listener.</para>
        /// </value>
        public string ServiceListenAddress
        {
            get;
            internal set;
        }

        /// <summary>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </summary>
        /// <value>
        /// <para>The address which the service should publish as the listen address.</para>
        /// </value>
        public string ServicePublishAddress
        {
            get;
            internal set;
        }

        /// <summary>
        /// The native activation context
        /// </summary>
        internal NativeRuntime.IFabricCodePackageActivationContext NativeActivationContext
        {
            get
            {
                return this.nativeActivationContext;
            }
        }

        /// <summary>
        /// <para>Retrieves the list of service types in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The list of service types.</para>
        /// </returns>
        public KeyedCollection<string, ServiceTypeDescription> GetServiceTypes()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<KeyedCollection<string, ServiceTypeDescription>>(this.GetServiceTypesHelper, "CodePackageActivationContext.GetServiceTypes");
        }

        /// <summary>
        /// <para>Retrieves the list of service group types in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The list of Service Group types in the service manifest.</para>
        /// </returns>
        public KeyedCollection<string, ServiceGroupTypeDescription> GetServiceGroupTypes()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<KeyedCollection<string, ServiceGroupTypeDescription>>(this.GetServiceGroupTypesHelper, "CodePackageActivationContext.GetServiceGroupTypes");
        }

        /// <summary>
        /// <para>Retrieves all the principals defined in the application manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The principals defined in the application manifest.</para>
        /// </returns>
        public ApplicationPrincipalsDescription GetApplicationPrincipals()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<ApplicationPrincipalsDescription>(this.GetApplicationPrincipalsHelper, "CodePackageActivationContext.GetApplicationPrincipals");
        }

        /// <summary>
        /// <para>Retrieves all the end points in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The end points in the service manifest.</para>
        /// </returns>
        public KeyedCollection<string, EndpointResourceDescription> GetEndpoints()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<KeyedCollection<string, EndpointResourceDescription>>(this.GetEndpointsHelper, "CodePackageActivationContext.GetEndpoints");
        }

        /// <summary>
        /// <para>Retrieves the list of code package names in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The list of code package names in the service manifest.</para>
        /// </returns>
        public IList<string> GetCodePackageNames()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<IList<string>>(this.GetCodePackageNamesHelper, "CodePackageActivationContext.GetCodePackageNames");
        }

        /// <summary>
        /// <para>Retrieves the list of configuration package names in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The list of configuration package names in the service manifest.</para>
        /// </returns>
        public IList<string> GetConfigurationPackageNames()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<IList<string>>(this.GetConfigurationPackageNamesHelper, "CodePackageActivationContext.GetConfigurationPackageNames");
        }

        /// <summary>
        /// <para>Retrieves the list of data package names in the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The list of data package names in the service manifest.</para>
        /// </returns>
        public IList<string> GetDataPackageNames()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke<IList<string>>(this.GetDataPackageNamesHelper, "CodePackageActivationContext.GetDataPackageNames");
        }

        /// <summary>
        /// <para>Retrieves an <see cref="System.Fabric.Description.EndpointResourceDescription"/> by name.</para>
        /// </summary>
        /// <param name="endpointName">
        /// <para>The name of the endpoint.</para>
        /// </param>
        /// <returns>
        /// <para>The description of the endpoint with a specified name.</para>
        /// </returns>
        /// <exception cref="System.ArgumentNullException">
        /// <para>endpointName is null or empty</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricElementNotFoundException">
        /// <para>The endpoint was not found in the service manifest.</para>
        /// </exception>
        public EndpointResourceDescription GetEndpoint(string endpointName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetEndpointHelper(endpointName), "CodePackageActivationContext.GetEndpoint");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.CodePackage"/> object by name.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.CodePackage" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        public CodePackage GetCodePackageObject(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetCodePackageHelper(packageName), "CodePackageActivationContext.GetCodePackageObject");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.ConfigurationPackage"/> object by name.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        public ConfigurationPackage GetConfigurationPackageObject(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetConfigurationPackageHelper(packageName), "CodePackageActivationContext.GetConfigurationPackageObject");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.DataPackage"/> object by name.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.DataPackage" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        public DataPackage GetDataPackageObject(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetDataPackageHelper(packageName), "CodePackageActivationContext.GetDataPackageObject");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.Description.CodePackageDescription"/> object by name.</para>
        /// <para>This method is obsolete. Use <see cref="GetCodePackageObject"/>.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.Description.CodePackageDescription" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        [Obsolete("Use GetCodePackageObject method.")]
        public CodePackageDescription GetCodePackage(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetCodePackageDescriptionHelper(packageName), "CodePackageActivationContext.GetCodePackage");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.Description.ConfigurationPackageDescription"/> object by name.</para>
        /// <para>This method is obsolete. Use <see cref="GetConfigurationPackageObject"/>.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.Description.ConfigurationPackageDescription" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        [Obsolete("Use GetConfigurationPackageObject method.")]
        public ConfigurationPackageDescription GetConfigurationPackage(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetConfigurationPackageDescriptionHelper(packageName), "CodePackageActivationContext.GetConfigurationPackage");
        }

        /// <summary>
        /// <para>Retrieves the <see cref="System.Fabric.Description.DataPackageDescription"/> by name.</para>
        /// <para>This method is obsolete. Use <see cref="GetDataPackageObject"/>.</para>
        /// </summary>
        /// <param name="packageName">
        /// <para>The name of the package to find</para>
        /// </param>
        /// <returns>
        /// <para>Returns <see cref="System.Fabric.Description.DataPackageDescription" />.</para>
        /// </returns>
        /// <exception cref="FabricElementNotFoundException">The packageName was not found in the service manifest.</exception>
        [Obsolete("Use GetDataPackageObject method.")]
        public DataPackageDescription GetDataPackage(string packageName)
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetDataPackageDescriptionHelper(packageName), "CodePackageActivationContext.GetDataPackage");
        }

        /// <summary>
        /// <para>Retrieves the name of the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The name of the service manifest.</para>
        /// </returns>
        public string GetServiceManifestName()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetServiceManifestNameHelper(), "CodePackageActivationContext.GetServiceManifestName");
        }

        /// <summary>
        /// <para>Retrieves the version of the service manifest.</para>
        /// </summary>
        /// <returns>
        /// <para>The version of the service manifest.</para>
        /// </returns>
        public string GetServiceManifestVersion()
        {
            this.ThrowIfDisposed();
            return Utility.WrapNativeSyncInvoke(() => this.GetServiceManifestVersionHelper(), "CodePackageActivationContext.GetServiceManifestVersion");
        }

        /// <summary>
        /// Reports health for current application. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.CodePackageActivationContext.ReportApplicationHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportApplicationHealth(Health.HealthInformation healthInfo)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportApplicationHealthHelper(healthInfo, null), "CodePackageActivationContext.ReportApplicationHealth");
        }

        /// <summary>
        /// Reports health for current application.
        /// Specifies options to control how the report is sent.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportApplicationHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportApplicationHealthHelper(healthInfo, sendOptions), "CodePackageActivationContext.ReportApplicationHealth");
        }

        /// <summary>
        /// Reports health for current deployed application. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.CodePackageActivationContext.ReportDeployedApplicationHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportDeployedApplicationHealth(Health.HealthInformation healthInfo)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportDeployedApplicationHealthHelper(healthInfo, null), "CodePackageActivationContext.ReportDeployedApplicationHealth");
        }

        /// <summary>
        /// Reports health for current deployed application. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportDeployedApplicationHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportDeployedApplicationHealthHelper(healthInfo, sendOptions), "CodePackageActivationContext.ReportDeployedApplicationHealth");
        }

        /// <summary>
        /// Reports health for current deployed service package. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately by using
        /// <see cref="System.Fabric.CodePackageActivationContext.ReportDeployedServicePackageHealth(System.Fabric.Health.HealthInformation, System.Fabric.Health.HealthReportSendOptions)"/>.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportDeployedServicePackageHealth(Health.HealthInformation healthInfo)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportDeployedServicePackageHealthHelper(healthInfo, null), "CodePackageActivationContext.ReportDeployedServicePackageHealth");
        }

        /// <summary>
        /// Reports health for current deployed service package. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information,
        /// such as source, property, and health state.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <exception cref="System.ArgumentNullException">
        /// <para>Returned when a null reference is passed to a method that does not accept it as a valid argument.</para>
        /// </exception>
        /// <exception cref="System.Fabric.FabricException">
        ///     <para>Caused by one of the following:</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthStaleReport" />.</para>
        ///     <para>
        ///         <see cref="System.Fabric.FabricErrorCode.FabricHealthMaxReportsReached" />.</para>
        /// </exception>
        /// <remarks>
        /// <para>The health information describes the report details, like the source ID, the property, the health state
        /// and other relevant details.
        /// The code package activation context uses an internal health client to send the reports to the health store.
        /// The client optimizes messages to Health Manager by batching reports per a configured duration (Default: 30 seconds).
        /// If the report has high priority, you can specify send options to send it immediately.
        /// </para>
        /// <para>Read more about <see href="https://docs.microsoft.com/azure/service-fabric/service-fabric-report-health">health reporting</see>.</para>
        /// </remarks>
        public void ReportDeployedServicePackageHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            this.ThrowIfDisposed();
            Requires.Argument("healthInfo", healthInfo).NotNull();
            Utility.WrapNativeSyncInvoke(() => this.ReportDeployedServicePackageHealthHelper(healthInfo, sendOptions), "CodePackageActivationContext.ReportDeployedServicePackageHealth");
        }

        /// <summary>
        /// Retrieves the directory path for the directory inside the work directory.
        /// </summary>
        public string GetDirectory(string logicalDirectoryName)
        {
            this.ThrowIfDisposed();
            Requires.Argument("logicalDirectoryName", logicalDirectoryName).NotNull();
            return Utility.WrapNativeSyncInvoke(() => this.GetDirectoryHelper(logicalDirectoryName), "CodePackageActivationContext.GetDirectory");
        }


        /// <summary>
        /// <para>
        /// Disposes of the code package activation context.
        /// </para>
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        internal static CodePackageActivationContext CreateFromNative(NativeRuntime.IFabricCodePackageActivationContext6 nativeContext)
        {
            string contextId = NativeTypes.FromNativeString(nativeContext.get_ContextId());
            string workDirectory = NativeTypes.FromNativeString(nativeContext.get_WorkDirectory());
            string logDirectory = NativeTypes.FromNativeString(nativeContext.get_LogDirectory());
            string tempDirectory = NativeTypes.FromNativeString(nativeContext.get_TempDirectory());
            string codePackageName = NativeTypes.FromNativeString(nativeContext.get_CodePackageName());
            string codePackageVersion = NativeTypes.FromNativeString(nativeContext.get_CodePackageVersion());
            string applicationName = NativeTypes.FromNativeString(nativeContext.get_ApplicationName());
            string applicationTypeName = NativeTypes.FromNativeString(nativeContext.get_ApplicationTypeName());

            string serviceListenAddress = NativeTypes.FromNativeString(nativeContext.get_ServiceListenAddress());
            string servicePublishAddress = NativeTypes.FromNativeString(nativeContext.get_ServicePublishAddress());

            AppTrace.TraceSource.WriteNoise(
                "CodePackageActivationContext.FromNative",
                "Created CodePackageActivationContext: contextId={0}, codePackageName={1} codePackageVersion={2}, workDirectory={3}, logDirectory={4}, tempDirectory={5}," +
                " applicationName={6}, applicationTypeName={7}",
                contextId, codePackageName, codePackageVersion, workDirectory, logDirectory, tempDirectory, applicationName, applicationTypeName);

            return new CodePackageActivationContext(nativeContext)
            {
                ContextId = contextId,
                WorkDirectory = workDirectory,
                LogDirectory = logDirectory,
                TempDirectory = tempDirectory,
                CodePackageName = codePackageName,
                CodePackageVersion = codePackageVersion,
                ApplicationName = applicationName,
                ApplicationTypeName = applicationTypeName,
                ServiceListenAddress = serviceListenAddress,
                ServicePublishAddress = servicePublishAddress
            };
        }

        private void ThrowIfDisposed()
        {
            if (this.disposed)
            {
                throw new ObjectDisposedException("CodePackageActivationContext");
            }
        }

        // ReSharper disable once UnusedParameter.Local
        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                //
                // Releases the notification broker references registered with the native context.
                //
#if !DotNetCoreClrLinux
                this.nativeActivationContext.UnregisterCodePackageChangeHandler(this.codePackageChangeHandlerId);
                this.nativeActivationContext.UnregisterConfigurationPackageChangeHandler(this.configPackageChangeHandlerId);
                this.nativeActivationContext.UnregisterDataPackageChangeHandler(this.dataPackageChangeHandlerId);
#endif
                this.disposed = true;
            }
        }

        private unsafe KeyedCollection<string, ServiceTypeDescription> GetServiceTypesHelper()
        {
            KeyedItemCollection<string, ServiceTypeDescription> serviceTypes = new KeyedItemCollection<string, ServiceTypeDescription>(d => d.ServiceTypeName);
            var nativeServiceTypes = (NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION_LIST*)this.nativeActivationContext.get_ServiceTypes();
            for (int i = 0; i < nativeServiceTypes->Count; i++)
            {
                serviceTypes.Add(ServiceTypeDescription.CreateFromNative(nativeServiceTypes->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_TYPE_DESCRIPTION)))));
            }

            return serviceTypes;
        }

        private unsafe KeyedCollection<string, ServiceGroupTypeDescription> GetServiceGroupTypesHelper()
        {
            KeyedItemCollection<string, ServiceGroupTypeDescription> serviceGroupTypes = new KeyedItemCollection<string, ServiceGroupTypeDescription>(d => d.ServiceTypeDescription.ServiceTypeName);
            var nativeServiceGroupTypes = (NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION_LIST*)this.nativeActivationContext.get_ServiceGroupTypes();
            for (int i = 0; i < nativeServiceGroupTypes->Count; i++)
            {
                serviceGroupTypes.Add(ServiceGroupTypeDescription.CreateFromNative(nativeServiceGroupTypes->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_SERVICE_GROUP_TYPE_DESCRIPTION)))));
            }

            return serviceGroupTypes;
        }

        private unsafe ApplicationPrincipalsDescription GetApplicationPrincipalsHelper()
        {
            var principals = (NativeTypes.FABRIC_APPLICATION_PRINCIPALS_DESCRIPTION*)this.nativeActivationContext.get_ApplicationPrincipals();
            return ApplicationPrincipalsDescription.CreateFromNative(principals);
        }

        private unsafe KeyedCollection<string, EndpointResourceDescription> GetEndpointsHelper()
        {
            KeyedItemCollection<string, EndpointResourceDescription> endpoints = new KeyedItemCollection<string, EndpointResourceDescription>(d => d.Name);
            var nativeEndpoints = (NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION_LIST*)this.nativeActivationContext.get_ServiceEndpointResources();
            for (int i = 0; i < nativeEndpoints->Count; i++)
            {
                endpoints.Add(EndpointResourceDescription.CreateFromNative(nativeEndpoints->Items + (i * Marshal.SizeOf(typeof(NativeTypes.FABRIC_ENDPOINT_RESOURCE_DESCRIPTION)))));
            }

            return endpoints;
        }

        private EndpointResourceDescription GetEndpointHelper(string endpointName)
        {
            Requires.Argument("endpointName", endpointName).NotNullOrEmpty();
            using (var pin = new PinBlittable(endpointName))
            {
                return EndpointResourceDescription.CreateFromNative(this.nativeActivationContext.GetServiceEndpointResource(pin.AddrOfPinnedObject()));
            }
        }

        private IList<string> GetCodePackageNamesHelper()
        {
            var nativeResult = this.nativeActivationContext.GetCodePackageNames();
            return StringCollectionResult.FromNative(nativeResult);
        }

        private IList<string> GetConfigurationPackageNamesHelper()
        {
            var nativeResult = this.nativeActivationContext.GetConfigurationPackageNames();
            return StringCollectionResult.FromNative(nativeResult);
        }

        private IList<string> GetDataPackageNamesHelper()
        {
            var nativeResult = this.nativeActivationContext.GetDataPackageNames();
            return StringCollectionResult.FromNative(nativeResult);
        }

        private CodePackage GetCodePackageHelper(string packageName)
        {
            Requires.Argument("packageName", packageName).NotNullOrEmpty();
            using (var pin = new PinBlittable(packageName))
            {
                return CodePackage.CreateFromNative(this.nativeActivationContext.GetCodePackage(pin.AddrOfPinnedObject()));
            }
        }

        private CodePackageDescription GetCodePackageDescriptionHelper(string packageName)
        {
            return this.GetCodePackageHelper(packageName).Description;
        }

        private ConfigurationPackage GetConfigurationPackageHelper(string packageName)
        {
            Requires.Argument("packageName", packageName).NotNullOrEmpty();
            using (var pin = new PinBlittable(packageName))
            {
                return ConfigurationPackage.CreateFromNative(this.nativeActivationContext.GetConfigurationPackage(pin.AddrOfPinnedObject()));
            }
        }

        private ConfigurationPackageDescription GetConfigurationPackageDescriptionHelper(string packageName)
        {
            return this.GetConfigurationPackageHelper(packageName).Description;
        }

        private DataPackage GetDataPackageHelper(string packageName)
        {
            Requires.Argument("packageName", packageName).NotNullOrEmpty();
            using (var pin = new PinBlittable(packageName))
            {
                return DataPackage.CreateFromNative(this.nativeActivationContext.GetDataPackage(pin.AddrOfPinnedObject()));
            }
        }

        private DataPackageDescription GetDataPackageDescriptionHelper(string packageName)
        {
            return this.GetDataPackageHelper(packageName).Description;
        }

        private string GetServiceManifestNameHelper()
        {
            return StringResult.FromNative(this.nativeActivationContext.GetServiceManifestName());
        }

        private string GetServiceManifestVersionHelper()
        {
            return StringResult.FromNative(this.nativeActivationContext.GetServiceManifestVersion());
        }

        private void ReportApplicationHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativeActivationContext.ReportApplicationHealth2(healthInfo.ToNative(pin), nativeSendOptions);
            }
        }

        private void ReportDeployedApplicationHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativeActivationContext.ReportDeployedApplicationHealth2(healthInfo.ToNative(pin), nativeSendOptions);
            }
        }

        private void ReportDeployedServicePackageHealthHelper(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions)
        {
            using (var pin = new PinCollection())
            {
                var nativeSendOptions = IntPtr.Zero;
                if (sendOptions != null)
                {
                    nativeSendOptions = sendOptions.ToNative(pin);
                }

                this.nativeActivationContext.ReportDeployedServicePackageHealth2(healthInfo.ToNative(pin), nativeSendOptions);
            }
        }

        private string GetDirectoryHelper(string logicalDirectoryName)
        {
            using (var pin = new PinBlittable(logicalDirectoryName))
            {
                return StringResult.FromNative(this.nativeActivationContext.GetDirectory(pin.AddrOfPinnedObject()));
            }
        }


        private void RaiseCodePackageAddedEvent(PackageAddedEventArgs<CodePackage> eventArgs)
        {
            var ev = this.CodePackageAddedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.CodePackageAdded;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageAddedEventArgs<CodePackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseCodePackageRemovedEvent(PackageRemovedEventArgs<CodePackage> eventArgs)
        {
            var ev = this.CodePackageRemovedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.CodePackageRemoved;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageRemovedEventArgs<CodePackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseCodePackageModifiedEvent(PackageModifiedEventArgs<CodePackage> eventArgs)
        {
            var ev = this.CodePackageModifiedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.CodePackageModified;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(
                    this,
                    new PackageModifiedEventArgs<CodePackageDescription>()
                    {
                        OldPackage = eventArgs.OldPackage.Description,
                        NewPackage = eventArgs.NewPackage.Description
                    });
            }
        }

        private void RaiseConfigurationPackageAddedEvent(PackageAddedEventArgs<ConfigurationPackage> eventArgs)
        {
            var ev = this.ConfigurationPackageAddedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.ConfigurationPackageAdded;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageAddedEventArgs<ConfigurationPackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseConfigurationPackageRemovedEvent(PackageRemovedEventArgs<ConfigurationPackage> eventArgs)
        {
            var ev = this.ConfigurationPackageRemovedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.ConfigurationPackageRemoved;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageRemovedEventArgs<ConfigurationPackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseConfigurationPackageModifiedEvent(PackageModifiedEventArgs<ConfigurationPackage> eventArgs)
        {
            var ev = this.ConfigurationPackageModifiedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.ConfigurationPackageModified;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(
                    this,
                    new PackageModifiedEventArgs<ConfigurationPackageDescription>()
                    {
                        OldPackage = eventArgs.OldPackage.Description,
                        NewPackage = eventArgs.NewPackage.Description
                    });
            }
        }

        private void RaiseDataPackageAddedEvent(PackageAddedEventArgs<DataPackage> eventArgs)
        {
            var ev = this.DataPackageAddedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.DataPackageAdded;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageAddedEventArgs<DataPackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseDataPackageRemovedEvent(PackageRemovedEventArgs<DataPackage> eventArgs)
        {
            var ev = this.DataPackageRemovedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.DataPackageRemoved;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(this, new PackageRemovedEventArgs<DataPackageDescription>() { Package = eventArgs.Package.Description });
            }
        }

        private void RaiseDataPackageModifiedEvent(PackageModifiedEventArgs<DataPackage> eventArgs)
        {
            var ev = this.DataPackageModifiedEvent;

            if (ev != null)
            {
                ev(this, eventArgs);
            }

#pragma warning disable 618
            var oldEv = this.DataPackageModified;
#pragma warning restore 618

            if (oldEv != null)
            {
                oldEv(
                    this,
                    new PackageModifiedEventArgs<DataPackageDescription>()
                    {
                        OldPackage = eventArgs.OldPackage.Description,
                        NewPackage = eventArgs.NewPackage.Description
                    });
            }
        }

        private sealed class PackageNotificationBroker : NativeRuntime.IFabricCodePackageChangeHandler, NativeRuntime.IFabricConfigurationPackageChangeHandler, NativeRuntime.IFabricDataPackageChangeHandler
        {
            internal WeakReference OwnerWref;

            private Action<PackageAddedEventArgs<CodePackage>> CodePackageAddedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseCodePackageAddedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageRemovedEventArgs<CodePackage>> CodePackageRemovedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseCodePackageRemovedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageModifiedEventArgs<CodePackage>> CodePackageModifiedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseCodePackageModifiedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageAddedEventArgs<ConfigurationPackage>> ConfigurationPackageAddedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseConfigurationPackageAddedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageRemovedEventArgs<ConfigurationPackage>> ConfigurationPackageRemovedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseConfigurationPackageRemovedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageModifiedEventArgs<ConfigurationPackage>> ConfigurationPackageModifiedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseConfigurationPackageModifiedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageAddedEventArgs<DataPackage>> DataPackageAddedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseDataPackageAddedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageRemovedEventArgs<DataPackage>> DataPackageRemovedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseDataPackageRemovedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            private Action<PackageModifiedEventArgs<DataPackage>> DataPackageModifiedEventRaiser
            {
                get
                {
                    var obj = this.OwnerWref.Target as CodePackageActivationContext;
                    if (obj != null)
                    {
                        return obj.RaiseDataPackageModifiedEvent;
                    }
                    else
                    {
                        return null;
                    }
                }
            }

            public void OnPackageAdded(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricCodePackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => AddedEventHelper(newPackage, CodePackage.CreateFromNative, this.CodePackageAddedEventRaiser),
                    "PackageNotificationBroker.CodePackageAdded");
            }

            public void OnPackageRemoved(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricCodePackage oldPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => RemovedEventHelper(oldPackage, CodePackage.CreateFromNative, this.CodePackageRemovedEventRaiser),
                    "PackageNotificationBroker.CodePackageRemoved");
            }

            public void OnPackageModified(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricCodePackage oldPackage, NativeRuntime.IFabricCodePackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => ModifiedEventHelper(oldPackage, newPackage, CodePackage.CreateFromNative, this.CodePackageModifiedEventRaiser),
                    "PackageNotificationBroker.CodePackageModified");
            }

            public void OnPackageAdded(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricConfigurationPackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => AddedEventHelper(newPackage, ConfigurationPackage.CreateFromNative, this.ConfigurationPackageAddedEventRaiser),
                    "PackageNotificationBroker.ConfigurationPackageAdded");
            }

            public void OnPackageRemoved(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricConfigurationPackage oldPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => RemovedEventHelper(oldPackage, ConfigurationPackage.CreateFromNative, this.ConfigurationPackageRemovedEventRaiser),
                    "PackageNotificationBroker.ConfigurationPackageRemoved");
            }

            public void OnPackageModified(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricConfigurationPackage oldPackage, NativeRuntime.IFabricConfigurationPackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => ModifiedEventHelper(oldPackage, newPackage, ConfigurationPackage.CreateFromNative, this.ConfigurationPackageModifiedEventRaiser),
                    "PackageNotificationBroker.ConfigurationPackageModified");
            }

            public void OnPackageAdded(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricDataPackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => AddedEventHelper(newPackage, DataPackage.CreateFromNative, this.DataPackageAddedEventRaiser),
                    "PackageNotificationBroker.DataPackageAdded");
            }

            public void OnPackageRemoved(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricDataPackage oldPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => RemovedEventHelper(oldPackage, DataPackage.CreateFromNative, this.DataPackageRemovedEventRaiser),
                    "PackageNotificationBroker.DataPackageRemoved");
            }

            public void OnPackageModified(NativeRuntime.IFabricCodePackageActivationContext source, NativeRuntime.IFabricDataPackage oldPackage, NativeRuntime.IFabricDataPackage newPackage)
            {
                Utility.WrapNativeSyncMethodImplementation(
                    () => ModifiedEventHelper(oldPackage, newPackage, DataPackage.CreateFromNative, this.DataPackageModifiedEventRaiser),
                    "PackageNotificationBroker.DataPackageModified");
            }

            private static void AddedEventHelper<TPackage, TDescription>(TPackage newNativePackage, Func<TPackage, TDescription> factory, Action<PackageAddedEventArgs<TDescription>> eventRaiser)
                where TPackage : class
                where TDescription : class
            {
                if (eventRaiser != null)
                {
                    eventRaiser(
                        new PackageAddedEventArgs<TDescription>
                        {
                            Package = newNativePackage == null ? null : factory(newNativePackage),
                        });
                }
            }

            private static void RemovedEventHelper<TPackage, TDescription>(TPackage oldNativePackage, Func<TPackage, TDescription> factory, Action<PackageRemovedEventArgs<TDescription>> eventRaiser)
                where TPackage : class
                where TDescription : class
            {
                if (eventRaiser != null)
                {
                    eventRaiser(
                        new PackageRemovedEventArgs<TDescription>
                        {
                            Package = oldNativePackage == null ? null : factory(oldNativePackage),
                        });
                }
            }

            private static void ModifiedEventHelper<TPackage, TDescription>(TPackage oldNativePackage, TPackage newNativePackage, Func<TPackage, TDescription> factory, Action<PackageModifiedEventArgs<TDescription>> eventRaiser)
                where TPackage : class
                where TDescription : class
            {
                if (eventRaiser != null)
                {
                    eventRaiser(
                        new PackageModifiedEventArgs<TDescription>
                        {
                            OldPackage = oldNativePackage == null ? null : factory(oldNativePackage),
                            NewPackage = newNativePackage == null ? null : factory(newNativePackage),
                        });
                }
            }
        }
    }

    /// <summary>
    /// <para>Describes a package added event. </para>
    /// </summary>
    /// <typeparam name="TPackage">
    /// <para>Type of the package being described. See <see cref="System.Fabric.CodePackage" />, <see cref="System.Fabric.ConfigurationPackage" />, <see cref="System.Fabric.DataPackage" />.</para>
    /// </typeparam>
    /// <remarks>
    /// <para>See <see cref="System.Fabric.CodePackageActivationContext.CodePackageAddedEvent" />, 
    /// <see cref="System.Fabric.CodePackageActivationContext.ConfigurationPackageAddedEvent" />, and <see cref="System.Fabric.CodePackageActivationContext.DataPackageAddedEvent" />.</para>
    /// </remarks>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Small class.")]
    public sealed class PackageAddedEventArgs<TPackage> : EventArgs where TPackage : class
    {
        /// <summary>
        /// <para>Creates a new instance of the <see cref="System.Fabric.PackageAddedEventArgs{TPackage}" /> class.</para>
        /// </summary>
        public PackageAddedEventArgs()
        {
        }
            
        /// <summary>
        /// <para>Gets or sets the code, data, or configuration package that was added to the Service Manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The code, data, or configuration package that was added to the Service Manifest.</para>
        /// </value>
        public TPackage Package { get; set; }
    }

    /// <summary>
    /// <para>Describes a package removed event. </para>
    /// </summary>
    /// <typeparam name="TPackage">
    /// <para>The type of the package being described. See <see cref="System.Fabric.CodePackage" />, <see cref="System.Fabric.ConfigurationPackage" />, <see cref="System.Fabric.DataPackage" />.</para>
    /// </typeparam>
    /// <remarks>
    /// <para>See <see cref="System.Fabric.CodePackageActivationContext.CodePackageRemovedEvent" />, <see cref="System.Fabric.CodePackageActivationContext.ConfigurationPackageRemovedEvent" />, and <see cref="System.Fabric.CodePackageActivationContext.DataPackageRemovedEvent" />.</para>
    /// </remarks>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Small class.")]
    public sealed class PackageRemovedEventArgs<TPackage> : EventArgs where TPackage : class
    {
        /// <summary>
        /// <para>Initializes a new instance of the <see cref="System.Fabric.PackageRemovedEventArgs{TPackage}" /> class.</para>
        /// </summary>
        public PackageRemovedEventArgs()
        {
        }

        /// <summary>
        /// <para>Gets or sets the code, configuration, or data package that was removed from the Service Manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The code, configuration, or data package that was removed from the Service Manifest.</para>
        /// </value>
        public TPackage Package { get; set; }
    }

    /// <summary>
    ///   Represents the event arguments for package modification.
    /// </summary>
    /// <typeparam name="TPackage">
    ///   The type of the modified package.
    /// </typeparam>
    [SuppressMessage(FxCop.Category.StyleCopMaintainability, FxCop.Rule.FileMayOnlyContainASingleClass, Justification = "Small class.")]
    public sealed class PackageModifiedEventArgs<TPackage> : EventArgs where TPackage : class
    {
        /// <summary>
        ///   Initializes a new instance of the <see cref="System.Fabric.PackageModifiedEventArgs{TPackage}"/> class.
        /// </summary>
        public PackageModifiedEventArgs()
        {
        }

        /// <summary>
        ///   Gets or sets the old package that is modified.
        /// </summary>
        /// <value>
        /// <para>The old package that is modified.</para>
        /// </value>
        public TPackage OldPackage { get; set; }

        /// <summary>
        ///   Gets or sets the new package.
        /// </summary>
        /// <value>
        /// <para>The new package that replaces the old package.</para>
        /// </value>
        public TPackage NewPackage { get; set; }
    }
}