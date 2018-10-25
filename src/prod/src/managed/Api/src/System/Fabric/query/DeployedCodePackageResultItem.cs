// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a deployed code package.</para>
    /// </summary>
    public sealed class DeployedCodePackage
    {
        internal DeployedCodePackage(
            string codePackageName,
            string codePackageVersion,
            CodePackageEntryPoint setupEntryPoint,
            string serviceManifestName,
            string servicePackageActivationId,
            long runFrequencyInterval,
            HostType hostType,
            HostIsolationMode hostIsolationMode,
            DeploymentStatus deployedCodePackageStatus,
            CodePackageEntryPoint entryPoint)
        {
            this.CodePackageName = codePackageName;
            this.CodePackageVersion = codePackageVersion;
            this.SetupEntryPoint = setupEntryPoint;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
            this.RunFrequencyInterval = runFrequencyInterval;
            this.HostType = hostType;
            this.HostIsolationMode = hostIsolationMode;
            this.DeployedCodePackageStatus = deployedCodePackageStatus;
            this.EntryPoint = entryPoint;
        }

        private DeployedCodePackage() { }

        /// <summary>
        /// <para>Gets the code package name.</para>
        /// </summary>
        /// <value>
        /// <para>The code package name.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Name)]
        public string CodePackageName { get; private set; }

        /// <summary>
        /// <para>Gets the code package version.</para>
        /// </summary>
        /// <value>
        /// <para>The code package version.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Version)]
        public string CodePackageVersion { get; private set; }

        /// <summary>
        /// <para>Gets the setup entry point.</para>
        /// </summary>
        /// <value>
        /// <para>The setup entry point.</para>
        /// </value>
        public CodePackageEntryPoint SetupEntryPoint { get; private set; }

        /// <summary>
        /// <para>Gets the service manifest name.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest name.</para>
        /// </value>
        public string ServiceManifestName { get; private set; }

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
        public string ServicePackageActivationId { get; private set; }

        /// <summary>
        /// <para>Gets the run frequency interval.</para>
        /// </summary>
        /// <value>
        /// <para>The run frequency interval.</para>
        /// </value>
        public long RunFrequencyInterval { get; private set; }

        /// <summary>
        /// <para>Gets the code package status.</para>
        /// </summary>
        /// <value>
        /// <para>The status of the deployed code package.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.Status)]
        public DeploymentStatus DeployedCodePackageStatus { get; private set; }

        /// <summary>
        /// <para>Get the <see cref="System.Fabric.HostType"/> for main entry point.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.HostType"/> of main entry point.</para>
        /// </value>
        public HostType HostType { get; private set; }

        /// <summary>
        /// <para>Get the <see cref="System.Fabric.HostIsolationMode"/> for main entry point.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.HostIsolationMode"/> of main entry point.</para>
        /// </value>
        public HostIsolationMode HostIsolationMode { get; private set; }

        /// <summary>
        /// <para>Gets the main entry point.</para>
        /// </summary>
        /// <value>
        /// <para>The main entry point.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.MainEntryPoint)]
        public CodePackageEntryPoint EntryPoint { get; private set; }

        // Matching the properties with it's REST representation
        private bool HasSetupEntryPoint { get; set; }

        internal static unsafe DeployedCodePackage CreateFromNative(
           NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM nativeResultItem)
        {
            var servicePackageActivationId = string.Empty;
            var hostType = HostType.Invalid;
            var hostIsolationMode = HostIsolationMode.None;

            if (nativeResultItem.Reserved != IntPtr.Zero)
            {
                var nativeResultItemEx1 =
                    (NativeTypes.FABRIC_DEPLOYED_CODE_PACKAGE_QUERY_RESULT_ITEM_EX1*)nativeResultItem.Reserved;

                servicePackageActivationId = NativeTypes.FromNativeString(nativeResultItemEx1->ServicePackageActivationId);
                hostType = (HostType)nativeResultItemEx1->HostType;
                hostIsolationMode = (HostIsolationMode) nativeResultItemEx1->HostIsolationMode;
            }
            
            return new DeployedCodePackage(
                NativeTypes.FromNativeString(nativeResultItem.CodePackageName),
                NativeTypes.FromNativeString(nativeResultItem.CodePackageVersion),
                (nativeResultItem.SetupEntryPoint == IntPtr.Zero) ? null : CodePackageEntryPoint.FromNative(*(NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT*)nativeResultItem.SetupEntryPoint),
                NativeTypes.FromNativeString(nativeResultItem.ServiceManifestName),
                servicePackageActivationId,
                nativeResultItem.RunFrequencyInterval,
                hostType,
                hostIsolationMode,
                (DeploymentStatus)nativeResultItem.DeployedCodePackageStatus,
                CodePackageEntryPoint.FromNative(*(NativeTypes.FABRIC_CODE_PACKAGE_ENTRY_POINT*)nativeResultItem.EntryPoint));
        }
    }
}