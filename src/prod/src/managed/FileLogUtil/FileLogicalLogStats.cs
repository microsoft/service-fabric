// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FileLogUtility
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using Microsoft.ServiceFabric.Replicator;

    class FileLogicalLogStats
    {
        private Dictionary<LogRecordType, LogRecordStats> recordStats = new Dictionary<LogRecordType, LogRecordStats>();

        public long TotalRecordCount { get; private set; }
        public long TotalRecordBytes { get; private set; }

        public void ProcessRecord(LogRecord record)
        {
            LogRecordStats stats = null;
            if (!this.recordStats.TryGetValue(record.RecordType, out stats))
            {
                stats = LogRecordStatsUtil.GetStatsForRecordType(record.RecordType);
                this.recordStats.Add(record.RecordType, stats);
            }

            // Accumulate statistics about the records.
            this.TotalRecordCount++;
            this.TotalRecordBytes += record.RecordSize;

            // Allow the record stats type to process additional information about the record.
            stats.ProcessRecord(record);
        }

        public void DisplayStatistics()
        {
            // Display the stats in order of byte usage.
            foreach (var stats in this.recordStats.Values.OrderByDescending(lrs => lrs.TotalBytes))
            {
                stats.DisplayStatistics(this.TotalRecordCount, this.TotalRecordBytes);
            }

            // Display interesting statistics about specific record types.
            this.DisplaySummaryStatistics();
        }

        private void DisplayBarrierStatistics()
        {
            LogRecordStats barrierStats = null;
            LogRecordStats beginTransactionStats = null;
            if (this.recordStats.TryGetValue(LogRecordType.Barrier, out barrierStats) && this.recordStats.TryGetValue(LogRecordType.BeginTransaction, out beginTransactionStats))
            {
                double barrierCount = Math.Max(barrierStats.Count, 1);
                double transactionCount = beginTransactionStats.Count;

                Console.WriteLine();
                Console.WriteLine("Transactions per Barrier:\t\t{0}", Math.Round(transactionCount / barrierCount, digits: 2));
            }
        }

        private void DisplaySummaryStatistics()
        {
            OperationLogRecordStats beginTransactionStats = this.GetOperationStats(LogRecordType.BeginTransaction);
            OperationLogRecordStats operationStats = this.GetOperationStats(LogRecordType.Operation);

            Console.WriteLine();
            Console.WriteLine("SUMMARY");
            Console.WriteLine("-------------------------------------");
            Console.WriteLine("Total records:\t\t\t\t{0}", this.TotalRecordCount);
            Console.WriteLine("Total bytes:\t\t\t\t{0}", StatUtility.GetBytesString(this.TotalRecordBytes));
            this.DisplayBarrierStatistics();

            long operationBytes = beginTransactionStats.TotalBytes + operationStats.TotalBytes;

            long metaDataBytes = beginTransactionStats.MetaDataBytes + operationStats.MetaDataBytes;
            long redoBytes = beginTransactionStats.RedoBytes + operationStats.RedoBytes;
            long undoBytes = beginTransactionStats.UndoBytes + operationStats.UndoBytes;
            long operationDataBytes = metaDataBytes + redoBytes + undoBytes;

            long operationDataOverheadBytes = beginTransactionStats.RecordOverheadBytes + operationStats.RecordOverheadBytes;

            long stateManagerMetaDataBytes = beginTransactionStats.ApproximateStateManagerMetaDataBytes + operationStats.ApproximateStateManagerMetaDataBytes;

            long stateProviderMetaDataBytes = beginTransactionStats.StateProviderMetaDataBytes + operationStats.StateProviderMetaDataBytes;
            long stateProviderRedoBytes = beginTransactionStats.StateProviderRedoBytes + operationStats.StateProviderRedoBytes;
            long stateProviderUndoBytes = beginTransactionStats.StateProviderUndoBytes + operationStats.StateProviderUndoBytes;
            long stateProviderOperationDataBytes = stateProviderMetaDataBytes + stateProviderRedoBytes + stateProviderUndoBytes;

            long userMetaDataBytes = beginTransactionStats.UserMetaDataBytes + operationStats.UserMetaDataBytes;
            long userRedoBytes = beginTransactionStats.UserRedoBytes + operationStats.UserRedoBytes;
            long userUndoBytes = beginTransactionStats.UserUndoBytes + operationStats.UserUndoBytes;
            long userOperationDataBytes = userMetaDataBytes + userRedoBytes + userUndoBytes;

            Console.WriteLine("-------------------------------------");
            Console.WriteLine("Total Operation bytes:\t\t\t{0}", StatUtility.GetBytesString(operationBytes));
            Console.WriteLine("Total Operation Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(operationBytes, this.TotalRecordBytes));
            Console.WriteLine("- Metadata/Redo/Undo overhead bytes:\t{0}", StatUtility.GetBytesString(operationDataOverheadBytes));
            Console.WriteLine("- Metadata/Redo/Undo overhead Percent:\t{0}% of total bytes", StatUtility.GetPercent(operationDataOverheadBytes, this.TotalRecordBytes));
            Console.WriteLine("- Metadata/Redo/Undo bytes:\t\t{0}", StatUtility.GetBytesString(operationDataBytes));
            Console.WriteLine("- Metadata/Redo/Undo Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(operationDataBytes, this.TotalRecordBytes));
            Console.WriteLine("-------------------------------------");
            Console.WriteLine("StateManager Metadata bytes:\t\t\t{0}", StatUtility.GetBytesString(stateManagerMetaDataBytes));
            Console.WriteLine("StateManager Metadata Percent:\t\t\t{0}% of total bytes", StatUtility.GetPercent(stateManagerMetaDataBytes, this.TotalRecordBytes));
            Console.WriteLine("StateProvider Metadata/Redo/Undo bytes:\t\t{0}", StatUtility.GetBytesString(stateProviderOperationDataBytes));
            Console.WriteLine("StateProvider Metadata/Redo/Undo Percent:\t{0}% of total bytes", StatUtility.GetPercent(stateProviderOperationDataBytes, this.TotalRecordBytes));
            Console.WriteLine("- User Metadata/Redo/Undo bytes:\t\t{0}", StatUtility.GetBytesString(userOperationDataBytes));
            Console.WriteLine("- User Metadata/Redo/Undo Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(userOperationDataBytes, this.TotalRecordBytes));
        }

        private OperationLogRecordStats GetOperationStats(LogRecordType recordType)
        {
            LogRecordStats stats = null;
            if (this.recordStats.TryGetValue(recordType, out stats))
            {
                if (stats is OperationLogRecordStats)
                {
                    return stats as OperationLogRecordStats;
                }

                throw new ArgumentException("Record type is not an 'operation' log record.", "recordType");
            }

            // Return an empty stats to make the math easier.
            return new OperationLogRecordStats(recordType);
        }
    }
}