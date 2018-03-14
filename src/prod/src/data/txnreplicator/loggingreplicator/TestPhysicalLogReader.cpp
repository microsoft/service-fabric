// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;

using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;

TestPhysicalLogReader::SPtr TestPhysicalLogReader::Create(
    __in Data::Utilities::IAsyncEnumerator<LogRecord::SPtr> & logRecords,
    __in KAllocator & allocator)
{
    TestPhysicalLogReader * logReader = _new(TEST_PHYSICAL_LOG_READER_TAG, allocator) TestPhysicalLogReader(logRecords);
    CODING_ERROR_ASSERT(logReader != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(logReader->Status()));

    return SPtr(logReader);
}

ULONG64 TestPhysicalLogReader::get_StartingRecordPosition() const
{
    return 0;
}

ULONG64 TestPhysicalLogReader::get_EndingRecordPosition() const
{
    return 0;
}

bool TestPhysicalLogReader::get_IsValid() const
{
    return !isDisposed_;
}

Data::Utilities::IAsyncEnumerator<LogRecord::SPtr>::SPtr TestPhysicalLogReader::GetLogRecords(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType,
    __in LONG64 readAheadCacheSizeBytes,
    __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
    __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig)
{
    UNREFERENCED_PARAMETER(traceId);
    UNREFERENCED_PARAMETER(readerName);
    UNREFERENCED_PARAMETER(readerType);
    UNREFERENCED_PARAMETER(readAheadCacheSizeBytes);
    UNREFERENCED_PARAMETER(healthClient);
    UNREFERENCED_PARAMETER(transactionalReplicatorConfig);

    return logRecords_;
}

void TestPhysicalLogReader::Dispose()
{
    isDisposed_ = true;
}

TestPhysicalLogReader::TestPhysicalLogReader(
    __in Data::Utilities::IAsyncEnumerator<LogRecord::SPtr> & logRecords)
    : KObject()
    , KShared()
    , IPhysicalLogReader()
    , logRecords_(&logRecords)
{
}

TestPhysicalLogReader::~TestPhysicalLogReader()
{
}
