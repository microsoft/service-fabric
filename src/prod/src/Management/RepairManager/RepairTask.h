// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace RepairManager
    {
        class RepairTask
            : public Serialization::FabricSerializable
            , public Common::IFabricJsonSerializable
            , public Common::TextTraceComponent<Common::TraceTaskCodes::RepairManager>
        {
            DEFAULT_COPY_CONSTRUCTOR(RepairTask)
            DEFAULT_COPY_ASSIGNMENT(RepairTask)
        public:

            static const size_t MaxTaskIdSize;
            static const size_t MaxDescriptionSize;
            static const size_t MaxActionSize;
            static const size_t MaxExecutorSize;
            static const size_t MaxExecutorDataSize;
            static const size_t MaxResultDetailsSize;

            RepairTask();
            RepairTask(RepairTask &&);
            RepairTask(std::wstring const & taskId);

            __declspec(property(get=get_Scope)) RepairScopeIdentifierBaseConstSPtr Scope;
            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_Action)) std::wstring const & Action;
            __declspec(property(get=get_Version)) int64 Version;
            __declspec(property(get=get_IsCompleted)) bool IsCompleted;
            __declspec(property(get=get_IsCancelRequested)) bool IsCancelRequested;
            __declspec(property(get=get_State)) RepairTaskState::Enum State;
            __declspec(property(get=get_Executor)) std::wstring const & Executor;
            __declspec(property(get=get_Impact)) RepairImpactDescriptionBaseConstSPtr Impact;
            __declspec(property(get=get_History)) RepairTaskHistory const & History;
            __declspec(property(get=get_PreparingHealthCheckState)) RepairTaskHealthCheckState::Enum const & PreparingHealthCheckState;
            __declspec(property(get=get_RestoringHealthCheckState)) RepairTaskHealthCheckState::Enum const & RestoringHealthCheckState;
            __declspec(property(get=get_PerformPreparingHealthCheck)) bool const & PerformPreparingHealthCheck;
            __declspec(property(get=get_PerformRestoringHealthCheck)) bool const & PerformRestoringHealthCheck;

            RepairScopeIdentifierBaseConstSPtr get_Scope() const { return scope_; }
            RepairScopeIdentifierBaseSPtr get_ScopeMutable() { return scope_; }
            std::wstring const & get_TaskId() const { return taskId_; }
            std::wstring const & get_Action() const { return action_; }
            int64 get_Version() const { return version_; }
            bool get_IsCompleted() const { return state_ == RepairTaskState::Completed; }
            bool get_IsCancelRequested() const { return (flags_ & RepairTaskFlags::CancelRequested) != 0; }
            RepairTaskState::Enum get_State() const { return state_; }
            std::wstring const & get_Executor() const { return executor_; }
            RepairImpactDescriptionBaseConstSPtr get_Impact() const { return impact_; }
            RepairTaskHistory const & get_History() const { return history_; }
            RepairTaskHealthCheckState::Enum const & get_PreparingHealthCheckState() const { return preparingHealthCheckState_; }
            RepairTaskHealthCheckState::Enum const & get_RestoringHealthCheckState() const { return restoringHealthCheckState_; }
            bool get_PerformPreparingHealthCheck() const { return performPreparingHealthCheck_; }
            bool get_PerformRestoringHealthCheck() const { return performRestoringHealthCheck_; }

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_TASK const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_REPAIR_TASK &) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;

            bool IsVisibleTo(RepairScopeIdentifierBase const &) const;
            void ReplaceScope(RepairScopeIdentifierBaseSPtr const & newScope) { scope_ = newScope; }

            Common::ErrorCode TryPrepareForCreate();
            Common::ErrorCode TryPrepareForCancel(bool abortRequested);
            Common::ErrorCode TryPrepareForForceApprove();
            Common::ErrorCode TryPrepareForUpdate(RepairTask const &);
            Common::ErrorCode TryPrepareForUpdateHealthPolicy(                
                __in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
                __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr);

            void PrepareForSystemApproval();
            void PrepareForSystemCancel();
            void PrepareForSystemCompletion();
            void PrepareForPreparingHealthCheckStart();
            void PrepareForPreparingHealthCheckEnd(__in RepairTaskHealthCheckState::Enum const & repairTaskHealthCheckState);
            void PrepareForRestoringHealthCheckStart();
            void PrepareForRestoringHealthCheckEnd(__in RepairTaskHealthCheckState::Enum const & repairTaskHealthCheckState);

            bool CheckExpectedVersion(int64 expectedVersion) const;

            FABRIC_FIELDS_19(scope_, taskId_, version_, description_, state_, flags_, action_, target_, executor_, executorData_, impact_, result_, resultCode_, resultDetails_, history_, preparingHealthCheckState_, restoringHealthCheckState_, performPreparingHealthCheck_, performRestoringHealthCheck_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Scope, scope_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TaskId, taskId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Description, description_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::State, state_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Flags, (LONG&)flags_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Action, action_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Target, target_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Executor, executor_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ExecutorData, executorData_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Impact, impact_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::ResultStatus, result_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ResultCode, resultCode_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::ResultDetails, resultDetails_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::History, history_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::PreparingHealthCheckState, preparingHealthCheckState_)
                SERIALIZABLE_PROPERTY_ENUM(ServiceModel::Constants::RestoringHealthCheckState, restoringHealthCheckState_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PerformPreparingHealthCheck, performPreparingHealthCheck_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::PerformRestoringHealthCheck, performRestoringHealthCheck_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:

            static bool IsValidTaskId(std::wstring const & id);

            Common::ErrorCode ValidateCreate() const;
            void PrepareForCreate();

            Common::ErrorCode ValidateCancel() const;
            void PrepareForCancel(bool abortRequested);

            Common::ErrorCode ValidateForceApprove() const;
            void PrepareForForceApprove();

            Common::ErrorCode ValidateUpdateHealthPolicy(__in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr, __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr) const;
            void PrepareForUpdateHealthPolicy(                
                __in std::shared_ptr<bool> const & performPreparingHealthCheckSPtr,
                __in std::shared_ptr<bool> const & performRestoringHealthCheckSPtr);
            void SkipPreparingHealthCheck();

            Common::ErrorCode ValidateUpdate(RepairTask const &) const;
            Common::ErrorCode ValidateUpdateToClaimed(RepairTask const &) const;
            Common::ErrorCode ValidateUpdateToPreparing(RepairTask const &) const;
            Common::ErrorCode ValidateUpdateToExecuting(RepairTask const &) const;
            Common::ErrorCode ValidateUpdateToRestoring(RepairTask const &) const;
            Common::ErrorCode ValidateUpdateToCompleted(RepairTask const &) const;
            void PrepareForUpdate(RepairTask const &);

            Common::ErrorCode ValidateUpdateImmutableFields(RepairTask const & other) const;
            Common::ErrorCode ValidateUpdateExecutorField(RepairTask const & other) const;
            Common::ErrorCode ValidateUpdateResultFields(RepairTask const & other, bool isRequired) const;
            Common::ErrorCode ValidateUpdateCommon(RepairTask const & updatedTask, bool isExecutionComplete) const;

            Common::ErrorCode ErrorInvalidStateTransition(RepairTaskState::Enum const & newState) const;
            Common::ErrorCode ErrorMissingRequiredField(wchar_t const * const fieldName) const;
            Common::ErrorCode ErrorInvalidFieldModification(wchar_t const * const fieldName) const;
            Common::ErrorCode ErrorInvalidFieldValue(wchar_t const * const fieldName) const;
            Common::ErrorCode ErrorInvalidFieldValue(HRESULT hr, wchar_t const * fieldName) const;
            Common::ErrorCode ErrorInvalidOperation() const;

            void WriteTaskInfo(std::wstring const & prefix) const;

            RepairScopeIdentifierBaseSPtr scope_;

            std::wstring taskId_;
            int64 version_;
            std::wstring description_;

            RepairTaskState::Enum state_;
            RepairTaskFlags::Enum flags_;

            std::wstring action_;
            RepairTargetDescriptionBaseSPtr target_;

            std::wstring executor_;
            std::wstring executorData_;

            RepairImpactDescriptionBaseSPtr impact_;

            RepairTaskResult::Enum result_;
            HRESULT resultCode_;
            std::wstring resultDetails_;

            RepairTaskHistory history_;

            RepairTaskHealthCheckState::Enum preparingHealthCheckState_;
            RepairTaskHealthCheckState::Enum restoringHealthCheckState_;
            bool performPreparingHealthCheck_;
            bool performRestoringHealthCheck_;            
        };
    }
}

DEFINE_QUERY_RESULT_INTERNAL_LIST_ITEM(Management::RepairManager::RepairTask, ServiceModel::QueryResultHelpers::RepairTask)
