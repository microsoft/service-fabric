// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeactivateNodesRequestMessageBody : public ServiceModel::ClientServerMessageBody
    {
    public:

        DeactivateNodesRequestMessageBody()
        {
        }

        DeactivateNodesRequestMessageBody(
            std::map<Federation::NodeId, NodeDeactivationIntent::Enum> const& nodes,
            std::wstring const& batchId,
            int64 sequenceNumber)
            : nodes_(nodes),
              batchId_(batchId),
              sequenceNumber_(sequenceNumber)
        {
        }

        __declspec (property(get=get_Nodes)) std::map<Federation::NodeId, NodeDeactivationIntent::Enum> const& Nodes;
        std::map<Federation::NodeId, NodeDeactivationIntent::Enum> const& get_Nodes() const { return nodes_; }

        __declspec (property(get=get_BatchId)) std::wstring const& BatchId;
        std::wstring const& get_BatchId() const { return batchId_; }

        __declspec (property(get=get_SequenceNumber)) int64 SequenceNumber;
        int64 get_SequenceNumber() const { return sequenceNumber_; }

        std::map<Federation::NodeId, NodeDeactivationIntent::Enum> TakeNodes() { return std::move(nodes_); }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.WriteLine("BatchId: {0}, SeqNum: {1}", batchId_, sequenceNumber_);

            for (auto it = nodes_.begin(); it != nodes_.end(); it++)
            {
                w.WriteLine("{0}:{1}", it->first, it->second);
            }
        }

        FABRIC_FIELDS_03(nodes_, batchId_, sequenceNumber_);

    private:

        std::map<Federation::NodeId, NodeDeactivationIntent::Enum> nodes_;
        std::wstring batchId_;
        int64 sequenceNumber_;
    };

}

DEFINE_USER_MAP_UTILITY(Federation::NodeId, Reliability::NodeDeactivationIntent::Enum);

