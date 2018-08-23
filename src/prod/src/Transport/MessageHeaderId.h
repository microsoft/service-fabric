// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    namespace MessageHeaderId
    {
        // Disable the warning about Enum base type, since it is already part of C++0X.
        #pragma warning( push )
        #pragma warning( disable : 4480 )

        // !!! For backward compatibility, please do NOT delete existing values or insert new values !!!
        // !!! Always append new values to the end (before SYSTEM_HEADER_ID_END). !!!
        // !!! When adding a new enum value here, please also add its friendly string !!!
        // !!! name to MessageHeaderId.cpp so that we can dump enum value in trace.   !!!
        enum Enum : unsigned short
        {
            INVALID_SYSTEM_HEADER_ID = 0x8000,
            // Transport level headers.
            Actor = 0x8001,
            Action = 0x8002,
            MessageId = 0x8003,
            RelatesTo = 0x8004,
            ExpectsReply = 0x8005,
            Retry = 0x8006,
            Fault = 0x8007,
            Idempotent = 0x8008,
            HighPriority = 0x8009,
            REFrom = 0x800a,
            Ipc = 0x800b,

            // Federation Protocol (Federation) headers
            FederationPartnerNode = 0x800c,
            FederationNeighborhoodRange = 0x800d,
            FederationNeighborhoodVersion = 0x800e,
            FederationRoutingToken = 0x800f,
            Routing = 0x8010,
            FederationTraceProbe = 0x8011,
            FederationTokenEcho = 0x8012,

            // Point to Point (PToP) Headers
            PToP = 0x8013,

            // Broadcast Headers
            Broadcast = 0x8014,
            BroadcastRange = 0x8015,
            BroadcastRelatesTo = 0x8016,
            BroadcastStep = 0x8017,

            // Reliability
            Generation = 0x8018,

            // Replication
            ReplicationActor = 0x8019,
            ReplicationOperation = 0x801a,
            CopyOperation = 0x801b,
            CompletedLSN = 0x801c,
            CopyContextOperation = 0x801d,
            OperationAck = 0x801e,
            OperationError = 0x801f,

            // System Services (Common)
            FabricActivity = 0x8020,
            RequestInstance = 0x8021,
            SystemServiceFilter = 0x8022,
            Timeout = 0x8023,

            // Naming Service
            CacheMode = 0x8024,
            ClientProtocolVersion = 0x8025,
            GatewayRetry = 0x8026,
            PrimaryRecovery = 0x8027,

            // Cluster Manager Service
            ForwardMessage = 0x8028,

            // Security headers
            MessageSecurity = 0x8029,

            // Query address header
            QueryAddress = 0x802a,

            FabricCodeVersion = 0x802b,

            ServiceRoutingAgent = 0x802c,
            ServiceRoutingAgentProxy = 0x802d,

            // Reliable Messaging

            ReliableMessagingSession = 0x802e,
            ReliableMessagingSource = 0x802f,
            ReliableMessagingTarget = 0x8030,
            ReliableMessagingProtocolResponse = 0x8031,
            ReliableMessagingSessionParams = 0x8032,

            DeleteName = 0x8033,

            PartitionTarget = 0x8034,
            CustomClientAuth = 0x8035,
            NamingProperty = 0x8036,
            SecondaryLocations = 0x8037,
            ClientRole = 0x8038,

            Multicast = 0x8039,
            MulticastTargets = 0x803a,

            FileUploadRequest = 0x803b,
            FileSequence = 0x803c,

            ServiceTarget = 0x803d,

            UncorrelatedReply = 0x803e,

            ServiceDirectMessaging = 0x803f,

            ClientIdentity = 0x8040,
            ServerAuth = 0x8041,

            GlobalTimeExchange = 0x8041,
            VoterStore = 0x8042,

            //Service Tcp Communication
            ServiceLocationActor = 0x8043,
            TcpServiceMessageHeader = 0x8044,
            TcpClientIdHeader = 0x8045,
            ServiceCommunicationError = 0x8046,
            IsAsyncOperationHeader = 0x8047,

            SecurityNegotiation = 0x8048,

            JoinThrottle = 0x8049,

            // Replication
            ReplicationOperationBody = 0x804a,

            CreateComposeDeploymentRequest = 0x804b,
            // To be compatible with v6.0 which was using 0x804c conflicted with FabricTransportMessageHeader. From v6.1, the header id would be 0x804d.
            UpgradeComposeDeploymentRequest_Compatibility = 0x804c,
            //Fabric Transport V2
            FabricTransportMessageHeader = 0x804c,
            UpgradeComposeDeploymentRequest = 0x804d,
            CreateVolumeRequest = 0x804e,
            FileUploadCreateRequest = 0x804f,

            // Add new internal message header ids must be explicitly defined
            // ----------------------------------------------------------------
            // Header IDs for tests follow this line.

            Example,
            Example2,
            Example3,
            MulticastTest_TargetSpecificHeader,

            // !!! Mark the end of header ID definition !!!
            SYSTEM_HEADER_ID_END
        };

        #pragma warning( pop )

        void WriteToTextWriter(Common::TextWriter & w, Enum const & enumValue);
    }
}
