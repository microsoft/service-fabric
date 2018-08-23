// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// An asynchronous enumerator for reading log records from a stream.
    /// </summary>
    internal sealed class LogRecords : IAsyncEnumerator<LogRecord>
    {
        // When the reader is moving forward, it is guarenteed that it cannot go back.
        // To enable faster possible truncations (which is trigerred by the log reader range changes) keep updating the starting position in the log manager after the below number of bytes are read)
        private const ulong UpdateStartingPositionAfterBytes = 1024*1024;

        private readonly ulong enumerationEndingPosition;

        private readonly LogManager logManager;

        private readonly string readerName;

        private readonly LogManager.LogReaderType readerType;

        private readonly Stream readStream;

        private LogRecord currentRecord;

        private long enumerationStartedAtLsn;

        private ulong enumerationStartingPosition;

        private bool isDisposed;

        private PhysicalLogRecord lastPhysicalRecord;

        /// <summary>
        /// Asynchronous enumerator for log records.
        /// </summary>
        /// <param name="logManager">The source log manager.</param>
        /// <param name="enumerationStartingPosition">Inclusive starting position.</param>
        /// <param name="enumerationEndingPosition">Inclusive ending position: record at this starting position will be read.</param>
        /// <param name="enumerationStartedAtLsn">Lsn of the first record. Used for determining what record is safe: no redo or undo.</param>
        /// <param name="readerName">Name of the reader for tracing.</param>
        /// <param name="readerType">Type of the reader.</param>
        /// <remarks>
        /// enumerationStartingPosition and enumerationEndingPosition is inclusive: the last record at enumerationEndingPosition position will be read.
        /// </remarks>
        internal LogRecords(
            LogManager logManager,
            ulong enumerationStartingPosition,
            ulong enumerationEndingPosition,
            long enumerationStartedAtLsn,
            string readerName,
            LogManager.LogReaderType readerType)
        {
            this.readerName = readerName;
            this.readerType = readerType;
            this.logManager = logManager;
            this.enumerationEndingPosition = enumerationEndingPosition;
            this.enumerationStartedAtLsn = enumerationStartedAtLsn;
            this.logManager.AddLogReader(
                enumerationStartedAtLsn,
                enumerationStartingPosition,
                enumerationEndingPosition,
                readerName,
                readerType);
            this.readStream = logManager.CreateReaderStream();
            this.Init(enumerationStartingPosition);
            this.logManager.SetSequentialAccessReadSize(this.readStream, LogManager.ReadAheadBytesForSequentialStream);
        }

        internal LogRecords(Stream readStream, ulong enumerationStartingPosition, ulong enumerationEndingPosition)
        {
            this.logManager = null;
            this.enumerationEndingPosition = enumerationEndingPosition;
            this.readStream = readStream;
            this.enumerationStartedAtLsn = long.MaxValue;
            this.Init(enumerationStartingPosition);
        }

        ~LogRecords()
        {
            this.Dispose(false);
        }

        public LogRecord Current
        {
            get { return this.currentRecord; }
        }

        internal PhysicalLogRecord LastPhysicalRecord
        {
            get { return this.lastPhysicalRecord; }
        }

        public void Dispose()
        {
            this.Dispose(true);
        }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            if (this.readStream.Position <= ((long) this.enumerationEndingPosition) &&
                this.isDisposed == false)
            {
                var record = await LogRecord.ReadNextRecordAsync(this.readStream).ConfigureAwait(false);
                if (record != null)
                {
                    var physicalRecord = record as PhysicalLogRecord;
                    if (LogRecord.IsInvalidRecord(this.lastPhysicalRecord) == false)
                    {
                        record.PreviousPhysicalRecord = this.lastPhysicalRecord;
                        if (physicalRecord != null)
                        {
                            this.lastPhysicalRecord.NextPhysicalRecord = physicalRecord;
                            this.lastPhysicalRecord = physicalRecord;
                        }
                    }
                    else if (physicalRecord != null)
                    {
                        this.lastPhysicalRecord = physicalRecord;
                    }

                    this.currentRecord = record;

                    // When the starting LSN is  not initialized (recovery reader case), initialize it to the first record's LSN
                    if (this.enumerationStartedAtLsn == long.MaxValue)
                    {
                        this.enumerationStartedAtLsn = this.currentRecord.Lsn.LSN;
                    }

                    // Aggresively trim the starting position for enabling truncations
                    if (this.logManager != null
                        && this.currentRecord.RecordPosition - this.enumerationStartingPosition
                        > UpdateStartingPositionAfterBytes)
                    {
                        // First, add the new range and only then remove the older one and update the starting position for removing it later again
                        var isValid = this.logManager.AddLogReader(
                            this.enumerationStartedAtLsn,
                            this.currentRecord.RecordPosition,
                            this.enumerationEndingPosition,
                            this.readerName,
                            this.readerType);
                        Utility.Assert(isValid, "isValid == true");
                        this.logManager.RemoveLogReader(this.enumerationStartingPosition);
                        this.enumerationStartingPosition = this.currentRecord.RecordPosition;
                    }

                    return true;
                }
            }

            return false;
        }

        public void Reset()
        {
            this.readStream.Position = (long) this.enumerationStartingPosition;
            this.currentRecord = LogRecord.InvalidLogRecord;
            this.lastPhysicalRecord = PhysicalLogRecord.InvalidPhysicalLogRecord;
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing)
            {
                if (this.isDisposed == false)
                {
                    if (this.logManager != null)
                    {
                        if (this.readStream != null)
                        {
                            this.readStream.Dispose();
                        }
                        this.logManager.RemoveLogReader(this.enumerationStartingPosition);
                    }

                    GC.SuppressFinalize(this);
                    this.isDisposed = true;
                }
            }
        }

        private void Init(ulong enumerationStartingPosition)
        {
            Utility.Assert(
                enumerationStartingPosition <= this.enumerationEndingPosition,
                "enumerationStartingPosition <= enumerationEndingPosition");
            this.isDisposed = false;
            this.enumerationStartingPosition = enumerationStartingPosition;
            this.Reset();
        }
    }
}