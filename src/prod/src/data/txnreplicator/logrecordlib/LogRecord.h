// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // A base class for all types of log records
        // Refer to the heirarchy structure of log records here - 
        // onenote:https://microsoft.sharepoint.com/teams/WindowsFabric/Notebooks/V2/XReplicator/Design%20Notes.one#Log%20Records&section-id={3BBAA96D-450C-4ED7-B059-A94424A8F811}&page-id={56704092-FF78-45AF-AEE0-E009705FF649}&end
        //
        class LogRecord 
            : public KObject<LogRecord>
            , public KShared<LogRecord>
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED_WITH_INHERITANCE(LogRecord)

        public:

            //
            // Returns TRUE if record is either nullptr or the type is Invalid
            //
            static bool IsInvalid(__in_opt LogRecord const * const record);

            //
            // Returns TRUE if record type is one of the physical log record types
            //
            static bool IsPhysical(__in LogRecord const & record);

            static bool IsBarrierRecord(
                __in LogRecord const & record,
                __in bool considerCommitAsBarrier);

            static bool IsTransactionLogRecord(__in LogRecord const & logRecord);

            static bool IsOperationLogRecord(__in LogRecord const & logRecord);

            //
            // Returns TRUE if the record is a commit record.
            //
            static bool IsCommitLogRecord(__in LogRecord const & logRecord);

            //
            // Creates a wstring in json format with the information of the log record 
            //
            virtual std::wstring ToString() const;

            //
            // Invoked by the PhysicalLogWriter::FlushTask() method in order to serialize the log record before writing it to the log
            //      record - The log record to be serialized
            //      binaryWriter - A temporary binary writer that can be used for serializing data
            //
            static Utilities::OperationData::CSPtr WriteRecord(
                __in LogRecord & record,
                __in Utilities::BinaryWriter & binaryWriter,
                __in KAllocator & allocator,
                __in bool isPhysicalWrite = true,
                __in bool setRecordLength = true,
                __in bool forceRecomputeOffsets = false);
            
            //
            // Invoked by the idle/secondary to deserialize the log record data from the operation data object
            //      operationData - input data to be deserialized into the log record
            //      
            static LogRecord::SPtr ReadFromOperationData(
                __in Utilities::OperationData const & operationData,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            static KArray<LogRecord::SPtr> ReadFromOperationDataBatch(
                __in Utilities::OperationData const & operationData,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            //
            // Primarily used by the recovery and backup log readers to read the next record in the input file stream
            //
            static ktl::Awaitable<LogRecord::SPtr> ReadNextRecordAsync(
                __in ktl::io::KStream & stream,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator,
                __in bool isPhysicalRead = true,
                __in bool useInvalidRecordPosition = false,
                __in bool setRecordLength = true);

            //
            // Primarily used by the recovery and backup log readers to read the next record in the input file stream
            //
            static ktl::Awaitable<LogRecord::SPtr> ReadNextRecordAsync(
                __in ktl::io::KStream & stream,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator,
                __out ULONG64 & nextRecordPosition,
                __in bool isPhysicalRead = true,
                __in bool useInvalidRecordPosition = false,
                __in bool setRecordLength = true);

            //
            // Primarily used by the recovery and backup log readers to read the previous record in the input file stream
            //
            static ktl::Awaitable<LogRecord::SPtr> ReadPreviousRecordAsync(
                __in Data::Log::ILogicalLogReadStream & stream,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator);

            __declspec(property(get = get_RecordType)) LogRecordType::Enum RecordType;
            LogRecordType::Enum get_RecordType() const
            {
                return recordType_;
            }

            std::wstring get_RecordTypeName() const;

            //
            // TRUE if the log record has been applied, which implies it has been logged and processed by the dispatcher
            //
            __declspec(property(get = get_IsApplied)) bool IsApplied;
            bool get_IsApplied() const
            {
                return appliedTask_->IsCompleted;
            }

            //
            // TRUE if the log record has been flushed to the disk
            //
            __declspec(property(get = get_IsFlushed)) bool IsFlushed;
            bool get_IsFlushed() const
            {
                return flushedTask_->IsCompleted;
            }

            //
            // TRUE if the log record has been flushed, applied and replicated. 
            // If this is a secondary, it just means the record is flushed and applied
            //
            __declspec(property(get = get_IsProcessed)) bool IsProcessed;
            bool get_IsProcessed() const
            {
                return processedTask_->IsCompleted;
            }

            __declspec(property(get = get_FlushError)) NTSTATUS FlushError;
            NTSTATUS get_FlushError() const
            {
                return flushedTask_->CompletionCode;
            }

            //
            // Logical Sequence Number of the log record. It is guaranteed to be increasing (Not strictly increasing)
            //
            __declspec(property(get = get_Lsn, put = set_Lsn)) LONG64 Lsn;
            LONG64 get_Lsn() const
            {
                return lsn_;
            }
            void set_Lsn(LONG64 value)
            {
                lsn_ = value;
            }

            //
            // Physical Sequence Number of the log record. It is guaranteed to be strictly increasing 
            //
            __declspec(property(get = get_Psn, put = set_Psn)) LONG64 Psn;
            LONG64 get_Psn() const
            {
                return psn_;
            }
            void set_Psn(LONG64 value)
            {
                psn_ = value;
            }

            __declspec(property(get = get_RecordSize)) ULONG RecordSize;
            ULONG get_RecordSize() const
            {
                ASSERT_IFNOT(recordLength_ > 0, "Invalid record size: {0}", recordLength_);

                return recordLength_ + (2 * sizeof(ULONG));
            }

            // 
            // Represents the serialized size of a log record on disk
            // This is valid only after a Read() call on recovery code paths
            // This is valid at all times after construction on primary and secondary replicator roles.
            //
            __declspec(property(get = get_ApproximateSizeOnDisk, put = set_ApproximateSizeOnDisk)) ULONG ApproximateSizeOnDisk;
            ULONG get_ApproximateSizeOnDisk() const
            {
                return approximateDiskSize_;
            }

            // 
            // Size of the log record without considering the 2 ULONG's that are stored with every log record to store the size itself
            // (1 before and 1 after the record)
            //
            __declspec(property(get = get_RecordLength, put = set_RecordLength)) ULONG RecordLength;
            ULONG get_RecordLength() const
            {
                return recordLength_;
            }
            void set_RecordLength(ULONG value)
            {
                if (recordLength_ == Constants::InvalidRecordLength)
                {
                    recordLength_ = value;
                    return;
                }

                ASSERT_IFNOT(recordLength_ == value, "Invalid log record length in setter: {0} {1}", recordLength_, value);
            }
 
            // 
            // Offset of the log record in the logical log stream. Always increasing
            //
            __declspec(property(get = get_RecordPosition, put = set_RecordPosition)) ULONG64 RecordPosition;
            ULONG64 get_RecordPosition() const
            {
                return recordPosition_;
            }
            void set_RecordPosition(ULONG64 value)
            {
                ASSERT_IFNOT(recordPosition_ == Constants::InvalidRecordPosition, "Invalid log record position in setter: {0}", recordPosition_);

                recordPosition_ = value;
            }

            __declspec(property(get = get_previousPhysicalRecordOffset)) ULONG64 PreviousPhysicalRecordOffset;
            ULONG64 get_previousPhysicalRecordOffset() const
            {
                return previousPhysicalRecordOffset_;
            }

            void Test_SetPreviousPhysicalRecordOffsetToZero()
            {
                previousPhysicalRecordOffset_ = 0;
            }

            __declspec(property(get = get_previousPhysicalRecord, put = set_previousPhysicalRecord)) KSharedPtr<PhysicalLogRecord> PreviousPhysicalRecord;
            KSharedPtr<PhysicalLogRecord> get_previousPhysicalRecord() const;
            void set_previousPhysicalRecord(__in_opt PhysicalLogRecord * const value);

            // 
            // Used to avoid dynamic cast and fast casting
            //
            virtual LogicalLogRecord * AsLogicalLogRecord()
            {
                return nullptr;
            }

            // 
            // Used to avoid dynamic cast and fast casting
            //
            virtual PhysicalLogRecord * AsPhysicalLogRecord()
            {
                return nullptr;
            }

            // 
            // Used to free any backward links older than the current head when a new head is chosen
            // Returns true if there were such links that got free'd, else returns false - for trace diag
            //
            virtual bool FreePreviousLinksLowerThanPsn(
                __in LONG64 newHeadPsn,
                __in InvalidLogRecords & invalidLogRecords);

            TxnReplicator::CompletionTask::SPtr const & GetAppliedTask() const
            {
                return appliedTask_;
            }

            ktl::Awaitable<NTSTATUS> AwaitApply()
            {
                NTSTATUS status = co_await appliedTask_->AwaitCompletion();
                co_return status;
            }

            ktl::Awaitable<NTSTATUS> AwaitFlush()
            {
                NTSTATUS status = co_await flushedTask_->AwaitCompletion();
                co_return status;
            }

            ktl::Awaitable<NTSTATUS> AwaitProcessing()
            {
                NTSTATUS status = co_await processedTask_->AwaitCompletion();
                co_return status;
            }

            void CompletedApply(__in NTSTATUS errorCode)
            {
                appliedTask_->CompleteAwaiters(errorCode);
            }

            void CompletedFlush(__in NTSTATUS errorCode)
            {
                flushedTask_->CompleteAwaiters(errorCode);
            }

            void CompletedProcessing()
            {
                processedTask_->CompleteAwaiters(STATUS_SUCCESS);
            }

            virtual bool Test_Equals(LogRecord const & other) const;

        protected:

            LogRecord(
                __in LogRecordType::Enum recordType,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            //
            // Used during recovery
            //
            LogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);
            
            //
            // Makes it accessible to the test code
            //
            static LogRecord::SPtr ReadRecord(
                __in Utilities::BinaryReader & binaryReader,
                __in ULONG64 recordPosition,
                __in InvalidLogRecords & invalidLogRecords,
                __in bool isPhysicalRead,
                __in KAllocator & allocator);

            //
            // Populates the contents of the log record from the memory stream backed by the binary reader
            // If 'isPhysicalRead' is TRUE, the physical record fields like PSN are also populated. Otherwise, they are skipped.
            // A scenario where a read may not be physical is during backup/copy
            //
            virtual void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            //
            // Populates the logical (replicated) contents of the log record from the operation data received
            // This is primarily invoked in the deserialization of the replicated data
            //
            virtual void ReadLogical(
                __in Utilities::OperationData const & operationData,
                __inout INT & index);

            //
            // Writes the content of the log record into a buffer and appends the buffer to the operationData parameter
            // The binaryWriter is a temporary object that can be used for serialization of the data
            //
            virtual void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets);

            void set_ApproximateSizeOnDisk(ULONG value)
            {
                approximateDiskSize_ = value;
            }

            KSharedPtr<PhysicalLogRecord> previousPhysicalRecord_;

        private:

            static LogRecord::SPtr ReadRecordWithHeaders(
                __in Utilities::BinaryReader & binaryReader,
                __in ULONG64 recordPosition,
                __in InvalidLogRecords & invalidLogRecords,
                __in KAllocator & allocator,
                __in bool isPhysicalRead = true,
                __in bool setRecordLength = true);

            static LogRecord::SPtr InternalReadFromOperationData(
                __in Utilities::OperationData const & operationData,
                __in InvalidLogRecords & invalidLogRecords,
                __inout int & index,
                __in KAllocator & allocator);

            void UpdateApproximateDiskSize();

            static const ULONG DiskSpaceUsed;

            LogRecordType::Enum recordType_;
            LONG64 lsn_;
            LONG64 psn_;
            ULONG recordLength_;
            ULONG64 previousPhysicalRecordOffset_;

            TxnReplicator::CompletionTask::SPtr appliedTask_;
            TxnReplicator::CompletionTask::SPtr flushedTask_;
            TxnReplicator::CompletionTask::SPtr processedTask_;

            // The following fields are not persisted
            ULONG64 recordPosition_;
            ULONG32 approximateDiskSize_;
        };
    }
}
