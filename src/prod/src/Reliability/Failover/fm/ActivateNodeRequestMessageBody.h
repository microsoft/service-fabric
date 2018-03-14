// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ActivateNodeRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        ActivateNodeRequestMessageBody()
        {
        }

        ActivateNodeRequestMessageBody(Federation::NodeId nodeId, int64 sequenceNumber)
            : nodeId_(nodeId), sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        __declspec (property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        FABRIC_FIELDS_02(nodeId_, sequenceNumber_);

    private:

        Federation::NodeId nodeId_;
        int64 sequenceNumber_;
    };
}
