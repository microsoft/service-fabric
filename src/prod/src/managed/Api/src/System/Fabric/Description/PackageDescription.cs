// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Description
{
    using System;

    /// <summary>
    /// <para>Represents a base class for all package descriptions.</para>
    /// </summary>
    public abstract class PackageDescription
    {
        internal PackageDescription()
        {
        }

        /// <summary>
        /// <para>Gets the name of the package.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the package.</para>
        /// </value>
        public string Name { get; internal set; }

        /// <summary>
        /// <para>Gets the version of the package.</para>
        /// </summary>
        /// <value>
        /// <para>The version of the package.</para>
        /// </value>
        public string Version { get; internal set; }

        /// <summary>
        /// <para>Gets the name of the service manifest.</para>
        /// </summary>
        /// <value>
        /// <para>The name of the service manifest.</para>
        /// </value>
        public string ServiceManifestName { get; internal set; }

        /// <summary>
        /// <para>Gets the service manifest version.</para>
        /// </summary>
        /// <value>
        /// <para>The service manifest version.</para>
        /// </value>
        public string ServiceManifestVersion { get; internal set; }

        /// <summary>
        /// <para>Gets the path to the package.</para>
        /// </summary>
        /// <value>
        /// <para>The path to the package.</para>
        /// </value>
        /// <remarks>This property is obsolete.
        /// Use Path property in <see cref="System.Fabric.CodePackage"/>, <see cref="System.Fabric.ConfigurationPackage"/> or <see cref="System.Fabric.DataPackage"/> types.</remarks>
        [Obsolete("Use Path property in System.Fabric.CodePackage, System.Fabric.ConfigurationPackage or System.Fabric.DataPackage types.")]
        public string Path { get; internal set; }
    }
}