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
        // Represents a group commit operation. Also generated when a user explicitly requests a barrier (E.g BackupManager)
        //
        class BarrierLogRecord final 
            : public LogicalLogRecord
        {
            K_FORCE_SHARED(BarrierLogRecord)

        public:

            //
            // Used during recovery
            //
            static BarrierLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static BarrierLogRecord::SPtr Create(
                __in LONG64 lastStableLsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            //
            // Used during initialization of a brand new log by the logmanager
            //
            static BarrierLogRecord::SPtr CreateOneBarrierRecord(
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            virtual std::wstring ToString() const override;
            
            // 
            // Stable Lsn represents the sequence number upto which there is write quorum at the point of replicating this record
            //
            __declspec(property(get = get_LastStableLsn)) LONG64 LastStableLsn;
            LONG64 get_LastStableLsn() const
            {
                return lastStableLsn_;
            }

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

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

        private:

            BarrierLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            BarrierLogRecord(
                __in LONG64 lastStableLsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            BarrierLogRecord(
                __in LONG64 lsn,
                __in LONG64 lastStableLsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            void ReadPrivate(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead);

            static const ULONG DiskSpaceUsed;
            static const ULONG SizeOnWireIncrement;

            void UpdateApproximateDiskSize();

            LONG64 lastStableLsn_;

            KBuffer::SPtr replicatedData_;
        };
    }
}
