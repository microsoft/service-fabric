// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;

SendMessageAction::SendMessageAction(MessageUPtr && message, PartnerNodeSPtr const & target)
    :   message_(move(message)),
        target_(target),
        targetId_(target->Id)
{
}

SendMessageAction::SendMessageAction(MessageUPtr && message, NodeId targetId)
    :   message_(move(message)),
        target_(),
        targetId_(targetId)
{
}

void SendMessageAction::Execute(SiteNode & siteNode)
{
    if (target_)
    {
        siteNode.Send(move(message_), target_);
    }
    else
    {
        siteNode.Route(move(message_), targetId_, 0, false);
    }
}

std::wstring SendMessageAction::ToString()
{
    return wformatString("{0}:{1}", message_->Action, targetId_);
}
