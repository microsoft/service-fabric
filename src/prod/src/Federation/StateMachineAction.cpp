// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Federation/TicketGap.h"

using namespace std;
using namespace Common;
using namespace Federation;

void StateMachineActionCollection::Add(StateMachineActionUPtr && action)
{
    if (!actions_)
    {
        actions_ = Common::make_unique<StateMachineActionVector>();
    }

    actions_->push_back(std::move(action));
}

void StateMachineActionCollection::Execute(SiteNode & siteNode)
{
    if (actions_)
    {
        auto siteNodeSPtr = siteNode.GetSiteNodeSPtr();
        MoveUPtr<StateMachineActionVector> actionMover(move(actions_));

       Threadpool::Post([siteNodeSPtr, actionMover] () mutable
        { StateMachineActionCollection::ExecuteActions(*(actionMover.TakeUPtr()), *siteNodeSPtr); });   
    }
}

void StateMachineActionCollection::ExecuteActions(StateMachineActionVector const & actions, SiteNode & siteNode)
{
    for (StateMachineActionUPtr const & action : actions)
    {
        action->Execute(siteNode);
    }
}

void StateMachineActionCollection::WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
{
    if (actions_)
    {
        for (StateMachineActionUPtr const & action : *actions_)
        {
            w.Write(action->ToString());
            w.Write("\r\n");
        }
    }
}
