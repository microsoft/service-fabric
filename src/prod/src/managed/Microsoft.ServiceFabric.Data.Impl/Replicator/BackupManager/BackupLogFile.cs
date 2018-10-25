// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using Data;
    using System;
    using System.Diagnostics;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Interop;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Backup log record.
    /// Contains information on the last completed backup.
    /// </summary>
    internal sealed class BackupLogFile
    {
        /// <summary>
        /// Issue a flush if the buffer is greater than 32 KB.
        /// </summary>
        internal const int MinimumIntermediateFlushSize = InitialSizeOfMemoryStream;

        /// <summary>
        /// Initial size of the buffered memory stream. 32 KB.
        /// </summary>
        private const int InitialSizeOfMemoryStream = 32*1024;

        /// <summary>
        /// Serialization version for the backup log file.
        /// </summary>
        private const int Version = 1;

        /// <summary>
        /// Footer for the BackupLogFile.
        /// </summary>
        private FileFooter footer;

        /// <summary>
        /// Properties for the BackupLogFile.
        /// </summary>
        private BackupLogFileProperties properties;

        /// <summary>
        /// Initializes a new instance of the BackupLogFile class.
        /// </summary>
        /// <param name="fileName">Name of the backup file.</param>
        private BackupLogFile(string fileName)
        {
            // Reserved for BackupLogFile.CreateAsync and BackupLogFile.OpenAsync
            this.FileName = fileName;
            this.WriteTimeInMilliseconds = 0;
            this.Size = 0;
        }

        /// <summary>
        /// Gets the number of records.
        /// </summary>
        public uint Count
        {
            get { return this.properties.Count; }

            private set { this.properties.Count = value; }
        }

        /// <summary>
        /// Gets the name of the backup file.
        /// </summary>
        public string FileName { get; private set; }

        /// <summary>
        /// Gets the epoch of the first record in the backup log.
        /// </summary>
        public Epoch IndexingRecordEpoch
        {
            get { return this.properties.IndexingRecordEpoch; }

            private set { this.properties.IndexingRecordEpoch = value; }
        }

        /// <summary>
        /// Gets the logical sequence number of the first record in the backup log.
        /// </summary>
        public LogicalSequenceNumber IndexingRecordLsn
        {
            get { return this.properties.IndexingRecordLsn; }

            private set { this.properties.IndexingRecordLsn = value; }
        }

        /// <summary>
        /// Gets the Epoch of the last record in the backup log.
        /// </summary>
        public Epoch LastBackedUpEpoch
        {
            get { return this.properties.LastBackedUpEpoch; }

            private set { this.properties.LastBackedUpEpoch = value; }
        }

        /// <summary>
        /// Gets the logical sequence number of the last record in the backup log.
        /// </summary>
        public LogicalSequenceNumber LastBackedUpLsn
        {
            get { return this.properties.LastBackedUpLsn; }

            private set { this.properties.LastBackedUpLsn = value; }
        }

        /// <summary>
        /// Gets the size of the backup log file.
        /// </summary>
        public long Size { get; private set; }

        /// <summary>
        /// Gets the size of the backup log file.
        /// </summary>
        public uint SizeInKB
        {
            get { return (uint) Math.Ceiling(this.Size/(double) 1024); }
        }

        /// <summary>
        /// Gets the number of milliseconds it took to write the file.
        /// </summary>
        public long WriteTimeInMilliseconds { get; private set; }

        /// <summary>
        /// Create a new <see cref="BackupLogFile"/> and write it to the given file.
        /// </summary>
        /// <param name="fileName">Name of the backup file.</param>
        /// <param name="logRecords">The log records.</param>
        /// <param name="lastBackupLogRecord"></param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The new <see cref="BackupLogFile"/>.</returns>
        public static async Task<BackupLogFile> CreateAsync(
            string fileName,
            IAsyncEnumerator<LogRecord> logRecords,
            BackupLogRecord lastBackupLogRecord,
            CancellationToken cancellationToken)
        {
            var stopwatch = new Stopwatch();
            stopwatch.Start();

            var backupLogFile = new BackupLogFile(fileName);
            backupLogFile.properties = new BackupLogFileProperties();

            // Create the file with asynchronous flag and 4096 cache size (C# default).
            using (var filestream = FabricFile.Open(
                    fileName,
                    FileMode.CreateNew,
                    FileAccess.Write,
                    FileShare.Write,
                    4096,
                    FileOptions.Asynchronous))
            {
                Utility.SetIoPriorityHint(filestream.SafeFileHandle, Kernel32Types.PRIORITY_HINT.IoPriorityHintLow);

                var incrementalBackupRecords = logRecords as IncrementalBackupLogRecordAsyncEnumerator;
                if (incrementalBackupRecords == null)
                {
                    await backupLogFile.WriteLogRecordsAsync(filestream, logRecords, cancellationToken).ConfigureAwait(false);
                }
                else
                {
                    await backupLogFile.WriteLogRecordsAsync(filestream, incrementalBackupRecords, cancellationToken).ConfigureAwait(false);
                    await incrementalBackupRecords.VerifyDrainedAsync().ConfigureAwait(false);

                    Utility.Assert(backupLogFile.Count == incrementalBackupRecords.Count, "Unexpected count");

                    backupLogFile.properties.IndexingRecordEpoch = incrementalBackupRecords.StartingEpoch;
                    backupLogFile.properties.IndexingRecordLsn = incrementalBackupRecords.StartingLsn;

                    if (incrementalBackupRecords.HighestBackedupEpoch == LoggingReplicator.InvalidEpoch)
                    {
                        backupLogFile.properties.LastBackedUpEpoch = lastBackupLogRecord.HighestBackedUpEpoch;
                    }
                    else
                    {
                        backupLogFile.properties.LastBackedUpEpoch = incrementalBackupRecords.HighestBackedupEpoch;
                    }

                    backupLogFile.properties.LastBackedUpLsn = incrementalBackupRecords.HighestBackedupLsn;
                }

                // Write the log records.
                backupLogFile.properties.RecordsHandle = new BlockHandle(offset: 0, size: filestream.Position);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the properties.
                var propertiesHandle =
                    await FileBlock.WriteBlockAsync(filestream, writer => backupLogFile.properties.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Write the footer.
                backupLogFile.footer = new FileFooter(propertiesHandle, Version);
                await FileBlock.WriteBlockAsync(filestream, writer => backupLogFile.footer.Write(writer)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Store the size.
                backupLogFile.Size = filestream.Length;
            }

            stopwatch.Stop();
            backupLogFile.WriteTimeInMilliseconds = stopwatch.ElapsedMilliseconds;

            return backupLogFile;
        }

        /// <summary>
        /// Open an existing <see cref="BackupLogFile"/> from the given file path.
        /// </summary>
        /// <param name="fileName">Path to the backup log file.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>The opened <see cref="BackupLogFile"/>.</returns>
        public static async Task<BackupLogFile> OpenAsync(string fileName, CancellationToken cancellationToken)
        {
            if (string.IsNullOrEmpty(fileName))
            {
                throw new ArgumentException(SR.Error_FilePath_Null_NoArgs);
            }

            if (!FabricFile.Exists(fileName))
            {
                throw new ArgumentException(string.Format(CultureInfo.CurrentCulture,SR.Error_FilePath_Null, fileName));
            }

            // Open the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Uses default IOPriorityHint since this operation is called during Restore.
            using (var filestream = FabricFile.Open(
                    fileName,
                    FileMode.Open,
                    FileAccess.Read,
                    FileShare.Read,
                    4096,
                    FileOptions.Asynchronous))
            {
                var backupLogFile = new BackupLogFile(fileName);
                backupLogFile.Size = filestream.Length;

                // Read and validate the Footer section.  The footer is always at the end of the stream, minus space for the checksum.
                var footerHandle = new BlockHandle(
                    filestream.Length - FileFooter.SerializedSize - sizeof(ulong),
                    FileFooter.SerializedSize);
                backupLogFile.footer =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            footerHandle,
                            (reader, handle) => FileFooter.Read(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                // Verify we know how to deserialize this version of the backup log file.
                if (backupLogFile.footer.Version != Version)
                {
                    throw new InvalidDataException(
                        string.Format(
                            CultureInfo.CurrentCulture,
                            SR.Error_BackupLogFile_Deserialized,
                            backupLogFile.footer.Version,
                            Version));
                }

                // Read and validate the properties section.
                var propertiesHandle = backupLogFile.footer.PropertiesHandle;
                backupLogFile.properties =
                    await
                        FileBlock.ReadBlockAsync(
                            filestream,
                            propertiesHandle,
                            (reader, handle) => FileProperties.Read<BackupLogFileProperties>(reader, handle)).ConfigureAwait(false);

                cancellationToken.ThrowIfCancellationRequested();

                return backupLogFile;
            }
        }

        /// <summary>
        /// Gets an async backup log reader.
        /// </summary>
        /// <returns>Asynchronous log record enumerator.</returns>
        public IAsyncEnumerator<LogRecord> GetAsyncEnumerator()
        {
            return new BackupLogFileAsyncEnumerator(this, this.properties.RecordsHandle);
        }

        /// <summary>
        /// Checks the data integrity.
        /// </summary>
        /// <returns>Task that represents the asynchronous operation.</returns>
        public async Task VerifyAsync()
        {
            using (var enumerator = this.GetAsyncEnumerator())
            {
                while (await enumerator.MoveNextAsync(CancellationToken.None).ConfigureAwait(false))
                {
                    // Enumerator the log to check data integrity.
                }
            }
        }

        /// <summary>
        /// Write a block of log records to the output stream, including the size of the block and a checksum.
        /// </summary>
        /// <param name="outputStream">Backup log stream.</param>
        /// <param name="blockStream">Stream for the block of log records.</param>
        /// <param name="blockWriter">Writer for the block of log records.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task WriteLogRecordBlockAsync(
            Stream outputStream,
            MemoryStream blockStream,
            BinaryWriter blockWriter,
            CancellationToken cancellationToken)
        {
            // Get the size of the block.
            var blockSize = checked((int) blockStream.Position);
            Utility.Assert(blockSize > sizeof(int), "BackupLogFile tried to write a zero-sized block of log records.");

            // Write the block size at the start of the block.
            blockStream.Position = 0;
            blockWriter.Write(blockSize);

            // Checksum this block of data (with the block size included).
            var blockChecksum = CRC64.ToCRC64(blockStream.GetBuffer(), 0, blockSize);
            // Add the checksum at the end of the memory stream.
            blockStream.Position = blockSize;
            blockWriter.Write(blockChecksum);

            // Flush the records block to the output stream.
            await outputStream.WriteAsync(
                    blockStream.GetBuffer(),
                    0,
                    checked((int) blockStream.Position),
                    cancellationToken).ConfigureAwait(false);
            // Reset back to the start (leaving space for the 'int' block size).
            blockStream.Position = sizeof(int);
        }

        /// <summary>
        /// Write the backup log file.
        /// </summary>
        /// <param name="stream">Backup log stream.</param>
        /// <param name="logRecords">The log records.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task WriteLogRecordsAsync(
            Stream stream,
            IAsyncEnumerator<LogRecord> logRecords,
            CancellationToken cancellationToken)
        {
            var firstIndexingRecordProcessed = false;

            using (var memoryStream = new MemoryStream(InitialSizeOfMemoryStream))
            using (var binaryWriter = new BinaryWriter(memoryStream))
            using (var recordWriter = new BinaryWriter(new MemoryStream()))
            {
                // Reserve an 'int' at the start to store the size of the block
                memoryStream.Position += sizeof(int);

                while (true == await logRecords.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                {
                    var record = logRecords.Current;

                    var indexingLogRecord = record as IndexingLogRecord;
                    if (indexingLogRecord != null)
                    {
                        if (false == firstIndexingRecordProcessed)
                        {
                            this.IndexingRecordEpoch = indexingLogRecord.CurrentEpoch;
                            this.IndexingRecordLsn = indexingLogRecord.Lsn;

                            firstIndexingRecordProcessed = true;
                        }

                        this.LastBackedUpEpoch = indexingLogRecord.CurrentEpoch;
                    }

                    var updateEpochLogRecord = record as UpdateEpochLogRecord;
                    if (updateEpochLogRecord != null)
                    {
                        this.LastBackedUpEpoch = updateEpochLogRecord.Epoch;
                    }

                    this.LastBackedUpLsn = record.Lsn;

                    var operationData = LogRecord.WriteRecord(record, recordWriter, isPhysicalWrite: false, setRecordLength: false);

                    // TODO: Fix this by using operationdata in this class
                    foreach (var buffer in operationData)
                    {
                        binaryWriter.Write(buffer.Array, buffer.Offset, buffer.Count);
                    }

                    this.Count++;

                    // Flush the block after it's large enough.
                    if (memoryStream.Position >= MinimumIntermediateFlushSize)
                    {
                        await this.WriteLogRecordBlockAsync(stream, memoryStream, binaryWriter, cancellationToken).ConfigureAwait(false);
                    }

                    // Check for cancellation.
                    cancellationToken.ThrowIfCancellationRequested();
                }

                // Flush the block, if there's any remaining data (ignoring the block size 'int' at the start).
                if (memoryStream.Position > sizeof(int))
                {
                    await this.WriteLogRecordBlockAsync(stream, memoryStream, binaryWriter, cancellationToken).ConfigureAwait(false);
                }

                await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
            }

            // If no logical record is being backed up, lastBackedupLsn can be Invalid.
            if (this.LastBackedUpLsn == LogicalSequenceNumber.InvalidLsn)
            {
                this.LastBackedUpLsn = this.IndexingRecordLsn;
            }

            Utility.Assert(true == firstIndexingRecordProcessed, "Indexing log record must have been processed.");

            Utility.Assert(
                this.IndexingRecordEpoch.CompareTo(LoggingReplicator.InvalidEpoch) > 0,
                "Indexing record epoch has not been set.");
            Utility.Assert(
                this.LastBackedUpEpoch.CompareTo(LoggingReplicator.InvalidEpoch) > 0,
                "Ending epoch has not been set.");

            Utility.Assert(
                this.IndexingRecordLsn != LogicalSequenceNumber.InvalidLsn,
                "Indexing record lsn has not been set.");
            Utility.Assert(this.LastBackedUpLsn != LogicalSequenceNumber.InvalidLsn, "Ending lsn has not been set.");
        }

        /// <summary>
        /// Write the backup log file.
        /// </summary>
        /// <param name="stream">Backup log stream.</param>
        /// <param name="logRecords">The log records.</param>
        /// <param name="cancellationToken">Token used to signal cancellation.</param>
        /// <returns>Task that represents the asynchronous operation.</returns>
        private async Task WriteLogRecordsAsync(
            Stream stream,
            IncrementalBackupLogRecordAsyncEnumerator logRecords,
            CancellationToken cancellationToken)
        {
            using (var memoryStream = new MemoryStream(InitialSizeOfMemoryStream))
            using (var binaryWriter = new BinaryWriter(memoryStream))
            using (var recordWriter = new BinaryWriter(new MemoryStream()))
            {
                // Reserve an 'int' at the start to store the size of the block
                memoryStream.Position += sizeof(int);

                while (true == await logRecords.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                {
                    var record = logRecords.Current;

                    var operationData = LogRecord.WriteRecord(record, recordWriter, isPhysicalWrite: false, setRecordLength: false);

                    // TODO: Fix this by using operationdata in this class
                    foreach (var buffer in operationData)
                    {
                        binaryWriter.Write(buffer.Array, buffer.Offset, buffer.Count);
                    }

                    this.Count++;

                    // Flush the block after it's large enough.
                    if (memoryStream.Position >= MinimumIntermediateFlushSize)
                    {
                        await this.WriteLogRecordBlockAsync(stream, memoryStream, binaryWriter, cancellationToken).ConfigureAwait(false);
                    }

                    // Check for cancellation.
                    cancellationToken.ThrowIfCancellationRequested();
                }

                // Flush the block, if there's any remaining data (ignoring the block size 'int' at the start).
                if (memoryStream.Position > sizeof(int))
                {
                    await this.WriteLogRecordBlockAsync(stream, memoryStream, binaryWriter, cancellationToken).ConfigureAwait(false);
                }

                await stream.FlushAsync(cancellationToken).ConfigureAwait(false);
            }
        }
    }
}