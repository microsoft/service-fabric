// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;
using namespace Data::Utilities;

PhysicalLogReader::PhysicalLogReader(__in ILogManagerReadOnly & logManager)
    : IPhysicalLogReader()
    , KObject()
    , KShared()
    , isDisposed_(false)
    , logManager_(&logManager)
    , isValid_(false)
    , startingRecordPosition_(0)
    , endingRecordPosition_(MAXLONG64)
    , startLsn_(Constants::ZeroLsn)
    , readerName_()
    , readerType_(LogReaderType::Enum::Default)
{
    BOOLEAN result = readerName_.Concat(L"Invalid");
    ASSERT_IFNOT(
        result == TRUE,
        "PhysicalLogReader::Ctor : Failed to concatenate 'Invalid'"); // otherwise reader is too long
}

PhysicalLogReader::PhysicalLogReader(
    __in ILogManagerReadOnly & logManager,
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in LONG64 startLsn,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType)
    : IPhysicalLogReader()
    , KObject()
    , KShared()
    , isDisposed_(false)
    , logManager_(&logManager)
    , isValid_(
        logManager.AddLogReader(
            startLsn,
            startingRecordPosition,
            endingRecordPosition,
            readerName,
            readerType))
    , startingRecordPosition_(startingRecordPosition)
    , endingRecordPosition_(endingRecordPosition)
    , startLsn_(startLsn)
    , readerName_()
    , readerType_(readerType)
{
    BOOLEAN result = readerName_.Concat(readerName);
    ASSERT_IFNOT(
        result == TRUE,
        "PhysicalLogReader::Ctor : Failed to concatenate {0}",
        ToStringLiteral(readerName)); // otherwise reader is too long
}

PhysicalLogReader::~PhysicalLogReader()
{
    ASSERT_IFNOT(isDisposed_, "Physical log reader object is not disposed in desctructor");
}

IPhysicalLogReader::SPtr PhysicalLogReader::Create(
    __in ILogManagerReadOnly & logManager,
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in LONG64 startLsn,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType,
    __in KAllocator & allocator)
{
    PhysicalLogReader * pointer = _new(PHYSICALLOGREADER_TAG, allocator)PhysicalLogReader(
        logManager,
        startingRecordPosition,
        endingRecordPosition,
        startLsn,
        readerName,
        readerType);

    THROW_ON_ALLOCATION_FAILURE(pointer);

    return IPhysicalLogReader::SPtr(pointer);
}

void PhysicalLogReader::Dispose()
{
    if (isDisposed_)
    {
        return;
    }

    if (isValid_ && logManager_ != nullptr)
    {
        logManager_->RemoveLogReader(startingRecordPosition_);
    }

    isDisposed_ = true;
}
 
IAsyncEnumerator<LogRecord::SPtr>::SPtr PhysicalLogReader::GetLogRecords(
    __in PartitionedReplicaId const & traceId,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType,
    __in LONG64 readAheadCacheSizeBytes,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TRInternalSettingsSPtr const & transactionalReplicatorConfig)
{
    IAsyncEnumerator<LogRecord::SPtr>::SPtr logRecords = LogRecords::Create(
        traceId,
        *logManager_,
        startingRecordPosition_,
        endingRecordPosition_,
        startLsn_,
        readerName,
        readerType,
        readAheadCacheSizeBytes,
        healthClient,
        transactionalReplicatorConfig,
        GetThisAllocator());

    return logRecords;
}
