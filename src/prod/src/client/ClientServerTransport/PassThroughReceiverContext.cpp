// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Client
{
    using namespace Transport;

    void PassThroughReceiverContext::Reply(MessageUPtr& reply, TransportPriority::Enum) const
    {
        transport_.FinishNotification(internalContext_, reply);
    }
}
