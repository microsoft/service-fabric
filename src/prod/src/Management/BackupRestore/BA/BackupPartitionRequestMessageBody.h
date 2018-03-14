// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupPartitionRequestMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            BackupPartitionRequestMessageBody();
            BackupPartitionRequestMessageBody(GUID operationId, const FABRIC_BACKUP_CONFIGURATION& configuration);
            ~BackupPartitionRequestMessageBody();

            __declspec(property(get = get_OperationId, put = set_OperationId)) GUID OperationId;
            GUID get_OperationId() const { return operationId_; }
            void set_OperationId(GUID value) { operationId_ = value; }

            __declspec(property(get = get_BackupConfiguration, put = set_BackupConfiguration)) BackupRestoreAgentComponent::BackupConfiguration BackupConfiguration;
            BackupRestoreAgentComponent::BackupConfiguration get_BackupConfiguration() const { return backupConfiguration_; }
            void set_BackupConfiguration(BackupRestoreAgentComponent::BackupConfiguration value) { backupConfiguration_ = value; }

            FABRIC_FIELDS_02(
                operationId_,
                backupConfiguration_);

        private:
            GUID operationId_;
            BackupRestoreAgentComponent::BackupConfiguration backupConfiguration_;
        };
    }
}
