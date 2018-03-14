// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class BackupOperationResultMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            BackupOperationResultMessageBody();
            BackupOperationResultMessageBody(const FABRIC_BACKUP_OPERATION_RESULT& operationResult);
            ~BackupOperationResultMessageBody();

            __declspec(property(get = get_ServiceName, put = set_ServiceName)) wstring ServiceName;
            wstring get_ServiceName() const { return serviceName_; }
            void set_ServiceName(wstring value) { serviceName_ = value; }

            __declspec(property(get = get_PartitionId, put = set_PartitionId)) Common::Guid PartitionId;
            Common::Guid get_PartitionId() const { return partitionId_; }
            void set_PartitionId(Common::Guid value) { partitionId_ = value; }

            __declspec(property(get = get_OperationId, put = set_OperationId)) Common::Guid OperationId;
            Common::Guid get_OperationId() const { return operationId_; }
            void set_OperationId(Common::Guid value) { operationId_ = value; }

            __declspec(property(get = get_ErrorCode, put = set_ErrorCode)) HRESULT ErrorCode;
            HRESULT get_ErrorCode() const { return errorCode_; }
            void set_ErrorCode(HRESULT value) { errorCode_ = value; }

            __declspec(property(get = get_OperationTimestampUtc, put = set_OperationTimestampUtc)) Common::DateTime OperationTimestampUtc;
            Common::DateTime get_OperationTimestampUtc() const { return operationTimestampUtc_; }
            void set_OperationTimestampUtc(Common::DateTime value) { operationTimestampUtc_ = value; }

            __declspec(property(get = get_Message, put = set_Message)) wstring Message;
            wstring get_Message() const { return message_; }
            void set_Message(wstring value) { message_ = value; }
            
            __declspec(property(get = get_BackupId, put = set_BackupId)) Common::Guid BackupId;
            Common::Guid get_BackupId() const { return backupId_; }
            void set_BackupId(Common::Guid value) { backupId_ = value; }

            __declspec(property(get = get_BackupLocation, put = set_BackupLocation)) wstring BackupLocation;
            wstring get_BackupLocation() const { return backupLocation_; }
            void set_BackupLocation(wstring value) { backupLocation_ = value; }

            __declspec(property(get = get_Epoch, put = set_Epoch)) FABRIC_EPOCH Epoch;
            FABRIC_EPOCH get_Epoch() const { return epoch_; }
            void set_Epoch(FABRIC_EPOCH value) { epoch_ = value; }

            __declspec(property(get = get_Lsn, put = set_Lsn)) FABRIC_SEQUENCE_NUMBER Lsn;
            FABRIC_SEQUENCE_NUMBER get_Lsn() const { return lsn_; }
            void set_Lsn(FABRIC_SEQUENCE_NUMBER value) { lsn_ = value; }

            FABRIC_FIELDS_11(
                serviceName_,
                partitionId_,
                operationId_,
                errorCode_,
                message_,
                operationTimestampUtc_,
                backupId_,
                backupLocation_,
                epoch_.ConfigurationNumber,
                epoch_.DataLossNumber,
                lsn_);

        private:
            wstring serviceName_;
            Common::Guid partitionId_;
            Common::Guid operationId_;
            HRESULT errorCode_;
            wstring message_;
            Common::DateTime operationTimestampUtc_;
            Common::Guid backupId_;
            wstring backupLocation_;
            FABRIC_EPOCH epoch_;
            FABRIC_SEQUENCE_NUMBER lsn_;
        };
    }
}

