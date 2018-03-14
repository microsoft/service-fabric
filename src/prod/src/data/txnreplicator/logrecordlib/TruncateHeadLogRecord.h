// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class TruncateHeadLogRecord final 
            : public LogHeadRecord
        {
            K_FORCE_SHARED(TruncateHeadLogRecord)

        public:
            static TruncateHeadLogRecord::SPtr Create(
                __in IndexingLogRecord & logHeadRecord,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in bool isStable,
                __in KAllocator & allocator);

            static TruncateHeadLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in IndexingLogRecord & invalidIndexingLogRecord,
                __in KAllocator & allocator);

            __declspec(property(get = get_isStable)) bool IsStable;
            bool get_isStable() const
            {
                return isStable_;
            }

            __declspec(property(get = get_TruncationState, put = set_TruncationState)) TruncationState::Enum TruncationState;
            TruncationState::Enum & get_TruncationState()
            {
                return truncationState_;
            }
            void set_TruncationState(TruncationState::Enum value)
            {
                ASSERT_IFNOT(
                    truncationState_ < value, 
                    "Invalid truncation state transition: {0} to {1}", truncationState_, value);

                truncationState_ = value;
            }

            bool Test_Equals(__in LogRecord const & other) const override;

        protected:

            void Read(
                __in Utilities::BinaryReader & binaryReader,
                __in bool isPhysicalRead) override;

            void Write(
                __in Utilities::BinaryWriter & binaryWriter,
                __inout Utilities::OperationData & operationData,
                __in bool isPhysicalWrite) override;

        private:

            TruncateHeadLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in IndexingLogRecord & invalidIndexingLogRecord);

            TruncateHeadLogRecord(
                __in IndexingLogRecord & logHeadRecord,
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const lastLinkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in bool isStable);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            bool isStable_;

            // The state is not persisted
            TruncationState::Enum truncationState_;
        };
    }
}
