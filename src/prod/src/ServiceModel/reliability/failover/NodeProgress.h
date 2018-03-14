// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeProgress : public Serialization::FabricSerializable
    {
        DEFAULT_COPY_CONSTRUCTOR(NodeProgress);
        DEFAULT_COPY_ASSIGNMENT(NodeProgress);

    public:

        NodeProgress();

        NodeProgress(
            Federation::NodeId nodeId,
            std::wstring const& nodeName,
            std::vector<SafetyCheckWrapper> const& safetyChecks);

        NodeProgress(NodeProgress && other);
        NodeProgress & operator=(NodeProgress && other);

        __declspec (property(get = get_NodeId)) Federation::NodeId NodeId;
        Federation::NodeId get_NodeId() const { return nodeId_; }

        __declspec (property(get = get_NodeName)) std::wstring const& NodeName;
        std::wstring const& get_NodeName() const { return nodeName_; }

        __declspec (property(get = get_safetyChecks)) std::vector<SafetyCheckWrapper> const& SafetyChecks;
        std::vector<SafetyCheckWrapper> const& get_safetyChecks() const { return safetyChecks_; }

        virtual void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const;

        FABRIC_FIELDS_03(nodeId_, nodeName_, safetyChecks_);

    protected:

        Federation::NodeId nodeId_;
        std::wstring nodeName_;
        std::vector<SafetyCheckWrapper> safetyChecks_;
    };
}

DEFINE_USER_ARRAY_UTILITY(Reliability::NodeProgress);
