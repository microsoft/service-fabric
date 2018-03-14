// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;

ReceiverContext::ReceiverContext(
    PartnerNodeSPtr const & from,
    NodeInstance const & fromInstance,
    Transport::MessageId const & relatesToId)
    : from_(from), fromInstance_(fromInstance), relatesToId_(relatesToId)
{
}

ReceiverContext::~ReceiverContext()
{
}

PartnerNodeSPtr const & ReceiverContext::get_FromWithSameInstance() const
{
    return (from_ && from_->Instance == fromInstance_ ? from_ : RoutingTable::NullNode);
}
