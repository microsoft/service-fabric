// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>
    /// Describes a deployed service package. This can be obtained by using query 
    /// <see cref="System.Fabric.FabricClient.QueryClient.GetDeployedServicePackageListAsync(string, System.Uri)"/>
    /// which return a list of <see cref="System.Fabric.Query.DeployedServicePackage"/> on a given node.
    /// </para>
    /// </summary>
    public sealed class DeployedServicePackage
    {
        internal DeployedServicePackage(
            string serviceManifestName,
            string servicePackageActivationId,
            string serviceManifestVersion, 
            DeploymentStatus deploymentStatus)
        {
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
            this.ServiceManifestVersion = serviceManifestVersion;
            this.DeployedServicePackageStatus = deploymentStatus;
        }

        private DeployedServicePackage() { }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string ServiceManifestName { get; private set; }

        /// <summary>
        /// Gets the ActivationId of service package.
        /// </summary>
        /// <value>
        /// <para>
        /// A string representing ActivationId of deployed service package. 
        /// If <see cref="System.Fabric.Description.ServicePackageActivationMode"/> specified at the time of creating the service is 
        /// <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/> (or if it is not specified, in
        /// which case it defaults to <see cref="System.Fabric.Description.ServicePackageActivationMode.SharedProcess"/>), then value of 
        /// <see cref="System.Fabric.Query.DeployedServicePackage.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest version.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest version.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Version)]
        public string ServiceManifestVersion { get; private set; }

        /// <summary>
        /// <para>Gets the deployed service package status.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the deployed service package.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public DeploymentStatus DeployedServicePackageStatus { get; private set; }

        internal static unsafe DeployedServicePackage CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM nativeResultItem)
        {
            var servicePackageActivationId = string.Empty;
            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_SERVICE_PACKAGE_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;

                servicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx1->ServicePackageActivationId);
            }

            return new DeployedServicePackage(
                    NativeTypes.FromNativeString(nativeResultItem.ServiceManifestName),
                    servicePackageActivationId,
                    NativeTypes.FromNativeString(nativeResultItem.ServiceManifestVersion),
                    (DeploymentStatus)nativeResultItem.DeployedServicePackageStatus);
        }
    }
}