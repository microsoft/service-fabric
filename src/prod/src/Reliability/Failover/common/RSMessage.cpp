// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Common.Stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;

// Actors for all messages sent to the FailoverManager
Transport::Actor::Enum const RSMessage::FMActor(Transport::Actor::FM);

// Actors for all messages sent to the FailoverManagerMaster
Transport::Actor::Enum const RSMessage::FMMActor(Transport::Actor::FMM);

// Actors for all messages sent to the Realibility Subsystem
Transport::Actor::Enum const RSMessage::RSActor(Transport::Actor::RS);

// Actors for all messages sent to the ReconfigurationAgent
Transport::Actor::Enum const RSMessage::RaActor(Transport::Actor::RA);

// Actors for all messages sent to the ServiceResover
Transport::Actor::Enum const RSMessage::ServiceResolverActor(Transport::Actor::ServiceResolver);

//
// List of Messages used by the Reliability Subsystem
//

// Message sent from a node to FM when it joins the ring
Global<RSMessage> RSMessage::NodeUp = CreateCommonToFMMessage(L"NodeUp", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::NodeUpAck = CreateFMToCommonMessage(L"NodeUpAck", true /* isDefaultHighPriority */);

// Message sent from node to FM when its token changes
Global<RSMessage> RSMessage::ChangeNotification = CreateCommonToFMMessage(L"ChangeNotification", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::ChangeNotificationAck = CreateFMToCommonMessage(L"ChangeNotificationAck", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::ChangeNotificationChallenge = CreateFMToCommonMessage(L"ChangeNotificationChallenge", true /* isDefaultHighPriority */);

// Message sent from node to FM for creating, updating, and deleting a service
Global<RSMessage> RSMessage::CreateService = CreateCommonToFMMessage(L"CreateService");
Global<RSMessage> RSMessage::CreateServiceReply = CreateFMToCommonMessage(L"CreateServiceReply");
Global<RSMessage> RSMessage::UpdateService = CreateCommonToFMMessage(L"UpdateService");
Global<RSMessage> RSMessage::UpdateSystemService = CreateCommonToFMMessage(L"UpdateSystemService");
Global<RSMessage> RSMessage::UpdateServiceReply = CreateFMToCommonMessage(L"UpdateServiceReply");
Global<RSMessage> RSMessage::DeleteApplicationRequest = CreateCommonToFMMessage(L"DeleteApplicationRequest");
Global<RSMessage> RSMessage::DeleteApplicationReply = CreateCommonToFMMessage(L"DeleteApplicationReply");
Global<RSMessage> RSMessage::DeleteService = CreateCommonToFMMessage(L"DeleteService");
Global<RSMessage> RSMessage::DeleteServiceReply = CreateFMToCommonMessage(L"DeleteServiceReply");

// Message sent from RA to FM when a service type has been enabled or disabled
Global<RSMessage> RSMessage::ServiceTypeEnabled = CreateRAToFMMessage(L"ServiceTypeEnabled");
Global<RSMessage> RSMessage::ServiceTypeEnabledReply = CreateFMToRAMessage(L"ServiceTypeEnabledReply");
Global<RSMessage> RSMessage::ServiceTypeDisabled = CreateRAToFMMessage(L"ServiceTypeDisabled");
Global<RSMessage> RSMessage::ServiceTypeDisabledReply = CreateFMToRAMessage(L"ServiceTypeDisabledReply");

// Message sent from node to FM for updating the ServiceTable
Global<RSMessage> RSMessage::ServiceTableRequest = CreateCommonToFMMessage(L"ServiceTableRequest", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::ServiceTableUpdate = CreateMessageToServiceResolver(L"ServiceTableUpdate", true /* isDefaultHighPriority */);

// Message sent from FM to RA to create a new instance of a stateless service
Global<RSMessage> RSMessage::AddInstance = CreateFMToRAMessage(L"AddInstance", true);
Global<RSMessage> RSMessage::AddInstanceReply = CreateRAToFMMessage(L"AddInstanceReply", true);

// Message sent from FM to RA to remove an instance of a stateless service
Global<RSMessage> RSMessage::RemoveInstance = CreateFMToRAMessage(L"RemoveInstance", true);
Global<RSMessage> RSMessage::RemoveInstanceReply = CreateRAToFMMessage(L"RemoveInstanceReply", true);

// Message sent from FM to RA to create a new Primary replica
Global<RSMessage> RSMessage::AddPrimary = CreateFMToRAMessage(L"AddPrimary", true);
Global<RSMessage> RSMessage::AddPrimaryReply = CreateRAToFMMessage(L"AddPrimaryReply", true);

// Message sent from FM to RA to reconfigure the replica-set
Global<RSMessage> RSMessage::DoReconfiguration = CreateFMToRAMessage(L"DoReconfiguration", true);
Global<RSMessage> RSMessage::DoReconfigurationReply = CreateRAToFMMessage(L"DoReconfigurationReply", true);

// Message sent from FM to RA to add a new replica to the replica-set
Global<RSMessage> RSMessage::AddReplica = CreateFMToRAMessage(L"AddReplica", true);
Global<RSMessage> RSMessage::AddReplicaReply = CreateRAToFMMessage(L"AddReplicaReply", true);

// Message sent from FM to RA to remove a replica from the replica-set
Global<RSMessage> RSMessage::RemoveReplica = CreateFMToRAMessage(L"RemoveReplica", true);
Global<RSMessage> RSMessage::RemoveReplicaReply = CreateRAToFMMessage(L"RemoveReplicaReply", true);

// Message sent from FM to RA to delete a replica
Global<RSMessage> RSMessage::DeleteReplica = CreateFMToRAMessage(L"DeleteReplica", true);
Global<RSMessage> RSMessage::DeleteReplicaReply = CreateRAToFMMessage(L"DeleteReplicaReply", true);

// Message sent from FM to RA to inform about FM being too busy
Global<RSMessage> RSMessage::ServiceBusy = CreateFMToRAMessage(L"ServiceBusy", true);

// Message sent from RA to FM requesting to choose a new Primary for the configuration
Global<RSMessage> RSMessage::ChangeConfiguration = CreateRAToFMMessage(L"ChangeConfiguration", true);
Global<RSMessage> RSMessage::ChangeConfigurationReply = CreateFMToRAMessage(L"ChangeConfigurationReply", true);

Global<RSMessage> RSMessage::DatalossReport = CreateRAToFMMessage(L"DatalossReport", true);

// Message sent from RA to FM to inform that a replica has been removed by RA.
Global<RSMessage> RSMessage::ReplicaDropped = CreateRAToFMMessage(L"ReplicaDropped", true);
Global<RSMessage> RSMessage::ReplicaDroppedReply = CreateFMToRAMessage(L"ReplicaDroppedReply", true);

// Message sent from RA to FM to inform that a replica is down (but not removed from the configuration)
Global<RSMessage> RSMessage::ReplicaDown = CreateRAToFMMessage(L"ReplicaDown", true);
Global<RSMessage> RSMessage::ReplicaDownReply = CreateFMToRAMessage(L"ReplicaDownReply", true);

// Message sent from RA to FM to inform that a replica is Up
Global<RSMessage> RSMessage::ReplicaUp = CreateRAToFMMessage(L"ReplicaUp", true);
Global<RSMessage> RSMessage::ReplicaUpReply = CreateFMToRAMessage(L"ReplicaUpReply", true);

// Message sent from RA to FM to inform that a replica endpoint has been updated
Global<RSMessage> RSMessage::ReplicaEndpointUpdated = CreateRAToFMMessage(L"ReplicaEndpointUpdated");
Global<RSMessage> RSMessage::ReplicaEndpointUpdatedReply = CreateFMToRAMessage(L"ReplicaEndpointUpdatedReply");

// Message sent from RA to RA (Primary to replica) to create a new Idle replica
Global<RSMessage> RSMessage::CreateReplica = CreateRAToRAMessage(L"CreateReplica", true);
Global<RSMessage> RSMessage::CreateReplicaReply = CreateRAToRAMessage(L"CreateReplicaReply", true);

// Message sent from RA to RA (Primary to other replicas) informing them of the new configuration.
// This message serves as the deactivation and activation steps of the reconfiguration protocol.
Global<RSMessage> RSMessage::Deactivate = CreateRAToRAMessage(L"Deactivate", true);
Global<RSMessage> RSMessage::DeactivateReply = CreateRAToRAMessage(L"DeactivateReply", true);
Global<RSMessage> RSMessage::Activate = CreateRAToRAMessage(L"Activate", true);
Global<RSMessage> RSMessage::ActivateReply = CreateRAToRAMessage(L"ActivateReply", true);

// Message sent from RA to RA (Primary to other replicas) asking them for their last quorum committed LSN.
Global<RSMessage> RSMessage::GetLSN = CreateRAToRAMessage(L"GetLSN", true);
Global<RSMessage> RSMessage::GetLSNReply = CreateRAToRAMessage(L"GetLSNReply", true);

//Message sent from RA to FM reporting the current load at a replica/instance
Global<RSMessage> RSMessage::ReportLoad = CreateRAToFMMessage(L"ReportLoad");

// Message sent from node to FM to deactivate a node
Global<RSMessage> RSMessage::DeactivateNode = CreateCommonToFMMessage(L"DeactivateNode");
Global<RSMessage> RSMessage::DeactivateNodeReply = CreateFMToCommonMessage(L"DeactivateNodeReply");

// Message sent from node to FM to activate a node
Global<RSMessage> RSMessage::ActivateNode = CreateCommonToFMMessage(L"ActivateNode");
Global<RSMessage> RSMessage::ActivateNodeReply = CreateFMToCommonMessage(L"ActivateNodeReply");

// Message sent from CM to FM to deactivate a list of nodes
Global<RSMessage> RSMessage::DeactivateNodesRequest = CreateCommonToFMMessage(L"DeactivateNodesRequest");
Global<RSMessage> RSMessage::DeactivateNodesReply = CreateFMToCommonMessage(L"DeactivateNodesReply");

// Message sent from CM to FM to deactivate a list of nodes
Global<RSMessage> RSMessage::NodeDeactivationStatusRequest = CreateCommonToFMMessage(L"NodeDeactivationStatusRequest");
Global<RSMessage> RSMessage::NodeDeactivationStatusReply = CreateFMToCommonMessage(L"NodeDeactivationStatusReply");

// Message sent from CM to FM to deactivate a list of nodes
Global<RSMessage> RSMessage::RemoveNodeDeactivationRequest = CreateCommonToFMMessage(L"RemoveNodeDeactivationRequest");
Global<RSMessage> RSMessage::RemoveNodeDeactivationReply = CreateFMToCommonMessage(L"RemoveNodeDeactivationReply");

// Message sent from node to FM to drop all the replicas on a host
Global<RSMessage> RSMessage::NodeStateRemoved = CreateCommonToFMMessage(L"NodeStateRemoved");
Global<RSMessage> RSMessage::NodeStateRemovedReply = CreateFMToCommonMessage(L"NodeStateRemovedReply");

// Message sent from node to FM to bring all partitions out of quorum loss
Global<RSMessage> RSMessage::RecoverPartitions = CreateCommonToFMMessage(L"RecoverPartitions");
Global<RSMessage> RSMessage::RecoverPartitionsReply = CreateFMToCommonMessage(L"RecoverPartitionsReply");

// Message sent from node to FM to bring a given partition out of quorum loss
Global<RSMessage> RSMessage::RecoverPartition = CreateCommonToFMMessage(L"RecoverPartition");
Global<RSMessage> RSMessage::RecoverPartitionReply = CreateFMToCommonMessage(L"RecoverPartitionReply");

// Message sent from node to FM to bring a given service out of quorum loss
Global<RSMessage> RSMessage::RecoverServicePartitions = CreateCommonToFMMessage(L"RecoverServicePartitions");
Global<RSMessage> RSMessage::RecoverServicePartitionsReply = CreateFMToCommonMessage(L"RecoverServicePartitionsReply");

// Message sent from node to FM to bring all system services out of quorum loss
Global<RSMessage> RSMessage::RecoverSystemPartitions = CreateCommonToFMMessage(L"RecoverSystemPartitions");
Global<RSMessage> RSMessage::RecoverSystemPartitionsReply = CreateFMToCommonMessage(L"RecoverSystemPartitionsReply");

// Message sent from node to FM to get the list of all the nodes
Global<RSMessage> RSMessage::QueryRequest = CreateCommonToFMMessage(L"QueryRequest");
Global<RSMessage> RSMessage::QueryReply = CreateFMToCommonMessage(L"QueryReply");

// Rebuild protocol messages
Global<RSMessage> RSMessage::GenerationProposal = CreateFMToRAMessage(L"GenerationProposal", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::GenerationProposalReply = CreateRAToFMMessage(L"GenerationProposalReply", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::GenerationUpdate = CreateFMToRAMessage(L"GenerationUpdate", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::GenerationUpdateReject = CreateRAToFMMessage(L"GenerationUpdateReject", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::LFUMUpload = CreateRAToFMMessage(L"LFUMUpload", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::LFUMUploadReply = CreateFMToRAMessage(L"LFUMUploadReply", true /* isDefaultHighPriority */);
Global<RSMessage> RSMessage::GFUMTransfer = CreateRAToFMMessage(L"GFUMTransfer", true /* isDefaultHighPriority */);

// Message sent from node to FM for application upgrade
Global<RSMessage> RSMessage::ApplicationUpgradeRequest = CreateCommonToFMMessage(L"UpgradeRequest");
Global<RSMessage> RSMessage::ApplicationUpgradeReply = CreateFMToCommonMessage(L"UpgradeReply");

// Message sent from node to FM for Fabric upgrade
Global<RSMessage> RSMessage::FabricUpgradeRequest = CreateCommonToFMMessage(L"FabricUpgradeRequest", true);
Global<RSMessage> RSMessage::FabricUpgradeReply = CreateFMToCommonMessage(L"FabricUpgradeReply", true);

// Message sent from FM to RA for fabric upgrade
Global<RSMessage> RSMessage::NodeFabricUpgradeRequest = CreateFMToRAMessage(L"NodeFabricUpgradeRequest", true);
Global<RSMessage> RSMessage::NodeFabricUpgradeReply = CreateRAToFMMessage(L"NodeFabricUpgradeReply", true);

// Message sent from FM to RA for application upgrade
Global<RSMessage> RSMessage::NodeUpgradeRequest = CreateFMToRAMessage(L"NodeUpgradeRequest");
Global<RSMessage> RSMessage::NodeUpgradeReply = CreateRAToFMMessage(L"NodeUpgradeReply");

// Message sent from FM to RA to cancel old application upgrade
Global<RSMessage> RSMessage::CancelApplicationUpgradeRequest = CreateFMToRAMessage(L"CancelApplicationUpgradeRequest");
Global<RSMessage> RSMessage::CancelApplicationUpgradeReply = CreateRAToFMMessage(L"CancelApplicationUpgradeReply");

// Message sent from FM to RA to cancel old fabric application upgrade
Global<RSMessage> RSMessage::CancelFabricUpgradeRequest = CreateFMToRAMessage(L"CancelFabricUpgradeRequest", true);
Global<RSMessage> RSMessage::CancelFabricUpgradeReply = CreateRAToFMMessage(L"CancelFabricUpgradeReply", true);

// Message sent from FM to RA for service update
Global<RSMessage> RSMessage::NodeUpdateServiceRequest = CreateFMToRAMessage(L"NodeUpdateServiceRequest");
Global<RSMessage> RSMessage::NodeUpdateServiceReply = CreateRAToFMMessage(L"NodeUpdateServiceReply");

// Message sent from FM to RA for Node Deactivation
Global<RSMessage> RSMessage::NodeDeactivateRequest = CreateFMToRAMessage(L"NodeDeactivateRequest");
Global<RSMessage> RSMessage::NodeDeactivateReply = CreateRAToFMMessage(L"NodeDeactivateReply");

// Message sent from FM to RA for Node Activation
Global<RSMessage> RSMessage::NodeActivateRequest = CreateFMToRAMessage(L"NodeActivateRequest");
Global<RSMessage> RSMessage::NodeActivateReply = CreateRAToFMMessage(L"NodeActivateReply");

// Message sent from common to RA for report fault
Global<RSMessage> RSMessage::ReportFaultRequest = CreateCommonToRAMessage(L"ClientReportFaultRequest", true);
Global<RSMessage> RSMessage::ReportFaultReply = CreateRAToCommonMessage(L"ClientReportFaultReply", true);

// Message sent from common to FM for getting ServiceDescription of system services
Global<RSMessage> RSMessage::ServiceDescriptionRequest = CreateCommonToFMMessage(L"ServiceDescriptionRequest");
Global<RSMessage> RSMessage::ServiceDescriptionReply = CreateFMToCommonMessage(L"ServiceDescriptionReply");

// Message sent from common to reset load for a failoverunit
Global<RSMessage> RSMessage::ResetPartitionLoad = CreateCommonToFMMessage(L"ResetPartitionLoad");
Global<RSMessage> RSMessage::ResetPartitionLoadReply = CreateFMToCommonMessage(L"ResetPartitionLoadReply");

// Message sent from common to toggle the Health Reporting Verbosity
Global<RSMessage> RSMessage::ToggleVerboseServicePlacementHealthReporting = CreateCommonToFMMessage(L"ToggleVerboseServicePlacementHealthReporting");
Global<RSMessage> RSMessage::ToggleVerboseServicePlacementHealthReportingReply = CreateFMToCommonMessage(L"ToggleVerboseServicePlacementHealthReportingReply");

Global<RSMessage> RSMessage::ServiceTypeNotification = CreateRAToFMMessage(L"ServiceTypeNotification");
Global<RSMessage> RSMessage::ServiceTypeNotificationReply = CreateFMToRAMessage(L"ServiceTypeNotificationReply");

Global<RSMessage> RSMessage::CreateApplicationRequest = CreateCommonToFMMessage(L"CreateApplicationRequest");
Global<RSMessage> RSMessage::CreateApplicationReply = CreateCommonToFMMessage(L"CreateApplicationReply");

Global<RSMessage> RSMessage::UpdateApplicationRequest = CreateCommonToFMMessage(L"UpdateApplicationRequest");
Global<RSMessage> RSMessage::UpdateApplicationReply = CreateCommonToFMMessage(L"UpdateApplicationReply");

Global<RSMessage> RSMessage::PartitionNotification = CreateRAToFMMessage(L"PartitionNotification");
Global<RSMessage> RSMessage::PartitionNotificationReply = CreateFMToRAMessage(L"PartitionNotificationReply");

// Message sent from common to FM that will deliver available container images
Global<RSMessage> RSMessage::AvailableContainerImages = CreateCommonToFMMessage(L"AvailableContainerImages");

void RSMessage::AddActivityHeader(Transport::Message & message, Transport::FabricActivityHeader const & activityHeader)
{
    bool exists = false;
    auto iter = message.Headers.Begin();
    while (iter != message.Headers.End())
    {
        if (iter->Id() == Transport::FabricActivityHeader::Id)
        {
            exists = true;
            break;
        }

        ++iter;
    }

    if (exists)
    {
        message.Headers.Remove(iter);
    }

    message.Headers.Add(activityHeader);
}

ActivityId RSMessage::GetActivityId(Transport::Message & message)
{
    Transport::FabricActivityHeader header;
    if (!message.Headers.TryReadFirst(header))
    {
        // Functionally okay to continue in production, since we only
        // lose trace correlation
        //
        Assert::TestAssert("RSMessage::GetActivityId Missing activity id header");
    }
    return header.ActivityId;
}

void RSMessage::AddHeaders(Transport::Message & message, bool isHighPriority) const
{
    message.Headers.Add(Transport::ActorHeader(actor_));
    message.Headers.Add(actionHeader_);
    message.Idempotent = idempotent_;

    // The message can be HighPriority by default (NodeUp/NodeUpAck etc)
    // Or the component creating the message can call a specific message as High Priority (DoReconfiguration for system service)
    bool actualHighPriority = isDefaultHighPriority_ || isHighPriority;

    message.Headers.Add(Transport::HighPriorityHeader(actualHighPriority));
}
