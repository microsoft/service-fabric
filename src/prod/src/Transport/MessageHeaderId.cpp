// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Transport
{
    namespace MessageHeaderId
    {
        void WriteToTextWriter(Common::TextWriter & w, Enum const & enumValue)
        {
            switch (enumValue)
            {
            case INVALID_SYSTEM_HEADER_ID: w << "INVALID_SYSTEM_HEADER_ID"; return;

            // Transport level headers.
            case Actor: w << "Actor"; return;
            case Action: w << "Action"; return;
            case MessageId: w << "MessageId"; return;
            case RelatesTo: w << "RelatesTo"; return;
            case ExpectsReply: w << "ExpectsReply"; return;
            case Retry: w << "Retry"; return;
            case Fault: w << "Fault"; return;
            case Idempotent: w << "Idempotent"; return;
            case HighPriority: w << "HighPriority"; return;
            case REFrom: w << "REFrom"; return;
            case Ipc: w << "Ipc"; return;

            // Federation Protocol (Federation) headers
            case FederationPartnerNode: w << "FederationPartnerNode"; return;
            case FederationNeighborhoodRange: w << "FederationNeighborhoodRange"; return;
            case FederationNeighborhoodVersion: w << "FederationNeighborhoodVersion"; return;
            case FederationRoutingToken: w << "FederationRoutingToken"; return;
            case Routing: w << "Routing"; return;
            case FederationTraceProbe: w << "FederationTraceProbe"; return;
            case FederationTokenEcho: w << "FederationTokenEcho"; return;
            case GlobalTimeExchange: w << "GlobalTimeExchange"; return;
            case VoterStore: w << "VoterStore"; return;

            // Point to Point (PToP) Headers
            case PToP: w << "PToP"; return;

            // Broadcast Headers
            case Broadcast: w << "Broadcast"; return;
            case BroadcastRange: w << "BroadcastRange"; return;
            case BroadcastRelatesTo: w << "BroadcastRelatesTo"; return;
            case BroadcastStep: w << "BroadcastStep"; return;

            // Reliability Headers
            case Generation: w << "Generation"; return;

            // Replication
            case ReplicationActor: w << "ReplicationActor"; return;
            case ReplicationOperation: w << "ReplicationOperation"; return;
            case ReplicationOperationBody: w << "ReplicationOperationBody"; return;
            case CopyOperation: w << "CopyOperation"; return;
            case CompletedLSN: w << "CompletedLSN"; return;
            case CopyContextOperation: w << "CopyContextOperation"; return;
            case OperationAck: w << "OperationAck"; return;
            case OperationError: w << "OperationError"; return;

            // System Services (Common)
            case FabricActivity: w << "FabricActivity"; return;
            case RequestInstance: w << "RequestInstance"; return;
            case SystemServiceFilter: w << "SystemServiceFilter"; return;
            case Timeout: w << "Timeout"; return;

            // Naming Service
            case CacheMode: w << "CacheMode"; return;
            case ClientProtocolVersion: w << "ClientProtocolVersion"; return;
            case GatewayRetry: w << "GatewayRetry"; return;
            case PrimaryRecovery: w << "PrimaryRecovery"; return;
            case DeleteName: w << "DeleteName"; return;

            // Cluster Manager Service
            case ForwardMessage: w << "ForwardMessage"; return;
            case PartitionTarget: w << "PartitionTarget"; return;

            // Security Headers
            case MessageSecurity: w << "MessageSecurity"; return;
            case CustomClientAuth: w << "CustomClientAuth"; return;

            // Query Address                
            case QueryAddress: w << "QueryAddress"; return;

            case FabricCodeVersion: w << "FabricCodeVersion"; return;

            case ServiceRoutingAgent: w << "ServiceRoutingAgent"; return;
            case ServiceRoutingAgentProxy: w << "ServiceRoutingAgentProxy"; return;

            case NamingProperty: w << "NamingProperty"; return;
            case SecondaryLocations: w << "SecondaryLocations"; return;
            case ClientRole: w << "ClientRole"; return;

            case Multicast: w << "Multicast"; return;
            case MulticastTargets: w << "MulticastTargets"; return;

            case FileUploadRequest: w << "FileUploadRequest"; return;
            case FileSequence: w << "FileSequence"; return;

            case ServiceTarget: w << "ServiceTarget"; return;

            case UncorrelatedReply: w << "UncorrelatedReply"; return;

            case ServiceDirectMessaging: w << "ServiceDirectMessaging"; return;

            case ClientIdentity: w << "ClientIdentity"; return;

            case ServiceLocationActor: w << "ServiceLocationActor"; return;
            case TcpServiceMessageHeader: w << "TcpServiceMessageHeader"; return;
            case TcpClientIdHeader: w << "TcpClientIdHeader"; return;
            case ServiceCommunicationError: w << "ServiceCommunicationErrorHeader" ; return;

            case JoinThrottle: w << "JoinThrottle"; return;
            case CreateComposeDeploymentRequest: w << "CreateComposeDeploymentRequest"; return;
            case FabricTransportMessageHeader: w << "FabricTransportMessageHeader"; return;
            case UpgradeComposeDeploymentRequest: w << "UpgradeComposeDeploymentRequest"; return;
            case CreateVolumeRequest: w << "CreateVolumeRequest"; return;
            case FileUploadCreateRequest: w << "FileUploadCreateRequest"; return;

            // Header IDs for tests follow this line.
            case Example: w << "Example"; return;
            case Example2: w << "Example2"; return;
            case Example3: w << "Example3"; return;

            default: w << "MessageHeaderId(" << static_cast<int>(enumValue) << ')';
            }
        }
    }
}
