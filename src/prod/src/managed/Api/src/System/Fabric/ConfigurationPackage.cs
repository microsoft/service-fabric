// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System;
    using System.Diagnostics.CodeAnalysis;
    using System.Fabric.Common;
    using System.Fabric.Description;
    using System.Fabric.Interop;

    /// <summary>
    /// <para>Represents a configuration package.</para>
    /// </summary>
    public sealed class ConfigurationPackage
    {
        internal ConfigurationPackage()
        {
        }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.ConfigurationPackageDescription" /> 
        /// associated with the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.ConfigurationPackageDescription" /> 
        /// associated with the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </value>
        public ConfigurationPackageDescription Description { get; internal set; }
        
        /// <summary>
        /// <para>Gets the local path for the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The local path for the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </value>
        public string Path { get; internal set; }

        /// <summary>
        /// <para>Gets the <see cref="System.Fabric.Description.ConfigurationSettings" /> 
        /// associated with the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </summary>
        /// <value>
        /// <para>The <see cref="System.Fabric.Description.ConfigurationSettings" /> 
        /// associated with the <see cref="System.Fabric.ConfigurationPackage" />.</para>
        /// </value>
        public ConfigurationSettings Settings { get; internal set; }

        [SuppressMessage(FxCop.Category.Reliability, FxCop.Rule.RemoveCallsToGCKeepAlive, Justification = FxCop.Justification.EnsureNativeBuffersLifetime)]
        internal static unsafe ConfigurationPackage CreateFromNative(NativeRuntime.IFabricConfigurationPackage nativePackage)
        {
            ReleaseAssert.AssertIfNull(nativePackage, "nativePackage");
            
            string path = NativeTypes.FromNativeString(nativePackage.get_Path());
            var settings = ConfigurationSettings.CreateFromNative(nativePackage.get_Settings());

            var nativeDescription = *((NativeTypes.FABRIC_CONFIGURATION_PACKAGE_DESCRIPTION*)nativePackage.get_Description());
            var description = ConfigurationPackageDescription.CreateFromNative(nativeDescription, path, settings);

            var returnValue = new ConfigurationPackage()
            {
                Description = description,
                Path = path,
                Settings = settings
            };

            GC.KeepAlive(nativePackage);
            return returnValue;
        }
    }
}
