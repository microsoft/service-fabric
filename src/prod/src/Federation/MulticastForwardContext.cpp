// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Transport;
using namespace Federation;


MulticastForwardContext::MulticastForwardContext()
{
}

MulticastForwardContext::MulticastForwardContext(
    NodeId nodeId,
    MessageUPtr && message,
    MessageId const & multicastId, 
    vector<SubTree> && pending,
    MulticastOperationSPtr const & operation,
    RequestReceiverContextUPtr && requestContext)
    :   nodeId_(nodeId),
        message_(move(message)),
        multicastId_(multicastId),
        error_(ErrorCode::Success()),
        pending_(move(pending)),
        operation_(operation)
{
    if (requestContext)
    {
        requestContexts_.push_back(move(requestContext));
    }
}

MulticastForwardContext::MulticastForwardContext(MulticastForwardContext && other)
    :   nodeId_(other.nodeId_),
        message_(move(other.message_)),
        multicastId_(other.multicastId_),
        error_(other.error_),
        acked_(move(other.acked_)),
        failed_(move(other.failed_)),
        pending_(move(other.pending_)),
        operation_(move(other.operation_)),
        requestContexts_(move(other.requestContexts_))
{
}

MessageUPtr MulticastForwardContext::CreateAckMessage() const
{
    return FederationMessage::GetMulticastAck().CreateMessage(MulticastAckBody(failed_, unknown_));
}

void MulticastForwardContext::AddFailedTarget(NodeInstance const & target)
{
    if (find(failed_.begin(), failed_.end(), target) == failed_.end())
    {
        failed_.push_back(target);
    }
}

void MulticastForwardContext::AddUnknownTarget(NodeInstance const & target)
{
    if (find(unknown_.begin(), unknown_.end(), target) == unknown_.end())
    {
        unknown_.push_back(target);
    }
}

void MulticastForwardContext::CompletIfNeeded()
{
    if (error_.IsSuccess() && pending_.size() > 0)
    {
        return;
    }

    if (unknown_.size() == 0)
    {
        for (auto it = pending_.begin(); it != pending_.end(); ++it)
        {
            unknown_.push_back(it->Root);
            unknown_.insert(unknown_.end(), it->Targets.begin(), it->Targets.end());
        }
    }

    MulticastManager::WriteInfo("Complete", "Multicast context {0} completed", *this);

    if (operation_)
    {
        operation_->SetResults(failed_, unknown_);
        operation_->TryComplete(operation_, error_.IsSuccess() && unknown_.size() > 0 ? ErrorCodeValue::Timeout : error_);
    }

    for (auto it = requestContexts_.begin(); it != requestContexts_.end(); ++it)
    {
        if (error_.IsSuccess() || error_.IsError(ErrorCodeValue::Timeout))
        {
            (*it)->Reply(CreateAckMessage());
        }
        else
        {
            (*it)->Ignore();
        }
    }

    requestContexts_.clear();
}

void MulticastForwardContext::SetError(ErrorCode error)
{
    error_ = error;
    CompletIfNeeded();
}

void MulticastForwardContext::Complete(NodeInstance const & target)
{
    for (auto it = pending_.begin(); it != pending_.end(); ++it)
    {
        if (it->Root == target)
        {
            acked_.insert(it->Root);
            acked_.insert(it->Targets.begin(), it->Targets.end());
            pending_.erase(it);

            CompletIfNeeded();
            return;
        }
    }
}

MessageUPtr MulticastForwardContext::FailTarget(NodeInstance const & target, __out vector<SubTree> & subTrees)
{
    for (auto it = pending_.begin(); it != pending_.end(); ++it)
    {
        if (it->Root == target)
        {
            AddFailedTarget(target);
            if (it->Targets.size() > 0)
            {
                SubTree subTree;
                auto itNewRoot = it->Targets.begin() + (it->Targets.size() - 1) / 2;
                it->Root = subTree.Root = *itNewRoot;
                it->Targets.erase(itNewRoot);
                subTree.Targets = it->Targets;
                subTrees.push_back(move(subTree));
                return message_->Clone();
            }
            else
            {
                pending_.erase(it);
                CompletIfNeeded();
            }

            return nullptr;
        }
    }

    return nullptr;
}

bool MulticastForwardContext::RemoveDuplicateTargets(vector<NodeInstance> & targets) const
{
    set<NodeInstance> nodes;
    nodes.insert(targets.begin(), targets.end());
    targets.clear();

    for (auto it = acked_.begin(); it != acked_.end(); ++it)
    {
        nodes.erase(*it);
    }

    for (auto it = failed_.begin(); it != failed_.end(); ++it)
    {
        nodes.erase(*it);
    }

    if (nodes.size() == 0)
    {
        return true;
    }

    for (auto it1 = pending_.begin(); it1 != pending_.end(); ++it1)
    {
        nodes.erase(it1->Root);
        for (auto it2 = it1->Targets.begin(); it2 != it1->Targets.end(); ++it2)
        {
            nodes.erase(*it2);
        }
    }

    targets.insert(targets.end(), nodes.begin(), nodes.end());
    sort(targets.begin(), targets.end());

    return false;
}

void MulticastForwardContext::AddSubTrees(
    MessageUPtr & message,
    vector<SubTree> const & subTrees,
    RequestReceiverContextUPtr && requestContext)
{
    if (subTrees.size() > 0)
    {
        if (!message_)
        {
            message_ = message->Clone();
        }

        pending_.insert(pending_.end(), subTrees.begin(), subTrees.end());
    }

    requestContexts_.push_back(move(requestContext));
}

void MulticastForwardContext::WriteTo(TextWriter& w, FormatOptions const &) const
{
    w.Write("{0} on node {1} Acked: {2}, Failed: {3}, Unknown: {4}, error: {5}",
        multicastId_, nodeId_, acked_.size(), failed_, unknown_, error_);

    if (pending_.size() > 0)
    {
        w.Write(L" Pending:");
    
        for (auto it = pending_.begin(); it != pending_.end(); ++it)
        {
            w.Write(" {0}({1})", it->Root, it->Targets.size());
        }
    }
}
