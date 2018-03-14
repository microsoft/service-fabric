// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace RepairManager
    {
        class UpdateRepairTaskMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            UpdateRepairTaskMessageBody() : task_() { }
            
            UpdateRepairTaskMessageBody(RepairTask const & task) : task_(task) { }
            
            UpdateRepairTaskMessageBody(RepairTask && task) : task_(std::move(task)) { }

            __declspec(property(get=get_Task)) RepairTask const & Task;
            __declspec(property(get=get_MutableTask)) RepairTask & MutableTask;

            RepairTask const & get_Task() const { return task_; }
            RepairTask & get_MutableTask() { return task_; }

            FABRIC_FIELDS_01(task_);

        private:
            RepairTask task_;
        };
    }
}
