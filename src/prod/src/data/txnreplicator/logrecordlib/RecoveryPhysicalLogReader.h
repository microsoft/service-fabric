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
        // Implementation of the IPhysicalLogReader interface, which is specifically used during recovery
        //
        class RecoveryPhysicalLogReader final 
            : public PhysicalLogReader
        {
            K_FORCE_SHARED(RecoveryPhysicalLogReader)
            K_SHARED_INTERFACE_IMP(IDisposable)

        public:

             static RecoveryPhysicalLogReader::SPtr Create(
                __in ILogManagerReadOnly & logManager,
                __in KAllocator & allocator);

             __declspec(property(get = get_StartingRecord, put = set_StartingRecord)) LogRecord::SPtr StartingRecord;
             LogRecord::SPtr get_StartingRecord() const
             {
                 ASSERT_IFNOT(!LogRecord::IsInvalid(startingRecord_.RawPtr()), "Invalid starting log record in getter");

                 return startingRecord_;
             }
             void set_StartingRecord(LogRecord & value)
             {
                 ASSERT_IFNOT(LogRecord::IsInvalid(startingRecord_.RawPtr()), "Unexpected valid starting log record in setter");

                 startingRecord_ = &value;
             }

             __declspec(property(get = get_ReadStream)) Data::Log::ILogicalLogReadStream::SPtr ReadStream;
             Data::Log::ILogicalLogReadStream::SPtr get_ReadStream() const
             {
                 return readStream_;
             }

             void Dispose() override;

             ktl::Awaitable<LogRecord::SPtr> GetLastCompletedBeginCheckpointRecord(__in LogRecord & record);
             
             ktl::Awaitable<BeginCheckpointLogRecord::SPtr> GetLastCompletedBeginCheckpointRecord(__inout EndCheckpointLogRecord & record);

             ktl::Awaitable<PhysicalLogRecord::SPtr> GetLinkedPhysicalRecord(__in PhysicalLogRecord & record);

             ULONG64 GetNextLogRecordPosition(__in ULONG64 recordPosition);

             ktl::Awaitable<LogRecord::SPtr> GetNextLogRecord(__in ULONG64 recordPosition);

             ktl::Awaitable<LogRecord::SPtr> GetPreviousLogRecord(__in ULONG64 recordPosition);

             ktl::Awaitable<void> IndexPhysicalRecords(__in PhysicalLogRecord::SPtr & physicalRecord);

             ktl::Awaitable<PhysicalLogRecord::SPtr> GetPreviousPhysicalRecord(__in LogRecord & record);

             void MoveStartingRecordPosition(
                 __in LONG64 startingLsn,
                 __in ULONG64 newStartingRecordPosition);

             ktl::Awaitable<LogRecord::SPtr> SeekToLastRecord();

			 ktl::Awaitable<LogRecord::SPtr> SeekToFirstRecord(__in ULONG64 );

        private:

            RecoveryPhysicalLogReader(__in ILogManagerReadOnly & logManager);

            ktl::Task DisposeReadStream();

            Data::Log::ILogicalLogReadStream::SPtr const readStream_;
            LogRecord::SPtr startingRecord_;
            LogRecord::SPtr endingRecord_;

            InvalidLogRecords::SPtr invalidLogRecords_;
        };
    }
}
