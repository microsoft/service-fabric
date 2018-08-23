// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.IO;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    internal sealed class PhysicalLogReader : IPhysicalLogReader
    {
        private readonly ulong endingRecordPosition;

        private readonly bool isValid;

        private readonly LogManager logManager;

        private readonly Stream readStream;

        private LogRecord endRecord;

        private bool isDisposed;

        private long startingLsn;

        private LogRecord startingRecord;

        private ulong startingRecordPosition;

        /// <summary>
        /// Initializes a new instance of the PhysicalLogReader class.
        /// </summary>
        /// <param name="logManager">The log manager.</param>
        /// <param name="startingRecordPosition">Stream position to start reading.</param>
        /// <param name="endingRecordPosition">Stream position of the start of the last record that will be read.</param>
        /// <param name="startingLsn">Logical sequence number of the first record.</param>
        /// <param name="readerName">Name of the reader that will be used in tracing.</param>
        /// <param name="readerType">Type of the reader.</param>
        /// <remarks>
        /// Note that ending record position is INCLUSIVE: the last record will be read.
        /// Starting logical sequence number is used to decide the when an lsn is safe (will not be undone or redone).
        /// </remarks>
        public PhysicalLogReader(
            LogManager logManager,
            ulong startingRecordPosition,
            ulong endingRecordPosition,
            long startingLsn,
            string readerName,
            LogManager.LogReaderType readerType)
        {
            this.logManager = logManager;
            this.readStream = null;
            this.startingRecordPosition = startingRecordPosition;
            this.startingLsn = startingLsn;
            this.endingRecordPosition = endingRecordPosition;
            Utility.Assert(
                startingRecordPosition <= endingRecordPosition,
                "startingRecordPosition <= endingRecordPosition");
            this.isValid = logManager.AddLogReader(
                startingLsn,
                startingRecordPosition,
                endingRecordPosition,
                readerName,
                readerType);
            this.isDisposed = false;

            this.startingRecord = LogRecord.InvalidLogRecord;
            this.endRecord = LogRecord.InvalidLogRecord;
        }

        public PhysicalLogReader(LogManager logManager)
        {
            this.logManager = logManager;
            this.readStream = this.logManager.CreateReaderStream();
            this.startingRecordPosition = 0;
            this.startingLsn = 0;
            this.endingRecordPosition = long.MaxValue;
            this.isDisposed = false;
            this.isValid = false;
            this.startingRecord = LogRecord.InvalidLogRecord;
            this.endRecord = LogRecord.InvalidLogRecord;
        }

        ~PhysicalLogReader()
        {
            this.Dispose(false);
        }

        public ulong EndingRecordPosition
        {
            get { return this.endingRecordPosition; }
        }

        public bool IsValid
        {
            get { return this.isValid; }
        }

        public ulong StartingRecordPosition
        {
            get { return this.startingRecordPosition; }
        }

        internal Stream ReadStream
        {
            get { return this.readStream; }
        }

        internal LogRecord StartingRecord
        {
            get
            {
                Utility.Assert(
                    LogRecord.IsInvalidRecord(this.startingRecord) == false,
                    "LogRecord.IsInvalidRecord(this.startingRecord) == false");
                return this.startingRecord;
            }

            set
            {
                Utility.Assert(
                    LogRecord.IsInvalidRecord(this.startingRecord),
                    "LogRecord.IsInvalidRecord(this.startingRecord) == true");
                this.startingRecord = value;
            }
        }

        public void Dispose()
        {
            this.Dispose(true);
        }

        public IAsyncEnumerator<LogRecord> GetLogRecords(string readerName, LogManager.LogReaderType readerType)
        {
            var logRecords = new LogRecords(
                this.logManager,
                this.startingRecordPosition,
                this.endingRecordPosition,
                this.startingLsn,
                readerName,
                readerType);

            return logRecords;
        }

        internal async Task<BeginCheckpointLogRecord> GetLastCompletedBeginCheckpointRecord(
            EndCheckpointLogRecord record)
        {
            var lastCompletedBeginCheckpointRecord = record.LastCompletedBeginCheckpointRecord;
            if (!LogRecord.IsInvalidRecord(lastCompletedBeginCheckpointRecord))
            {
                return lastCompletedBeginCheckpointRecord;
            }

            var lastCompletedBeginCheckpointRecordOffset = record.LastCompletedBeginCheckpointRecordOffset;
            Utility.Assert(
                lastCompletedBeginCheckpointRecordOffset > 0,
                "lastCompletedBeginCheckpointRecordOffset {0} > 0",
                lastCompletedBeginCheckpointRecordOffset);

            // Read desired checkpoint record
            var recordPosition = record.RecordPosition;
            Utility.Assert(
                recordPosition != LogRecord.InvalidRecordPosition,
                "recordPosition ({0}) != LogRecord.INVALID_RECORD_POSITION",
                recordPosition);

            var lastCompletedBeginCheckpointRecordPosition = recordPosition - lastCompletedBeginCheckpointRecordOffset;

            Utility.Assert(
                lastCompletedBeginCheckpointRecordPosition >= this.startingRecordPosition,
                "lastCompletedBeginCheckpointRecordPosition ({0}) >= this.startingRecordPosition ({1})",
                lastCompletedBeginCheckpointRecordPosition, this.startingRecordPosition);

            lastCompletedBeginCheckpointRecord =
                (BeginCheckpointLogRecord) await this.GetNextLogRecord(lastCompletedBeginCheckpointRecordPosition).ConfigureAwait(false);
            record.LastCompletedBeginCheckpointRecord = lastCompletedBeginCheckpointRecord;

            return lastCompletedBeginCheckpointRecord;
        }

        internal async Task<PhysicalLogRecord> GetLinkedPhysicalRecord(PhysicalLogRecord record)
        {
            var linkedPhysicalRecord = record.LinkedPhysicalRecord;
            if (LogRecord.IsInvalidRecord(linkedPhysicalRecord))
            {
                var linkedPhysicalRecordOffset = record.LinkedPhysicalRecordOffset;
                Utility.Assert(
                    linkedPhysicalRecordOffset != LogRecord.InvalidPhysicalRecordOffset,
                    "linkedPhysicalRecordOffset != PhysicalLogRecord.INVALID_PHYSICAL_RECORD_OFFSET");
                if (linkedPhysicalRecordOffset == 0)
                {
                    record.LinkedPhysicalRecord = null;
                    return null;
                }

                // Read desired linked record
                var recordPosition = record.RecordPosition;
                Utility.Assert(
                    recordPosition != LogRecord.InvalidRecordPosition,
                    "recordPosition != LogRecord.INVALID_RECORD_POSITION");
                var linkedPhysicalRecordPosition = recordPosition - linkedPhysicalRecordOffset;
                Utility.Assert(
                    linkedPhysicalRecordPosition >= this.startingRecordPosition,
                    "linkedPhysicalRecordPosition >= this.startingRecordPosition");

                linkedPhysicalRecord = (PhysicalLogRecord) await this.GetNextLogRecord(linkedPhysicalRecordPosition).ConfigureAwait(false);
                record.LinkedPhysicalRecord = linkedPhysicalRecord;
            }

            Utility.Assert(
                (linkedPhysicalRecord == null) == (record.LinkedPhysicalRecordOffset == 0),
                "(linkedPhysicalRecord == null) == (record.LinkedPhysicalRecordOffset == 0)");
            return linkedPhysicalRecord;
        }

        internal async Task<LogRecord> GetNextLogRecord(ulong recordPosition)
        {
            this.readStream.Position = (long) recordPosition;
            return await LogRecord.ReadNextRecordAsync(this.readStream).ConfigureAwait(false);
        }

        internal async Task<LogRecord> GetPreviousLogRecord(ulong recordPosition)
        {
            this.readStream.Position = (long) recordPosition;
            return await LogRecord.ReadPreviousRecordAsync(this.readStream).ConfigureAwait(false);
        }

        internal async Task<PhysicalLogRecord> GetPreviousPhysicalRecord(LogRecord record)
        {
            var previousPhysicalRecord = record.PreviousPhysicalRecord;
            if (LogRecord.IsInvalidRecord(previousPhysicalRecord))
            {
                var previousPhysicalRecordOffset = record.PreviousPhysicalRecordOffset;
                Utility.Assert(
                    previousPhysicalRecordOffset != LogRecord.InvalidPhysicalRecordOffset,
                    "previousPhysicalRecordOffset != LogRecord.INVALID_PHYSICAL_RECORD_OFFSET");
                if (previousPhysicalRecordOffset == 0)
                {
                    record.PreviousPhysicalRecord = null;
                    return null;
                }

                // Read previous physical record
                var recordPosition = record.RecordPosition;
                Utility.Assert(
                    recordPosition != LogRecord.InvalidRecordPosition,
                    "recordPosition != LogRecord.INVALID_RECORD_POSITION");
                var previousPhysicalRecordPosition = recordPosition - previousPhysicalRecordOffset;
                Utility.Assert(
                    previousPhysicalRecordPosition >= this.startingRecordPosition,
                    "previousPhysicalRecordPosition >= this.startingRecordPosition");

                previousPhysicalRecord = (PhysicalLogRecord) await this.GetNextLogRecord(previousPhysicalRecordPosition).ConfigureAwait(false);
                var nextPhysicalLogRecord = record as PhysicalLogRecord;
                if (nextPhysicalLogRecord != null)
                {
                    previousPhysicalRecord.NextPhysicalRecord = nextPhysicalLogRecord;
                }

                record.PreviousPhysicalRecord = previousPhysicalRecord;
                if (previousPhysicalRecordPosition == this.startingRecordPosition)
                {
                    this.startingRecord = previousPhysicalRecord;
                }
            }

            Utility.Assert(
                (previousPhysicalRecord == null) == (record.PreviousPhysicalRecordOffset == 0),
                "(previousPhysicalRecord == null) == (record.PreviousPhysicalRecordOffset == 0)");
            return previousPhysicalRecord;
        }

        internal async Task IndexPhysicalRecords(PhysicalLogRecord record)
        {
            while (record.RecordPosition > this.startingRecordPosition)
            {
                record = await this.GetPreviousPhysicalRecord(record).ConfigureAwait(false);
            }

            Utility.Assert(
                LogRecord.IsInvalidRecord(this.startingRecord) == false,
                "LogRecord.IsInvalidRecord(this.startingRecord) == false");
        }

        internal void MoveStartingRecordPosition(
            long startingLsn,
            ulong newStartingRecordPosition,
            string readerName,
            LogManager.LogReaderType readerType)
        {
            if (this.isValid)
            {
                var isValid = this.logManager.AddLogReader(
                    startingLsn,
                    newStartingRecordPosition,
                    this.endingRecordPosition,
                    readerName,
                    readerType);
                Utility.Assert(isValid, "isValid == true");
                this.logManager.RemoveLogReader(this.startingRecordPosition);
            }

            this.startingRecordPosition = newStartingRecordPosition;
            this.startingLsn = startingLsn;
            this.startingRecord = LogRecord.InvalidLogRecord;
        }

        internal async Task<LogRecord> SeekToLastRecord()
        {
            Utility.Assert(this.endingRecordPosition == long.MaxValue, "this.endingRecordPosition == Int64.MaxValue");
            Utility.Assert(
                LogRecord.IsInvalidRecord(this.endRecord),
                "LogRecord.IsInvalidRecord(this.endRecord) == true. this.endRecord: {0}",
                this.endRecord);

            this.readStream.Position = this.readStream.Length;
            this.endRecord = await LogRecord.ReadPreviousRecordAsync(this.readStream).ConfigureAwait(false);

            return this.endRecord;
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing)
            {
                if (this.isDisposed == false)
                {
                    if (this.isValid && this.logManager != null)
                    {
                        this.logManager.RemoveLogReader(this.startingRecordPosition);
                    }

                    if (this.readStream != null)
                    {
                        this.readStream.Dispose();
                    }

                    this.isDisposed = true;
                    GC.SuppressFinalize(this);
                }
            }
        }
    }
}