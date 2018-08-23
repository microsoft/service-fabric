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
    /// Contains information about the image store upload session
    /// </summary>
    public sealed class UploadSessionInfo
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="UploadSessionInfo"/> structure
        /// </summary>
        /// <param name="storeRelativePath">
        /// Get the relative path of image store file which is from the native image store root.
        /// This parameter cannot be empty, or consist only of whitespace
        /// </param>
        /// <param name="sessionId">
        ///  Gets the upload session ID, this session ID cannot be used for two different relative path concurrently. 
        ///  Only if the old session was committed or removed by system, the old session ID can be reused in the new session.
        ///  This parameter cannot be empty
        /// </param>
        /// <param name="modifiedDate">
        /// Gets the date and time when the file was last modified
        /// This parameter cannot be empty
        /// </param>
        /// <param name="fileSize">
        /// Gets the size in byte of the native image store file
        /// This parameter cannot be empty
        /// </param>
        /// <param name="expectedRange">
        /// Get the range of the uploading file that have not been received
        /// </param>
        internal UploadSessionInfo(
            string storeRelativePath,
            Guid sessionId,
            DateTime modifiedDate,
            Int64 fileSize,
            UploadChunkRange[] expectedRange)
        {
            this.StoreRelativePath = storeRelativePath;
            this.SessionId = sessionId;
            this.ModifiedDate = modifiedDate;
            this.FileSize = fileSize;
            this.ExpectedRanges = expectedRange;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="UploadSessionInfo"/> structure
        /// </summary>
        internal UploadSessionInfo() { }

        /// <summary>
        /// <para>Get the relative path of the file from the native image store root.</para>
        /// </summary>
        /// <value>
        /// <para>The relative path of the file from the native image store root.</para>
        /// </value>
        public string StoreRelativePath { get; private set; }

        /// <summary>
        /// <para>Gets the upload session ID.</para>
        /// </summary>
        /// <value>
        /// <para>The upload session ID.</para>
        /// </value>
        public Guid SessionId { get; private set; }

        /// <summary>
        /// <para>Gets the date and time when the file was last modified.</para>
        /// </summary>
        /// <value>
        /// <para>The date and time when the file was last modified.</para>
        /// </value>
        public DateTime ModifiedDate { get; private set; }

        /// <summary>
        /// <para>Gets the size in bytes of the image store file.</para>
        /// </summary>
        /// <value>
        /// <para>The size in bytes of the image store file.</para>
        /// </value>
        public Int64 FileSize { get; private set; }

        /// <summary>
        ///<para>Get the range of the uploading file that have not been received.</para>
        /// </summary>
        /// <value>
        /// <para>The range of uploading file that have not been received.</para>
        /// </value>
        public UploadChunkRange[] ExpectedRanges { get; private set; }
    }
}