// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //
        // A unit of write - Has an array of log records that got 'Logged' to the disk
        // The LogException property indicates if the write failed due to an exception
        //
        class LoggedRecords final 
            : public KObject<LoggedRecords>
            , public KShared<LoggedRecords>
        {
            K_FORCE_SHARED(LoggedRecords)

        public:

            static LoggedRecords::SPtr Create(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS logError,
                __in KAllocator & allocator);

            static LoggedRecords::SPtr Create(
                __in KArray<LogRecordLib::LogRecord::SPtr> const & records,
                __in KAllocator & allocator);

            static LoggedRecords::CSPtr Create(
                __in KArray<LogRecordLib::LogRecord::SPtr> const & records,
                __in NTSTATUS logError,
                __in KAllocator & allocator);

            __declspec(property(get = get_Count)) ULONG Count;
            ULONG get_Count() const
            {
                return records_.Count();
            }

            __declspec(property(get = get_LogError)) NTSTATUS LogError;
            NTSTATUS get_LogError() const
            {
                return logError_;
            }

            LogRecordLib::LogRecord::SPtr operator[](ULONG i) const
            {
                return records_[i];
            }

            void Add(LoggedRecords const & records);

            void Clear();

        private:

            LoggedRecords(
                __in LogRecordLib::LogRecord & record,
                __in NTSTATUS logError);

            LoggedRecords(__in KArray<LogRecordLib::LogRecord::SPtr> const & records);

            LoggedRecords(
                __in KArray<LogRecordLib::LogRecord::SPtr> const & records,
                __in NTSTATUS logError);

            NTSTATUS const logError_;
            KArray<LogRecordLib::LogRecord::SPtr> records_;
        };
    }
}
