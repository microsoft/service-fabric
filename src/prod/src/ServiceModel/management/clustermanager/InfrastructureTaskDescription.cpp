// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

using namespace Management::ClusterManager;

InfrastructureTaskDescription::InfrastructureTaskDescription()
    : srcPartitionId_()
    , taskInstance_()
    , nodeTasks_()
    , asyncAck_(false)
{
}

InfrastructureTaskDescription::InfrastructureTaskDescription(InfrastructureTaskDescription && other)
    : srcPartitionId_(forward<Guid>(other.srcPartitionId_))
    , taskInstance_(forward<TaskInstance>(other.taskInstance_))
    , nodeTasks_(forward<vector<NodeTaskDescription>>(other.nodeTasks_))
    , asyncAck_(forward<bool>(other.asyncAck_))
{
}

InfrastructureTaskDescription::InfrastructureTaskDescription(
    wstring const & taskId,
    uint64 instanceId)
    : srcPartitionId_()
    , taskInstance_(taskId, instanceId)
    , nodeTasks_()
    , asyncAck_(false)
{
}

InfrastructureTaskDescription::InfrastructureTaskDescription(
    wstring const & taskId,
    uint64 instanceId,
    vector<NodeTaskDescription> && nodeTasks)
    : srcPartitionId_()
    , taskInstance_(taskId, instanceId)
    , nodeTasks_(move(nodeTasks))
    , asyncAck_(false)
{
}

bool InfrastructureTaskDescription::operator == (InfrastructureTaskDescription const & other) const
{
    bool matched = (srcPartitionId_ == other.srcPartitionId_);
    if (!matched) { return matched; }

    matched = (taskInstance_ == other.taskInstance_);
    if (!matched) { return matched; }

    matched = this->AreNodeTasksEqual(other);

    // do not match notify, which is just a behavioral flag

    return matched;
}

bool InfrastructureTaskDescription::operator != (InfrastructureTaskDescription const & other) const
{
    return !(*this == other);
}

bool InfrastructureTaskDescription::AreNodeTasksEqual(InfrastructureTaskDescription const & other) const
{
    bool matched = (nodeTasks_.size() == other.nodeTasks_.size());

    if (matched)
    {
        for (size_t ix = 0; ix < nodeTasks_.size(); ++ix)
        {
            if (nodeTasks_[ix] != other.nodeTasks_[ix])
            {
                matched = false;
                break;
            }
        }
    }

    return matched;
}

ErrorCode InfrastructureTaskDescription::FromPublicApi(FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION const & publicDescription)
{
    srcPartitionId_ = Guid(publicDescription.SourcePartitionId);

    wstring id;
    HRESULT hr = StringUtility::LpcwstrToWstring(publicDescription.TaskId, false, id);
    if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

    uint64 instance = publicDescription.InstanceId;

    taskInstance_ = TaskInstance(id, instance);

    vector<NodeTaskDescription> nodeTasks;
    FABRIC_NODE_TASK_DESCRIPTION_LIST * taskList = publicDescription.NodeTasks;

    if (taskList != NULL)
    {
        int taskCount = taskList->Count;
        FABRIC_NODE_TASK_DESCRIPTION const * taskDescription = taskList->Items;

        for (auto ix=0; ix<taskCount; ++ix, ++taskDescription)
        {
            wstring nodeName;
            hr = StringUtility::LpcwstrToWstring(taskDescription->NodeName, false, nodeName);
            if (FAILED(hr)) { return ErrorCode::FromHResult(hr); }

            nodeTasks.push_back(NodeTaskDescription(move(nodeName), NodeTask::FromPublicApi(taskDescription->TaskType)));
        }
    }

    nodeTasks_.swap(nodeTasks);

    // Sort for efficient == implementation
    sort(nodeTasks_.begin(), nodeTasks_.end(),
        [] (NodeTaskDescription const & left, NodeTaskDescription const & right) { return (left < right); });

    // Trim duplicate tasks by NodeName
    nodeTasks_.erase(
        unique(nodeTasks_.begin(), nodeTasks_.end(),
            [] (NodeTaskDescription const & left, NodeTaskDescription const & right) { return (left.NodeName == right.NodeName); }),
        nodeTasks_.end());

    return ErrorCodeValue::Success;
}

void InfrastructureTaskDescription::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_INFRASTRUCTURE_TASK_DESCRIPTION & result) const
{
    result.SourcePartitionId = srcPartitionId_.AsGUID();

    result.TaskId = heap.AddString(taskInstance_.Id);
    result.InstanceId = taskInstance_.Instance;

    result.NodeTasks = heap.AddItem<FABRIC_NODE_TASK_DESCRIPTION_LIST>().GetRawPointer();
    result.NodeTasks->Count = static_cast<ULONG>(nodeTasks_.size());
    result.NodeTasks->Items = heap.AddArray<FABRIC_NODE_TASK_DESCRIPTION>(result.NodeTasks->Count).GetRawArray();

    for (ULONG ix=0; ix<result.NodeTasks->Count; ++ix)
    {
        FABRIC_NODE_TASK_DESCRIPTION & taskDescription = const_cast<FABRIC_NODE_TASK_DESCRIPTION &>(result.NodeTasks->Items[ix]);

        taskDescription.NodeName = heap.AddString(nodeTasks_[ix].NodeName);
        taskDescription.TaskType = NodeTask::ToPublicApi(nodeTasks_[ix].TaskType);
    }
}

void InfrastructureTaskDescription::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write("Task[{0} {1} ack = {2} src = {3}]", taskInstance_, nodeTasks_, asyncAck_, srcPartitionId_);
}

string InfrastructureTaskDescription::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "Task[{0} {1} ack = {2} src = {3}]";
    size_t index = 0;

    traceEvent.AddEventField<TaskInstance>(format, name + ".taskInstance", index);
    traceEvent.AddEventField<vector<NodeTaskDescription>>(format, name + ".tasks", index);
    traceEvent.AddEventField<bool>(format, name + ".ack", index);
    traceEvent.AddEventField<Guid>(format, name + ".src", index);

    return format;
}

void InfrastructureTaskDescription::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<TaskInstance>(taskInstance_);
    context.Write<vector<NodeTaskDescription>>(nodeTasks_);
    context.Write<bool>(asyncAck_);
    context.Write<Guid>(srcPartitionId_);
}

void InfrastructureTaskDescription::UpdateTaskInstance(uint64 instance)
{
    taskInstance_ = TaskInstance(taskInstance_.Id, instance);
}

void InfrastructureTaskDescription::UpdateDoAsyncAck(bool value)
{
    asyncAck_ = value;
}
