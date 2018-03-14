// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace InfrastructureService
    {
        class ReportTaskMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            ReportTaskMessageBody() : taskId_(), instanceId_(0) { }

            ReportTaskMessageBody(std::wstring const & taskId, uint64 instanceId) : taskId_(taskId), instanceId_(instanceId) { }

            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_InstanceId)) uint64 InstanceId;

            std::wstring const & get_TaskId() const { return taskId_; }
            uint64 get_InstanceId() const { return instanceId_; }

            FABRIC_FIELDS_02(taskId_, instanceId_);

        private:
            std::wstring taskId_;
            uint64 instanceId_;
        };
    }
}
