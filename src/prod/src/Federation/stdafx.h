// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Federation/Federation.h"

#include "client/HealthClient.h"
#include "LeaseAgent/LeaseAgent.h"

#include "Transport/UnreliableTransportSpecification.h"
#include "Transport/UnreliableTransportConfig.h"

// Headers internal to Federation
#include "Federation/FederationConfig.h"
#include "Federation/ThrottleManager.h"
#include "Federation/NodeIdRangeTable.h"
#include "Federation/GlobalTimeExchangeHeader.h"
#include "Federation/GlobalTimeManager.h"
#include "Federation/BroadcastReplyContext.h"
#include "Federation/StateMachineAction.h"
#include "Federation/NodeRing.h"
#include "Federation/SiteNode.h"

#include "Federation/GlobalLease.h"
#include "Federation/TicketTransfer.h"
#include "Federation/VoteRequest.h"
#include "Federation/UpdateRequestBody.h"
#include "Federation/UpdateReplyBody.h"
#include "Federation/NeighborhoodQueryRequestBody.h"
#include "Federation/LivenessQueryBody.h"
#include "Federation/BroadcastHeader.h"
#include "Federation/PToPActor.h"
#include "Federation/PToPHeader.h"
#include "Federation/ArbitrationType.h"
#include "Federation/ArbitrationRequestBody.h"
#include "Federation/ArbitrationTable.h"
#include "Federation/ArbitrationReplyBody.h"
#include "Federation/RoutingHeader.h"
#include "Federation/LockIdBody.h"
#include "Federation/FederationNeighborhoodRangeHeader.h"
#include "Federation/FederationTokenEchoHeader.h"
#include "Federation/BroadcastRangeHeader.h"
#include "Federation/BroadcastRelatesToHeader.h"
#include "Federation/BroadcastStepHeader.h"
#include "Federation/MulticastHeader.h"
#include "Federation/MulticastTargetsHeader.h"
#include "Federation/MulticastForwardContext.h"
#include "Federation/NodeDoesNotMatchFaultBody.h"
#include "Federation/FederationRoutingTokenHeader.h"
#include "Federation/TraceProbeNode.h"
#include "Federation/FederationTraceProbeHeader.h"
#include "Federation/FederationNeighborhoodVersionHeader.h"
#include "Federation/EdgeProbeMessageBody.h"
#include "Federation/SiteNodeSavedInfo.h"
#include "Federation/FabricCodeVersionHeader.h"
#include "Federation/FederationPartnerNodeHeader.h"
#include "Federation/JoinThrottleHeader.h"
#include "Federation/FederationMessage.h"

#include "Federation/Multicast.h"
#include "Federation/SendMessageAction.h"
#include "Federation/NodeRing.h"
#include "Federation/RoutingTable.h"
#include "Federation/JoinLock.h"
#include "Federation/JoinLockManager.h"
#include "Federation/VoteEntry.h"
#include "Federation/RoutedOneWayReceiverContext.h"
#include "Federation/RoutedRequestReceiverContext.h"
#include "Federation/VoteManager.h"
#include "Federation/JoinManager.h"
#include "Federation/PingManager.h"
#include "Federation/PointToPointManager.h"
#include "Federation/RoutingManager.h"
#include "Federation/BroadcastForwardContext.h"
#include "Federation/UpdateManager.h"
#include "Federation/WindowsAzureProxy.h"

#include "Federation/MulticastManager.h"
#include "Federation/MulticastAckReceiverContext.h"
#include "Federation/MulticastAckBody.h"

#include "Federation/BroadcastManager.h"
#include "Federation/RouteAsyncOperation.h"
#include "Federation/BroadcastAckReceiverContext.h"
#include "Federation/BroadcastRequestReceiverContext.h"

#include "Federation/VoterStoreHeader.h"
#include "Federation/VoterStore.h"

#include "Federation/GlobalStore.h"

#include "Federation/FederationEventSource.h"



