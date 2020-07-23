// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class TruncateTailLogRecord final 
            : public PhysicalLogRecord
        {
            K_FORCE_SHARED(TruncateTailLogRecord)

        public:
            static TruncateTailLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            static TruncateTailLogRecord::SPtr Create(
                __in LONG64 lsn,
                __in PhysicalLogRecord & linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            bool Test_Equals(__in LogRecord const & other) const override;

            virtual std::wstring ToString() const override;

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

            TruncateTailLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            TruncateTailLogRecord(
                __in LONG64 LSN,
                __in PhysicalLogRecord & lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();
        };
    }
}
