// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class BackupLogRecord final 
            : public LogicalLogRecord
        {
            K_FORCE_SHARED(BackupLogRecord)

        public:
            
            // Needed by BeginCheckpoint log record and is hence declared publicly
            static const ULONG DiskSpaceUsed; 

            static BackupLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static BackupLogRecord::SPtr Create(
                __in KGuid const & backupId,
                __in TxnReplicator::Epoch const & highestBackedupEpoch,
                __in LONG64 highestBackedupLsn,
                __in UINT backupLogRecordCount,
                __in UINT backupLogSize,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static BackupLogRecord::SPtr BackupLogRecord::CreateZeroBackupLogRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_BackupId)) KGuid & BackupId;
            KGuid const & get_BackupId() const
            {
                return backupId_;
            }

            __declspec(property(get = get_HighestBackedupEpoch)) TxnReplicator::Epoch & HighestBackedupEpoch;
            TxnReplicator::Epoch const & get_HighestBackedupEpoch() const
            {
                return highestBackedUpEpoch_;
            }

            __declspec(property(get = get_HighestBackedupLsn)) LONG64 HighestBackedupLsn;
            LONG64 get_HighestBackedupLsn() const
            {
                return highestBackedupLsn_;
            }

            __declspec(property(get = get_BackupLogRecordCount)) UINT BackupLogRecordCount;
            UINT get_BackupLogRecordCount() const
            {
                return backupLogRecordCount_;
            }

            __declspec(property(get = get_BackupLogSize)) UINT BackupLogSize;
            UINT get_BackupLogSize() const
            {
                return backupLogSize_;
            }

            bool IsZeroBackupLogRecord() const;

            bool Test_Equals(__in LogRecord const & other) const override;

        private:
            static const ULONG SizeOnWireIncrement;

            //
            // Read LogRecord.h base member for more info
            //
            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            //
            // Read LogRecord.h base member for more info
            //
            void ReadLogical(
                __in Utilities::OperationData const & operationData,
                __inout INT & index) override;

            //
            // Read LogRecord.h base member for more info
            //
            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

            //
            // Read LogRecord.h base member for more info
            //
            ULONG GetSizeOnWire() const override;

            BackupLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            BackupLogRecord(
                __in KGuid const & backupId,
                __in TxnReplicator::Epoch const & highestBackedupEpoch,
                __in LONG64 highestBackedupLsn,
                __in UINT backupLogRecordCount,
                __in UINT backupLogSize,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            void UpdateApproximateDiskSize();

            KGuid backupId_;
            TxnReplicator::Epoch highestBackedUpEpoch_;
            LONG64 highestBackedupLsn_;
            UINT backupLogRecordCount_;
            UINT backupLogSize_;
            KBuffer::SPtr replicatedData_;
        };
    }
}
