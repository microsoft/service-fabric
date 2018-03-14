// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class TaskInstance : public ServiceModel::ClientServerMessageBody
        {
            DEFAULT_COPY_CONSTRUCTOR(TaskInstance)
            DEFAULT_COPY_ASSIGNMENT(TaskInstance)
            
        public:
            TaskInstance() : taskId_(), instanceId_(0) { }

            TaskInstance(TaskInstance && other) : taskId_(std::move(other.taskId_)), instanceId_(std::move(other.instanceId_)) { }

            TaskInstance(
                std::wstring const & taskId,
                uint64 instanceId)
                : taskId_(taskId)
                , instanceId_(instanceId)
            {
            }

            TaskInstance & operator=(TaskInstance && other)
            {
                if (this != &other)
                {
                    taskId_ = std::move(other.taskId_);
                    instanceId_ = other.instanceId_;
                }
                return *this;
            }

            __declspec(property(get=get_TaskId)) std::wstring const & Id;
            __declspec(property(get=get_InstanceId)) uint64 Instance;

            std::wstring const & get_TaskId() const { return taskId_; }
            uint64 const & get_InstanceId() const { return instanceId_; }

            bool operator == (TaskInstance const & other) const
            {
                return (taskId_ == other.taskId_ && instanceId_ == other.instanceId_);
            }
            
            bool operator != (TaskInstance const & other) const
            {
                return !(*this == other);
            }

            void WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
            {
                w << taskId_ << ":" << instanceId_;
            }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name)
            {
                std::string format = "{0}:{1}";
                size_t index = 0;

                traceEvent.AddEventField<std::wstring>(format, name + ".id", index);
                traceEvent.AddEventField<uint64>(format, name + ".instance", index);

                return format;
            }

            void FillEventData(Common::TraceEventContext & context) const
            {
                context.Write<std::wstring>(taskId_);
                context.Write<uint64>(instanceId_);
            }

            FABRIC_FIELDS_02(taskId_, instanceId_);

        private:
            std::wstring taskId_;
            uint64 instanceId_;
        };
    }
}
