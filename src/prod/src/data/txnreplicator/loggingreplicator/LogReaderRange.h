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
        // Encapsulates a log reader'state.
        // A reader can be created during primary->idle copy or backup or recovery
        // 
        // The reader name and type and used for informational/tracing purposes.
        // However, the primary intention of this abstraction is to maintain a ref counted system of log readers to ensure the log is not truncated while there are active readers
        //
        class LogReaderRange final 
            : public KObject<LogReaderRange>
        {
        public:
            static LogReaderRange const & DefaultLogReaderRange();

            LogReaderRange();

            LogReaderRange(
                __in LONG64 fullcopyStartingLsn,
                __in ULONG64 startingRecordPosition,
                __in KStringView const & readerName,
                __in LogRecordLib::LogReaderType::Enum logReaderType);

            LogReaderRange(__in LogReaderRange && other);

            LogReaderRange(__in LogReaderRange const & other);

            LogReaderRange & operator=(LogReaderRange const & other);

            __declspec(property(get = get_FullCopyStartingLsn)) LONG64 FullCopyStartingLsn;
            LONG64 get_FullCopyStartingLsn() const
            {
                return fullcopyStartingLsn_;
            }

            __declspec(property(get = get_StartingRecordPosition)) ULONG64 StartingRecordPosition;
            ULONG64 get_StartingRecordPosition() const
            {
                return startingRecordPosition_;
            }

            __declspec(property(get = get_ReaderType, put = set_ReaderType)) LogRecordLib::LogReaderType::Enum ReaderType;
            LogRecordLib::LogReaderType::Enum get_ReaderType() const
            {
                return logReaderType_;
            }

            void set_ReaderType(LogRecordLib::LogReaderType::Enum readerType)
            {
                logReaderType_ = readerType;
            }

            __declspec(property(get = get_ReaderName)) KStringView const & ReaderName;
            KStringView const & get_ReaderName() const
            {
                return readerName_;
            }

            void AddRef();

            int Release();

        private:

            static const LogReaderRange DefaultLogReaderRangeValue;
            
            LONG64 fullcopyStartingLsn_;
            int referenceCount_;
            LogRecordLib::LogReaderType::Enum logReaderType_;
            ULONG64 startingRecordPosition_;
            KLocalString<LogRecordLib::Constants::LogReaderNameMaxLength> readerName_; // Readers are less than 128
        };
    }
}
