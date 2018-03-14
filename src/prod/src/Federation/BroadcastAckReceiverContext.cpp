// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

BroadcastAckReceiverContext::BroadcastAckReceiverContext(
    BroadcastManager & manager,
    MessageId const & broadcastId,
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance)
    :   OneWayReceiverContext(from, fromInstance, broadcastId),
        manager_(manager),
        broadcastId_(broadcastId)
{
}

BroadcastAckReceiverContext::~BroadcastAckReceiverContext()
{
}

void BroadcastAckReceiverContext::Accept()
{
    manager_.ProcessBroadcastLocalAck(broadcastId_, ErrorCode::Success());
}

void BroadcastAckReceiverContext::Reject(ErrorCode const & error, ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(activityId);
    manager_.ProcessBroadcastLocalAck(broadcastId_, error);
}

void BroadcastAckReceiverContext::Ignore()
{
    Reject(ErrorCodeValue::NotReady);
}
