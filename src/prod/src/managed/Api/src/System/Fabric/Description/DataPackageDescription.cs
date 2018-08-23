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
    /// <para>Describes a data package. </para>
    /// </summary>
    public sealed class DataPackageDescription : PackageDescription
    {
        internal DataPackageDescription()
        {
        }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe DataPackageDescription CreateFromNative(NativeTypes.FABRIC_DATA_PACKAGE_DESCRIPTION nativeDescription, string path)
        {
            string packageName = NativeTypes.FromNativeString(nativeDescription.Name);
            AppTrace.TraceSource.WriteNoise("DataPackageDescription.CreateFromNative", "PackageName {0}", packageName);

            return new DataPackageDescription
            {
                Name = packageName,
                Version = NativeTypes.FromNativeString(nativeDescription.Version),
                ServiceManifestName = NativeTypes.FromNativeString(nativeDescription.ServiceManifestName),
                ServiceManifestVersion = NativeTypes.FromNativeString(nativeDescription.ServiceManifestVersion),
#pragma warning disable 618
                Path = path,
#pragma warning restore 618
            };
        }
    }
}