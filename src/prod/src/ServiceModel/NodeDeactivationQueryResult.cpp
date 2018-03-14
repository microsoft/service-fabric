// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Reliability;
using namespace ServiceModel;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NodeDeactivationTaskId

INITIALIZE_SIZE_ESTIMATION( NodeDeactivationTaskId )

NodeDeactivationTaskId::NodeDeactivationTaskId()
    : id_()
    , type_(FABRIC_NODE_DEACTIVATION_TASK_TYPE_INVALID)
{
}

NodeDeactivationTaskId::NodeDeactivationTaskId(
    std::wstring const& id,
    FABRIC_NODE_DEACTIVATION_TASK_TYPE type)
    : id_(id)
    , type_(type)
{
}

NodeDeactivationTaskId::NodeDeactivationTaskId(wstring const& batchId)
{
    if (StringUtility::StartsWith<wstring>(batchId, ServiceModel::Constants::NodeDeactivationTaskIdPrefixWindowsAzure.cbegin()))
    {
        type_ = FABRIC_NODE_DEACTIVATION_TASK_TYPE::FABRIC_NODE_DEACTIVATION_TASK_TYPE_INFRASTRUCTURE;
        id_ = batchId;
    }
    else if (StringUtility::StartsWith<wstring>(batchId, ServiceModel::Constants::NodeDeactivationTaskIdPrefixRM.cbegin()))
    {
        type_ = FABRIC_NODE_DEACTIVATION_TASK_TYPE::FABRIC_NODE_DEACTIVATION_TASK_TYPE_REPAIR;

        // Trim the prefix to return just the repair task ID
        id_ = batchId.substr(ServiceModel::Constants::NodeDeactivationTaskIdPrefixRM.size());
    }
    else
    {
        type_ = FABRIC_NODE_DEACTIVATION_TASK_TYPE::FABRIC_NODE_DEACTIVATION_TASK_TYPE_CLIENT;
        id_ = batchId;
    }
}

NodeDeactivationTaskId::NodeDeactivationTaskId(NodeDeactivationTaskId && other)
    : id_(move(other.id_))
    , type_(other.type_)
{
}

NodeDeactivationTaskId & NodeDeactivationTaskId::operator=(NodeDeactivationTaskId && other)
{
    if (this != &other)
    {
        id_ = move(other.id_);
        type_ = other.type_;
    }

    return *this;
}

NodeDeactivationTaskId::~NodeDeactivationTaskId()
{
}

ErrorCode NodeDeactivationTaskId::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_NODE_DEACTIVATION_TASK_ID & publicNodeDeactivationTaskId) const
{
    publicNodeDeactivationTaskId.Id = heap.AddString(id_);
    publicNodeDeactivationTaskId.Type = type_;
    publicNodeDeactivationTaskId.Reserved = NULL;

    return ErrorCode::Success();
}

Common::ErrorCode NodeDeactivationTaskId::FromPublicApi(__in FABRIC_NODE_DEACTIVATION_TASK_ID const& publicNodeDeactivationTaskId)
{
    id_ = publicNodeDeactivationTaskId.Id;
    type_ = publicNodeDeactivationTaskId.Type;

    return ErrorCode::Success();
}

void NodeDeactivationTaskId::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeDeactivationTaskId&>(*this), objectString);
    if (error.IsSuccess())
    {
        w.Write(objectString);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NodeDeactivationTask

INITIALIZE_SIZE_ESTIMATION(NodeDeactivationTask)

NodeDeactivationTask::NodeDeactivationTask()
    : taskId_()
    , intent_(FABRIC_NODE_DEACTIVATION_INTENT_INVALID)
{
}

NodeDeactivationTask::NodeDeactivationTask(
    NodeDeactivationTaskId const& taskId,
    FABRIC_NODE_DEACTIVATION_INTENT intent)
    : taskId_(taskId)
    , intent_(intent)
{
}

NodeDeactivationTask::NodeDeactivationTask(NodeDeactivationTask && other)
    : taskId_(move(other.taskId_))
    , intent_(other.intent_)
{
}

NodeDeactivationTask & NodeDeactivationTask::operator=(NodeDeactivationTask && other)
{
    if (this != &other)
    {
        taskId_ = move(other.taskId_);
        intent_ = other.intent_;
    }

    return *this;
}

NodeDeactivationTask::~NodeDeactivationTask()
{
}

ErrorCode NodeDeactivationTask::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_NODE_DEACTIVATION_TASK & publicNodeDeactivationTask) const
{
    auto publicNodeDeactivationTaskId = heap.AddItem<FABRIC_NODE_DEACTIVATION_TASK_ID>();
    taskId_.ToPublicApi(heap, *publicNodeDeactivationTaskId);

    publicNodeDeactivationTask.TaskId = publicNodeDeactivationTaskId.GetRawPointer();
    publicNodeDeactivationTask.Intent = intent_;
    publicNodeDeactivationTask.Reserved = NULL;

    return ErrorCode::Success();
}

Common::ErrorCode NodeDeactivationTask::FromPublicApi(__in FABRIC_NODE_DEACTIVATION_TASK const& publicNodeDeactivationTask)
{
    ErrorCode error = taskId_.FromPublicApi(*publicNodeDeactivationTask.TaskId);
    if (error.IsSuccess())
    {
        intent_ = publicNodeDeactivationTask.Intent;
    }

    return error;
}

void NodeDeactivationTask::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeDeactivationTask&>(*this), objectString);
    if (error.IsSuccess())
    {
        w.Write(objectString);
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// NodeDeactivationQueryResult

INITIALIZE_SIZE_ESTIMATION(NodeDeactivationQueryResult)

NodeDeactivationQueryResult::NodeDeactivationQueryResult()
    : effectiveIntent_(FABRIC_NODE_DEACTIVATION_INTENT_INVALID)
    , status_(FABRIC_NODE_DEACTIVATION_STATUS_NONE)
    , tasks_()
    , pendingSafetyChecks_()
{
}

NodeDeactivationQueryResult::NodeDeactivationQueryResult(
    FABRIC_NODE_DEACTIVATION_INTENT effectiveIntent,
    FABRIC_NODE_DEACTIVATION_STATUS status,
    vector<NodeDeactivationTask> && tasks,
    vector<SafetyCheckWrapper> const& pendingSafetyChecks)
    : effectiveIntent_(effectiveIntent)
    , status_(status)
    , tasks_(move(tasks))
    , pendingSafetyChecks_(pendingSafetyChecks)
{
}

NodeDeactivationQueryResult::NodeDeactivationQueryResult(NodeDeactivationQueryResult && other)
    : effectiveIntent_(other.effectiveIntent_)
    , status_(other.status_)
    , tasks_(move(other.tasks_))
    , pendingSafetyChecks_(move(other.pendingSafetyChecks_))
{
}

NodeDeactivationQueryResult & NodeDeactivationQueryResult::operator=(NodeDeactivationQueryResult && other)
{
    if (this != &other)
    {
        effectiveIntent_ = other.effectiveIntent_;
        status_ = other.status_;
        tasks_ = move(other.tasks_);
        pendingSafetyChecks_ = move(other.pendingSafetyChecks_);
    }

    return *this;
}

NodeDeactivationQueryResult::~NodeDeactivationQueryResult()
{
}

ErrorCode NodeDeactivationQueryResult::ToPublicApi(
    __in Common::ScopedHeap & heap,
    __out FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM & publicNodeDeactivationQueryResult) const
{
    publicNodeDeactivationQueryResult.EffectiveIntent = effectiveIntent_;
    publicNodeDeactivationQueryResult.Status = status_;

    auto publicTaskList = heap.AddItem<FABRIC_NODE_DEACTIVATION_TASK_LIST>();
    ErrorCode error = PublicApiHelper::ToPublicApiList<NodeDeactivationTask, FABRIC_NODE_DEACTIVATION_TASK, FABRIC_NODE_DEACTIVATION_TASK_LIST>(
        heap,
        tasks_,
        *publicTaskList);
    if (!error.IsSuccess()) { return error; }
    publicNodeDeactivationQueryResult.Tasks = publicTaskList.GetRawPointer();

    auto ex1 = heap.AddItem<FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM_EX1>();
    auto publicPendingSafetyCheckList = heap.AddItem<FABRIC_SAFETY_CHECK_LIST>();
    error = PublicApiHelper::ToPublicApiList<SafetyCheckWrapper, FABRIC_SAFETY_CHECK, FABRIC_SAFETY_CHECK_LIST>(
        heap,
        pendingSafetyChecks_,
        *publicPendingSafetyCheckList);
    if (!error.IsSuccess()) { return error; }
    ex1->PendingSafetyChecks = publicPendingSafetyCheckList.GetRawPointer();

    ex1->Reserved = NULL;
    publicNodeDeactivationQueryResult.Reserved = ex1.GetRawPointer();

    return error;
}

Common::ErrorCode NodeDeactivationQueryResult::FromPublicApi(__in FABRIC_NODE_DEACTIVATION_QUERY_RESULT_ITEM const& publicNodeDeactivationQueryResult)
{
    effectiveIntent_ = publicNodeDeactivationQueryResult.EffectiveIntent;
    status_ = publicNodeDeactivationQueryResult.Status;

    ErrorCode error = PublicApiHelper::FromPublicApiList<NodeDeactivationTask, FABRIC_NODE_DEACTIVATION_TASK_LIST>(
        publicNodeDeactivationQueryResult.Tasks,
        tasks_);

    return error;
}

void NodeDeactivationQueryResult::WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
{
    wstring objectString;
    auto error = JsonHelper::Serialize(const_cast<NodeDeactivationQueryResult&>(*this), objectString);
    if (error.IsSuccess())
    {
        w.Write(objectString);
    }
}
