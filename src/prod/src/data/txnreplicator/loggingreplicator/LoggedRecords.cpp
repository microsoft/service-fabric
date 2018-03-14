// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

LoggedRecords::LoggedRecords(
    __in LogRecord & record,
    __in NTSTATUS logError)
    : KShared()
    , KObject()
    , logError_(logError)
    , records_(GetThisAllocator())
{
    THROW_ON_CONSTRUCTOR_FAILURE(records_);
    NTSTATUS status = records_.Append(&record);
    THROW_ON_FAILURE(status);
}

LoggedRecords::LoggedRecords(__in KArray<LogRecord::SPtr> const & records)
    : KShared()
    , KObject()
    , logError_(STATUS_SUCCESS)
    , records_(records)
{
    THROW_ON_CONSTRUCTOR_FAILURE(records_);
}

LoggedRecords::LoggedRecords(
    __in KArray<LogRecord::SPtr> const & records,
    __in NTSTATUS logError)
    : KShared()
    , KObject()
    , logError_(logError)
    , records_(records)
{
    THROW_ON_CONSTRUCTOR_FAILURE(records_);
}

LoggedRecords::~LoggedRecords()
{
}

LoggedRecords::SPtr LoggedRecords::Create(
    __in LogRecord & record,
    __in NTSTATUS logError,
    __in KAllocator & allocator)
{
    LoggedRecords* pointer = _new(LOGGEDRECORDS_TAG, allocator) LoggedRecords(
        record, 
        logError);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LoggedRecords::SPtr(pointer);
}

LoggedRecords::SPtr LoggedRecords::Create(
    __in KArray<LogRecord::SPtr> const & records,
    __in KAllocator & allocator)
{ 
    LoggedRecords* pointer = _new(LOGGEDRECORDS_TAG, allocator) LoggedRecords(records);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LoggedRecords::SPtr(pointer);
}

LoggedRecords::CSPtr LoggedRecords::Create(
    __in KArray<LogRecord::SPtr> const & records,
    __in NTSTATUS logError,
    __in KAllocator & allocator)
{ 
    LoggedRecords * pointer = _new(LOGGEDRECORDS_TAG, allocator) LoggedRecords(
        records, 
        logError);

    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LoggedRecords::CSPtr(pointer);
}

void LoggedRecords::Add(LoggedRecords const & records)
{
    for (ULONG i = 0; i < records.Count; i++)
    {
        NTSTATUS status = records_.Append(records[i]);
        THROW_ON_FAILURE(status);
    }
}

void LoggedRecords::Clear()
{
    records_.Clear();
}
