// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class CompleteCheckPointLogRecord final 
            : public LogHeadRecord
        {
            K_FORCE_SHARED(CompleteCheckPointLogRecord)

        public:
            
            //
            // Used during recovery
            //
            static CompleteCheckPointLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in IndexingLogRecord & invalidIndexingLogRecord,
                __in KAllocator & allocator);

            static CompleteCheckPointLogRecord::SPtr Create(
                __in LONG64 lsn,
                __in IndexingLogRecord & logHeadRecord,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

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

            CompleteCheckPointLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in IndexingLogRecord & invalidIndexingLogRecord);

            CompleteCheckPointLogRecord(
                __in LONG64 lsn,
                __in IndexingLogRecord & logHeadRecord,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();
        };
    }
}
