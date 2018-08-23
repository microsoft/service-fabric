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
    /// UploadSession contains information about the image store upload sessions
    /// </summary>
    public sealed class UploadSession
    {
        /// <summary>
        /// Initializes a new instance of the <cref name="UploadSession"/> structure
        /// </summary>
        /// <param name="uploadSessions">
        /// Get the upload session based on the given upload session ID or image store relative path
        /// </param>
        internal UploadSession(
            UploadSessionInfo[] uploadSessions)
        {
            this.UploadSessions = uploadSessions;
        }

        /// <summary>
        /// Initializes a new instance of the <cref name="UploadSession"/> structure
        /// </summary>
        internal UploadSession() { }

        /// <summary>
        /// <para>Get the upload session based on the given upload session ID or image store relative path.</para>
        /// </summary>
        /// <value>
        /// <para>The upload session.</para>
        /// </value>
        public UploadSessionInfo[] UploadSessions { get; private set; }
    }
}