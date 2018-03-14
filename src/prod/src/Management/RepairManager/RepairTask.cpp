// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;
using namespace ServiceModel;
using namespace Management::RepairManager;

#define GET_RC( ResourceName ) StringResource::Get( IDS_RM_##ResourceName )

#define LQUOTE1(x) L ## #x
#define LQUOTE(x) LQUOTE1(x)
#define DEFINE_FIELD_LITERAL( Name ) static wchar_t const * const FieldName_##Name = LQUOTE(Name)

DEFINE_FIELD_LITERAL(Scope);
DEFINE_FIELD_LITERAL(TaskId);
DEFINE_FIELD_LITERAL(Version);
DEFINE_FIELD_LITERAL(Description);
DEFINE_FIELD_LITERAL(State);
DEFINE_FIELD_LITERAL(Flags);
DEFINE_FIELD_LITERAL(Action);
DEFINE_FIELD_LITERAL(Target);
DEFINE_FIELD_LITERAL(Executor);
DEFINE_FIELD_LITERAL(ExecutorData);
DEFINE_FIELD_LITERAL(Impact);
DEFINE_FIELD_LITERAL(ResultStatus);
DEFINE_FIELD_LITERAL(ResultCode);
DEFINE_FIELD_LITERAL(ResultDetails);
DEFINE_FIELD_LITERAL(PreparingHealthCheckState);
DEFINE_FIELD_LITERAL(RestoringHealthCheckState);
DEFINE_FIELD_LITERAL(PerformPreparingHealthCheck);
DEFINE_FIELD_LITERAL(PerformRestoringHealthCheck);

const size_t RepairTask::MaxTaskIdSize = 256;
const size_t RepairTask::MaxDescriptionSize = 256;
const size_t RepairTask::MaxActionSize = 64;
const size_t RepairTask::MaxExecutorSize = 256;
const size_t RepairTask::MaxExecutorDataSize = 1024;
const size_t RepairTask::MaxResultDetailsSize = 1024;

DEFINE_QUERY_RESULT_TYPE_ACTIVATOR_FACTORY(Management::RepairManager::RepairTask, RepairTask)

StringLiteral const TraceComponent("RepairTask");

RepairTask::RepairTask()
    : taskId_()
    , version_(0)
    , description_()
    , state_(RepairTaskState::Invalid)
    , flags_(RepairTaskFlags::None)
    , action_()
    , target_()
    , executor_()
    , executorData_()
    , impact_()
    , result_(RepairTaskResult::Invalid)
    , resultCode_(S_OK)
    , resultDetails_()
    , history_()
    , scope_()
    , preparingHealthCheckState_()    
    , restoringHealthCheckState_()
    , performPreparingHealthCheck_(false)    
    , performRestoringHealthCheck_(false)
{
}

RepairTask::RepairTask(RepairTask && other)
    : taskId_(move(other.taskId_))
    , version_(move(other.version_))
    , description_(move(other.description_))
    , state_(move(other.state_))
    , flags_(move(other.flags_))
    , action_(move(other.action_))
    , target_(move(other.target_))
    , executor_(move(other.executor_))
    , executorData_(move(other.executorData_))
    , impact_(move(other.impact_))
    , result_(move(other.result_))
    , resultCode_(move(other.resultCode_))
    , resultDetails_(move(other.resultDetails_))
    , history_(move(other.history_))
    , scope_(move(other.scope_))
    , preparingHealthCheckState_(move(other.preparingHealthCheckState_))
    , restoringHealthCheckState_(move(other.restoringHealthCheckState_))
    , performPreparingHealthCheck_(move(other.performPreparingHealthCheck_))
    , performRestoringHealthCheck_(move(other.performRestoringHealthCheck_))        
{
}

RepairTask::RepairTask(wstring const & taskId)
    : taskId_(taskId)
    , version_(0)
    , description_()
    , state_(RepairTaskState::Invalid)
    , flags_(RepairTaskFlags::None)
    , action_()
    , target_()
    , executor_()
    , executorData_()
    , impact_()
    , result_(RepairTaskResult::Invalid)
    , resultCode_(S_OK)
    , resultDetails_()
    , history_()
    , scope_()
    , preparingHealthCheckState_()    
    , restoringHealthCheckState_()
    , performPreparingHealthCheck_(false)    
    , performRestoringHealthCheck_(false)    
{
}

ErrorCode RepairTask::FromPublicApi(FABRIC_REPAIR_TASK const & publicTask)
{
    HRESULT hr = S_OK;
    ErrorCode error;
    
    if (publicTask.Scope == NULL) { return ErrorMissingRequiredField(FieldName_Scope); }

    error = RepairScopeIdentifierBase::CreateSPtrFromPublicApi(*publicTask.Scope, scope_);
    if (!error.IsSuccess()) { return error; }

    // TODO validate as NamingUri also
    hr = StringUtility::LpcwstrToWstring(
        publicTask.TaskId,
        false,
        1,
        MaxTaskIdSize,
        taskId_);
    if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_TaskId); }

    version_ = publicTask.Version;

    hr = StringUtility::LpcwstrToWstring(
        publicTask.Description,
        true,
        0,
        MaxDescriptionSize,
        description_);
    if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_Description); }

    error = RepairTaskState::FromPublicApi(publicTask.State, state_);
    if (!error.IsSuccess()) { return error; }

    error = RepairTaskFlags::FromPublicApi(static_cast<FABRIC_REPAIR_TASK_FLAGS>(publicTask.Flags), flags_);
    if (!error.IsSuccess()) { return error; }

    hr = StringUtility::LpcwstrToWstring(
        publicTask.Action,
        true,
        0,
        MaxActionSize,
        action_);
    if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_Action); }

    if (publicTask.Target != NULL)
    {
        target_ = RepairTargetDescriptionBase::CreateSPtr(publicTask.Target->Kind);
        error = target_->FromPublicApi(*publicTask.Target);
        if (!error.IsSuccess()) { return error; }
    }

    if (publicTask.ExecutorState != NULL)
    {
        if (publicTask.ExecutorState->Reserved != NULL) { return ErrorCodeValue::InvalidArgument; }

        hr = StringUtility::LpcwstrToWstring(
            publicTask.ExecutorState->Executor,
            true,
            0,
            MaxExecutorSize,
            executor_);
        if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_Executor); }

        hr = StringUtility::LpcwstrToWstring(
            publicTask.ExecutorState->ExecutorData,
            true,
            0,
            MaxExecutorDataSize,
            executorData_);
        if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_ExecutorData); }
    }

    if (publicTask.Impact != NULL)
    {
        impact_ = RepairImpactDescriptionBase::CreateSPtr(publicTask.Impact->Kind);
        error = impact_->FromPublicApi(*publicTask.Impact);
        if (!error.IsSuccess()) { return error; }
    }

    if (publicTask.Result != NULL)
    {
        if (publicTask.Result->Reserved != NULL) { return ErrorCodeValue::InvalidArgument; }

        error = RepairTaskResult::FromPublicApi(publicTask.Result->ResultStatus, result_);
        if (!error.IsSuccess()) { return error; }

        resultCode_ = publicTask.Result->ResultCode;

        hr = StringUtility::LpcwstrToWstring(
            publicTask.Result->ResultDetails,
            true,
            0,
            MaxResultDetailsSize,
            resultDetails_);
        if (FAILED(hr)) { return ErrorInvalidFieldValue(hr, FieldName_ResultDetails); }
    }

    if (publicTask.History != NULL)
    {
        error = history_.FromPublicApi(*publicTask.History);
        if (!error.IsSuccess()) { return error; }
    }

    if (publicTask.Reserved != NULL)
    {
        auto ex1 = static_cast<FABRIC_REPAIR_TASK_EX1*>(publicTask.Reserved);

        error = RepairTaskHealthCheckState::FromPublicApi(ex1->PreparingHealthCheckState, preparingHealthCheckState_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FromPublicApi Extracting Reserved.PreparingHealthCheckState failed. Error: {0}", error);
            return error;
        }

        error = RepairTaskHealthCheckState::FromPublicApi(ex1->RestoringHealthCheckState, restoringHealthCheckState_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceComponent,
                "FromPublicApi Extracting Reserved.RestoringHealthCheckState failed. Error: {0}", error);
            return error;
        }

        performPreparingHealthCheck_ = ex1->PerformPreparingHealthCheck ? true : false;
        performRestoringHealthCheck_ = ex1->PerformRestoringHealthCheck ? true : false;
    }

    return ErrorCode::Success();
}

void RepairTask::ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_REPAIR_TASK & result) const
{
    result.Scope = NULL;
    if (scope_)
    {
        auto scope = heap.AddItem<FABRIC_REPAIR_SCOPE_IDENTIFIER>();
        scope_->ToPublicApi(heap, *scope);
        result.Scope = scope.GetRawPointer();
    }

    result.TaskId = heap.AddString(taskId_);
    result.Version = version_;
    
    result.Description = heap.AddString(description_);
    
    result.State = RepairTaskState::ToPublicApi(state_);
    result.Flags = RepairTaskFlags::ToPublicApi(flags_);
    
    result.Action = heap.AddString(action_);
    
    result.Target = NULL;
    if (target_)
    {
        auto target = heap.AddItem<FABRIC_REPAIR_TARGET_DESCRIPTION>();
        target_->ToPublicApi(heap, *target);
        result.Target = target.GetRawPointer();
    }

    auto executorState = heap.AddItem<FABRIC_REPAIR_EXECUTOR_STATE>();
    executorState->Executor = heap.AddString(executor_);
    executorState->ExecutorData = heap.AddString(executorData_);
    result.ExecutorState = executorState.GetRawPointer();

    result.Impact = NULL;
    if (impact_)
    {
        auto impact = heap.AddItem<FABRIC_REPAIR_IMPACT_DESCRIPTION>();
        impact_->ToPublicApi(heap, *impact);
        result.Impact = impact.GetRawPointer();
    }

    auto resultInfo = heap.AddItem<FABRIC_REPAIR_RESULT_DESCRIPTION>();
    resultInfo->ResultStatus = RepairTaskResult::ToPublicApi(result_);
    resultInfo->ResultCode = resultCode_;
    resultInfo->ResultDetails = heap.AddString(resultDetails_);
    result.Result = resultInfo.GetRawPointer();

    auto history = heap.AddItem<FABRIC_REPAIR_TASK_HISTORY>();
    history_.ToPublicApi(heap, *history);
    result.History = history.GetRawPointer();

    auto ex1 = heap.AddItem<FABRIC_REPAIR_TASK_EX1>();
    ex1->PreparingHealthCheckState = RepairTaskHealthCheckState::ToPublicApi(preparingHealthCheckState_);
    ex1->RestoringHealthCheckState = RepairTaskHealthCheckState::ToPublicApi(restoringHealthCheckState_);
    ex1->PerformPreparingHealthCheck = performPreparingHealthCheck_ ? TRUE : FALSE;
    ex1->PerformRestoringHealthCheck = performRestoringHealthCheck_ ? TRUE : FALSE;
    result.Reserved = ex1.GetRawPointer();
}

void RepairTask::WriteTaskInfo(std::wstring const & prefix) const
{
    wstring buffer;
    Common::StringWriter w(buffer, 2048);
    WriteTo(w, null_format);

    WriteInfo(TraceComponent, "{0}. {1}", prefix, buffer);
}

void RepairTask::WriteTo(Common::TextWriter& w, Common::FormatOptions const &) const
{
    w.Write(
        "RepairTask[scope={7}, taskId={0}, version={1}, description={2}, state={3}, flags={4}, action={5}, target={6}, ",
        taskId_,
        version_,
        description_,
        state_,
        flags_,
        action_,
        target_,
        scope_);

    w.Write(
        "executor={0}, executorData={1}, impact={2}, result={3}, resultCode={4}, resultDetails={5}, history={6}, ",
        executor_,
        executorData_,
        impact_,
        result_,
        resultCode_,
        resultDetails_,
        history_);

    w.Write(
        "preparingHealthCheckState={0}, restoringHealthCheckState={1}, performPreparingHealthCheck={2}, performRestoringHealthCheck={3}]",
        preparingHealthCheckState_,
        restoringHealthCheckState_,
        performPreparingHealthCheck_,
        performRestoringHealthCheck_);    
}

string RepairTask::AddField(Common::TraceEvent & traceEvent, string const & name)
{
    string format = "{0}:{1}:{2}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".taskId", index);
    traceEvent.AddEventField<int64>(format, name + ".version", index);
    traceEvent.AddEventField<RepairTaskState::Trace>(format, name + ".state", index);

    return format;
}

void RepairTask::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<wstring>(taskId_);
    context.Write<int64>(version_);
    context.Write<RepairTaskState::Trace>(state_);
}

bool RepairTask::IsVisibleTo(RepairScopeIdentifierBase const & operationScope) const
{
    return scope_->IsVisibleTo(operationScope);
}

ErrorCode RepairTask::TryPrepareForCreate()
{
    auto error = ValidateCreate();
    if (error.IsSuccess())
    {
        PrepareForCreate();
    }
    return error;
}

ErrorCode RepairTask::TryPrepareForCancel(bool abortRequested)
{
    auto error = ValidateCancel();
    if (error.IsSuccess())
    {
        PrepareForCancel(abortRequested);
    }
    return error;
}

ErrorCode RepairTask::TryPrepareForForceApprove()
{
    auto error = ValidateForceApprove();
    if (error.IsSuccess())
    {
        PrepareForForceApprove();
    }
    return error;
}

ErrorCode RepairTask::TryPrepareForUpdate(RepairTask const & updatedTask)
{
    auto error = ValidateUpdate(updatedTask);
    if (error.IsSuccess())
    {
        PrepareForUpdate(updatedTask);
    }
    return error;
}

bool RepairTask::IsValidTaskId(wstring const & id)
{
    return all_of(id.cbegin(), id.cend(), [](wchar_t c)
    {
        return
            (c >= L'A' && c <= L'Z') ||
            (c >= L'a' && c <= L'z') ||
            (c >= L'0' && c <= L'9') ||
            (c == L'.') ||
            (c == L'-') ||
            (c == L'_') ||
            (c == L'/');
    });
}

ErrorCode RepairTask::ValidateCreate() const
{
    if (!scope_)
    {
        return ErrorMissingRequiredField(FieldName_Scope);
    }

    if (!scope_->IsValid())
    {
        return ErrorInvalidFieldValue(FieldName_Scope);
    }

    if (taskId_.empty())
    {
        return ErrorMissingRequiredField(FieldName_TaskId);
    }

    if (!IsValidTaskId(taskId_))
    {
        return ErrorInvalidFieldValue(FieldName_TaskId);
    }

    if (version_ != 0)
    {
        return ErrorInvalidFieldValue(FieldName_Version);
    }

    switch (state_)
    {
    case RepairTaskState::Created:
    case RepairTaskState::Claimed:
    case RepairTaskState::Preparing:
        break;

    default:
        return ErrorInvalidFieldValue(FieldName_State);
    }

    if (flags_ != RepairTaskFlags::None)
    {
        return ErrorInvalidFieldValue(FieldName_Flags);
    }

    if (action_.empty())
    {
        return ErrorMissingRequiredField(FieldName_Action);
    }

    if (target_ && !target_->IsValid())
    {
        return ErrorInvalidFieldValue(FieldName_Target);
    }

    // Executor should be empty when the state is Created
    if (executor_.empty() != (state_ == RepairTaskState::Created))
    {
        return ErrorInvalidFieldValue(FieldName_Executor);
    }

    // Executor data can be set except in Created
    if ((state_ == RepairTaskState::Created) && !executorData_.empty())
    {
        return ErrorInvalidFieldValue(FieldName_ExecutorData);
    }

    // Impact must be valid if state is Preparing
    if (state_ == RepairTaskState::Preparing)
    {
        if (!(impact_ && impact_->IsValid()))
        {
            return ErrorInvalidFieldValue(FieldName_Impact);
        }
    }
    else if (impact_)
    {
        return ErrorInvalidFieldValue(FieldName_Impact);
    }

    // Must not be set at creation time
    if (!(result_ == RepairTaskResult::Invalid || result_ == RepairTaskResult::Pending))
    {
        return ErrorInvalidFieldValue(FieldName_ResultStatus);
    }

    if (resultCode_ != S_OK)
    {
        return ErrorInvalidFieldValue(FieldName_ResultCode);
    }

    if ((state_ == RepairTaskState::Created) && !resultDetails_.empty())
    {
        return ErrorInvalidFieldValue(FieldName_ResultDetails);
    }

    if (preparingHealthCheckState_ != RepairTaskHealthCheckState::NotStarted)
    {
        return ErrorInvalidFieldValue(FieldName_PreparingHealthCheckState);
    }

    if (restoringHealthCheckState_ != RepairTaskHealthCheckState::NotStarted)
    {
        return ErrorInvalidFieldValue(FieldName_RestoringHealthCheckState);
    }

    return ErrorCode::Success();
}

void RepairTask::PrepareForCreate()
{
    result_ = RepairTaskResult::Pending;
    version_ = DateTime::Now().Ticks;
    history_.Initialize(state_);
}

ErrorCode RepairTask::ValidateCancel() const
{
    switch (state_)
    {
    case RepairTaskState::Restoring:
    case RepairTaskState::Completed:
        return ErrorInvalidOperation();
    }

    return ErrorCode::Success();
}

void RepairTask::PrepareForCancel(bool abortRequested)
{
    auto newFlags = flags_;

    newFlags |= RepairTaskFlags::CancelRequested;

    if (abortRequested)
    {
        newFlags |= RepairTaskFlags::AbortRequested;
    }

    if (flags_ == newFlags)
    {
        // No update needed
        return;
    }

    flags_ = newFlags;

    switch (state_)
    {
    case RepairTaskState::Created:
    case RepairTaskState::Claimed:
        {
            // Unilateral cancellation is permitted in these states
            state_ = RepairTaskState::Completed;
            result_ = RepairTaskResult::Cancelled;
            resultCode_ = ErrorCode(ErrorCodeValue::OperationCanceled).ToHResult();
            resultDetails_ = GET_RC(Repair_Cancelled);
            history_.SetTimestamps(RepairTaskState::Completed);
            break;
        }
    }

    ++version_;
}

ErrorCode RepairTask::ValidateForceApprove() const
{
    switch (state_)
    {
    // ForceApprove not allowed in these early states right now, because RM does not handle them
    // case RepairTaskState::Created:
    // case RepairTaskState::Claimed:

    case RepairTaskState::Preparing:
        break;

    default:
        return ErrorInvalidOperation();
    }

    return ErrorCode::Success();
}

void RepairTask::PrepareForForceApprove()
{
    if (flags_ & RepairTaskFlags::ForcedApproval)
    {
        // No update needed
        return;
    }

    flags_ |= RepairTaskFlags::ForcedApproval;

    if (state_ == RepairTaskState::Preparing)
    {
        SkipPreparingHealthCheck();

        state_ = RepairTaskState::Approved;
        history_.SetTimestamps(RepairTaskState::Approved);
    }

    ++version_;
}

ErrorCode RepairTask::ValidateUpdate(RepairTask const & updatedTask) const
{
    // Validation logic is grouped by the target state
    switch (updatedTask.state_)
    {
    case RepairTaskState::Claimed:
        return ValidateUpdateToClaimed(updatedTask);

    case RepairTaskState::Preparing:
        return ValidateUpdateToPreparing(updatedTask);

    case RepairTaskState::Executing:
        return ValidateUpdateToExecuting(updatedTask);

    case RepairTaskState::Restoring:
        return ValidateUpdateToRestoring(updatedTask);

    case RepairTaskState::Completed:
        return ValidateUpdateToCompleted(updatedTask);
    }

    return ErrorInvalidStateTransition(updatedTask.state_);
}

ErrorCode RepairTask::ValidateUpdateImmutableFields(RepairTask const & other) const
{
    if (description_ != other.description_)
    {
        return ErrorInvalidFieldModification(FieldName_Description);
    }

    if (flags_ != other.flags_)
    {
        return ErrorInvalidFieldModification(FieldName_Flags);
    }

    if (action_ != other.action_)
    {
        return ErrorInvalidFieldModification(FieldName_Action);
    }

    if (target_)
    {
        if (!(other.target_ && (*target_ == *other.target_)))
        {
            return ErrorInvalidFieldModification(FieldName_Target);
        }
    }
    else
    {
        if (other.target_)
        {
            return ErrorInvalidFieldModification(FieldName_Target);
        }
    }

    if (!(other.scope_ && (*scope_ == *other.scope_)))
    {
        return ErrorInvalidFieldModification(FieldName_Scope);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateExecutorField(RepairTask const & other) const
{
    if (state_ == RepairTaskState::Created)
    {
        if (other.executor_.empty())
        {
            return ErrorMissingRequiredField(FieldName_Executor);
        }
    }
    else if (executor_ != other.executor_)
    {
        return ErrorInvalidFieldModification(FieldName_Executor);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateResultFields(RepairTask const & other, bool isRequired) const
{
    if (isRequired)
    {
        switch (other.result_)
        {
        case RepairTaskResult::Succeeded:
        case RepairTaskResult::Cancelled:
        case RepairTaskResult::Interrupted:
        case RepairTaskResult::Failed:
            break;
        default:
            return ErrorInvalidFieldValue(FieldName_ResultStatus);
        }
    }
    else
    {
        if (other.result_ != RepairTaskResult::Pending)
        {
            return ErrorInvalidFieldValue(FieldName_ResultStatus);
        }

        if (other.resultCode_ != S_OK)
        {
            return ErrorInvalidFieldValue(FieldName_ResultCode);
        }
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateCommon(RepairTask const & updatedTask, bool isExecutionComplete) const
{
    auto error = ValidateUpdateImmutableFields(updatedTask);
    if (!error.IsSuccess()) { return error; }

    error = ValidateUpdateExecutorField(updatedTask);
    if (!error.IsSuccess()) { return error; }

    error = ValidateUpdateResultFields(updatedTask, isExecutionComplete);
    if (!error.IsSuccess()) { return error; }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateToClaimed(RepairTask const & updatedTask) const
{
    switch (state_)
    {
    case RepairTaskState::Created:
    case RepairTaskState::Claimed:
        break;

    default:
        return ErrorInvalidStateTransition(updatedTask.state_);
    }

    auto error = ValidateUpdateCommon(updatedTask, false);
    if (!error.IsSuccess()) { return error; }

    // Impact not allowed
    if (updatedTask.impact_)
    {
        return ErrorInvalidFieldValue(FieldName_Impact);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateToPreparing(RepairTask const & updatedTask) const
{
    switch (state_)
    {
    case RepairTaskState::Created:
    case RepairTaskState::Claimed:
        break;

    default:
        return ErrorInvalidStateTransition(updatedTask.state_);
    }

    auto error = ValidateUpdateCommon(updatedTask, false);
    if (!error.IsSuccess()) { return error; }

    // Impact required
    if (!(updatedTask.impact_ && updatedTask.impact_->IsValid()))
    {
        return ErrorInvalidFieldValue(FieldName_Impact);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateToExecuting(RepairTask const & updatedTask) const
{
    switch (state_)
    {
    case RepairTaskState::Approved:
    case RepairTaskState::Executing:
        break;

    default:
        return ErrorInvalidStateTransition(updatedTask.state_);
    }

    auto error = ValidateUpdateCommon(updatedTask, false);
    if (!error.IsSuccess()) { return error; }

    // Impact immutable
    if (!(updatedTask.impact_ && (*impact_ == *updatedTask.impact_)))
    {
        return ErrorInvalidFieldModification(FieldName_Impact);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateToRestoring(RepairTask const & updatedTask) const
{
    switch (state_)
    {
    case RepairTaskState::Approved:
    case RepairTaskState::Executing:
        break;

    default:
        return ErrorInvalidStateTransition(updatedTask.state_);
    }

    auto error = ValidateUpdateCommon(updatedTask, true);
    if (!error.IsSuccess()) { return error; }

    // Impact immutable
    if (!(updatedTask.impact_ && (*impact_ == *updatedTask.impact_)))
    {
        return ErrorInvalidFieldModification(FieldName_Impact);
    }

    return ErrorCode::Success();
}

ErrorCode RepairTask::ValidateUpdateToCompleted(RepairTask const & updatedTask) const
{
    switch (state_)
    {
    case RepairTaskState::Created:
    case RepairTaskState::Claimed:
    case RepairTaskState::Completed: // allow updating specific fields like ExecutorData even if Completed
        break;

    default:
        return ErrorInvalidStateTransition(updatedTask.state_);
    }    

    auto error = ValidateUpdateCommon(updatedTask, true);
    if (error.IsSuccess() && state_ == RepairTaskState::Completed)
    {
        // if state_ is already Completed, then allow updating only ExecutorData field and nothing else

        if (result_ != updatedTask.result_) { return ErrorInvalidFieldModification(FieldName_ResultStatus); }
        if (resultCode_ != updatedTask.resultCode_) { return ErrorInvalidFieldModification(FieldName_ResultCode); }
        if (resultDetails_ != updatedTask.resultDetails_) { return ErrorInvalidFieldModification(FieldName_ResultDetails); }
                
        if ((impact_ && !updatedTask.impact_) || (!impact_ && updatedTask.impact_)) { return ErrorInvalidFieldModification(FieldName_Impact); }
        if (impact_ && (*impact_ != *updatedTask.impact_)) { return ErrorInvalidFieldModification(FieldName_Impact); }

        // TODO, ensure health check status fields are also immutable when health check code is merged (Bug 7097968)
    }
    
    return error;
}

void RepairTask::PrepareForUpdate(RepairTask const & updatedTask)
{
    // Immutable fields on update:
    //  taskId_
    //  description_
    //  flags_
    //  action_
    //  targets_

    ++version_;
        
    state_ = updatedTask.state_;
    executor_ = updatedTask.executor_;
    executorData_ = updatedTask.executorData_;

    impact_.reset();
    if (updatedTask.impact_)
    {
        impact_ = updatedTask.impact_->Clone();
    }

    result_ = updatedTask.result_;
    resultCode_ = updatedTask.resultCode_;
    resultDetails_ = updatedTask.resultDetails_;

    // Update timestamps
    switch (state_)
    {
    case RepairTaskState::Claimed:
        history_.SetTimestamps(RepairTaskState::Claimed);
        break;

    case RepairTaskState::Preparing:
        history_.SetTimestamps(RepairTaskState::Claimed | RepairTaskState::Preparing);
        break;

    case RepairTaskState::Approved:
        history_.SetTimestamps(RepairTaskState::Claimed | RepairTaskState::Preparing | RepairTaskState::Approved);
        break;

    case RepairTaskState::Executing:
        history_.SetTimestamps(RepairTaskState::Executing);
        break;

    case RepairTaskState::Restoring:
        history_.SetTimestamps(RepairTaskState::Executing | RepairTaskState::Restoring);
        break;

    case RepairTaskState::Completed:
        if (updatedTask.state_ == RepairTaskState::Restoring)
        {
            // Caller requested transition to Restoring, and it was a no-op to Completed
            history_.SetTimestamps(RepairTaskState::Executing | RepairTaskState::Restoring | RepairTaskState::Completed);
        }
        else
        {
            history_.SetTimestamps(RepairTaskState::Claimed | RepairTaskState::Completed);
        }
        break;
    }

    // we won't touch the health check related fields and don't validate if it has
    // changed too since it may break backward compatibility
}

void RepairTask::PrepareForSystemApproval()
{
    ASSERT_IFNOT(state_ == RepairTaskState::Preparing, "Expected state = Preparing, actual state = {0}", state_);

    SkipPreparingHealthCheck();

    state_ = RepairTaskState::Approved;
    history_.SetTimestamps(RepairTaskState::Approved);
    ++version_;
}

/// <summary>
/// Moves past the health check in the Preparing state. This is useful
/// when users ForceApprove or Cancel a task in Preparing. It updates the
/// health check timestamps and the preparing health check state.
/// </summary>
void RepairTask::SkipPreparingHealthCheck()
{
    PrepareForPreparingHealthCheckStart();
    PrepareForPreparingHealthCheckEnd(RepairTaskHealthCheckState::Skipped);
}


/// <summary>
/// This is invoked in cases when the Preparing state of a repair task is canceled.
/// E.g. when the preparing health check times out.
/// This is different from the user canceling a repair task.
/// </summary>
void RepairTask::PrepareForSystemCancel()
{
    ASSERT_IFNOT(state_ == RepairTaskState::Preparing, "Expected state = Preparing, actual state = {0}", state_);

    SkipPreparingHealthCheck();

    state_ = RepairTaskState::Restoring;

    result_ = RepairTaskResult::Cancelled;
    resultCode_ = ErrorCode(ErrorCodeValue::OperationCanceled).ToHResult();
    resultDetails_ = GET_RC(Repair_Cancelled);

    history_.SetTimestamps(RepairTaskState::Restoring);

    ++version_;
}

void RepairTask::PrepareForSystemCompletion()
{
    ASSERT_IFNOT(state_ == RepairTaskState::Restoring, "Expected state = Restoring, actual state = {0}", state_);

    state_ = RepairTaskState::Completed;
    history_.SetTimestamps(RepairTaskState::Completed);
    ++version_;
}

void RepairTask::PrepareForPreparingHealthCheckStart()
{
    if (preparingHealthCheckState_ == RepairTaskHealthCheckState::NotStarted)
    {
        preparingHealthCheckState_ = RepairTaskHealthCheckState::InProgress;
        history_.SetPreparingHealthCheckStartTimestamp(DateTime::Now());
    }

    ++version_;
}

void RepairTask::PrepareForPreparingHealthCheckEnd(__in RepairTaskHealthCheckState::Enum const & repairTaskHealthCheckState)
{
    ASSERT_IFNOT(
        repairTaskHealthCheckState == RepairTaskHealthCheckState::Skipped ||
        repairTaskHealthCheckState == RepairTaskHealthCheckState::Succeeded ||
        repairTaskHealthCheckState == RepairTaskHealthCheckState::TimedOut,
        "Expected health check state = Skipped, Succeeded or TimedOut, actual health check state = {0}",
        repairTaskHealthCheckState);

    // update only if we aren't already in a terminal state
    if (preparingHealthCheckState_ == RepairTaskHealthCheckState::InProgress)
    {
        preparingHealthCheckState_ = repairTaskHealthCheckState;
        history_.SetPreparingHealthCheckEndTimestamp(DateTime::Now());
    }
    
    ++version_;
}

void RepairTask::PrepareForRestoringHealthCheckStart()
{
    if (restoringHealthCheckState_ == RepairTaskHealthCheckState::NotStarted)
    {
        restoringHealthCheckState_ = RepairTaskHealthCheckState::InProgress;
        history_.SetRestoringHealthCheckStartTimestamp(DateTime::Now());
    }

    ++version_;
}

void RepairTask::PrepareForRestoringHealthCheckEnd(__in RepairTaskHealthCheckState::Enum const & repairTaskHealthCheckState)
{
    ASSERT_IFNOT(
        repairTaskHealthCheckState == RepairTaskHealthCheckState::Skipped || 
        repairTaskHealthCheckState == RepairTaskHealthCheckState::Succeeded || 
        repairTaskHealthCheckState == RepairTaskHealthCheckState::TimedOut,
        "Expected health check state = Skipped, Succeeded or TimedOut, actual health check state = {0}", 
        repairTaskHealthCheckState);

    // update only if we aren't already in a terminal state
    if (restoringHealthCheckState_ == RepairTaskHealthCheckState::InProgress)
    {
        restoringHealthCheckState_ = repairTaskHealthCheckState;
        history_.SetRestoringHealthCheckEndTimestamp(DateTime::Now());
    }

    ++version_;
}

ErrorCode RepairTask::ValidateUpdateHealthPolicy(    
    __in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr, 
    __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr) const
{
    if (performPreparingHealthCheckSPtr)
    {
        switch (preparingHealthCheckState_)
        {
        case RepairTaskHealthCheckState::NotStarted:
        case RepairTaskHealthCheckState::InProgress:
            break;

        default:
            return ErrorInvalidFieldValue(FieldName_PreparingHealthCheckState);
        }
    }

    if (performRestoringHealthCheckSPtr)
    {
        switch (restoringHealthCheckState_)
        {
        case RepairTaskHealthCheckState::NotStarted:
        case RepairTaskHealthCheckState::InProgress:
            break;
            
        default:
            return ErrorInvalidFieldValue(FieldName_RestoringHealthCheckState);
        }
    }

    // other states such as the flags being set to 0 etc. are okay.
    return ErrorCode::Success();
}

ErrorCode RepairTask::TryPrepareForUpdateHealthPolicy(    
    __in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
    __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr)
{
    auto error = ValidateUpdateHealthPolicy(performPreparingHealthCheckSPtr, performRestoringHealthCheckSPtr);
    if (error.IsSuccess())
    {
        PrepareForUpdateHealthPolicy(performPreparingHealthCheckSPtr, performRestoringHealthCheckSPtr);
    }
    return error;
}

void RepairTask::PrepareForUpdateHealthPolicy(    
    __in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
    __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr)
{
    WriteTaskInfo(L"PrepareForUpdateHealthPolicy start");

    // only update the performPreparingHealthCheck and not PreparingHealthCheckState. The latter is updated when the 
    // health check actually happens. Same with restoring too
    if (performPreparingHealthCheckSPtr)
    {
        performPreparingHealthCheck_ = *performPreparingHealthCheckSPtr;
    }

    if (performRestoringHealthCheckSPtr)
    {
        performRestoringHealthCheck_ = *performRestoringHealthCheckSPtr;
    }

    // more straight forward to increment version even if there was no actual update.
    // that way, if there is no error, clients can assume that some internal state is 
    // updated and therefore version is incremented.
    ++version_;

    WriteTaskInfo(L"PrepareForUpdateHealthPolicy complete");
}

bool RepairTask::CheckExpectedVersion(int64 expectedVersion) const
{
    return ((expectedVersion == 0) || (expectedVersion == version_));
}

ErrorCode RepairTask::ErrorInvalidStateTransition(RepairTaskState::Enum const & newState) const
{
    return ErrorCode(
        ErrorCodeValue::InvalidState,
        wformatString("{0} ({1},{2})", GET_RC(Invalid_State_Transition), state_, newState));
}

ErrorCode RepairTask::ErrorMissingRequiredField(wchar_t const * fieldName) const
{
    return ErrorCode(
        ErrorCodeValue::InvalidArgument,
        wformatString("{0} {1}", GET_RC(Missing_Required_Field), fieldName));
}

ErrorCode RepairTask::ErrorInvalidFieldModification(wchar_t const * fieldName) const
{
    return ErrorCode(
        ErrorCodeValue::InvalidArgument,
        wformatString("{0} {1}", GET_RC(Invalid_Field_Modification), fieldName));
}

ErrorCode RepairTask::ErrorInvalidFieldValue(wchar_t const * fieldName) const
{
    return ErrorCode(
        ErrorCodeValue::InvalidArgument,
        wformatString("{0} {1}", GET_RC(Invalid_Field_Value), fieldName));
}

ErrorCode RepairTask::ErrorInvalidFieldValue(HRESULT hr, wchar_t const * fieldName) const
{
    return ErrorCode(
        ErrorCode::FromHResult(hr).ReadValue(),
        wformatString("{0} {1}", GET_RC(Invalid_Field_Value), fieldName));
}

ErrorCode RepairTask::ErrorInvalidOperation() const
{
    return ErrorCode(
        ErrorCodeValue::InvalidState,
        wformatString("{0} {1}", GET_RC(Invalid_Operation), state_));
}
