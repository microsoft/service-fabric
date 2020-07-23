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
        // The only log record type that can be the head of the log
        //
        class IndexingLogRecord final 
            : public PhysicalLogRecord
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED(IndexingLogRecord)

        public:

            static IndexingLogRecord::SPtr Create(
                __in TxnReplicator::Epoch const & currentEpoch,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalRecord,
                __in KAllocator & allocator);
            
            //
            // Used during recovery
            //
            static IndexingLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            //
            // Used when creating a new log by the logmanager
            //
            static IndexingLogRecord::SPtr CreateZeroIndexingLogRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            _declspec(property(get = get_CurrentEpoch)) TxnReplicator::Epoch & CurrentEpoch;
            TxnReplicator::Epoch const & get_CurrentEpoch() const
            {
                return recordEpoch_;
            }
            
            //
            // Invoked when this indexing record is chosen as the new head 
            // Returns the number of physical links we jumped ahead to free previous backward references and how many actually free'd links older than the head
            //
            // This also implies we cannot have backward references from logical records to other records
            // If this requirement comes up, we must be able to nagivate through the logical records 1 by 1 to free the links
            //
            void OnTruncateHead(
                __in InvalidLogRecords & invalidLogRecords,
                __out ULONG32 & freeLinkCallCount,
                __out ULONG32 & freeLinkCallTrueCount);

            bool Test_Equals(__in LogRecord const & other) const override;

            virtual std::wstring ToString() const override;

        protected:

            virtual void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            virtual void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite,
                __in bool forceRecomputeOffsets) override;

        private:

            IndexingLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            IndexingLogRecord(
                __in TxnReplicator::Epoch const & currentEpoch,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            TxnReplicator::Epoch recordEpoch_;
        };
    }
}
