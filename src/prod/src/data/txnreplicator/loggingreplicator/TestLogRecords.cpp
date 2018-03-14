// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

using namespace LoggingReplicatorTests;
using namespace Data::LogRecordLib;
using namespace Data::LoggingReplicator;
using namespace ktl;

TestLogRecords::SPtr TestLogRecords::Create(
    __in KAllocator & allocator)
{
    TestLogRecords * logRecords = _new(TEST_LOGRECORDS, allocator) TestLogRecords();
    CODING_ERROR_ASSERT(logRecords != nullptr);
    CODING_ERROR_ASSERT(NT_SUCCESS(logRecords->Status()));

    return SPtr(logRecords);
}

ULONG TestLogRecords::GetCount() const
{
    return logRecords_.Count();
}

ULONG TestLogRecords::GetLogicalLogRecordCount() const
{
    ULONG count = 0;

    for (ULONG i = 0; i < logRecords_.Count(); i++)
    {
        LogRecord::SPtr currentLogRecord = logRecords_[i];

        if (currentLogRecord->AsLogicalLogRecord() != nullptr)
        {
            count++;
        }
    }

    return count;
}

TxnReplicator::Epoch TestLogRecords::GetLastEpoch()
{
    return TxnReplicator::Epoch(dataLossNumber_, configurationNumber_);
}

FABRIC_SEQUENCE_NUMBER TestLogRecords::GetLSN()
{
    return currentLSN_;
}

IndexingLogRecord::SPtr TestLogRecords::GetLastIndexingLogRecord()
{
    CODING_ERROR_ASSERT(lastIndexingLogRecordSPtr_ != nullptr);

    return lastIndexingLogRecordSPtr_;
}

BackupLogRecord::SPtr TestLogRecords::GetLastBackupLogRecord()
{
    CODING_ERROR_ASSERT(lastBackupLogRecordSPtr_ != nullptr);

    return lastBackupLogRecordSPtr_;
}

LogRecord::SPtr TestLogRecords::GetLastLogRecord()
{
    CODING_ERROR_ASSERT(logRecords_.Count() > 0);

    return logRecords_[logRecords_.Count() - 1];
}

void TestLogRecords::InitializeWithNewLogRecords()
{
    NTSTATUS status = STATUS_UNSUCCESSFUL;

    IndexingLogRecord::SPtr zeroIndexRecord = IndexingLogRecord::CreateZeroIndexingLogRecord(
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());
    UpdateEpochLogRecord::SPtr zeroUpdateEpochLogRecord = UpdateEpochLogRecord::CreateZeroUpdateEpochLogRecord(
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());
    BeginCheckpointLogRecord::SPtr zeroBeginCheckpointRecord = BeginCheckpointLogRecord::CreateZeroBeginCheckpointLogRecord(
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_BeginTransactionOperationLogRecord,
        GetThisAllocator());
    BarrierLogRecord::SPtr oneBarrierLogRecord = BarrierLogRecord::CreateOneBarrierRecord(
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    lastBarrierLogRecordSPtr_ = oneBarrierLogRecord;

    EndCheckpointLogRecord::SPtr oneEndCheckpointRecord =EndCheckpointLogRecord::CreateOneEndCheckpointLogRecord(
        *zeroBeginCheckpointRecord, 
        *zeroIndexRecord,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());
    CompleteCheckPointLogRecord::SPtr oneCompleteCheckpointRecord = CompleteCheckPointLogRecord::Create(
        1,
        *zeroIndexRecord,
        oneEndCheckpointRecord.RawPtr(),
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    // Test workaround
    oneCompleteCheckpointRecord->LinkedPhysicalRecordOffset = 0;

    // Initialize
    lastIndexingLogRecordSPtr_ = zeroIndexRecord;
    lastBarrierLogRecordSPtr_ = oneBarrierLogRecord;

    status = logRecords_.Append(zeroIndexRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    lastPhysicalLogRecordSPtr = zeroIndexRecord.RawPtr();

    status = logRecords_.Append(zeroUpdateEpochLogRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    zeroUpdateEpochLogRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    status = logRecords_.Append(zeroBeginCheckpointRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    zeroBeginCheckpointRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());
    lastPhysicalLogRecordSPtr = zeroBeginCheckpointRecord.RawPtr();

    status = logRecords_.Append(oneBarrierLogRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    oneBarrierLogRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    status = logRecords_.Append(oneEndCheckpointRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    oneEndCheckpointRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());
    lastPhysicalLogRecordSPtr = oneEndCheckpointRecord.RawPtr();

    status = logRecords_.Append(oneCompleteCheckpointRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
    oneCompleteCheckpointRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());
    lastPhysicalLogRecordSPtr = oneCompleteCheckpointRecord.RawPtr();
}

void TestLogRecords::PopulateWithRandomRecords(
    __in ULONG32 count,
    __in_opt bool includeBackup,
    __in_opt bool includeUpdateEpoch)
{
    for (ULONG32 i = 0; i < count; i++)
    {
        int divineChoice = random_.Next(0, 5);

        switch (divineChoice)
        {
        case 0:
            AddAtomicOperationLogRecord();
            break;
        case 1:
            AddAtomicRedoOperationLogRecord();
            break;
        case 2:
            if (includeBackup)
            {
                AddBackupLogRecord();
            }
            else 
            {
                AddAtomicOperationLogRecord();
            }

            break;
        case 3:
            if (includeUpdateEpoch)
            {
                AddUpdateEpochLogRecord();
            }
            else
            {
                AddAtomicRedoOperationLogRecord();
            }

            break;
        case 4:
            AddBarrierLogRecord();
            break;
        }
    }
}

void TestLogRecords::AddAtomicOperationLogRecord()
{
    FABRIC_SEQUENCE_NUMBER LSN = InterlockedIncrement64(&currentLSN_);

    TxnReplicator::AtomicOperation::SPtr atomicOperation = TxnReplicator::AtomicOperation::CreateAtomicOperation(
        -LSN,
        true,
        GetThisAllocator());

    OperationLogRecord::SPtr operationLogRecord = OperationLogRecord::Create(
        *atomicOperation,
        mockPayLoad_.RawPtr(),
        mockPayLoad_.RawPtr(),
        mockPayLoad_.RawPtr(),
        nullptr,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    operationLogRecord->Lsn = LSN;
    operationLogRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    NTSTATUS status = logRecords_.Append(operationLogRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

void TestLogRecords::AddAtomicRedoOperationLogRecord()
{
    FABRIC_SEQUENCE_NUMBER LSN = InterlockedIncrement64(&currentLSN_);

    TxnReplicator::AtomicRedoOperation::SPtr atomicRedoOperation = TxnReplicator::AtomicRedoOperation::CreateAtomicRedoOperation(
        -LSN,
        true,
        GetThisAllocator());

    OperationLogRecord::SPtr operationLogRecord = OperationLogRecord::Create(
        *atomicRedoOperation,
        mockPayLoad_.RawPtr(),
        mockPayLoad_.RawPtr(),
        nullptr,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        *invalidLogRecords_->Inv_TransactionLogRecord,
        GetThisAllocator());

    operationLogRecord->Lsn = LSN;
    operationLogRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    NTSTATUS status = logRecords_.Append(operationLogRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

BackupLogRecord::SPtr TestLogRecords::AddBackupLogRecord()
{
    FABRIC_SEQUENCE_NUMBER lastBackedUpLSN = currentLSN_;

    KGuid backupId;
    backupId.CreateNew();

    TxnReplicator::Epoch epoch(dataLossNumber_, configurationNumber_);

    BackupLogRecord::SPtr backupLogRecord = BackupLogRecord::Create(
        backupId,
        epoch,
        lastBackedUpLSN,
        10,
        100,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    AddBackupLogRecord(*backupLogRecord);
    return backupLogRecord;
}

void TestLogRecords::AddBackupLogRecord(
    __in BackupLogRecord & backupLogRecord)
{
    FABRIC_SEQUENCE_NUMBER LSN = InterlockedIncrement64(&currentLSN_);
    backupLogRecord.Lsn = LSN;
    backupLogRecord.set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    NTSTATUS status = logRecords_.Append(&backupLogRecord);

    lastBackupLogRecordSPtr_ = &backupLogRecord;
    CODING_ERROR_ASSERT(NT_SUCCESS(status));
}

void TestLogRecords::AddUpdateEpochLogRecord()
{
    KGuid backupId;
    backupId.CreateNew();

    LONG64 dl = InterlockedIncrement64(&dataLossNumber_);
    LONG64 cn = InterlockedIncrement64(&configurationNumber_);
    LONG64 replicaId = InterlockedIncrement64(&primaryReplicaId_);

    TxnReplicator::Epoch epoch(dl, cn);

    UpdateEpochLogRecord::SPtr updateEpochLogRecord = UpdateEpochLogRecord::Create(
        epoch,
        replicaId,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    updateEpochLogRecord->Lsn = currentLSN_;
    updateEpochLogRecord->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    NTSTATUS status = logRecords_.Append(updateEpochLogRecord.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    progressVectorSPtr_->Insert(*updateEpochLogRecord);
}

BarrierLogRecord::SPtr TestLogRecords::AddBarrierLogRecord()
{ 
    FABRIC_SEQUENCE_NUMBER LSN = InterlockedIncrement64(&currentLSN_);

    BarrierLogRecord::SPtr barrierLogRecordSPtr = BarrierLogRecord::Create(
        lastBarrierLogRecordSPtr_->Lsn,
        *invalidLogRecords_->Inv_PhysicalLogRecord,
        GetThisAllocator());

    barrierLogRecordSPtr->Lsn = LSN;
    barrierLogRecordSPtr->set_previousPhysicalRecord(lastPhysicalLogRecordSPtr.RawPtr());

    NTSTATUS status = logRecords_.Append(barrierLogRecordSPtr.RawPtr());
    CODING_ERROR_ASSERT(NT_SUCCESS(status));

    lastBarrierLogRecordSPtr_ = barrierLogRecordSPtr;
    return barrierLogRecordSPtr;
}

void TestLogRecords::Truncate(
    __in FABRIC_SEQUENCE_NUMBER lsn, 
    __in bool inclusive)
{
    for (ULONG i = 0; i < logRecords_.Count(); i++)
    {
        LogRecord::SPtr logRecord = logRecords_[i];

        if (logRecord->Lsn > lsn)
        {
            return;
        }

        if (!inclusive && logRecord->Lsn == lsn)
        {
            return;
        }

        logRecords_.Remove(i);
        i--;

        // We remove only the first matching LSN instance (UpdateEpochs would remain)
        if (logRecord->Lsn == lsn)
        {
            return;
        }
    }
}

ProgressVector::SPtr TestLogRecords::CopyProgressVector() const
{
    return ProgressVector::Clone(*progressVectorSPtr_, 0, TxnReplicator::Epoch::ZeroEpoch(), TxnReplicator::Epoch::ZeroEpoch(), GetThisAllocator());
}

LogRecord::SPtr TestLogRecords::GetCurrent()
{
    ASSERT_IFNOT(currentIndex_ >= 0, "current index must not be negative");
    return logRecords_[currentIndex_];
}

ktl::Awaitable<bool> TestLogRecords::MoveNextAsync(
    __in CancellationToken const & cancellationToken)
{
    UNREFERENCED_PARAMETER(cancellationToken);

    LONG32 count = static_cast<LONG32>(logRecords_.Count());
    if (currentIndex_ >= (count - 1))
    {
        co_return false;
    }

    co_await suspend_never();

    currentIndex_++;
    co_return true;
}

void TestLogRecords::Reset()
{
    currentIndex_ = -1;
}

void TestLogRecords::Dispose()
{   
}

TestLogRecords::TestLogRecords()
    : KObject<TestLogRecords>()
    , KShared<TestLogRecords>()
    , invalidLogRecords_(InvalidLogRecords::Create(GetThisAllocator()))
    , logRecords_(GetThisAllocator())
    , progressVectorSPtr_(ProgressVector::Create(GetThisAllocator()))
{
    if (NT_SUCCESS(invalidLogRecords_->Status()) == false)
    {
        SetConstructorStatus(invalidLogRecords_->Status());
        return;
    }

    if (NT_SUCCESS(logRecords_.Status()) == false)
    {
        SetConstructorStatus(logRecords_.Status());
        return;
    }

    KBuffer::SPtr buffer;
    NTSTATUS status = KBuffer::Create(1024, buffer, GetThisAllocator());
    if (NT_SUCCESS(status) == false)
    {
        SetConstructorStatus(status);
        return;
    }

    Data::Utilities::OperationData::SPtr operationData = Data::Utilities::OperationData::Create(GetThisAllocator());
    operationData->Append(*buffer);
    operationData->Append(*buffer);

    SetConstructorStatus(STATUS_SUCCESS);
}

TestLogRecords::~TestLogRecords()
{
}

