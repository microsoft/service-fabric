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
        // This log record type has an index back into the current head of the log
        // It is an abstract type and has valid derived objects
        //
        class LogHeadRecord
            : public PhysicalLogRecord
        {
            K_FORCE_SHARED_WITH_INHERITANCE(LogHeadRecord)            

        public:

            _declspec(property(get = get_indexingLogRecord, put = set_indexingLogRecord)) IndexingLogRecord::SPtr HeadRecord;
            IndexingLogRecord::SPtr get_indexingLogRecord() const
            {
                return logHeadRecord_;
            }
            void set_indexingLogRecord(__in IndexingLogRecord & head)
            {
                logHeadRecord_ = &head;
            }

            _declspec(property(get = get_currentLogHeadEpoch)) TxnReplicator::Epoch & LogHeadEpoch;
            TxnReplicator::Epoch const & get_currentLogHeadEpoch() const
            {
                return logHeadEpoch_;
            }

            _declspec(property(get = get_currentLogHeadLsn)) LONG64 LogHeadLsn;
            LONG64 get_currentLogHeadLsn() const
            {
                return logHeadLsn_;
            }

            _declspec(property(get = get_currentLogHeadPsn)) LONG64 LogHeadPsn;
            LONG64 get_currentLogHeadPsn() const
            {
                return logHeadPsn_;
            }

            __declspec(property(get = get_currentLogHeadRecordOffset)) ULONG64 LogHeadRecordOffset;
            ULONG64 get_currentLogHeadRecordOffset() const
            {
                return logHeadRecordOffset_;
            }

            __declspec(property(get = get_currentLogHeadRecordPosition)) ULONG64 LogHeadRecordPosition;
            ULONG64 get_currentLogHeadRecordPosition() const
            {
                return RecordPosition - logHeadRecordOffset_;
            }

            virtual bool FreePreviousLinksLowerThanPsn(
                __in LONG64 newHeadPsn,
                __in InvalidLogRecords & invalidLogRecords) override;

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            // 
            // Used during recovery
            //
            LogHeadRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in IndexingLogRecord & invalidIndexingLogRecord);

            LogHeadRecord(
                __in LogRecordType::Enum recordType,
                __in IndexingLogRecord & logHeadRecord,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
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

            LONG64 logHeadLsn_;
            LONG64 logHeadPsn_;
            ULONG64 logHeadRecordOffset_;
            TxnReplicator::Epoch logHeadEpoch_;

            // The following fields are not persisted
            IndexingLogRecord::SPtr logHeadRecord_;
        };
    }
}
