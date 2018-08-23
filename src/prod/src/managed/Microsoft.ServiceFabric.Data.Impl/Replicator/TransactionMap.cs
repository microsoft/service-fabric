// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System.Collections.Generic;
    using System.Linq;

    internal class TransactionMap
    {
        private readonly List<BeginTransactionOperationLogRecord> completedTransactions; // lsn ordered

        private readonly Dictionary<long, TransactionLogRecord> latestRecords;

        private readonly SortedDictionary<long, BeginTransactionOperationLogRecord> lsnPendingTransactionsPair;

        // needed to quickly find earliest pending tx. Order by lsn
        private readonly Dictionary<long, BeginTransactionOperationLogRecord> transactionIdPendingTransactionsPair;

        private readonly object txmapLock;

        private readonly List<EndTransactionLogRecord> unstableTransactions; // lsn ordered

        internal TransactionMap()
        {
            this.latestRecords = new Dictionary<long, TransactionLogRecord>();
            this.transactionIdPendingTransactionsPair = new Dictionary<long, BeginTransactionOperationLogRecord>();
            this.lsnPendingTransactionsPair = new SortedDictionary<long, BeginTransactionOperationLogRecord>();
            this.completedTransactions = new List<BeginTransactionOperationLogRecord>();
            this.unstableTransactions = new List<EndTransactionLogRecord>();
            this.txmapLock = new object();

            // Clear any new item in Reuse()
        }

        internal void Reuse()
        {
            this.latestRecords.Clear();
            this.transactionIdPendingTransactionsPair.Clear();
            this.lsnPendingTransactionsPair.Clear();
            this.completedTransactions.Clear();
            this.unstableTransactions.Clear();
        }

        internal void AddOperation(OperationLogRecord record)
        {
            Utility.Assert(record.Transaction.IsValidTransaction, "record.Transaction.IsValidTransaction == true");

            if (record.Transaction.IsAtomicOperation)
            {
                record.IsEnlistedTransaction = true;
            }
            else
            {
                lock (this.txmapLock)
                {
                    if (this.latestRecords.ContainsKey(record.Transaction.Id))
                    {
                        record.IsEnlistedTransaction = this.latestRecords[record.Transaction.Id].IsEnlistedTransaction;
                        record.ParentTransactionRecord = this.latestRecords[record.Transaction.Id];
                        record.ParentTransactionRecord.ChildTransactionRecord = record;
                        this.latestRecords[record.Transaction.Id] = record;
                        return;
                    }

                    this.latestRecords[record.Transaction.Id] = record;
                }

                Utility.Assert(record.IsEnlistedTransaction == false, "record.IsEnlistedTransaction == false");
            }

            record.ParentTransactionRecord = null;
        }

        internal void CompleteTransaction(EndTransactionLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");

            lock (this.txmapLock)
            {
                if (this.latestRecords.ContainsKey(record.Transaction.Id))
                {
                    record.IsEnlistedTransaction = this.latestRecords[record.Transaction.Id].IsEnlistedTransaction;
                    record.ParentTransactionRecord = this.latestRecords[record.Transaction.Id];
                    record.ParentTransactionRecord.ChildTransactionRecord = record;
                    this.latestRecords.Remove(record.Transaction.Id);
                }
                else
                {
                    Utility.Assert(record.IsEnlistedTransaction == false, "record.IsEnlistedTransaction == false");
                    record.ParentTransactionRecord = null;
                }

                BeginTransactionOperationLogRecord beginTransactionRecord = null;
                if (this.transactionIdPendingTransactionsPair.ContainsKey(record.Transaction.Id))
                {
                    beginTransactionRecord = this.transactionIdPendingTransactionsPair[record.Transaction.Id];
                    this.lsnPendingTransactionsPair.Remove(
                        this.transactionIdPendingTransactionsPair[record.Transaction.Id].Lsn.LSN);
                    this.transactionIdPendingTransactionsPair.Remove(record.Transaction.Id);
                }

                Utility.Assert(
                    (record.IsEnlistedTransaction == true) == (beginTransactionRecord != null),
                    "(record.IsEnlistedTransaction == true) == (beginTransactionRecord != null)");
                if (beginTransactionRecord != null)
                {
                    this.AddUnstableTransactionCallerHoldsLock(beginTransactionRecord, record);
                }
            }
        }

        internal void CreateTransaction(BeginTransactionOperationLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");
            Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");

            if (record.IsSingleOperationTransaction)
            {
                record.IsEnlistedTransaction = true;
            }
            else
            {
                lock (this.txmapLock)
                {
                    this.latestRecords[record.Transaction.Id] = record;
                    record.IsEnlistedTransaction = true;
                    this.transactionIdPendingTransactionsPair[record.Transaction.Id] = record;
                    this.lsnPendingTransactionsPair[record.Lsn.LSN] = record;
                }
            }
        }

        internal BeginTransactionOperationLogRecord DeleteTransaction(BeginTransactionOperationLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");
            Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");

            if (record.IsSingleOperationTransaction)
            {
                Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");
            }
            else
            {
                lock (this.txmapLock)
                {
                    Utility.Assert(
                        this.latestRecords.ContainsKey(record.Transaction.Id),
                        "transaction log record not found in latest records");
                    Utility.Assert(
                        this.transactionIdPendingTransactionsPair.ContainsKey(record.Transaction.Id),
                        "transaction log record not found in penging records");

                    record = (BeginTransactionOperationLogRecord)this.latestRecords[record.Transaction.Id];

                    Utility.Assert(
                        record == this.transactionIdPendingTransactionsPair[record.Transaction.Id],
                        "pendingTransactions_[id] == record");

                    this.latestRecords.Remove(record.Transaction.Id);
                    record.IsEnlistedTransaction = false;

                    this.lsnPendingTransactionsPair.Remove(
                        this.transactionIdPendingTransactionsPair[record.Transaction.Id].Lsn.LSN);
                    this.transactionIdPendingTransactionsPair.Remove(record.Transaction.Id);
                }
            }

            return record;
        }

        internal OperationLogRecord FindOperation(OperationLogRecord record)
        {
            Utility.Assert(record.Transaction.IsValidTransaction, "record.Transaction.IsValidTransaction == true");
            Utility.Assert(record.IsOperationContextPresent == false, "record.IsOperationContextPresent == false");

            if (record.Transaction.IsAtomicOperation)
            {
                Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");
            }
            else
            {
                Utility.Assert(
                    LogRecord.IsInvalidRecord(record.ParentTransactionRecord),
                    "LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true");
                lock (this.txmapLock)
                {
                    Utility.Assert(
                        this.latestRecords.ContainsKey(record.Transaction.Id),
                        "Could not find operation in latest records");
                    return (OperationLogRecord)this.latestRecords[record.Transaction.Id];
                }
            }

            return record;
        }

        internal BeginTransactionOperationLogRecord FindTransaction(BeginTransactionOperationLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");
            Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");

            if (record.IsSingleOperationTransaction)
            {
                Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");
            }
            else
            {
                lock (this.txmapLock)
                {
                    Utility.Assert(
                        this.latestRecords.ContainsKey(record.Transaction.Id),
                        "Begin transaction log record was not found in latest records");
                    record = (BeginTransactionOperationLogRecord)this.latestRecords[record.Transaction.Id];
                }
            }

            return record;
        }

        internal EndTransactionLogRecord FindUnstableTransaction(EndTransactionLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");
            Utility.Assert(
                LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true,
                "LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true");

            int i;
            lock (this.txmapLock)
            {
                for (i = this.unstableTransactions.Count - 1; i >= 0; i--)
                {
                    if (this.unstableTransactions[i].Transaction == record.Transaction)
                    {
                        record = this.unstableTransactions[i];
                        break;
                    }
                }
            }

            Utility.Assert(
                i >= 0,
                "Transaction associated with the end transaction log record is not presnet in unstable transactions");
            return record;
        }

        internal BeginTransactionOperationLogRecord GetEarliestPendingTransaction()
        {
            var failedBarrierCheck = false;
            return this.GetEarliestPendingTransaction(long.MaxValue, out failedBarrierCheck);
        }

        internal BeginTransactionOperationLogRecord GetEarliestPendingTransaction(
            long barrierLsn,
            out bool failedBarrierCheck)
        {
            lock (this.txmapLock)
            {
                if (this.unstableTransactions.Count == 0
                    || (this.unstableTransactions.Count > 0
                        && this.unstableTransactions[this.unstableTransactions.Count - 1].Lsn.LSN <= barrierLsn))
                {
                    failedBarrierCheck = false;

                    if (this.lsnPendingTransactionsPair.Count > 0
                        && this.lsnPendingTransactionsPair.Keys.Min() < barrierLsn)
                    {
                        return this.lsnPendingTransactionsPair[this.lsnPendingTransactionsPair.Keys.Min()];
                    }

                    return null; 
                }

                failedBarrierCheck = true;
                return null;
            }
        }

        internal IEnumerable<TransactionLogRecord> GetPendingRecords()
        {
            lock (this.txmapLock)
            {
                return this.latestRecords.Values;
            }
        }

        internal List<BeginTransactionOperationLogRecord> GetPendingTransactions()
        {
            lock (this.txmapLock)
            {
                return this.transactionIdPendingTransactionsPair.Values.ToList();
            }
        }

        internal List<BeginTransactionOperationLogRecord> GetPendingTransactionsOlderThanPosition(ulong recordPosition)
        {
            List<BeginTransactionOperationLogRecord> pendingTxList = null;

            lock (this.txmapLock)
            {
                foreach (var pendingTx in this.lsnPendingTransactionsPair)
                {
                    if (pendingTx.Value.RecordPosition <= recordPosition)
                    {
                        if (pendingTxList == null)
                        {
                            pendingTxList = new List<BeginTransactionOperationLogRecord>();
                        }

                        pendingTxList.Add(pendingTx.Value);
                    }
                }
            }

            return pendingTxList;
        }

        internal OperationLogRecord RedactOperation(OperationLogRecord record)
        {
            Utility.Assert(record.Transaction.IsValidTransaction, "record.Transaction.IsValidTransaction == true");
            Utility.Assert(record.IsOperationContextPresent == false, "record.IsOperationContextPresent == false");

            if (record.Transaction.IsAtomicOperation)
            {
                Utility.Assert(record.ParentTransactionRecord == null, "record.ParentTransactionRecord == null");
            }
            else
            {
                Utility.Assert(
                    LogRecord.IsInvalidRecord(record.ParentTransactionRecord),
                    "LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true");
                lock (this.txmapLock)
                {
                    Utility.Assert(
                        this.latestRecords.ContainsKey(record.Transaction.Id),
                        "latestRecords_.ContainsKey(record.Transaction.Id)");

                    record = (OperationLogRecord)this.latestRecords[record.Transaction.Id];
                    this.latestRecords[record.Transaction.Id] = record.ParentTransactionRecord;
                    record.ParentTransactionRecord.ChildTransactionRecord =
                        TransactionLogRecord.InvalidTransactionLogRecord;
                    return record;
                }
            }

            return record;
        }

        internal EndTransactionLogRecord ReifyTransaction(EndTransactionLogRecord record)
        {
            Utility.Assert(
                record.Transaction.IsAtomicOperation == false,
                "record.Transaction.IsAtomicOperation == false");
            Utility.Assert(
                LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true,
                "LogRecord.IsInvalidRecord(record.ParentTransactionRecord) == true");

            BeginTransactionOperationLogRecord reifiedBeginTransactionRecord = null;
            EndTransactionLogRecord reifiedEndTransactionRecord = null;

            int i;
            lock (this.txmapLock)
            {
                for (i = this.completedTransactions.Count - 1; i >= 0; i--)
                {
                    if (this.completedTransactions[i].Transaction == record.Transaction)
                    {
                        reifiedBeginTransactionRecord = this.completedTransactions[i];
                        Utility.Assert(
                            reifiedBeginTransactionRecord.IsSingleOperationTransaction == false,
                            "reifiedBeginTransactionRecord.IsSingleOperationTransaction == false");
                        this.completedTransactions.RemoveAt(i);
                        break;
                    }
                }

                Utility.Assert(i > -1, "Begin transaction record is not present in completed transactions");

                for (i = this.unstableTransactions.Count - 1; i >= 0; i--)
                {
                    if (this.unstableTransactions[i].Transaction == record.Transaction)
                    {
                        reifiedEndTransactionRecord = this.unstableTransactions[i];
                        this.unstableTransactions.RemoveAt(i);
                        break;
                    }
                }

                Utility.Assert(i > -1, "End transaction record is not present in unstable transactions");

                this.latestRecords[record.Transaction.Id] = reifiedEndTransactionRecord.ParentTransactionRecord;
                reifiedEndTransactionRecord.ParentTransactionRecord.ChildTransactionRecord =
                    TransactionLogRecord.InvalidTransactionLogRecord;

                TransactionLogRecord parentRecord = reifiedEndTransactionRecord;
                BeginTransactionOperationLogRecord chainedBeginTransactionRecord;
                do
                {
                    parentRecord = parentRecord.ParentTransactionRecord;
                    chainedBeginTransactionRecord = parentRecord as BeginTransactionOperationLogRecord;
                } while (chainedBeginTransactionRecord == null);

                Utility.Assert(
                    reifiedBeginTransactionRecord == chainedBeginTransactionRecord,
                    "reifiedBeginTransactionRecord == chainedBeginTransactionRecord");
                Utility.Assert(
                    reifiedBeginTransactionRecord.IsEnlistedTransaction,
                    "reifiedBeginTransactionRecord.IsEnlistedTransaction == true");
                this.transactionIdPendingTransactionsPair[reifiedBeginTransactionRecord.Transaction.Id] =
                    reifiedBeginTransactionRecord;
                this.lsnPendingTransactionsPair[reifiedBeginTransactionRecord.Lsn.LSN] = reifiedBeginTransactionRecord;
            }

            return reifiedEndTransactionRecord;
        }

        internal void RemoveStableTransactions(LogicalSequenceNumber lastStableLsn)
        {
            lock (this.txmapLock)
            {
                for (var i = this.unstableTransactions.Count - 1; i >= 0; i--)
                {
                    if (this.unstableTransactions[i].Lsn <= lastStableLsn)
                    {
                        for (var j = 0; j <= i; j++)
                        {
                            for (var k = 0; k < this.completedTransactions.Count; k++)
                            {
                                if (this.completedTransactions[k].Transaction
                                    == this.unstableTransactions[j].Transaction)
                                {
                                    this.completedTransactions.RemoveAt(k);
                                    break;
                                }
                            }
                        }

                        this.unstableTransactions.RemoveRange(0, i);
                        break;
                    }
                }
            }
        }

        private void AddUnstableTransactionCallerHoldsLock(
            BeginTransactionOperationLogRecord beginTransactionRecord,
            EndTransactionLogRecord endTransactionRecord)
        {
            Utility.Assert(
                beginTransactionRecord.IsSingleOperationTransaction == false,
                "beginTransactionRecord.IsSingleOperationTransaction == false");

            // Transactions tend to complete in order
            int i;
            for (i = this.completedTransactions.Count - 1; i >= 0; i--)
            {
                if (this.completedTransactions[i].Lsn < beginTransactionRecord.Lsn)
                {
                    this.completedTransactions.Insert(i + 1, beginTransactionRecord);
                    break;
                }
            }

            if (i == -1)
            {
                this.completedTransactions.Insert(0, beginTransactionRecord);
            }

            for (i = this.unstableTransactions.Count - 1; i >= 0; i--)
            {
                if (this.unstableTransactions[i].Lsn < endTransactionRecord.Lsn)
                {
                    this.unstableTransactions.Insert(i + 1, endTransactionRecord);
                    break;
                }
            }

            if (i == -1)
            {
                this.unstableTransactions.Insert(0, endTransactionRecord);
            }
        }
    }
}