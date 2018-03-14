// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;

NodeHostProcessClosedEventArgs::NodeHostProcessClosedEventArgs(wstring const & nodeId, wstring const & detectedByHostId)
    : nodeId_(nodeId),
      hostId_(detectedByHostId)
{
}

NodeHostProcessClosedEventArgs::NodeHostProcessClosedEventArgs(NodeHostProcessClosedEventArgs const & other)
    : nodeId_(other.nodeId_),
      hostId_(other.hostId_)
{
}

NodeHostProcessClosedEventArgs::NodeHostProcessClosedEventArgs(NodeHostProcessClosedEventArgs && other)
    : nodeId_(move(other.nodeId_)),
      hostId_(move(other.hostId_))
{
}

NodeHostProcessClosedEventArgs const & NodeHostProcessClosedEventArgs::operator = (NodeHostProcessClosedEventArgs const & other)
{
    if (this != &other)
    {
        this->nodeId_ = other.nodeId_;
        this->hostId_ = other.hostId_;
    }

    return *this;
}

NodeHostProcessClosedEventArgs const & NodeHostProcessClosedEventArgs::operator = (NodeHostProcessClosedEventArgs && other)
{
    if (this != &other)
    {
        this->nodeId_ = move(other.nodeId_);
        this->hostId_ = move(other.hostId_);
    }

    return *this;
}

void NodeHostProcessClosedEventArgs::WriteTo(TextWriter& w, FormatOptions const&) const
{
    w.Write("NodeHostProcessClosedEventArgs { ");
    w.Write("NodeId = {0}", NodeId);
    w.Write("DetectedByHostId = {0}", DetectedByHostId);
    w.Write("}");
}
