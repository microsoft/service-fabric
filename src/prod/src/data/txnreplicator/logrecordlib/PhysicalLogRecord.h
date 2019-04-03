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
        // Represents a physical log record, which means it is never replicated
        // The PSN for these records is unique, while the LSN is equal to the previous logical log record's LSN
        //
        class PhysicalLogRecord
            : public LogRecord
            , public KWeakRefType<PhysicalLogRecord>
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED_WITH_INHERITANCE(PhysicalLogRecord)

        public:

            __declspec(property(get = get_linkedPhysicalRecordOffset, put = set_linkedPhysicalRecordOffset)) ULONG64 LinkedPhysicalRecordOffset;
            ULONG64 get_linkedPhysicalRecordOffset() const
            {
                return linkedPhysicalRecordOffset_;
            }

            void set_linkedPhysicalRecordOffset(ULONG64 value)
            {
                linkedPhysicalRecordOffset_ = value;
            }

            __declspec(property(get = get_linkedPhysicalRecord, put = set_linkedPhysicalRecord)) PhysicalLogRecord::SPtr LinkedPhysicalRecord;
            PhysicalLogRecord::SPtr get_linkedPhysicalRecord()
            {
                // This is being set only once below and will never be reset. So it is fine to not protect it with a lock or thread safe sptr
                return linkedPhysicalRecord_;
            }
            void set_linkedPhysicalRecord(PhysicalLogRecord::SPtr const & value)
            {
                ASSERT_IFNOT(
                    value == nullptr ||
                    value->RecordType == LogRecordType::Enum::EndCheckpoint ||
                    value->RecordType == LogRecordType::Enum::CompleteCheckpoint ||
                    value->RecordType == LogRecordType::Enum::TruncateHead,
                    "Invalid linked physical record. Acceptable values are EndCheckpointLogRecord, CompleteCheckpointLogRecord, TruncateHeadLogRecord or nullptr");

                linkedPhysicalRecord_ = value;
            }

            virtual std::wstring ToString() const override;

            __declspec(property(get = get_nextPhysicalRecord, put = set_nextPhysicalRecord)) PhysicalLogRecord::SPtr NextPhysicalRecord;
            PhysicalLogRecord::SPtr get_nextPhysicalRecord()
            {
                PhysicalLogRecord::SPtr strongRef = nextPhysicalRecord_->TryGetTarget();
                return strongRef;
            }
            void set_nextPhysicalRecord(__in PhysicalLogRecord::SPtr const & value)
            {
                nextPhysicalRecord_ = value->GetWeakRef();
            }

            // 
            // Used to avoid dynamic cast and fast casting
            //
            LogicalLogRecord * AsLogicalLogRecord() override
            {
                return nullptr;
            }

            // 
            // Used to avoid dynamic cast and fast casting
            //
            PhysicalLogRecord * AsPhysicalLogRecord()  override
            {
                return this;
            }

            virtual bool FreePreviousLinksLowerThanPsn(
                __in LONG64 newHeadPsn,
                __in InvalidLogRecords & invalidLogRecords) override;

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            //
            // Used during recovery
            //
            PhysicalLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            PhysicalLogRecord(
                __in LogRecordType::Enum recordType,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

        private:

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            // Proactively release back links to avoid stack overflow
            void FreePhysicalLinkChainProactively();

            ULONG64 linkedPhysicalRecordOffset_;

            // The following fields are not persisted
            PhysicalLogRecord::SPtr linkedPhysicalRecord_;

            // Any links forward must be weak references to ensure there is no circular chain of log records that can lead to leaks
            KWeakRef<PhysicalLogRecord>::SPtr nextPhysicalRecord_;
        };
    }
}
