// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class NodeTaskDescription : public Serialization::FabricSerializable
        {
            DEFAULT_COPY_ASSIGNMENT(NodeTaskDescription)
            
        public:
            NodeTaskDescription();
            NodeTaskDescription(NodeTaskDescription &&);
            NodeTaskDescription(NodeTaskDescription const &);

            NodeTaskDescription &operator=(NodeTaskDescription && other);

            NodeTaskDescription(std::wstring const &, NodeTask::Enum);

            __declspec(property(get=get_NodeName)) std::wstring const & NodeName;
            __declspec(property(get=get_TaskType)) NodeTask::Enum const TaskType;

            std::wstring const & get_NodeName() const { return nodeName_; }
            NodeTask::Enum const get_TaskType() const { return taskType_; }

            bool operator < (NodeTaskDescription const &) const;
            bool operator == (NodeTaskDescription const &) const;
            bool operator != (NodeTaskDescription const &) const;

            FABRIC_FIELDS_02(nodeName_, taskType_);

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

        private:
            std::wstring nodeName_;
            NodeTask::Enum taskType_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::ClusterManager::NodeTaskDescription)
