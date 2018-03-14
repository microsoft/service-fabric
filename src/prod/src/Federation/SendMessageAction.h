// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class SendMessageAction : public StateMachineAction
    {
        DENY_COPY(SendMessageAction)

    public:
        SendMessageAction(Transport::MessageUPtr && message, PartnerNodeSPtr const & target);
        SendMessageAction(Transport::MessageUPtr && message, NodeId targetId);

        virtual void Execute(SiteNode & siteNode);
        virtual std::wstring ToString();
    private:
        Transport::MessageUPtr message_;
        PartnerNodeSPtr target_;
        NodeId targetId_;
    };
}
