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
        // Represents an Information Log Record
        // 
        class InformationLogRecord final 
            : public PhysicalLogRecord
        {
            friend class InvalidLogRecords;

            K_FORCE_SHARED(InformationLogRecord)

        public:

            static InformationLogRecord::SPtr Create(
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in InformationEvent::Enum informationEvent,
                __in KAllocator & allocator);

            static InformationLogRecord::SPtr Create(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in KAllocator & allocator);

            virtual std::wstring ToString() const override;

            _declspec(property(get = get_InformationEvent)) InformationEvent::Enum InformationEvent;
            InformationEvent::Enum get_InformationEvent() const
            {
                return informationEvent_;
            }

            bool Test_Equals(__in LogRecord const & other) const override;

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

            InformationLogRecord(
                __in LogRecordType::Enum recordType,
                __in ULONG64 recordPosition,
                __in LONG64 lsn,
                __in PhysicalLogRecord & invalidPhysicalLogRecord);

            InformationLogRecord(
                __in LONG64 lsn,
                __in_opt PhysicalLogRecord * const linkedPhysicalRecord,
                __in PhysicalLogRecord & invalidPhysicalLogRecord,
                __in InformationEvent::Enum informationEvent);

            static const ULONG DiskSpaceUsed;

            void UpdateApproximateDiskSize();

            InformationEvent::Enum informationEvent_;
        };
    }
}
