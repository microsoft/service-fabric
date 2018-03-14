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
        // API's that are only needed to read the log without any write functions
        // Its used for tools to read and deserialize log records
        //
        interface ILogManagerReadOnly
        {
            K_SHARED_INTERFACE(ILogManagerReadOnly);

        public:
            __declspec(property(get = get_CurrentLogTailRecord)) LogRecord::SPtr CurrentLogTailRecord;
            virtual LogRecord::SPtr get_CurrentLogTailRecord() const = 0;

            __declspec(property(get = get_InvalidLogRecords)) InvalidLogRecords::SPtr InvalidLogRecords;
            virtual InvalidLogRecords::SPtr get_InvalidLogRecords() const = 0;

            virtual KSharedPtr<IPhysicalLogReader> GetPhysicalLogReader(
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in LONG64 startingLsn,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType) = 0;

            virtual Data::Log::ILogicalLogReadStream::SPtr CreateReaderStream() = 0;

            virtual void SetSequentialAccessReadSize(
                __in Data::Log::ILogicalLogReadStream & readStream,
                __in LONG64 sequentialReadSize) = 0;

            virtual bool AddLogReader(
                __in LONG64 startingLsn,
                __in ULONG64 startingRecordPosition,
                __in ULONG64 endingRecordPosition,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType) = 0;

            virtual void RemoveLogReader(__in ULONG64 startingRecordPosition) = 0;
        };
    }
}
