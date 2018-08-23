// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Collections.Generic;
    using System.Collections.ObjectModel;
    using System.Fabric.Description;

    /// <summary>
    /// Represents activation context for the Service Fabric activated service.
    /// </summary>
    /// <remarks>Includes information from the service manifest as well as information
    /// about the currently activated code package like work directory, context id etc.</remarks>
    public interface ICodePackageActivationContext : IDisposable
    {
        /// <summary>
        /// Event raised when new <see cref="System.Fabric.CodePackage"/> is added to the service manifest.
        /// </summary>
        event EventHandler<PackageAddedEventArgs<CodePackage>> CodePackageAddedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.CodePackage"/> is removed from the service manifest.
        /// </summary>
        event EventHandler<PackageRemovedEventArgs<CodePackage>> CodePackageRemovedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.CodePackage"/> in the service manifest is modified.
        /// </summary>
        event EventHandler<PackageModifiedEventArgs<CodePackage>> CodePackageModifiedEvent;

        /// <summary>
        /// Event raised when new <see cref="System.Fabric.ConfigurationPackage"/> is added to the service manifest.
        /// </summary>
        event EventHandler<PackageAddedEventArgs<ConfigurationPackage>> ConfigurationPackageAddedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.ConfigurationPackage"/> is removed from the service manifest.
        /// </summary>
        event EventHandler<PackageRemovedEventArgs<ConfigurationPackage>> ConfigurationPackageRemovedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.ConfigurationPackage"/> in the service manifest is modified.
        /// </summary>
        event EventHandler<PackageModifiedEventArgs<ConfigurationPackage>> ConfigurationPackageModifiedEvent;

        /// <summary>
        /// Event raised when new <see cref="System.Fabric.DataPackage"/> is added to the service manifest.
        /// </summary>
        event EventHandler<PackageAddedEventArgs<DataPackage>> DataPackageAddedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.DataPackage"/> is removed from the service manifest.
        /// </summary>
        event EventHandler<PackageRemovedEventArgs<DataPackage>> DataPackageRemovedEvent;

        /// <summary>
        /// Event raised when a <see cref="System.Fabric.DataPackage"/> in the service manifest is modified.
        /// </summary>
        event EventHandler<PackageModifiedEventArgs<DataPackage>> DataPackageModifiedEvent;

        /// <summary>
        /// Gets the path to the Work directory that the application can use.
        /// </summary>
        /// <value>The path to the code package Work directory.</value>
        string WorkDirectory { get; }

        /// <summary>
        /// Gets the path to the Log directory that the application can use.
        /// </summary>
        /// <value>The path to the code package Log directory.</value>
        /// <remarks>If you have specified a parameter name Log in LogicalDirectories section of ClusterManifest 
        /// then the returned path is a symbolic link to the parameter value.
        /// for ex: if <LogicalDirectory LogicalDirectoryName="Log" MappedTo="F:\Log" /> is specified then 
        /// the returned path to LogDirectory will be a symbolic link to F:\Log\NodeId\ApplicationType_ApplicationVersion</remarks>
        string LogDirectory { get; }

        /// <summary>
        /// Gets the path to the Temp directory that the application can use.
        /// </summary>
        /// <value>The path to the code package Temp directory.</value>
        string TempDirectory { get; }

        /// <summary>
        /// Gets the context id.
        /// </summary>
        /// <value>The context id.</value>
        /// <remarks>The context id is the same for all code packages in the same service manifest.</remarks>
        string ContextId { get; }

        /// <summary>
        /// Gets the name of the fabric activated code package.
        /// </summary>
        /// <value>The name of the fabric activated code package.</value>
        string CodePackageName { get; }

        /// <summary>
        /// Gets the version of the fabric activated code package.
        /// </summary>
        /// <value>The version of the fabric activated code package.</value>
        string CodePackageVersion { get; }

        /// <summary>
        /// Gets the application name.
        /// </summary>
        /// <value>The name of the application.</value>
        string ApplicationName { get; }

        /// <summary>
        /// Gets the application type name.
        /// </summary>
        /// <value>The name of the application type.</value>
        string ApplicationTypeName { get; }

        /// <summary>
        /// Retrieves the list of Service types in the service manifest.
        /// </summary>
        /// <returns>The list of service types in the service manifest.</returns>
        KeyedCollection<string, ServiceTypeDescription> GetServiceTypes();

        /// <summary>
        /// Retrieves the list of Service Group types in the service manifest.
        /// </summary>
        /// <returns>The list of Service Group types in the service manifest.</returns>
        KeyedCollection<string, ServiceGroupTypeDescription> GetServiceGroupTypes();

        /// <summary>
        /// Retrieves the principals defined in the application manifest.
        /// </summary>
        /// <returns>The principals defined in the application manifest.</returns>
        ApplicationPrincipalsDescription GetApplicationPrincipals();

        /// <summary>
        /// Retrieves the endpoint resources in the service manifest.
        /// </summary>
        /// <returns>The endpoint resources in the service manifest.</returns>
        KeyedCollection<string, EndpointResourceDescription> GetEndpoints();

        /// <summary>
        /// Retrieves the endpoint resource with a given name from the service manifest.
        /// </summary>
        /// <param name="endpointName">The name of the endpoint.</param>
        /// <returns>The endpoint resource with the specified name.</returns>
        EndpointResourceDescription GetEndpoint(string endpointName);

        /// <summary>
        /// Retrieves the list of code package names in the service manifest.
        /// </summary>
        /// <returns>The list of code package names in the service manifest.</returns>
        IList<string> GetCodePackageNames();

        /// <summary>
        /// Retrieves the list of configuration package names in the service manifest.
        /// </summary>
        /// <returns>The list of configuration package names in the service manifest.</returns>
        IList<string> GetConfigurationPackageNames();

        /// <summary>
        /// Retrieves the list of data package names in the service manifest.
        /// </summary>
        /// <returns>The list of data package names in the service manifest.</returns>
        IList<string> GetDataPackageNames();

        /// <summary>
        /// Returns the <see cref="System.Fabric.CodePackage"/> object from Service Package that matches the desired package name.
        /// </summary>
        /// <param name="packageName">The name of the code package.</param>
        /// <returns>The <see cref="System.Fabric.CodePackage"/> object from Service Package that matches the desired package name.</returns>
        /// /// <remarks>Throws KeyNotFoundException exception if the package is not found.</remarks>
        CodePackage GetCodePackageObject(string packageName);

        /// <summary>
        /// Returns the <see cref="System.Fabric.ConfigurationPackage"/> object from Service Package that matches the desired package name.
        /// </summary>
        /// <param name="packageName">The name of the configuration package.</param>
        /// <returns>The <see cref="System.Fabric.ConfigurationPackage"/> object from Service Package that matches the desired package name.</returns>
        /// <remarks>Throws KeyNotFoundException exception if the package is not found.</remarks>
        ConfigurationPackage GetConfigurationPackageObject(string packageName);

        /// <summary>
        /// Returns the <see cref="System.Fabric.DataPackage"/> object from Service Package that matches the desired package name.
        /// </summary>
        /// <param name="packageName">The name of the data package.</param>
        /// <returns>The <see cref="System.Fabric.DataPackage"/> object from Service Package that matches the desired package name.</returns>
        /// /// <remarks>Throws KeyNotFoundException exception if the package is not found.</remarks>
        DataPackage GetDataPackageObject(string packageName);

        /// <summary>
        /// Retrieves the name of the service manifest.
        /// </summary>
        /// <returns>The name of the service manifest.</returns>
        string GetServiceManifestName();

        /// <summary>
        /// Retrieves the version of the service manifest.
        /// </summary>
        /// <returns>The version of the service manifest.</returns>
        string GetServiceManifestVersion();

        /// <summary>
        /// Reports health for current application. 
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <returns></returns>
        void ReportApplicationHealth(Health.HealthInformation healthInfo);

        /// <summary>
        /// Reports health for current application. 
        /// Specifies options to control how the report is sent.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <returns></returns>
        void ReportApplicationHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions);

        /// <summary>
        /// Reports health for current deployed application. 
        /// </summary>
        /// <param name="healthInfo">Health information that is to be reported.</param>
        /// <returns></returns>
        void ReportDeployedApplicationHealth(Health.HealthInformation healthInfo);

        /// <summary>
        /// Reports health for current deployed application. 
        /// Specifies options to control how the report is sent.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <returns></returns>
        void ReportDeployedApplicationHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions);

        /// <summary>
        /// Reports health for current deployed service package. 
        /// </summary>
        /// <param name="healthInfo">Health information that is to be reported.</param>
        /// <returns></returns>
        void ReportDeployedServicePackageHealth(Health.HealthInformation healthInfo);

        /// <summary>
        /// Reports health for current deployed service package. 
        /// Specifies send options that control how the report is sent to the health store.
        /// </summary>
        /// <param name="healthInfo">The <see cref="System.Fabric.Health.HealthInformation"/> that describes the health report information.</param>
        /// <param name="sendOptions">
        /// <para>The <see cref="System.Fabric.Health.HealthReportSendOptions"/> that controls how the report is sent.</para>
        /// </param>
        /// <returns></returns>
        void ReportDeployedServicePackageHealth(Health.HealthInformation healthInfo, Health.HealthReportSendOptions sendOptions);
    }
}