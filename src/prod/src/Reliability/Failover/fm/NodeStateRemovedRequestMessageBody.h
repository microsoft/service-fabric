// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeStateRemovedRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        NodeStateRemovedRequestMessageBody()
        {
		}

        explicit NodeStateRemovedRequestMessageBody(
            Federation::NodeId nodeId)
            : nodeId_(nodeId)
        {
		}

        __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        FABRIC_FIELDS_01(nodeId_);

    private:

        Federation::NodeId nodeId_;
    };
}
