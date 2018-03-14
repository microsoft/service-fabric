// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeactivateNodeRequestMessageBody : public Serialization::FabricSerializable
    {
    public:

        DeactivateNodeRequestMessageBody()
        {
        }

        DeactivateNodeRequestMessageBody(Federation::NodeId nodeId, FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent, int64 sequenceNumber)
            : nodeId_(nodeId),
              deactivationIntent_(deactivationIntent),
              sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        __declspec (property(get=get_DeactivationIntent)) FABRIC_NODE_DEACTIVATION_INTENT DeactivationIntent;
        FABRIC_NODE_DEACTIVATION_INTENT get_DeactivationIntent() const { return deactivationIntent_; }

        __declspec (property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        FABRIC_FIELDS_03(nodeId_, deactivationIntent_, sequenceNumber_);

    private:

        Federation::NodeId nodeId_;
        FABRIC_NODE_DEACTIVATION_INTENT deactivationIntent_;
        int64 sequenceNumber_;
    };
}
