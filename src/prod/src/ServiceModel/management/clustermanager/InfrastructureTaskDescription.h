// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class InfrastructureTaskDescription : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(InfrastructureTaskDescription)
            DEFAULT_COPY_ASSIGNMENT(InfrastructureTaskDescription)
            
        public:
            InfrastructureTaskDescription();

            InfrastructureTaskDescription(InfrastructureTaskDescription &&);

            InfrastructureTaskDescription(
                std::wstring const & taskId,
                uint64 instanceId);

            // For testing
            InfrastructureTaskDescription(
                std::wstring const & taskId,
                uint64 instanceId,
                std::vector<NodeTaskDescription> &&);

            __declspec(property(get=get_SourcePartitionId)) Common::Guid const & SourcePartitionId;
            __declspec(property(get=get_TaskInstance)) TaskInstance const & Task;
            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_InstanceId)) uint64 InstanceId;
            __declspec(property(get=get_NodeTasks)) std::vector<NodeTaskDescription> const NodeTasks;
            __declspec(property(get=get_DoAsyncAck)) bool DoAsyncAck;

            Common::Guid const & get_SourcePartitionId() const { return srcPartitionId_; }
            TaskInstance const & get_TaskInstance() const { return taskInstance_; }
            std::wstring const & get_TaskId() const { return taskInstance_.Id; }
            uint64 const & get_InstanceId() const { return taskInstance_.Instance; }
            std::vector<NodeTaskDescription> const & get_NodeTasks() const { return nodeTasks_; }
            bool get_DoAsyncAck() const { return asyncAck_; }

            bool operator == (InfrastructureTaskDescription const &) const;
            bool operator != (InfrastructureTaskDescription const &) const;
            bool AreNodeTasksEqual(InfrastructureTaskDescription const &) const;

            Common::ErrorCode FromPublicApi(FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION &) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            void UpdateTaskInstance(uint64 instance);

            void UpdateDoAsyncAck(bool);

            FABRIC_FIELDS_04(srcPartitionId_, taskInstance_, nodeTasks_, asyncAck_);

        private:
            Common::Guid srcPartitionId_;
            TaskInstance taskInstance_;
            std::vector<NodeTaskDescription> nodeTasks_;
            bool asyncAck_;
        };
    }
}
