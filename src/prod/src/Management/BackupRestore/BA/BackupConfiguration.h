// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupConfiguration : public Serialization::FabricSerializable
        {
        public:
            BackupConfiguration();
            ~BackupConfiguration();

            __declspec(property(get = get_OperationTimeoutMilliseconds, put = set_OperationTimeoutMilliseconds)) DWORD OperationTimeoutMilliseconds;
            DWORD get_OperationTimeoutMilliseconds() const { return operationTimeoutMilliseconds_; }
            void set_OperationTimeoutMilliseconds(DWORD value) { operationTimeoutMilliseconds_ = value; }

            __declspec(property(get = get_BackupStoreInfo, put = set_BackupStoreInfo)) BackupRestoreAgentComponent::BackupStoreInfo BackupStoreInfo;
            BackupRestoreAgentComponent::BackupStoreInfo get_BackupStoreInfo() const { return backupStoreInfo_; }
            void set_BackupLocation(BackupRestoreAgentComponent::BackupStoreInfo value) { backupStoreInfo_ = value; }

            FABRIC_FIELDS_02(
                operationTimeoutMilliseconds_,
                backupStoreInfo_);

            Common::ErrorCode ToPublicApi(
                __in Common::ScopedHeap & heap,
                __out FABRIC_BACKUP_CONFIGURATION & fabricBackupConfiguration) const;

            Common::ErrorCode FromPublicApi(
                FABRIC_BACKUP_CONFIGURATION const & fabricBackupConfiguration);

        private:
            BackupRestoreAgentComponent::BackupStoreInfo backupStoreInfo_;
            DWORD operationTimeoutMilliseconds_;
        };
    }
}
