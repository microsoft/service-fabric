// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class InvalidLogRecords final 
            : public KObject<InvalidLogRecords>
            , public KShared<InvalidLogRecords>
        {
            K_FORCE_SHARED(InvalidLogRecords)

        public:

            static InvalidLogRecords::SPtr Create(__in KAllocator & allocator);

            __declspec(property(get = get_InvLogRecord)) LogRecord::SPtr Inv_LogRecord;
            LogRecord::SPtr get_InvLogRecord() const
            {
                return invalidLogRecord_;
            }

            __declspec(property(get = get_InvPhysicalLogRecord)) PhysicalLogRecord::SPtr Inv_PhysicalLogRecord;
            PhysicalLogRecord::SPtr get_InvPhysicalLogRecord() const
            {
                return invalidPhysicalLogRecord_;
            }

            __declspec(property(get = get_InvLogicalLogRecord)) LogicalLogRecord::SPtr Inv_LogicalLogRecord;
            LogicalLogRecord::SPtr get_InvLogicalLogRecord() const
            {
                return invalidLogicalLogRecord_;
            }

            __declspec(property(get = get_InvTransactionLogRecord)) TransactionLogRecord::SPtr Inv_TransactionLogRecord;
            TransactionLogRecord::SPtr get_InvTransactionLogRecord() const
            {
                return invalidTransactionLogRecord_;
            }

            __declspec(property(get = get_InvBeginTransactionOperationLogRecord)) BeginTransactionOperationLogRecord::SPtr Inv_BeginTransactionOperationLogRecord;
            BeginTransactionOperationLogRecord::SPtr get_InvBeginTransactionOperationLogRecord() const
            {
                return invalidBeginTransactionOperationLogRecord_;
            }

            __declspec(property(get = get_InvBeginCheckpointLogRecord)) BeginCheckpointLogRecord::SPtr Inv_BeginCheckpointLogRecord;
            BeginCheckpointLogRecord::SPtr get_InvBeginCheckpointLogRecord() const
            {
                return invalidBeginCheckpointLogRecord_;
            }

            __declspec(property(get = get_InvIndexingLogRecord)) IndexingLogRecord::SPtr Inv_IndexingLogRecord;
            IndexingLogRecord::SPtr get_InvIndexingLogRecord() const
            {
                return invalidIndexingLogRecord_;
            }

            __declspec(property(get = get_InvInformationLogRecord)) InformationLogRecord::SPtr Inv_InformationLogRecord;
            InformationLogRecord::SPtr get_InvInformationLogRecord() const
            {
                return invalidInformationLogRecord_;
            }

            std::wstring ToString() const;

        private:

            static LogRecord::SPtr InvalidLogRecord(__in KAllocator & allocator);
            static LogicalLogRecord::SPtr InvalidLogicalLogRecord(__in KAllocator & allocator);
            static PhysicalLogRecord::SPtr InvalidPhysicalLogRecord(__in KAllocator & allocator);
            static TransactionLogRecord::SPtr InvalidTransactionLogRecord(__in KAllocator & allocator);
            static BeginTransactionOperationLogRecord::SPtr InvalidBeginTransactionOperationLogRecord(__in KAllocator & allocator);
            static IndexingLogRecord::SPtr InvalidIndexingLogRecord(__in KAllocator & allocator);
            static BeginCheckpointLogRecord::SPtr InvalidBeginCheckpointLogRecord(__in KAllocator & allocator);
            static InformationLogRecord::SPtr InvalidInformationLogRecord(__in KAllocator & allocator);

            LogRecord::SPtr invalidLogRecord_;
            LogicalLogRecord::SPtr invalidLogicalLogRecord_;
            PhysicalLogRecord::SPtr invalidPhysicalLogRecord_;
            TransactionLogRecord::SPtr invalidTransactionLogRecord_;
            BeginTransactionOperationLogRecord::SPtr invalidBeginTransactionOperationLogRecord_;
            IndexingLogRecord::SPtr invalidIndexingLogRecord_;
            BeginCheckpointLogRecord::SPtr invalidBeginCheckpointLogRecord_;
            InformationLogRecord::SPtr invalidInformationLogRecord_;
        };
    }
}
