// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace ktl;
using namespace Data::LoggingReplicator;
using namespace Data::LogRecordLib;
using namespace TxnReplicator;

TransactionMap::TransactionMap(__in Data::Utilities::PartitionedReplicaId const & traceId)
    : KObject()
    , KShared()
    , PartitionedReplicaTraceComponent(traceId)
    , lock_()
    , completedTransactions_(GetThisAllocator())
    , latestRecords_()
    , lsnPendingTransactions_()
    , transactionIdPendingTransactionsPair_()
    , unstableTransactions_(GetThisAllocator())
{
    EventSource::Events->Ctor(
        TracePartitionId,
        ReplicaId,
        L"TransactionMap",
        reinterpret_cast<uintptr_t>(this));

    THROW_ON_CONSTRUCTOR_FAILURE(completedTransactions_);
    THROW_ON_CONSTRUCTOR_FAILURE(unstableTransactions_);
}

TransactionMap::~TransactionMap()
{
    EventSource::Events->Dtor(
        TracePartitionId,
        ReplicaId,
        L"TransactionMap",
        reinterpret_cast<uintptr_t>(this));
}

TransactionMap::SPtr TransactionMap::Create(
    __in Data::Utilities::PartitionedReplicaId const & traceId,
    __in KAllocator & allocator)
{
    TransactionMap * pointer = _new(TXMAP_TAG, allocator)TransactionMap(traceId);
    THROW_ON_ALLOCATION_FAILURE(pointer);
    return TransactionMap::SPtr(pointer);
}

void TransactionMap::Reuse()
{
    latestRecords_.clear();
    transactionIdPendingTransactionsPair_.clear();
    completedTransactions_.Clear();
    unstableTransactions_.Clear();
    lsnPendingTransactions_.clear();
}

void TransactionMap::AddOperation(__in OperationLogRecord & record)
{
    ASSERT_IFNOT(record.BaseTransaction.IsValidTransaction, "Transaction is not valid while adding op");

    if (record.BaseTransaction.IsAtomicOperation)
    {
        record.IsEnlistedTransaction = true;
    }
    else
    {
        OperationLogRecord::SPtr recordSPtr = &record;
        TransactionLogRecord::SPtr upcastedRecord = recordSPtr.RawPtr();

        K_LOCK_BLOCK(lock_)
        {
            LONG64 txId = record.BaseTransaction.TransactionId;
            TransactionLogRecord::SPtr keyFound = nullptr;
            auto keyFoundIter = latestRecords_.find(txId);

            if (keyFoundIter != latestRecords_.end())
            {
                keyFound = keyFoundIter->second;
                ASSERT_IF(keyFound == nullptr, "Log record not found during add op for xactid: {0}", txId);

                record.IsEnlistedTransaction = keyFound->IsEnlistedTransaction;
                record.ParentTransactionRecord = keyFound.RawPtr();
                keyFound->ChildTransactionRecord = record;
                latestRecords_[txId] = upcastedRecord;
                
                return;
            }
            
            latestRecords_[txId] = upcastedRecord;
        }

        ASSERT_IFNOT(!record.IsEnlistedTransaction, "Unexpected enlisted xact found during add op");
    }

    record.ParentTransactionRecord = nullptr;
}
            
void TransactionMap::CompleteTransaction(__in EndTransactionLogRecord & record)
{
    ASSERT_IFNOT(!record.BaseTransaction.IsAtomicOperation, "Unexpected atomic op xact during complete xact");

    K_LOCK_BLOCK(lock_)
    {
        LONG64 txId = record.BaseTransaction.TransactionId;
        TransactionLogRecord::SPtr keyFound = nullptr;
        auto keyFoundIter = latestRecords_.find(txId);

        if (keyFoundIter != latestRecords_.end())
        {
            keyFound = keyFoundIter->second;
            ASSERT_IF(keyFound == nullptr, "Log record not found during complete xact for xactid: {0}", txId);

            record.IsEnlistedTransaction = keyFound->IsEnlistedTransaction;
            record.ParentTransactionRecord = keyFound.RawPtr();
            keyFound->ChildTransactionRecord = record;
            latestRecords_.erase(txId);
        }
        else
        {
            ASSERT_IFNOT(!record.IsEnlistedTransaction, "Unexpected enlisted xact found during complext xact");
            record.ParentTransactionRecord = nullptr;
        }

        BeginTransactionOperationLogRecord::SPtr beginTransactionRecord = nullptr;
        auto txIdFoundIter = transactionIdPendingTransactionsPair_.find(txId);

        if (txIdFoundIter != transactionIdPendingTransactionsPair_.end())
        {
            beginTransactionRecord = txIdFoundIter->second;
            ASSERT_IFNOT(beginTransactionRecord != nullptr, "Could not find begin xact log record: {0}", txId);

            auto elementsErased = lsnPendingTransactions_.erase(LsnKeyType(beginTransactionRecord->Lsn));
            ASSERT_IFNOT(elementsErased == 1, "Could not remove lsn from pending list: {0} {1}", beginTransactionRecord->Lsn, elementsErased);
            transactionIdPendingTransactionsPair_.erase(txId);
        }

        ASSERT_IFNOT(
            (record.IsEnlistedTransaction == true) == (beginTransactionRecord != nullptr),
            "Unexpected xact record in complete xact");

        if (beginTransactionRecord != nullptr)
        {
            AddUnstableTransactionCallerHoldsLock(*beginTransactionRecord, record);
        }
    }
}

void TransactionMap::CreateTransaction(__in BeginTransactionOperationLogRecord & record)
{
    ASSERT_IFNOT(!record.BaseTransaction.IsAtomicOperation, "Unexpected atomic op xact during create xact");
    ASSERT_IFNOT(record.ParentTransactionRecord == nullptr, "Parent xact log record not empty during create xact");

    if (record.IsSingleOperationTransaction)
    {
        record.IsEnlistedTransaction = true;
    }
    else
    {
        K_LOCK_BLOCK(lock_)
        {
            LONG64 txId = record.BaseTransaction.TransactionId;
            BeginTransactionOperationLogRecord::SPtr recordSPtr = &record;
            TransactionLogRecord::SPtr upcastedRecord = recordSPtr.RawPtr();

            latestRecords_[txId] = upcastedRecord;
            record.IsEnlistedTransaction = true;

            transactionIdPendingTransactionsPair_[txId] = recordSPtr;
            
            lsnPendingTransactions_[LsnKeyType(record.Lsn)] = recordSPtr;
        }
    }
}

void TransactionMap::RemoveStableTransactions(__in LONG64 lastStableLsn)
{
    K_LOCK_BLOCK(lock_)
    {
        for (LONG i = unstableTransactions_.Count() - 1; i >= 0; i--)
        {
            if (unstableTransactions_[i]->Lsn <= lastStableLsn)
            {
                for (LONG j = 0; j <= i; j++)
                {
                    for (ULONG k = 0; k < completedTransactions_.Count(); k++)
                    {
                        if (completedTransactions_[k]->BaseTransaction == unstableTransactions_[j]->BaseTransaction)
                        {
                            completedTransactions_.Remove(k);
                            break;
                        }
                    }
                }

                if (i > 0)
                {
                    BOOLEAN result = unstableTransactions_.RemoveRange(0, i);
                    ASSERT_IFNOT(result == TRUE, "Could not remove unstable xact");
                }
                break;
            }
        }
    }
}

void TransactionMap::AddUnstableTransactionCallerHoldsLock(
    __in BeginTransactionOperationLogRecord & beginTransactionRecord,
    __in EndTransactionLogRecord & endTransactionRecord)
{
    ASSERT_IFNOT(!beginTransactionRecord.IsSingleOperationTransaction, "Unexpected single op xact");

    NTSTATUS status = STATUS_SUCCESS;

    // Transactions tend to complete in order
    LONG i;
    for (i = completedTransactions_.Count() - 1; i >= 0 ; i--)
    {
        if (completedTransactions_[i]->Lsn < beginTransactionRecord.Lsn)
        {
            status = completedTransactions_.InsertAt(i + 1, &beginTransactionRecord);
            THROW_ON_FAILURE(status);
            break;
        }
    }

    if (i == -1)
    {
        status = completedTransactions_.InsertAt(0, &beginTransactionRecord);
        THROW_ON_FAILURE(status);
    }

    for (i = unstableTransactions_.Count() - 1; i >= 0; i--)
    {
        if (unstableTransactions_[i]->Lsn < endTransactionRecord.Lsn)
        {
            status = unstableTransactions_.InsertAt(i + 1, &endTransactionRecord);
            THROW_ON_FAILURE(status);
            break;
        }
    }

    if (i == -1)
    {
        status = unstableTransactions_.InsertAt(0, &endTransactionRecord);
        THROW_ON_FAILURE(status);
    }
}

#pragma region FALSEPROGRESS
//
// NOTE: In all the FALSE PROGRESS methods below, the incoming parameter 'record' is the one read from disk
// by the truncate tail manager
//
// The return type is the record in memory as part of the transactions map
//
// The first step in these methods is to initialize the return type object to the incoming one. That is because 
// we have found bugs during unit testing where we were making state updates to the incoming 'record' instead of the one found in memory
// 
//
BeginTransactionOperationLogRecord::SPtr TransactionMap::DeleteTransaction(__in BeginTransactionOperationLogRecord & record)
{
    BeginTransactionOperationLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(!recordSPtr->BaseTransaction.IsAtomicOperation, "Unexpected atomic op xact during delete xact");
    ASSERT_IFNOT(recordSPtr->ParentTransactionRecord == nullptr, "Parent xact log record not empty during delete xact");

    if (recordSPtr->IsSingleOperationTransaction)
    {
        return recordSPtr;
    }

    K_LOCK_BLOCK(lock_)
    {
        LONG64 txId = recordSPtr->BaseTransaction.TransactionId;

        ASSERT_IFNOT(latestRecords_.count(txId) > 0, "Transaction log record not found in latest records");
        ASSERT_IFNOT(transactionIdPendingTransactionsPair_.count(txId) > 0, "Transaction log record not found in pending transaction pairs");

        TransactionLogRecord::SPtr keyFoundInLatestRecords = latestRecords_[txId];
        BeginTransactionOperationLogRecord::SPtr keyFoundInPendingTxPair = transactionIdPendingTransactionsPair_[txId];

        recordSPtr = dynamic_cast<BeginTransactionOperationLogRecord *>(keyFoundInLatestRecords.RawPtr());
        ASSERT_IFNOT(recordSPtr != nullptr, "Invalid begin xact op log record during delete");

        ASSERT_IFNOT(recordSPtr.RawPtr() == keyFoundInPendingTxPair.RawPtr(), "Invalid log record"); // TODO: Verify validity of this assert

        latestRecords_.erase(txId);

        recordSPtr->IsEnlistedTransaction = true;

        auto elementsErased = lsnPendingTransactions_.erase(LsnKeyType(recordSPtr->Lsn));
        ASSERT_IFNOT(elementsErased == 1, "Could not remove lsn from pending list: {0} {1}", recordSPtr->Lsn, elementsErased)

        transactionIdPendingTransactionsPair_.erase(txId);
    }

    return recordSPtr;
}

OperationLogRecord::SPtr TransactionMap::FindOperation(__in OperationLogRecord & record)
{
    OperationLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(recordSPtr->BaseTransaction.IsValidTransaction, "Invalid xact in find op");
    ASSERT_IFNOT(recordSPtr->IsOperationContextNull, "Op log record context should be null");

    if (recordSPtr->BaseTransaction.IsAtomicOperation)
    {
        ASSERT_IFNOT(recordSPtr->ParentTransactionRecord == nullptr, "Parent xact log record found for atomic xact in find op");
        return recordSPtr;
    }

    ASSERT_IFNOT(LogRecord::IsInvalid(recordSPtr->ParentTransactionRecord.RawPtr()), "Invalid parent log record in find op");
    
    K_LOCK_BLOCK(lock_)
    {
        LONG64 txId = recordSPtr->BaseTransaction.TransactionId;
        ASSERT_IFNOT(latestRecords_.count(txId) > 0, "Transaction log record not found in latest records during find");

        TransactionLogRecord::SPtr keyFoundInLatestRecords = latestRecords_[txId];

        recordSPtr = dynamic_cast<OperationLogRecord *>(keyFoundInLatestRecords.RawPtr());
        ASSERT_IFNOT(recordSPtr != nullptr, "Invalid op log record in find op");
    }

    return recordSPtr;
}

BeginTransactionOperationLogRecord::SPtr TransactionMap::FindTransaction(__in BeginTransactionOperationLogRecord & record)
{
    BeginTransactionOperationLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(!recordSPtr->BaseTransaction.IsAtomicOperation, "Unexpected atomic op in find xact");
    ASSERT_IFNOT(recordSPtr->ParentTransactionRecord == nullptr, "Parent xact log record not empty in find xact");

    if (recordSPtr->IsSingleOperationTransaction)
    {
        ASSERT_IFNOT(recordSPtr->ParentTransactionRecord == nullptr, "Parent xact log record not empty for single op xact");
        return recordSPtr;
    }

    K_LOCK_BLOCK(lock_)
    {
        LONG64 txId = recordSPtr->BaseTransaction.TransactionId;
        ASSERT_IFNOT(latestRecords_.count(txId) > 0, "Transaction log record not found in latest records during find");

        TransactionLogRecord::SPtr keyFoundInLatestRecords = latestRecords_[txId];

        recordSPtr = dynamic_cast<BeginTransactionOperationLogRecord *>(keyFoundInLatestRecords.RawPtr());
        ASSERT_IFNOT(recordSPtr != nullptr, "Invalid begin xact op log record in find xact");
    }

    return recordSPtr;
}

EndTransactionLogRecord::SPtr TransactionMap::FindUnstableTransaction(__in EndTransactionLogRecord & record)
{
    EndTransactionLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(!recordSPtr->BaseTransaction.IsAtomicOperation, "Unexpected atomic xact in find unstable xact");
    ASSERT_IFNOT(LogRecord::IsInvalid(recordSPtr->ParentTransactionRecord.RawPtr()), "Invalid log record in find unstable xact");
    
    LONG i = 0;

    K_LOCK_BLOCK(lock_)
    {
        for (i = unstableTransactions_.Count() - 1; i >= 0; i--)
        {
            if (unstableTransactions_[i]->BaseTransaction == recordSPtr->BaseTransaction)
            {
                recordSPtr = unstableTransactions_[i];
                break;
            }
        }
    }
    
    ASSERT_IFNOT(i >= 0, "Xact associated with end xact log record is not present in unstable transactions");

    return recordSPtr;
}

BeginTransactionOperationLogRecord::SPtr TransactionMap::GetEarliestPendingTransaction()
{
    bool failedBarrierCheck = false;
    return GetEarliestPendingTransaction(MAXLONG64, failedBarrierCheck);
}

BeginTransactionOperationLogRecord::SPtr TransactionMap::GetEarliestPendingTransaction(
    __in LONG64 barrierLsn,
    __out bool & failedBarrierCheck)
{
    K_LOCK_BLOCK(lock_)
    {
        if (unstableTransactions_.Count() == 0 ||
            unstableTransactions_[unstableTransactions_.Count() - 1]->Lsn <= barrierLsn)
        {
            failedBarrierCheck = false;

            auto lowestLsnIterator = lsnPendingTransactions_.begin();
            if (lowestLsnIterator != lsnPendingTransactions_.end())
            {
                LsnKeyType lsn = lowestLsnIterator->first;
                if (lsn.value < barrierLsn)
                {
                    BeginTransactionOperationLogRecord::SPtr result = lowestLsnIterator->second;
                    return result;
                }
            }

            return nullptr;
        }

        failedBarrierCheck = true;
    }

    return nullptr;
}

void TransactionMap::GetPendingRecords(__out KArray<TransactionLogRecord::SPtr> & pendingRecords)
{
    K_LOCK_BLOCK(lock_)
    {
        NTSTATUS status;

        for (auto const & pair : latestRecords_)
        {
            status = pendingRecords.Append(pair.second);
            THROW_ON_FAILURE(status);
        }
    }
}

void TransactionMap::GetPendingTransactions(__out KArray<BeginTransactionOperationLogRecord::SPtr> & pendingTransactions)
{
    K_LOCK_BLOCK(lock_)
    {
        NTSTATUS status;

        for (auto const & pair : transactionIdPendingTransactionsPair_)
        {
            status = pendingTransactions.Append(pair.second);
            THROW_ON_FAILURE(status);
        }
    }
}

void TransactionMap::GetPendingTransactionsOlderThanPosition(
    __in ULONG64 recordPosition,
    __out KArray<BeginTransactionOperationLogRecord::SPtr> & pendingTransactions)
{
    K_LOCK_BLOCK(lock_)
    {
        NTSTATUS status;

        for (auto lsnIterator = lsnPendingTransactions_.begin(); lsnIterator != lsnPendingTransactions_.end(); lsnIterator++)
        {
            if (lsnIterator->second->RecordPosition <= recordPosition)
            {
                status = pendingTransactions.Append(lsnIterator->second);
                THROW_ON_FAILURE(status);
            }
        }
    }
}

OperationLogRecord::SPtr TransactionMap::RedactOperation(
    __in OperationLogRecord & record,
    __in TransactionLogRecord & invalidTransactionLogRecord)
{
    OperationLogRecord::SPtr recordSPtr = &record;

    ASSERT_IFNOT(recordSPtr->BaseTransaction.IsValidTransaction, "Unexpected invalid xact during redact op");
    ASSERT_IFNOT(recordSPtr->IsOperationContextNull, "Op log record context should be null");

    if (recordSPtr->BaseTransaction.IsAtomicOperation)
    {
        ASSERT_IFNOT(recordSPtr->ParentTransactionRecord == nullptr, "Parent xact found for atomic op");
        return recordSPtr;
    }

    ASSERT_IFNOT(LogRecord::IsInvalid(recordSPtr->ParentTransactionRecord.RawPtr()), "Invalid parent log record during redact");
    
    K_LOCK_BLOCK(lock_)
    {
        LONG64 txId = recordSPtr->BaseTransaction.TransactionId;
        ASSERT_IFNOT(latestRecords_.count(txId) > 0, "Transaction log record not found in redact operation");

        TransactionLogRecord::SPtr keyFoundInLatestRecords = latestRecords_[txId];

        recordSPtr = dynamic_cast<OperationLogRecord *>(keyFoundInLatestRecords.RawPtr());
        ASSERT_IFNOT(recordSPtr != nullptr, "Invalid op log record during redact");

        TransactionLogRecord::SPtr parentRecord = recordSPtr->ParentTransactionRecord;
        latestRecords_[txId] = parentRecord;
        parentRecord->ChildTransactionRecord = invalidTransactionLogRecord;
    }

    return recordSPtr;
}

EndTransactionLogRecord::SPtr TransactionMap::ReifyTransaction(
    __in EndTransactionLogRecord & record,
    __in TransactionLogRecord & invalidTransactionLogRecord)
{
    ASSERT_IFNOT(!record.BaseTransaction.IsAtomicOperation, "Unexpected atomin op xact");
    ASSERT_IFNOT(LogRecord::IsInvalid(record.ParentTransactionRecord.RawPtr()), "Invalid log record");

    BeginTransactionOperationLogRecord::SPtr reifiedBeginTransactionRecord = nullptr;
    EndTransactionLogRecord::SPtr reifiedEndTransactionRecord = nullptr;

    LONG i;

    K_LOCK_BLOCK(lock_)
    {
        for (i = completedTransactions_.Count() - 1; i >= 0; i--)
        {
            if (completedTransactions_[i]->BaseTransaction == record.BaseTransaction)
            {
                reifiedBeginTransactionRecord = completedTransactions_[i];

                ASSERT_IFNOT(!reifiedBeginTransactionRecord->IsSingleOperationTransaction, "Unexpected single op xact");

                completedTransactions_.Remove(i);
                break;
            }
        }

        ASSERT_IFNOT(i >= 0, "Begin xact record is not present in completed xacts");

        for (i = unstableTransactions_.Count() - 1; i >= 0; i--)
        {
            if (unstableTransactions_[i]->BaseTransaction == record.BaseTransaction)
            {
                reifiedEndTransactionRecord = unstableTransactions_[i];
                unstableTransactions_.Remove(i);
                break;
            }
        }

        ASSERT_IFNOT(i >= 0, "End xact record is not present in unstable xacts");

        LONG64 txId = record.BaseTransaction.TransactionId;
        TransactionLogRecord::SPtr parentRecord = reifiedEndTransactionRecord->ParentTransactionRecord;

        latestRecords_[txId] = parentRecord;
        parentRecord->ChildTransactionRecord = invalidTransactionLogRecord;
        parentRecord = reifiedEndTransactionRecord.RawPtr();

        do
        {
            parentRecord = parentRecord->ParentTransactionRecord;
        } while (parentRecord->RecordType != LogRecordType::Enum::BeginTransaction);

        ASSERT_IFNOT(reifiedBeginTransactionRecord.RawPtr() == parentRecord.RawPtr(), "Invalid refied being xact log record");
        ASSERT_IFNOT(reifiedBeginTransactionRecord->IsEnlistedTransaction, "Non enlisted xact in reified xact record");

        transactionIdPendingTransactionsPair_[txId] = reifiedBeginTransactionRecord;
        lsnPendingTransactions_[LsnKeyType(reifiedBeginTransactionRecord->Lsn)] = reifiedBeginTransactionRecord;
    }

    return reifiedEndTransactionRecord;
}

#pragma endregion
