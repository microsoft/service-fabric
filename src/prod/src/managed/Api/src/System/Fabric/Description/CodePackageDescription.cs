// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Fabric.Strings;

    /// <summary>
    /// <para>Describes a code package that includes its entry point.</para>
    /// </summary>
    public sealed class CodePackageDescription : PackageDescription
    {
        internal CodePackageDescription()
            : base()
        {
        }

        /// <summary>
        /// <para>Gets the setup entry point for the code package.</para>
        /// </summary>
        /// <value>
        /// <para>The setup entry point for the code package.</para>
        /// </value>
        /// <remarks>
        /// <para>Service Fabric  provides support for an additional entry point for application/service developers to configure and set up the 
        /// environment for their services before the main entry point starts.  </para>
        /// </remarks>
        public ExeHostEntryPointDescription SetupEntryPoint { get; internal set; }

        /// <summary>
        /// <para>Gets the entry point for the code package.</para>
        /// </summary>
        /// <value>
        /// <para>The entry point for the code package.</para>
        /// </value>
        public EntryPointDescription EntryPoint { get; internal set; }

        /// <summary>
        /// Gets a flag indicating whether the code package is shared.
        /// </summary>
        /// <value>Flag indicating whether the code package is shared.</value>
        public bool IsShared { get; internal set; }

        internal static unsafe CodePackageDescription CreateFromNative(NativeTypes.FABRIC_CODE_PACKAGE_DESCRIPTION nativeDescription, string path)
        {
            string packageName = NativeTypes.FromNativeString(nativeDescription.Name);
            AppTrace.TraceSource.WriteNoise("CodePackageDescription.CreateFromNative", "PackageName {0}", packageName);

            ExeHostEntryPointDescription setupEntryPoint = null;
            if (nativeDescription.SetupEntryPoint != IntPtr.Zero)
            {
                setupEntryPoint = ExeHostEntryPointDescription.CreateFromNative(nativeDescription.SetupEntryPoint);
            }

            if (nativeDescription.EntryPoint == IntPtr.Zero)
            {
                AppTrace.TraceSource.WriteError("CodePackageDescription.CreateFromNative", "No EntryPoint");
                throw new ArgumentException(StringResources.Error_NoEntryPoint);
            }
            var entryPoint = EntryPointDescription.CreateFromNative(nativeDescription.EntryPoint);


            return new CodePackageDescription
            {
                Name = packageName,
                Version = NativeTypes.FromNativeString(nativeDescription.Version),
                ServiceManifestName = NativeTypes.FromNativeString(nativeDescription.ServiceManifestName),
                ServiceManifestVersion = NativeTypes.FromNativeString(nativeDescription.ServiceManifestVersion),
                IsShared = NativeTypes.FromBOOLEAN(nativeDescription.IsShared),
#pragma warning disable 618
                Path = path,
#pragma warning restore 618
                SetupEntryPoint = setupEntryPoint,
                EntryPoint = entryPoint,
            };
        }
    }
}