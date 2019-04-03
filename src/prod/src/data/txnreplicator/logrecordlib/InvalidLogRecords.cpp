// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

InvalidLogRecords::InvalidLogRecords()
    : KObject()
    , KShared()
    , invalidLogRecord_()
    , invalidLogicalLogRecord_()
    , invalidPhysicalLogRecord_()
    , invalidTransactionLogRecord_()
    , invalidBeginTransactionOperationLogRecord_()
    , invalidIndexingLogRecord_()
    , invalidBeginCheckpointLogRecord_()
    , invalidInformationLogRecord_()
{
    invalidLogRecord_ = InvalidLogRecord(GetThisAllocator());
    invalidLogicalLogRecord_ = InvalidLogicalLogRecord(GetThisAllocator());
    invalidPhysicalLogRecord_ = InvalidPhysicalLogRecord(GetThisAllocator());
    invalidTransactionLogRecord_ = InvalidTransactionLogRecord(GetThisAllocator());
    invalidBeginTransactionOperationLogRecord_ = InvalidBeginTransactionOperationLogRecord(GetThisAllocator());
    invalidIndexingLogRecord_ = InvalidIndexingLogRecord(GetThisAllocator());
    invalidBeginCheckpointLogRecord_ = InvalidBeginCheckpointLogRecord(GetThisAllocator());
    invalidInformationLogRecord_ = InvalidInformationLogRecord(GetThisAllocator());
}

InvalidLogRecords::~InvalidLogRecords()
{
}

InvalidLogRecords::SPtr InvalidLogRecords::Create(__in KAllocator & allocator)
{
    InvalidLogRecords * pointer = _new(INVALID_LOGRECORDS, allocator)InvalidLogRecords();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return InvalidLogRecords::SPtr(pointer);
}

std::wstring Data::LogRecordLib::InvalidLogRecords::ToString() const
{
     std::wstring logRecordString = 
         Constants::StartingJSON +
         L"Invalid LogRecord\n" +
         Constants::CloseJSON;

     return logRecordString;
}

LogRecord::SPtr InvalidLogRecords::InvalidLogRecord(__in KAllocator & allocator)
{
    LogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)LogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogRecord::SPtr(pointer);
}

LogicalLogRecord::SPtr InvalidLogRecords::InvalidLogicalLogRecord(__in KAllocator & allocator)
{
    LogicalLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)LogicalLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return LogicalLogRecord::SPtr(pointer);
}

PhysicalLogRecord::SPtr InvalidLogRecords::InvalidPhysicalLogRecord(__in KAllocator & allocator)
{
    PhysicalLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)PhysicalLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return PhysicalLogRecord::SPtr(pointer);
}

TransactionLogRecord::SPtr InvalidLogRecords::InvalidTransactionLogRecord(__in KAllocator & allocator)
{
    TransactionLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)TransactionLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TransactionLogRecord::SPtr(pointer);
}

BeginTransactionOperationLogRecord::SPtr InvalidLogRecords::InvalidBeginTransactionOperationLogRecord(__in KAllocator & allocator)
{
    BeginTransactionOperationLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)BeginTransactionOperationLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginTransactionOperationLogRecord::SPtr(pointer);
}

IndexingLogRecord::SPtr InvalidLogRecords::InvalidIndexingLogRecord(__in KAllocator & allocator)
{
    IndexingLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)IndexingLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return IndexingLogRecord::SPtr(pointer);
}

BeginCheckpointLogRecord::SPtr InvalidLogRecords::InvalidBeginCheckpointLogRecord(__in KAllocator & allocator)
{
    BeginCheckpointLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)BeginCheckpointLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return BeginCheckpointLogRecord::SPtr(pointer);
}

InformationLogRecord::SPtr InvalidLogRecords::InvalidInformationLogRecord(__in KAllocator & allocator)
{
    InformationLogRecord * pointer = _new(INVALID_LOGRECORDS, allocator)InformationLogRecord();
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return InformationLogRecord::SPtr(pointer);
}

