// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class EndCheckpointLogRecord final 
            : public LogHeadRecord
        {
            K_FORCE_SHARED(EndCheckpointLogRecord)

        public:

            //
            // Used during recovery
            //
            static EndCheckpointLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in IndexingLogRecord & invalidIndexLogRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginCheckpointLogRecord & invalidBeginCheckpointLog,
                __in KAllocator & allocator);

            static EndCheckpointLogRecord::SPtr Create(
                __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
                __in IndexingLogRecord & logHeadRecord,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static EndCheckpointLogRecord::SPtr CreateOneEndCheckpointLogRecord(
                __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
                __in IndexingLogRecord & logHeadRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_LastCompletedBeginCheckpointRecord, put = set_LastCompletedBeginCheckpointRecord)) BeginCheckpointLogRecord::SPtr LastCompletedBeginCheckpointRecord;
            BeginCheckpointLogRecord::SPtr get_LastCompletedBeginCheckpointRecord() const
            {
                return lastCompletedBeginCheckpointRecord_;
            }
            void set_LastCompletedBeginCheckpointRecord(BeginCheckpointLogRecord & value)
            {
                lastCompletedBeginCheckpointRecord_ = &value;
            }

            __declspec(property(get = get_LastStableLsn)) LONG64 LastStableLsn;
            LONG64 get_LastStableLsn() const
            {
                return lastStableLsn_;
            }

            __declspec(property(get = get_LastCompletedBeginCheckpointRecordOffset)) ULONG64 LastCompletedBeginCheckpointRecordOffset;
            ULONG64 get_LastCompletedBeginCheckpointRecordOffset() const
            {
                return lastCompletedBeginCheckpointRecordOffset_;
            }

            bool FreePreviousLinksLowerThanPsn(
                __in LONG64 newHeadpsn,
                __in InvalidLogRecords & invalidLogRecords) override;

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

        private:

            EndCheckpointLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in IndexingLogRecord & invalidIndexLogRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in BeginCheckpointLogRecord & invalidBeginCheckpointLog);

            EndCheckpointLogRecord(
                __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
                __in IndexingLogRecord & logHeadRecord,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            EndCheckpointLogRecord(
                __in BeginCheckpointLogRecord & lastCompletedBeginCheckpointRecord,
                __in IndexingLogRecord & logHeadRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            LONG64 lastStableLsn_;
            ULONG64 lastCompletedBeginCheckpointRecordOffset_;

            // The following fields are not persisted
            BeginCheckpointLogRecord::SPtr lastCompletedBeginCheckpointRecord_;
        };
    }
}
