// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Node.h"

using namespace std;
using namespace Common;
using namespace Reliability::LoadBalancingComponent;

GlobalWString const Node::FormatHeader = make_global<wstring>(L"NodeId NodeName");

Node::Node(NodeDescription && nodeDescription)
    : nodeDescription_(move(nodeDescription))
{
}

Node::Node(Node const& other)
    : nodeDescription_(other.NodeDescriptionObj)
{
}

Node::Node(Node && other)
    : nodeDescription_(move(other.nodeDescription_))
{
}

void Node::UpdateDescription(NodeDescription && description)
{
    nodeDescription_ = move(description);
}

void Node::WriteTo(TextWriter& writer, FormatOptions const&) const
{
    writer.Write("{0} {1}", nodeDescription_.NodeId, nodeDescription_.NodeName);
}

