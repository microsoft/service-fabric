// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;

TestLogManager::SPtr TestLogManager::Create(
    __in KAllocator & allocator)
{
    TestLogManager * logManager = _new(TEST_LOG_MANAGER_TAG, allocator) TestLogManager();
    CODING_ERROR_ASSERT(logManager != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(logManager->Status()));

    return SPtr(logManager);
}

void TestLogManager::InitializeWithNewLogRecords()
{
    testLogRecords_->InitializeWithNewLogRecords();
}

IndexingLogRecord::SPtr TestLogManager::GetLastIndexingLogRecord() const
{
    return testLogRecords_->GetLastIndexingLogRecord();
}

BackupLogRecord::SPtr TestLogManager::AddBackupLogRecord()
{
    return testLogRecords_->AddBackupLogRecord();
}

BarrierLogRecord::SPtr TestLogManager::AddBarrierLogRecord()
{
    return testLogRecords_->AddBarrierLogRecord();
}

KSharedPtr<IPhysicalLogReader> TestLogManager::GetPhysicalLogReader(
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in LONG64 startingLsn,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType)
{
    UNREFERENCED_PARAMETER(startingRecordPosition);
    UNREFERENCED_PARAMETER(endingRecordPosition);
    UNREFERENCED_PARAMETER(startingLsn);
    UNREFERENCED_PARAMETER(readerName);
    UNREFERENCED_PARAMETER(readerType);

    return TestPhysicalLogReader::Create(*testLogRecords_, GetThisAllocator()).RawPtr();
}

ktl::Awaitable<void> TestLogManager::FlushAsync(__in KStringView const & initiator)
{
    UNREFERENCED_PARAMETER(initiator);

    co_await suspend_never();
    co_return;
}

Data::Log::ILogicalLogReadStream::SPtr TestLogManager::CreateReaderStream()
{
    return nullptr;
}

void TestLogManager::SetSequentialAccessReadSize(
    __in Data::Log::ILogicalLogReadStream & readStream,
    __in LONG64 sequentialReadSize)
{
    UNREFERENCED_PARAMETER(readStream);
    UNREFERENCED_PARAMETER(sequentialReadSize);
}

bool TestLogManager::AddLogReader(
    __in LONG64 startingLsn,
    __in ULONG64 startingRecordPosition,
    __in ULONG64 endingRecordPosition,
    __in KStringView const & readerName,
    __in LogReaderType::Enum readerType)
{
    UNREFERENCED_PARAMETER(startingLsn);
    UNREFERENCED_PARAMETER(startingRecordPosition);
    UNREFERENCED_PARAMETER(endingRecordPosition);
    UNREFERENCED_PARAMETER(readerName);
    UNREFERENCED_PARAMETER(readerType);
    return false;
}

void TestLogManager::RemoveLogReader(__in ULONG64 startingRecordPosition)
{
    UNREFERENCED_PARAMETER(startingRecordPosition);
}

TestLogManager::TestLogManager()
    : KObject()
    , KShared()
    , ILogManager()
    , testLogRecords_(TestLogRecords::Create(GetThisAllocator()))
{
}

TestLogManager::~TestLogManager()
{
}
