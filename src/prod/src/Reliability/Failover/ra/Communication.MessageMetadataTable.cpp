// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

#include "ServiceModel/management/ResourceMonitor/public.h"

using namespace std;
using namespace Common;
using namespace Reliability::ReconfigurationAgentComponent;
using namespace Reliability::ReconfigurationAgentComponent::Communication;
using namespace Reliability::ReconfigurationAgentComponent::Communication::StalenessCheckType;

#define DEFINE_METADATA(name, target, processDuringNodeClose, createsEntity, stalenessCheckType) Global<MessageMetadata> MessageMetadataTable::name = make_global<MessageMetadata>(target, processDuringNodeClose, createsEntity, stalenessCheckType);

Global<MessageMetadata> MessageMetadataTable::Deprecated = make_global<MessageMetadata>();

//              Name                                        Target                      ProcessDuringNodeClose  CreatesEntity   StalenessCheckType
DEFINE_METADATA(RA_Normal,                                  MessageTarget::RA,          false,                  false,          None);
DEFINE_METADATA(RA_ProcessDuringNodeCloseMessages,          MessageTarget::RA,          true,                   false,          None);

DEFINE_METADATA(FT_Failover_Normal,                         MessageTarget::FT,          false,                  false,          FTFailover);
DEFINE_METADATA(FT_Failover_Normal_CreatesEntity,           MessageTarget::FT,          false,                  true,           FTFailover);
DEFINE_METADATA(FT_Failover_ProcessDuringNodeCloseMessages, MessageTarget::FT,          true,                   false,          FTFailover);

DEFINE_METADATA(FT_Proxy_Normal,                            MessageTarget::FT,          false,                  false,          FTProxy);
DEFINE_METADATA(FT_Proxy_ProcessDuringNodeCloseMessages,    MessageTarget::FT,          true,                   false,          FTProxy);

#define METADATA_TABLE_ENTRY(name,  type) if (action == name) { return &*type; }
#define UNHANDLED_MESSAGE_ENTRY() return nullptr;

MessageMetadata const * MessageMetadataTable::LookupMetadata(Transport::MessageUPtr const & message) const
{
    return LookupMetadata(message->Action);
}

MessageMetadata const * MessageMetadataTable::LookupMetadata(Transport::Message const & message) const
{
    return LookupMetadata(message.Action);
}

MessageMetadata const * MessageMetadataTable::LookupMetadata(std::wstring const & action) const
{
    METADATA_TABLE_ENTRY(RSMessage::GetChangeConfigurationReply().Action,                   Deprecated);

#pragma region Federation messages to node
    METADATA_TABLE_ENTRY(RSMessage::GetServiceBusy().Action,                                RA_Normal);

    METADATA_TABLE_ENTRY(RSMessage::GetGenerationUpdate().Action,                           RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetReplicaUpReply().Action,                             RA_ProcessDuringNodeCloseMessages);
    METADATA_TABLE_ENTRY(RSMessage::GetNodeActivateRequest().Action,                        RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetNodeDeactivateRequest().Action,                      RA_Normal);

    METADATA_TABLE_ENTRY(RSMessage::GetServiceTypeEnabledReply().Action,                    RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetServiceTypeDisabledReply().Action,                   RA_Normal);

    METADATA_TABLE_ENTRY(RSMessage::GetNodeUpgradeRequest().Action,                         RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetCancelApplicationUpgradeRequest().Action,            RA_Normal);

    METADATA_TABLE_ENTRY(RSMessage::GetNodeFabricUpgradeRequest().Action,                   RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetCancelFabricUpgradeRequest().Action,                 RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetNodeUpdateServiceRequest().Action,                   RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetServiceTypeNotificationReply().Action,               RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetPartitionNotificationReply().Action,                 RA_Normal);
#pragma endregion

#pragma region Federation messages for partition
    METADATA_TABLE_ENTRY(RSMessage::GetAddInstance().Action,                                FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetRemoveInstance().Action,                             FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetAddPrimary().Action,                                 FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetAddReplica().Action,                                 FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetRemoveReplica().Action,                              FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetDoReconfiguration().Action,                          FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetDeleteReplica().Action,                              FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetReplicaEndpointUpdatedReply().Action,                FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetCreateReplica().Action,                              FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetGetLSN().Action,                                     FT_Failover_Normal_CreatesEntity);
    METADATA_TABLE_ENTRY(RSMessage::GetDeactivate().Action,                                 FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetActivate().Action,                                   FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetCreateReplicaReply().Action,                         FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetGetLSNReply().Action,                                FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetDeactivateReply().Action,                            FT_Failover_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetActivateReply().Action,                              FT_Failover_Normal);

#pragma endregion

#pragma region IPC Messages
    METADATA_TABLE_ENTRY(RAMessage::GetReportLoad().Action,                                                 RA_Normal);
    METADATA_TABLE_ENTRY(ClientServerTransport::HealthManagerTcpMessage::ReportHealthAction,                RA_Normal);
    METADATA_TABLE_ENTRY(Management::ResourceMonitor::ResourceUsageReport::ResourceUsageReportAction,       RA_Normal);
#pragma endregion

#pragma region IPC messages for partition
    METADATA_TABLE_ENTRY(RAMessage::GetReplicaCloseReply().Action,                          FT_Proxy_ProcessDuringNodeCloseMessages);
    METADATA_TABLE_ENTRY(RAMessage::GetReportFault().Action,                                FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicaOpenReply().Action,                           FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetStatefulServiceReopenReply().Action,                 FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetUpdateConfigurationReply().Action,                   FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicatorBuildIdleReplicaReply().Action,            FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicatorRemoveIdleReplicaReply().Action,           FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicatorGetStatusReply().Action,                   FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicatorUpdateEpochAndGetStatusReply().Action,     FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetCancelCatchupReplicaSetReply().Action,               FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetUpdateServiceDescriptionReply().Action,              FT_Proxy_Normal);
    METADATA_TABLE_ENTRY(RAMessage::GetReadWriteStatusRevokedNotification().Action,         FT_Proxy_ProcessDuringNodeCloseMessages);
    METADATA_TABLE_ENTRY(RAMessage::GetReplicaEndpointUpdated().Action,                     FT_Proxy_Normal);
    
    // For test
    METADATA_TABLE_ENTRY(RAMessage::GetRAPQueryReply().Action,                              FT_Proxy_Normal);
#pragma endregion

#pragma region Federation Request-Response
    METADATA_TABLE_ENTRY(RSMessage::GetQueryRequest().Action,                               RA_Normal);
    METADATA_TABLE_ENTRY(RSMessage::GetReportFaultRequest().Action,                         RA_Normal);
#pragma endregion

    UNHANDLED_MESSAGE_ENTRY();
}
