// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Reliability;
using namespace std;

NodeProgress::NodeProgress()
{
}


NodeProgress::NodeProgress(
    Federation::NodeId nodeId,
    wstring const& nodeName,
    vector<SafetyCheckWrapper> const& safetyChecks)
    : nodeId_(nodeId)
    , nodeName_(nodeName)
    , safetyChecks_(safetyChecks)
{
}

NodeProgress::NodeProgress(NodeProgress && other)
    : nodeId_(other.nodeId_)
    , nodeName_(move(other.nodeName_))
    , safetyChecks_(other.safetyChecks_)
{
}

NodeProgress & NodeProgress::operator=(NodeProgress && other)
{
    if (this != &other)
    {
        nodeId_ = other.nodeId_;
        nodeName_ = other.nodeName_;
        safetyChecks_ = other.safetyChecks_;
    }

    return *this;
}

void NodeProgress::WriteTo(TextWriter & w, FormatOptions const&) const
{
    w.WriteLine("NodeId={0}, NodeName={1}", nodeId_, nodeName_);

    for (auto const& safetyCheck : safetyChecks_)
    {
        w.WriteLine(*safetyCheck.SafetyCheck);
    }
}
