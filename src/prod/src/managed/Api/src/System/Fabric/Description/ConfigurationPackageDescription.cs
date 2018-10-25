// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Describes a configuration package.</para>
    /// </summary>
    public sealed class ConfigurationPackageDescription : PackageDescription
    {
        internal ConfigurationPackageDescription()
        {
        }

        /// <summary>
        /// <para>DEPRECATED. Gets the parsed configuration settings from the configuration package.</para>
        /// </summary>
        /// <value>
        /// <para>The parsed configuration settings from the configuration package.</para>
        /// </value>
        /// <remarks>This property is obsolete. Use Settings property of System.Fabric.ConfigurationPackage type instead.</remarks>
        [Obsolete("Use Settings property of System.Fabric.ConfigurationPackage type.")]
        public ConfigurationSettings Settings { get; internal set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ConfigurationPackageDescription CreateFromNative(
            NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION nativeDescription,
            string path,
            ConfigurationSettings settings)
        {
            string packageName = NativeTypes.FromNativeString(nativeDescription.Name);
            AppTrace.TraceSource.WriteNoise("DataPackageDescription.CreateFromNative", "PackageName {0}", packageName);

            return new ConfigurationPackageDescription
            {
                Name = packageName,
                Version = NativeTypes.FromNativeString(nativeDescription.Version),
                ServiceManifestName = NativeTypes.FromNativeString(nativeDescription.ServiceManifestName),
                ServiceManifestVersion = NativeTypes.FromNativeString(nativeDescription.ServiceManifestVersion),
#pragma warning disable 618
                Path = path,
                Settings = settings
#pragma warning restore 618
            };
        }
    }
}