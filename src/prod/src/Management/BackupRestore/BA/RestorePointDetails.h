// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class RestorePointDetails : public Serialization::FabricSerializable
        {
        public:
            RestorePointDetails();
            ~RestorePointDetails();

            __declspec(property(get = get_OperationId, put = set_OperationId)) Common::Guid OperationId;
            Common::Guid get_OperationId() const { return operationId_; }
            void set_OperationId(Common::Guid value) { operationId_ = value; }

            __declspec(property(get = get_BackupLocationList)) std::vector<std::wstring> & BackupLocationList;
            std::vector<std::wstring> get_BackupLocationList() const { return backupLocationList_; }
            
            __declspec(property(get = get_UserInitiatedOperation, put = set_UserInitiatedOperation)) bool UserInitiatedOperation;
            bool get_UserInitiatedOperation() const { return userInitiatedOperation_; }
            void set_UserInitiatedOperation(bool value) { userInitiatedOperation_ = value; }

            __declspec(property(get = get_BackupStoreInfo, put = set_BackupStoreInfo)) BackupRestoreAgentComponent::BackupStoreInfo BackupStoreInfo;
            BackupRestoreAgentComponent::BackupStoreInfo get_BackupStoreInfo() const { return backupStoreInfo_; }
            void set_BackupLocation(BackupRestoreAgentComponent::BackupStoreInfo value) { backupStoreInfo_ = value; }

            FABRIC_FIELDS_04(
                operationId_,
                backupLocationList_,
                userInitiatedOperation_,
                backupStoreInfo_);

            Common::ErrorCode FromPublicApi(FABRIC_RESTORE_POINT_DETAILS const &);
            Common::ErrorCode ToPublicApi(__in Common::ScopedHeap & heap, __out FABRIC_RESTORE_POINT_DETAILS &) const;

        private:
            std::vector<std::wstring> backupLocationList_;
            bool userInitiatedOperation_;
            Common::Guid operationId_;
            BackupRestoreAgentComponent::BackupStoreInfo backupStoreInfo_;
        };
    }
}
