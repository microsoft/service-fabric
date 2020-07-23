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
    /// This class derives FabricRequest class and represents a request which is specific to commit upload session to image store operation
    /// The derived classes need to implement ToString() and PerformCoreAsync() methods defined in FabricRequest.
    /// </summary>
    public class CommitUploadSessionRequest : FabricRequest
    {
        /// <summary>
        /// Initializes a new instance of the CommitUploadSessionRequest
        /// </summary>
        public CommitUploadSessionRequest(
            IFabricClient fabricClient, 
            Guid sessionId, 
            TimeSpan timeout)
            : base(fabricClient, timeout)
        {
            ThrowIf.Empty(sessionId, "sessionId");
            this.SessionId = sessionId;
        }

        /// <summary>
        /// Get the upload session ID
        /// </summary>
        public Guid SessionId
        {
            get;
            private set;
        }

        /// <summary>
        /// A string represent the CommitUploadSessionRequest object
        /// </summary>
        /// <returns></returns>
        public override string ToString()
        {
            return string.Format(
            CultureInfo.InvariantCulture,
            "Move upload session {0}",
            this.SessionId);
        }

        /// <summary>
        /// Perform upload session commit operation with async
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public override async Task PerformCoreAsync(CancellationToken cancellationToken)
        {
            this.OperationResult = await this.FabricClient.CommitUploadSessionAsync(this.SessionId, this.Timeout, cancellationToken);
        }
    }
}