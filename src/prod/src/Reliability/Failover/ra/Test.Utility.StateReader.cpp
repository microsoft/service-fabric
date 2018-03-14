// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest;
using namespace Reliability::ReconfigurationAgentComponent::ReliabilityUnitTest::StateManagement;

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddPrimary,                                 L"AddPrimary");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddPrimaryReply,                            L"AddPrimaryReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddReplica,                                 L"AddReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddReplicaReply,                            L"AddReplicaReply"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddInstance,                                L"AddInstance"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::AddInstanceReply,                           L"AddInstanceReply"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::RemoveInstance,                             L"RemoveInstance"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::RemoveInstanceReply,                        L"RemoveInstanceReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DoReconfiguration,                          L"DoReconfiguration");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DoReconfigurationReply,                     L"DoReconfigurationReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ChangeConfiguration,                        L"ChangeConfiguration"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ChangeConfigurationReply,                   L"ChangeConfigurationReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GetLSN,                                     L"GetLSN");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GetLSNReply,                                L"GetLSNReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::Deactivate,                                 L"Deactivate");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeactivateReply,                            L"DeactivateReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::Activate,                                   L"Activate");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ActivateReply,                              L"ActivateReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::RemoveReplica,                              L"RemoveReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::RemoveReplicaReply,                         L"RemoveReplicaReply"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DropReplica,                                L"DropReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DropReplicaReply,                           L"DropReplicaReply"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeleteReplica,                              L"DeleteReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeleteReplicaReply,                         L"DeleteReplicaReply"); 
    
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CreateReplica,                              L"CreateReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CreateReplicaReply,                         L"CreateReplicaReply"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaUp,                                  L"ReplicaUp");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaUpReply,                             L"ReplicaUpReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpgradeRequest,                         L"NodeUpgradeRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpgradeReply,                           L"NodeUpgradeReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CancelApplicationUpgradeRequest,            L"CancelApplicationUpgradeRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CancelApplicationUpgradeReply,              L"CancelApplicationUpgradeReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeFabricUpgradeRequest,                   L"NodeFabricUpgradeRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeFabricUpgradeReply,                     L"NodeFabricUpgradeReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CancelFabricUpgradeRequest,                 L"CancelFabricUpgradeRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CancelFabricUpgradeReply,                   L"CancelFabricUpgradeReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeActivateRequest,                        L"NodeActivateRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeDeactivateRequest,                      L"NodeDeactivateRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeActivateReply,                          L"NodeActivateReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeDeactivateReply,                        L"NodeDeactivateReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GenerationUpdate,                           L"GenerationUpdate");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GenerationUpdateReject,                     L"GenerationUpdateReject");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GenerationProposal,                         L"GenerationProposal");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::GenerationProposalReply,                    L"GenerationProposalReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeEnabled,                         L"ServiceTypeEnabled");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeEnabledReply,                    L"ServiceTypeEnabledReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeDisabled,                        L"ServiceTypeDisabled");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeDisabledReply,                   L"ServiceTypeDisabledReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::PartitionNotification,                      L"PartitionNotification");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::PartitionNotificationReply,                 L"PartitionNotificationReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeNotification,                    L"ServiceTypeNotification");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ServiceTypeNotificationReply,               L"ServiceTypeNotificationReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaEndpointUpdated,                     L"ReplicaEndpointUpdated");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaEndpointUpdatedReply,                L"ReplicaEndpointUpdatedReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaOpen,                                L"ReplicaOpen"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::StatefulServiceReopen,                      L"StatefulServiceReopen");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaClose,                               L"ReplicaClose"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorBuildIdleReplica,                 L"ReplicatorBuildIdleReplica"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::UpdateConfiguration,                        L"UpdateConfiguration"); 
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorRemoveIdleReplica,                L"ReplicatorRemoveIdleReplica"); 

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaOpenReply,                           L"ReplicaOpenReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicaCloseReply,                          L"ReplicaCloseReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::StatefulServiceReopenReply,                 L"StatefulServiceReopenReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorBuildIdleReplicaReply,            L"ReplicatorBuildIdleReplicaReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::UpdateConfigurationReply,                   L"UpdateConfigurationReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorRemoveIdleReplicaReply,           L"ReplicatorRemoveIdleReplicaReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorGetStatus,                        L"ReplicatorGetStatus");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorGetStatusReply,                   L"ReplicatorGetStatusReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorUpdateEpochAndGetStatus,          L"ReplicatorUpdateEpochAndGetStatus");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReplicatorUpdateEpochAndGetStatusReply,     L"ReplicatorUpdateEpochAndGetStatusReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::CancelCatchupReplicaSetReply,               L"CancelCatchupReplicaSetReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReportFault,                                L"RAReportFault");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyReplicaEndpointUpdated,                L"ProxyReplicaEndpointUpdated");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyReplicaEndpointUpdatedReply,           L"ProxyReplicaEndpointUpdatedReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReadWriteStatusRevokedNotification,         L"ReadWriteStatusRevokedNotification");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReadWriteStatusRevokedNotificationReply,    L"ReadWriteStatusRevokedNotificationReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyUpdateServiceDescription,              L"ProxyUpdateServiceDescription");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyUpdateServiceDescriptionReply,         L"ProxyUpdateServiceDescriptionReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpdateServiceRequest,                   L"NodeUpdateServiceRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpdateServiceReply,                     L"NodeUpdateServiceReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::NodeUpAck,                                  L"NodeUpAck");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaQueryRequest,         L"QueryRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaQueryResult,          L"QueryReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaDetailQueryRequest,    L"QueryRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::DeployedServiceReplicaDetailQueryReply,     L"QueryReply");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyDeployedServiceReplicaDetailQueryRequest,   L"RAPQuery");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ProxyDeployedServiceReplicaDetailQueryReply,    L"RAPQueryReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ClientReportFaultRequest,                   L"ClientReportFaultRequest");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ClientReportFaultReply,                     L"ClientReportFaultReply");

DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ReportLoad,                                 L"ReportLoad");
DEFINE_MESSAGE_TYPE_TRAITS(MessageType::ResourceUsageReport,                        L"ResourceUsageReportAction");
