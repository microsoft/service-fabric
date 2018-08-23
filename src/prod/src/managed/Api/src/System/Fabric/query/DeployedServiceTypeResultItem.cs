// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric.Description;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Specifies the service type registration status.</para>
    /// </summary>
    public enum ServiceTypeRegistrationStatus
    {
        /// <summary>
        /// <para>Invalid.</para>
        /// </summary>
        Invalid = NativeTypes.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_INVALID,
        /// <summary>
        /// <para>Service type is disabled.</para>
        /// </summary>
        Disabled = NativeTypes.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_DISABLED,
        /// <summary>
        /// <para>Service type is not registered.</para>
        /// </summary>
        NotRegistered = NativeTypes.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_NOT_REGISTERED,
        /// <summary>
        /// <para>Service type is registered.</para>
        /// </summary>
        Registered = NativeTypes.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS.FABRIC_SERVICE_TYPE_REGISTRATION_STATUS_REGISTERED
    }

    /// <summary>
    /// <para>Represents a deployed service type.</para>
    /// </summary>
    public sealed class DeployedServiceType
    {
        internal DeployedServiceType(
            string serviceTypeName, 
            string codePackageName, 
            string serviceManifestName,
            string servicePackageActivationId,
            ServiceTypeRegistrationStatus serviceTypeRegistrationStatus)
        {
            this.ServiceTypeName = serviceTypeName;
            this.CodePackageName = codePackageName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
            this.ServiceTypeRegistrationStatus = serviceTypeRegistrationStatus;
        }

        private DeployedServiceType() { }

        /// <summary>
        /// <para>Gets the service type name.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service type.</para>
        /// </value>
        public string ServiceTypeName { get; private set; }

        /// <summary>
        /// <para>Gets the code package name.</para>
        /// </summary>
        /// <value>
        /// <para>The code package name.</para>
        /// </value>
        public string CodePackageName { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service manifest.</para>
        /// </value>
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
        /// <see cref="System.Fabric.Query.DeployedServiceType.ServicePackageActivationId"/> is always an empty string.
        /// For more details please see <see cref="System.Fabric.Description.ServicePackageActivationMode"/>.
        /// </para>
        /// </value>
        public string ServicePackageActivationId { get; private set; }

        /// <summary>
        /// <para>Gets the service type registration status.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the service type registration.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public ServiceTypeRegistrationStatus ServiceTypeRegistrationStatus { get; private set; }

        internal static unsafe DeployedServiceType CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM nativeResultItem)
        {
            var servicePackageActivationId = string.Empty;
            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_SERVICE_TYPE_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;

                servicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx1->ServicePackageActivationId);
            }
            
            return new DeployedServiceType(
                NativeTypes.FromNativeString(nativeResultItem.ServiceTypeName),
                NativeTypes.FromNativeString(nativeResultItem.CodePackageName),
                NativeTypes.FromNativeString(nativeResultItem.ServiceManifestName),
                servicePackageActivationId,
                (ServiceTypeRegistrationStatus)nativeResultItem.Status);
        }
    }
}