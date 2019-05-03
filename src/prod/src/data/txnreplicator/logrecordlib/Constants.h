// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        class Constants
        {
        public:

            static ULONG const InvalidRecordLength;
            static ULONG64 const InvalidRecordPosition;
            static ULONG64 const InvalidPhysicalRecordOffset;
            static ULONG64 const InvalidLogicalRecordOffset;
            static ULONG64 const NumberLogRecordTypes;
            static LONG64 const InvalidLsn;
            static LONG64 const InvalidPsn;
            static LONG64 const OneLsn;
            static LONG64 const ZeroLsn;
            static LONG64 const MaxLsn;
            static LONG64 const InvalidReplicaId;
            static LONG64 const UniversalReplicaId;
            static LONG64 const NullOperationDataCode;
            static ULONG const KGuid_KString_Length;
            static ULONG const LogReaderNameMaxLength = 128;
            static wstring const Constants::StartingJSON;
            static wstring const Constants::CloseJSON;
            static wstring const Constants::Quote;
            static wstring const Constants::DivisionJSON;
            static wstring const Constants::DivisionBoolJSON;
            static wstring const Constants::EndlJSON;
            static wstring const Constants::CompEndlJSON;
            static const Common::StringLiteral SFLogSuffix;
            static const Common::StringLiteral Test_EmptyString;
            static const LPCWSTR LogPathPrefix;
            static const std::wstring Test_Ktl_LoggingEngine;
            static const std::wstring Test_File_LoggingEngine;
            static const std::wstring SerialDispatchingMode;
            static const ULONG PhysicalLogWriterMovingAverageHistory;
            static LONG64 const PhysicalLogWriterSlowFlushDurationInMs;
            static LONG64 const ProgressVectorMaxStringSizeInKb;
            static const std::wstring SlowPhysicalLogWriteOperationName;
            static const std::wstring SlowPhysicalLogReadOperationName;
            static LONG64 const BytesInKBytes;
            static const std::wstring FabricHostApplicationDirectory;
        };
    }
}
