// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Used to populate trace message during ProcessFlushedRecordsCallback
        class FlushedRecordInfo
        {
            DENY_COPY(FlushedRecordInfo);

        public:
            FlushedRecordInfo(FlushedRecordInfo && other);
            FlushedRecordInfo(
                __in LogRecordLib::LogRecordType::Enum const & recordType,
                __in FABRIC_SEQUENCE_NUMBER LSN,
                __in FABRIC_SEQUENCE_NUMBER PSN,
                __in ULONG64 recordPosition);

            // Update the element value
            void Update(
                __in LogRecordLib::LogRecordType::Enum const & recordType,
                __in FABRIC_SEQUENCE_NUMBER LSN,
                __in FABRIC_SEQUENCE_NUMBER PSN,
                __in ULONG64 recordPosition);

            // Reset the element value
            void Reset();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            LogRecordLib::LogRecordType::Enum recordType_;
            FABRIC_SEQUENCE_NUMBER lsn_;
            FABRIC_SEQUENCE_NUMBER psn_;
            ULONG64 recordPosition_;
        };
    } // end namespace LoggingReplicator
} // end namespace Data
