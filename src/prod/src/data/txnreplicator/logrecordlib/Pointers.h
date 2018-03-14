// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        using ::_delete;

        // Used in all log records to be defined as friend to be able to invoke default private constructor
        class InvalidLogRecords;

        // Used in LogRecord.h - PreviousPhysicalLogRecord
        class PhysicalLogRecord;
        // Used in LogRecord.h - FastCast
        class LogicalLogRecord;
        // Used in LogRecord.h - ReadRecord
        class IndexingLogRecord;
        // Used in LogRecord.h - ReadRecord
        class BeginTransactionOperationLogRecord;
        // Used in LogRecord.h - ReadRecord
        class BeginCheckpointLogRecord;
        
        // Used in ProgressVector class
        class CopyContextParameters;
    }
}
