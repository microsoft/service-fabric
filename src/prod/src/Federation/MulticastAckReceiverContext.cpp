// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

MulticastAckReceiverContext::MulticastAckReceiverContext(
    MulticastManager & manager,
    MessageId const & multicastId,
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance)
    :   OneWayReceiverContext(from, fromInstance, multicastId),
        manager_(manager),
        multicastId_(multicastId)
{
}

MulticastAckReceiverContext::~MulticastAckReceiverContext()
{
}

void MulticastAckReceiverContext::Accept()
{
    manager_.ProcessMulticastLocalAck(multicastId_, ErrorCode::Success());
}

void MulticastAckReceiverContext::Reject(ErrorCode const & error)
{
    manager_.ProcessMulticastLocalAck(multicastId_, error);
}

void MulticastAckReceiverContext::Ignore()
{
    Reject(ErrorCodeValue::NotReady);
}
