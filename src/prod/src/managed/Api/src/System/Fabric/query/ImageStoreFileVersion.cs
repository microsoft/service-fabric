// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Query
{
    using System.Fabric;
    using System.Fabric.Common.Serialization;
    using System.Fabric.Health;
    using System.Fabric.Interop;
    using System.Numerics;

    /// <summary>
    /// A ImageStoreFileVersion contains version information about a native image store file
    /// </summary>
    public sealed class ImageStoreFileVersion
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFileVersion"/> structure
        /// </summary>
        /// <param name="epochDataLossNumber">
        /// The epoch number of the image store file for the accepting known data loss
        /// </param>
        /// <param name="versionNumber">
        /// The internal version of the image store file
        /// </param>
        internal ImageStoreFileVersion(
            Int64 epochDataLossNumber,
            Int64 versionNumber)
        { 
            this.EpochDataLossNumber = epochDataLossNumber;
            this.VersionNumber = versionNumber;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="ImageStoreFileVersion"/> structure
        /// </summary>
        private ImageStoreFileVersion() { }

        /// <summary>
        /// <para>Gets the epoch number of the image store file indicating the accepting known data loss.</para>
        /// </summary>
        /// <value>
        /// <para>The epoch data loss number of the image store file.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.EpochDataLossNumber)]
        public Int64 EpochDataLossNumber { get; private set; }

        /// <summary>
        /// <para>Gets the internal version number of the image store file.</para>
        /// </summary>
        /// <value>
        /// <para>The internal version number of the image store file.</para>
        /// </value>
        [JsonCustomization(PropertyName = JsonPropertyNames.VersionNumber)]
        public Int64 VersionNumber { get; private set; }

    }
}