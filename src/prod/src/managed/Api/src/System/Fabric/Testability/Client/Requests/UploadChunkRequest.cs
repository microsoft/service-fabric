// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Testability.Client.Requests
{
    using System;
    using System.Fabric.Description;
    using System.Fabric.Testability.Client.Structures;
    using System.Fabric.Testability.Common;
    using System.Globalization;
    using System.Text;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// This class derives FabricRequest class and my represent a request which is specific to copy image store content operation
    /// The derived classes needs to implement ToString() and PerformCoreAsync() methods defined in FabricRequest.
    /// </summary>
    public class UploadChunkRequest : FabricRequest
    {
        /// <summary>
        /// Initializes a new instance of the UploadChunkRequest
        /// </summary>
        /// <param name="fabricClient"></param>
        /// <param name="remoteLocation">Get image store relative path</param>
        /// <param name="sessionId">Get the upload session ID and allow invalid session ID for negative testing scenario</param>
        /// <param name="startPosition">Get the start position of byte chunk from a uploading file</param>
        /// <param name="endPosition">Get the end position of byte chunk from a upload file</param>
        /// <param name="filePath">Get the local file to be uploaded</param>
        /// <param name="fileSize">Get the size in byte of the file to be uploaded</param>
        /// <param name="timeout"></param>
        public UploadChunkRequest(
            IFabricClient fabricClient,
            string remoteLocation,
            string sessionId,
            UInt64 startPosition,
            UInt64 endPosition,
            string filePath,
            UInt64 fileSize,
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.NullOrEmpty(remoteLocation, "remoteLocation");
            ThrowIf.NullOrEmpty(filePath, "filePath");
            ThrowIf.IsFalse(startPosition <= endPosition, "startPosition:{0},endPosition:{1}", startPosition, endPosition);

            this.RemoteLocation = remoteLocation;
            this.SessionId = sessionId;
            this.StartPosition = startPosition;
            this.EndPosition = endPosition;
            this.FilePath = filePath;
            this.FileSize = fileSize;
        }

        /// <summary>
        /// Initializes a new instance of the UploadChunkRequest
        /// </summary>
        /// <param name="fabricClient"></param>
        /// <param name="remoteLocation">Get image store relative path</param>
        /// <param name="sessionId">Get the upload session ID</param>
        /// <param name="startPosition">Get the start position of byte chunk from a uploading file</param>
        /// <param name="endPosition">Get the end position of byte chunk from a upload file</param>
        /// <param name="filePath">Get the local file to be uploaded</param>
        /// <param name="fileSize">Get the size in byte of the file to be uploaded</param>
        /// <param name="timeout"></param>
        public UploadChunkRequest(
            IFabricClient fabricClient, 
            string remoteLocation,
            Guid sessionId,
            UInt64 startPosition,
            UInt64 endPosition,
            string filePath,
            UInt64 fileSize,
            TimeSpan timeout)
            : this(fabricClient, remoteLocation, sessionId.ToString(), startPosition, endPosition, filePath, fileSize, timeout)
        {
            ThrowIf.Empty(sessionId, "sessionId");
        }

        /// <summary>
        /// Get image store remote location
        /// </summary>
        public string RemoteLocation
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the upload session ID
        /// </summary>
        public string SessionId
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the start position of byte chunk
        /// </summary>
        public UInt64 StartPosition
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the end position of byte chunk
        /// </summary>
        public UInt64 EndPosition
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the souce file path
        /// </summary>
        public string FilePath
        {
            get;
            private set;
        }

        /// <summary>
        /// Get the size of the file to be uploaded
        /// </summary>
        public UInt64 FileSize
        {
            get;
            private set;
        }
       

        /// <summary>
        /// A string represent the UploadChunkRequest object
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
            CultureInfo.InvariantCulture,
            "StoreRelativePath:{0},SessionId:{1},StartPosition:{2},EndPosition:{3},SourceFilePath:{4},FileSize:{5}",
            this.RemoteLocation,
            this.SessionId,
            this.StartPosition,
            this.EndPosition,
            this.FilePath,
            this.FileSize);
        }

        /// <summary>
        /// Perform image store copying operation with async
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.UploadChunkAsync(
                this.RemoteLocation,
                this.SessionId.ToString(),
                this.StartPosition,
                this.EndPosition,
                this.FilePath,
                this.FileSize,
                this.Timeout, 
                cancellationToken);
        }
    }
}