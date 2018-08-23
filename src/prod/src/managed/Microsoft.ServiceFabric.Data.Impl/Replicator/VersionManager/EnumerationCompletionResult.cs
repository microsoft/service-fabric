// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Threading.Tasks;

    /// <summary>
    /// This is for internal use only.
    /// Used to recieve notification when a relevant visibility sequence number gets freed.
    /// </summary>
    public struct EnumerationCompletionResult
    {
        /// <summary>
        /// This is for internal use only.
        /// Visibility sequence number that the notification will be fired for.
        /// </summary>
        public readonly long VisibilitySequenceNumber;

        /// <summary>
        /// This is for internal use only.
        /// Task that will complete when the relevant visibility sequence number is freed.
        /// </summary>
        public readonly Task<long> Notification;

        /// <summary>
        /// This is for internal use only.
        /// Initializes a new instance of the EnumerationCompletionResult struct.
        /// </summary>
        /// <param name="visibilitySequenceNumber">Visibility sequence number that the notification will be fired for.</param>
        /// <param name="notification">The notification for the visibility sequence number.</param>
        public EnumerationCompletionResult(long visibilitySequenceNumber, Task<long> notification)
        {
            this.VisibilitySequenceNumber = visibilitySequenceNumber;
            this.Notification = notification;
        }
    }
}