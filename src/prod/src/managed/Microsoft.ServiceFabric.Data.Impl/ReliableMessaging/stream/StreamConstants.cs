// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Messaging.Stream
{
    internal class StreamConstants
    {
        // Guid byte length
        internal const int SizeOfGuid = 16;

        // Default first outbound message sequence number
        internal const long FirstMessageSequenceNumber = 1;

        // TODO: should this be configurable?  Make sure receive operation quota is larger than this
        // Park ReceiveAsync handles for incoming sessions
        internal const int ConcurrentReceiveOperationCount = 10;

        // Stream Protocol Version Info
        internal const int CurrentStreamMajorProtocolVersion = 1;
        internal const int CurrentStreamMinorProtocolVersion = 0;

        internal const long StreamProtocolResponseMessageSequenceNumber = -10;
        internal const long PartnerRecoveryPromptMessageSequenceNumber = -20;
        internal const long InitialValueOfLastSequenceNumberInStream = -1;

        /// <summary>
        /// Wait for this time delay before retrying to resolve the partition endpoint.
        /// </summary>
        internal const int BaseDelayForValidSessionEndpoint = 3000;

        /// <summary>
        /// Max time delay to resolve the partition endpoint.
        /// </summary>
        internal const int MaxDelayForValidSessionEndpoint = 12000;

        internal const int BaseDelayForQuotaExceededRetry = 1000;
        internal const int MaxDelayForQuotaExceededRetry = 16000;

        /// <summary>
        /// Wait for this time delay before retrying replicator(statemanager) operations
        /// </summary>
        internal const int BaseDelayForReplicatorRetry = 100;

        /// <summary>
        /// Max time delay for retrying replicator(statemanager) operation
        /// </summary>
        internal const int MaxDelayForReplicatorRetry = 4000;

        internal const int BaseDelayForPartnerNotReadyRetry = 100;
        internal const int MaxDelayForPartnerNotReadyRetry = 4000;

        internal const int MinRetryIntervalForOpenStream = 10000;

        internal const int RetryCountForDeleteStream = 100;
        internal const int MaxDelayForDeleteStreamRetry = 10000;

        /// <summary>
        /// Expiration set in day units.
        /// @Expiration Hard delete - Inbound stream in a state of 'Deleted'.
        /// </summary>
        internal const int ExpirationForDeletedInboundStream = 14;

        /// <summary>
        /// Register the Stream Manager with this name as a StateProvider in StateManager.
        /// </summary>
        internal const string StreamManagerName = "fabric:/ReliableStreamManagerSingletonStateProvider";
    }
}