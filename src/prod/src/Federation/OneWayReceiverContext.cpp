// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Transport;
using namespace Common;
using namespace Federation;
using namespace std;

OneWayReceiverContext::OneWayReceiverContext(
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance,
    MessageId const & relatesToId)
    : ReceiverContext(from, fromInstance, relatesToId)
{
}

OneWayReceiverContext::~OneWayReceiverContext()
{
}

void OneWayReceiverContext::Accept()
{
}

void OneWayReceiverContext::Reject(ErrorCode const & error, ActivityId const & activityId)
{
    UNREFERENCED_PARAMETER(activityId);
    // TODO: If we reject a one way message we can potentially send some messages back about ring state
    error.ReadValue();
}
