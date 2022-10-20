// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FileLogUtility
{
    using System;
    using System.Collections.Generic;
    using System.Fabric;
    using System.IO;
    using Microsoft.ServiceFabric.Replicator;

    static class LogRecordStatsUtil
    {
        public static LogRecordStats GetStatsForRecordType(LogRecordType recordType)
        {
            switch (recordType)
            {
                case LogRecordType.BeginTransaction:
                    return new OperationLogRecordStats(recordType);

                case LogRecordType.Operation:
                    return new OperationLogRecordStats(recordType);
            }

            return new LogRecordStats(recordType);
        }
    }

    class LogRecordStats
    {
        public LogRecordType RecordType { get; private set; }
        public int Count { get; private set; }
        public long TotalBytes { get; private set; }
        public double AverageBytes { get { return StatUtility.GetAverage(this.TotalBytes, this.Count); } }

        public LogRecordStats(LogRecordType recordType)
        {
            this.RecordType = recordType;
        }

        public virtual void ProcessRecord(LogRecord record)
        {
            this.Count++;
            this.TotalBytes += record.RecordSize;
        }

        public virtual void DisplayStatistics(long totalRecordCount, long totalRecordBytes)
        {
            Console.WriteLine();
            Console.WriteLine(this.RecordType);
            Console.WriteLine("\t Count:\t\t\t{0}", this.Count);
            Console.WriteLine("\t Count Percent:\t\t{0}% of total count", StatUtility.GetPercent(this.Count, totalRecordCount));
            Console.WriteLine("\t Bytes:\t\t\t{0}", StatUtility.GetBytesString(this.TotalBytes));
            Console.WriteLine("\t Bytes Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(this.TotalBytes, totalRecordBytes));
            Console.WriteLine("\t Average Bytes:\t\t{0}", StatUtility.GetBytesString(this.AverageBytes));
        }
    }

    class OperationLogRecordStats : LogRecordStats
    {
        // Overhead due to StateManager.
        public const long StateManagerMetaDataOverheadBytes = sizeof(int) + sizeof(long) + sizeof(bool); // DynamicStateManager.cs AddIdToReplicationData

        // Assume the state provider is TStore.
        public const long StateProviderMetaDataOverheadBytes = sizeof(int) + sizeof(long) + sizeof(byte) + sizeof(bool); // TStore: MetadataOperationData.cs
        public const long StateProviderRedoOverheadBytes = sizeof(int) + sizeof(int); // TStore: RedoUndoOperationData.cs
        public const long StateProviderUndoOverheadBytes = sizeof(int) + sizeof(int); // TStore: RedoUndoOperationData.cs

        // MetaData operation data information.
        public long MetaDataCount { get; private set; }
        public long MetaDataSegments { get; private set; }
        public long MetaDataBytes { get; private set; }

        // Redo operation data information.
        public long RedoCount { get; private set; }
        public long RedoSegments { get; private set; }
        public long RedoBytes { get; private set; }

        // Undo operation data information.
        public long UndoCount { get; private set; }
        public long UndoSegments { get; private set; }
        public long UndoBytes { get; private set; }

        // All data that is not due to MetaData, Redo, or Undo data is overhead attributed to the log.
        public long RecordOverheadBytes { get { return this.TotalBytes - (this.MetaDataBytes + this.RedoBytes + this.UndoBytes); } }

        // StateManager bytes are approximate, since we can't distinguish which log records are for state providers vs state manager itself.  Assume all log records are for state providers.
        public long ApproximateStateManagerMetaDataBytes { get; private set; }

        // Bytes inferred to be attributable to the state provider's operation data.
        public long StateProviderMetaDataBytes { get { return this.MetaDataBytes - this.ApproximateStateManagerMetaDataBytes; } }
        public long StateProviderRedoBytes { get { return this.RedoBytes; } }
        public long StateProviderUndoBytes { get { return this.UndoBytes; } }

        // Bytes inferred to be attributable to user's serialized data.
        public long UserMetaDataBytes { get { return this.StateProviderMetaDataBytes - (StateProviderMetaDataOverheadBytes * this.MetaDataCount); } }
        public long UserRedoBytes { get { return this.StateProviderRedoBytes - (StateProviderRedoOverheadBytes * this.RedoCount); } }
        public long UserUndoBytes { get { return this.StateProviderUndoBytes - (StateProviderUndoOverheadBytes * this.UndoCount); } }

        public OperationLogRecordStats(LogRecordType recordType) : base(recordType)
        {
        }

        public override void ProcessRecord(LogRecord record)
        {
            base.ProcessRecord(record);

            if (record.RecordType == LogRecordType.BeginTransaction)
            {
                var op = record as BeginTransactionOperationLogRecord;
                this.ProcessMetaData(op.MetaData);
                this.ProcessRedoData(op.Redo);
                this.ProcessUndoData(op.Undo);
            }
            else if (record.RecordType == LogRecordType.Operation)
            {
                var op = record as OperationLogRecord;
                this.ProcessMetaData(op.MetaData);
                this.ProcessRedoData(op.Redo);
                this.ProcessUndoData(op.Undo);
            }
        }

        public override void DisplayStatistics(long totalRecordCount, long totalRecordBytes)
        {
            base.DisplayStatistics(totalRecordCount, totalRecordBytes);

            Console.WriteLine("\t -------------------------------------");
            Console.WriteLine("\t Metadata segments:\t{0}\t(avg: {1})", this.MetaDataSegments, StatUtility.GetAverage(this.MetaDataSegments, this.Count));
            Console.WriteLine("\t Redo data segments:\t{0}\t(avg: {1})", this.RedoSegments, StatUtility.GetAverage(this.RedoSegments, this.Count));
            Console.WriteLine("\t Undo data segments:\t{0}\t(avg: {1})", this.UndoSegments, StatUtility.GetAverage(this.UndoSegments, this.Count));
            Console.WriteLine("\t Metadata bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.MetaDataBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.MetaDataBytes, this.Count)));
            Console.WriteLine("\t Metadata Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.MetaDataBytes, this.TotalBytes));
            Console.WriteLine("\t Metadata Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.MetaDataBytes, totalRecordBytes));
            Console.WriteLine("\t Redo data bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.RedoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.RedoBytes, this.Count)));
            Console.WriteLine("\t Redo data Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.RedoBytes, this.TotalBytes));
            Console.WriteLine("\t Redo data Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.RedoBytes, totalRecordBytes));
            Console.WriteLine("\t Undo data bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.UndoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.UndoBytes, this.Count)));
            Console.WriteLine("\t Undo data Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.UndoBytes, this.TotalBytes));
            Console.WriteLine("\t Undo data Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.UndoBytes, totalRecordBytes));
            Console.WriteLine("\t -------------------------------------");
            Console.WriteLine("\t Record overhead bytes:\t\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.RecordOverheadBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.RecordOverheadBytes, this.Count)));
            Console.WriteLine("\t Record overhead Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.RecordOverheadBytes, this.TotalBytes));
            Console.WriteLine("\t Record overhead Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.RecordOverheadBytes, totalRecordBytes));
            Console.WriteLine("\t -------------------------------------");
            Console.WriteLine("\t StateManager Metadata bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.ApproximateStateManagerMetaDataBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.ApproximateStateManagerMetaDataBytes, this.Count)));
            Console.WriteLine("\t StateManager Metadata Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.ApproximateStateManagerMetaDataBytes, this.TotalBytes));
            Console.WriteLine("\t StateManager Metadata Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.ApproximateStateManagerMetaDataBytes, totalRecordBytes));
            Console.WriteLine("\t -------------------------------------");
            Console.WriteLine("\t StateProvider Metadata bytes:\t\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.StateProviderMetaDataBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.StateProviderMetaDataBytes, this.Count)));
            Console.WriteLine("\t StateProvider Metadata Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.StateProviderMetaDataBytes, this.TotalBytes));
            Console.WriteLine("\t StateProvider Metadata Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.StateProviderMetaDataBytes, totalRecordBytes));
            Console.WriteLine("\t StateProvider Redo bytes:\t\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.StateProviderRedoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.StateProviderRedoBytes, this.Count)));
            Console.WriteLine("\t StateProvider Redo Percent:\t\t{0}% of record bytes", StatUtility.GetPercent(this.StateProviderRedoBytes, this.TotalBytes));
            Console.WriteLine("\t StateProvider Redo Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(this.StateProviderRedoBytes, totalRecordBytes));
            Console.WriteLine("\t StateProvider Undo bytes:\t\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.StateProviderUndoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.StateProviderUndoBytes, this.Count)));
            Console.WriteLine("\t StateProvider Undo Percent:\t\t{0}% of record bytes", StatUtility.GetPercent(this.StateProviderUndoBytes, this.TotalBytes));
            Console.WriteLine("\t StateProvider Undo Percent:\t\t{0}% of total bytes", StatUtility.GetPercent(this.StateProviderUndoBytes, totalRecordBytes));
            Console.WriteLine("\t -------------------------------------");
            Console.WriteLine("\t User Metadata bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.UserMetaDataBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.UserMetaDataBytes, this.Count)));
            Console.WriteLine("\t User Metadata Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.UserMetaDataBytes, this.TotalBytes));
            Console.WriteLine("\t User Metadata Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.UserMetaDataBytes, totalRecordBytes));
            Console.WriteLine("\t User Redo bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.UserRedoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.UserRedoBytes, this.Count)));
            Console.WriteLine("\t User Redo Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.UserRedoBytes, this.TotalBytes));
            Console.WriteLine("\t User Redo Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.UserRedoBytes, totalRecordBytes));
            Console.WriteLine("\t User Undo bytes:\t{0}\t(avg: {1})", StatUtility.GetBytesString(this.UserUndoBytes), StatUtility.GetBytesString(StatUtility.GetAverage(this.UserUndoBytes, this.Count)));
            Console.WriteLine("\t User Undo Percent:\t{0}% of record bytes", StatUtility.GetPercent(this.UserUndoBytes, this.TotalBytes));
            Console.WriteLine("\t User Undo Percent:\t{0}% of total bytes", StatUtility.GetPercent(this.UserUndoBytes, totalRecordBytes));
        }

        private void ProcessMetaData(OperationData metaData)
        {
            if (metaData != null)
            {
                this.MetaDataCount++;
                this.MetaDataSegments += metaData.Count;
                foreach (var meta in metaData)
                {
                    this.MetaDataBytes += meta.Count;
                }

                // StateManager always puts its operation data at the end.  Assume this is a state provider operation data.
                this.ApproximateStateManagerMetaDataBytes += metaData[metaData.Count - 1].Count;
            }
        }

        private void ProcessRedoData(OperationData redoData)
        {
            if (redoData != null)
            {
                this.RedoCount++;
                this.RedoSegments += redoData.Count;
                foreach (var redo in redoData)
                {
                    this.RedoBytes += redo.Count;
                }
            }
        }

        private void ProcessUndoData(OperationData undoData)
        {
            if (undoData != null)
            {
                this.UndoCount++;
                this.UndoSegments += undoData.Count;
                foreach (var undo in undoData)
                {
                    this.UndoBytes += undo.Count;
                }
            }
        }

    }
}