// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "Ra.Stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Reliability;
using namespace ReconfigurationAgentComponent;

Actor::Enum const RAMessage::Actor(Actor::RA);

//
// List of Messages used by the Reliability Subsystem
//

#define MAKE_MESSAGE(var_name, msg_name) Global<RAMessage> RAMessage::var_name = make_global<RAMessage>(msg_name)

MAKE_MESSAGE(ReplicaOpen,                               L"ReplicaOpen");
MAKE_MESSAGE(ReplicaClose,                              L"ReplicaClose");
MAKE_MESSAGE(StatefulServiceReopen,                     L"StatefulServiceReopen");
MAKE_MESSAGE(UpdateConfiguration,                       L"UpdateConfiguration");
MAKE_MESSAGE(ReplicatorBuildIdleReplica,                L"ReplicatorBuildIdleReplica");
MAKE_MESSAGE(ReplicatorRemoveIdleReplica,               L"ReplicatorRemoveIdleReplica");
MAKE_MESSAGE(ReplicatorGetStatus,                       L"ReplicatorGetStatus");
MAKE_MESSAGE(ReplicatorUpdateEpochAndGetStatus,         L"ReplicatorUpdateEpochAndGetStatus");
MAKE_MESSAGE(CancelCatchupReplicaSet,                   L"CancelCatchupReplicaSet");
MAKE_MESSAGE(ReportLoad,                                L"RAPReportLoad");
MAKE_MESSAGE(ReportFault,                               L"RAReportFault");
MAKE_MESSAGE(RAPQuery,                                  L"RAPQuery");
MAKE_MESSAGE(RAPQueryReply,                             L"RAPQueryReply");

MAKE_MESSAGE(ReplicaOpenReply,                          L"ReplicaOpenReply");
MAKE_MESSAGE(ReplicaCloseReply,                         L"ReplicaCloseReply");
MAKE_MESSAGE(StatefulServiceReopenReply,                L"StatefulServiceReopenReply");
MAKE_MESSAGE(UpdateConfigurationReply,                  L"UpdateConfigurationReply");
MAKE_MESSAGE(ReplicatorBuildIdleReplicaReply,           L"ReplicatorBuildIdleReplicaReply");
MAKE_MESSAGE(ReplicatorRemoveIdleReplicaReply,          L"ReplicatorRemoveIdleReplicaReply");
MAKE_MESSAGE(ReplicatorGetStatusReply,                  L"ReplicatorGetStatusReply");
MAKE_MESSAGE(ReplicatorUpdateEpochAndGetStatusReply,    L"ReplicatorUpdateEpochAndGetStatusReply");
MAKE_MESSAGE(CancelCatchupReplicaSetReply,              L"CancelCatchupReplicaSetReply");

MAKE_MESSAGE(ReplicaEndpointUpdated,                    L"ProxyReplicaEndpointUpdated");
MAKE_MESSAGE(ReplicaEndpointUpdatedReply,               L"ProxyReplicaEndpointUpdatedReply");

MAKE_MESSAGE(ReadWriteStatusRevokedNotification,        L"ReadWriteStatusRevokedNotification");
MAKE_MESSAGE(ReadWriteStatusRevokedNotificationReply,   L"ReadWriteStatusRevokedNotificationReply");

MAKE_MESSAGE(UpdateServiceDescription,                  L"ProxyUpdateServiceDescription");
MAKE_MESSAGE(UpdateServiceDescriptionReply,             L"ProxyUpdateServiceDescriptionReply");

void RAMessage::AddHeaders(Transport::Message & message) const
{
    message.Headers.Add(Transport::ActorHeader(Actor));
    message.Headers.Add(actionHeader_);
}
