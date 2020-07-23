// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System;    
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;    

    /// <summary>
    /// <para>Describes a deployed code package in a container network.</para>
    /// </summary>    
    public sealed class DeployedNetworkCodePackage
    {
        internal DeployedNetworkCodePackage()
        {            
        }

        /// <summary>
        /// <para>Gets the URI name of the application instance.</para>
        /// </summary>
        /// <value>
        /// <para>The URI name of the application instance.</para>
        /// </value>
        public Uri ApplicationName { get; internal set; }

        /// <summary>
        /// <para>Gets the network name where the code package is running.</para>
        /// </summary>
        /// <value>
        /// <para>The network name where the code package is running.</para>
        /// </value>
        public string NetworkName { get; internal set; }
        
        /// <summary>
        /// <para>Gets the code package name.</para>
        /// </summary>
        /// <value>
        /// <para>The code package name.</para>
        /// </value>        
        public string CodePackageName { get; internal set; }

        /// <summary>
        /// <para>Gets the code package version.</para>
        /// </summary>
        /// <value>
        /// <para>The code package version.</para>
        /// </value>        
        public string CodePackageVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        public string ServiceManifestName { get; internal set; }

        /// <summary>
        /// Gets the ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> of deployed service package. 
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedCodePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId { get; internal set; }

        /// <summary>
        /// <para>Gets the address of the container in the container network.</para>
        /// </summary>
        /// <value>
        /// <para>The address of the container in the container network.</para>
        /// </value>
        public string ContainerAddress { get; internal set; }

        /// <summary>
        /// <para>Gets the id of the container.</para>
        /// </summary>
        /// <value>
        /// <para>The id of the container.</para>
        /// </value>
        public string ContainerId { get; internal set; }

        internal static unsafe DeployedNetworkCodePackage CreateFromNative(NativeTypes.FABRIC_DEPLOYED_NETWORK_CODE_PACKAGE_QUERY_RESULT_ITEM nativeDeployedNetworkCodePackage)
        {
            DeployedNetworkCodePackage deployedNetworkCodePackage = new DeployedNetworkCodePackage();

            deployedNetworkCodePackage.ApplicationName = new Uri(NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.ApplicationName));

            deployedNetworkCodePackage.NetworkName = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.NetworkName);

            deployedNetworkCodePackage.CodePackageName = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.CodePackageName);

            deployedNetworkCodePackage.CodePackageVersion = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.CodePackageVersion);

            deployedNetworkCodePackage.ServiceManifestName = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.ServiceManifestName);

            deployedNetworkCodePackage.ServicePackageActivationId = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.ServicePackageActivationId);

            deployedNetworkCodePackage.ContainerAddress = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.ContainerAddress);

            deployedNetworkCodePackage.ContainerId = NativeTypes.FromNativeString(nativeDeployedNetworkCodePackage.ContainerId);

            return deployedNetworkCodePackage;
        }
    }
}
