// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeUpAckMessageBody : public Serialization::FabricSerializable
    {
    DEFAULT_COPY_ASSIGNMENT(NodeUpAckMessageBody);
    public:

        // For compatibility with v1 we set the node to be activated and the sequence number to be zero
        NodeUpAckMessageBody()
        : isNodeActivated_(true),
          nodeActivationSequenceNumber_(0)
        {
        }

        NodeUpAckMessageBody(Common::FabricVersionInstance && fabricVersionInstance, std::vector<UpgradeDescription> && upgrades, bool isNodeActivated, int64 nodeActivationSequenceNumber)
        : fabricVersionInstance_(std::move(fabricVersionInstance)), 
            upgrades_(std::move(upgrades)),
            isNodeActivated_(isNodeActivated),
            nodeActivationSequenceNumber_(nodeActivationSequenceNumber)
        {
        }

        NodeUpAckMessageBody(NodeUpAckMessageBody && other)
        : fabricVersionInstance_(std::move(other.fabricVersionInstance_)),
          upgrades_(std::move(other.upgrades_)),
          isNodeActivated_(std::move(other.isNodeActivated_)),
          nodeActivationSequenceNumber_(std::move(other.nodeActivationSequenceNumber_))
        {
        }

        __declspec (property(get=get_Upgrades)) std::vector<UpgradeDescription> const & Upgrades;
        std::vector<UpgradeDescription> const& get_Upgrades() const { return upgrades_; }

        __declspec(property(get=get_FabricVersionInstance)) Common::FabricVersionInstance const & FabricVersionInstance;
        Common::FabricVersionInstance const & get_FabricVersionInstance() const { return fabricVersionInstance_; }

        __declspec(property(get=get_IsNodeActivated)) bool IsNodeActivated;
        bool get_IsNodeActivated() const { return isNodeActivated_; }

        __declspec(property(get=get_NodeActivationSequenceNumber)) int64 NodeActivationSequenceNumber;
        int64 get_NodeActivationSequenceNumber() { return nodeActivationSequenceNumber_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        void WriteToEtw(uint16 contextSequenceId) const;

        FABRIC_FIELDS_04(upgrades_, fabricVersionInstance_, isNodeActivated_, nodeActivationSequenceNumber_);

    private:

        Common::FabricVersionInstance fabricVersionInstance_;
        std::vector<UpgradeDescription> upgrades_;
        bool isNodeActivated_;
        int64 nodeActivationSequenceNumber_;
    };
}
