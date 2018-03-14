// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace BackupRestoreAgentComponent
    {
        class RestoreOperationResultMessageBody :
            public Serialization::FabricSerializable
        {
        public:
            RestoreOperationResultMessageBody();
            RestoreOperationResultMessageBody(wstring serviceName, Common::Guid partitionId, Common::Guid operationId, HRESULT errorCode, Common::DateTime operationTimestampUtc, wstring message);
            RestoreOperationResultMessageBody(const FABRIC_RESTORE_OPERATION_RESULT& operationResult);
            ~RestoreOperationResultMessageBody();

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
            
            FABRIC_FIELDS_06(
                serviceName_,
                partitionId_,
                operationId_,
                errorCode_,
                operationTimestampUtc_,
                message_);

        private:
            wstring serviceName_;
            Common::Guid partitionId_;
            Common::Guid operationId_;
            HRESULT errorCode_;
            wstring message_;
            Common::DateTime operationTimestampUtc_;
        };
    }
}

