// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Management
{
    namespace RepairManager
    {
        class DeleteRepairTaskMessageBody : public ServiceModel::ClientServerMessageBody
        {
        public:
            DeleteRepairTaskMessageBody()
                : taskId_()
                , version_(0)
                , scope_()
            {
            }

            DeleteRepairTaskMessageBody(
                RepairScopeIdentifierBaseSPtr const & scope,
                std::wstring const & taskId,
                int64 version)
                : taskId_(taskId)
                , version_(version)
                , scope_(scope)
            {
            }
            
            __declspec(property(get=get_Scope)) RepairScopeIdentifierBaseConstSPtr Scope;
            __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
            __declspec(property(get=get_Version)) int64 Version;

            RepairScopeIdentifierBaseConstSPtr get_Scope() const { return scope_; }
            std::wstring const & get_TaskId() const { return taskId_; }
            int64 get_Version() const { return version_; }

            void ReplaceScope(RepairScopeIdentifierBaseSPtr const & newScope) { scope_ = newScope; }

            Common::ErrorCode FromPublicApi(FABRIC_REPAIR_DELETE_DESCRIPTION const & publicDescription)
            {
                if (publicDescription.Scope == NULL) { return Common::ErrorCodeValue::InvalidArgument; }
                auto error = RepairScopeIdentifierBase::CreateSPtrFromPublicApi(*publicDescription.Scope, scope_);
                if (!error.IsSuccess()) { return error; }

                HRESULT hr = Common::StringUtility::LpcwstrToWstring(
                    publicDescription.RepairTaskId,
                    false,
                    taskId_);
                if (FAILED(hr)) { return Common::ErrorCode::FromHResult(hr); }

                version_ = static_cast<uint64>(publicDescription.Version);

                return Common::ErrorCode::Success();
            }

            FABRIC_FIELDS_03(scope_, taskId_, version_);

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Scope, scope_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::TaskId, taskId_)
                SERIALIZABLE_PROPERTY(ServiceModel::Constants::Version, version_)
            END_JSON_SERIALIZABLE_PROPERTIES()

        private:
            RepairScopeIdentifierBaseSPtr scope_;
            std::wstring taskId_;
            int64 version_;
        };
    }
}
