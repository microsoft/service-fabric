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
    /// UploadChunkRange contains start and end position of byte chunk
    /// </summary>
    public sealed class UploadChunkRange
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="UploadChunkRange"/> structure
        /// </summary>
        /// <param name="startPosition">
        /// Get the start position of byte chunk from an uploading file.
        /// This parameter cannot be empty, or consist only of whitespace
        /// </param>
        /// <param name="endPosition">
        ///  Gets the end position of byte chunk from an uploading file
        ///  This parameter cannot be empty, or consist only of whitespace
        /// </param>
        internal UploadChunkRange(
            Int64 startPosition,
            Int64 endPosition)
        {
            this.StartPosition = startPosition;
            this.EndPosition = endPosition;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="UploadChunkRange"/> structure
        /// </summary>
        internal UploadChunkRange() { }
 
        /// <summary>
        /// <para>Get the start position of byte chunk from an uploading file.</para>
        /// </summary>
        /// <value>
        /// <para>The start position of byte chunk.</para>
        /// </value>
        public Int64 StartPosition 
        { 
            get; 
            private set; 
        }

        /// <summary>
        /// <para>Gets end position of byte chunk from an uploading file.</para>
        /// </summary>
        /// <value>
        /// <para>The end position of byte chunk.</para>
        /// </value>
        public Int64 EndPosition { get; private set; }
    }
}