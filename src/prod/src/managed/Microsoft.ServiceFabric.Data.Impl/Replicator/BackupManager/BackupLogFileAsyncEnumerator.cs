// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Fabric.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Backup log record.
    /// Contains information on the last completed backup.
    /// </summary>
    internal sealed class BackupLogFileAsyncEnumerator : IAsyncEnumerator<LogRecord>
    {
        /// <summary>
        /// Name of the backup log file.
        /// </summary>
        private BackupLogFile backupLogFile;

        /// <summary>
        /// Backup log stream.
        /// </summary>
        private Stream backupLogStream;

        /// <summary>
        /// Current index. Number of records read.
        /// </summary>
        private uint index;

        /// <summary>
        /// Backup log stream reader.
        /// </summary>
        private LogRecordBlockAsyncEnumerator logBlockEnumerator;

        /// <summary>
        /// Initializes a new instance of the <see cref="BackupLogFileAsyncEnumerator"/> class.
        /// </summary>
        /// <param name="backupLogFile">Backup log file.</param>
        /// <param name="logRecordHandle">Start and end offset for the log records.</param>
        public BackupLogFileAsyncEnumerator(BackupLogFile backupLogFile, BlockHandle logRecordHandle)
        {
            Utility.Assert(backupLogFile != null, "backupLogFile != null");

            this.backupLogFile = backupLogFile;

            // Open the file with asynchronous flag and 4096 cache size (C# default).
            // MCoskun: Uses default IOPriorityHint since this operation is called during Restore.
            this.backupLogStream = FabricFile.Open(
                this.backupLogFile.FileName,
                FileMode.Open,
                FileAccess.Read,
                FileShare.Read,
                4096,
                FileOptions.Asynchronous);

            this.logBlockEnumerator = new LogRecordBlockAsyncEnumerator(this.backupLogStream, logRecordHandle);

            this.index = 0;
        }

        /// <summary>
        /// Gets the current log record.
        /// </summary>
        public LogRecord Current { get; private set; }

        /// <summary>
        /// Disposes the stream.
        /// </summary>
        public void Dispose()
        {
            if (this.logBlockEnumerator != null)
            {
                this.logBlockEnumerator.Dispose();
            }
        }

        /// <summary>
        /// Move to the next log record.
        /// </summary>
        /// <returns>Boolean indicating whether the move was successful.</returns>
        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            // Check if the current block of log records is loaded and valid.
            var currentBlockStream = this.logBlockEnumerator.Current;
            if (currentBlockStream == null || currentBlockStream.Position >= currentBlockStream.Length)
            {
                // Check if there are more blocks of log records (this will read and validate the checksums).
                if (false == await this.logBlockEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false))
                {
                    // Finished enumerating.  Validate we read the expected number of log records.
                    Utility.Assert(
                        this.index == this.backupLogFile.Count,
                        "BackupLogFileAsyncEnumerator read {0} items but BackupLogFile had {1} items.",
                         this.index, this.backupLogFile.Count);

                    return false;
                }

                // Get the next block of log records.
                currentBlockStream = this.logBlockEnumerator.Current;
            }

            // Read and return the next log record.
            this.Current =
                await
                    LogRecord.ReadNextRecordAsync(
                        currentBlockStream,
                        isPhysicalRead: false,
                        useInvalidRecordPosition: true,
                        setRecordLength: false).ConfigureAwait(false);
            this.index++;
            return true;
        }


        /// <summary>
        /// Resets the asynchronous enumerator.
        /// </summary>
        public void Reset()
        {
            this.logBlockEnumerator.Reset();
            this.index = 0;
        }

        private class LogRecordBlockAsyncEnumerator : IAsyncEnumerator<Stream>
        {
            /// <summary>
            /// Temporary buffer for reading into.
            /// </summary>
            private readonly byte[] buffer = new byte[sizeof(int)];

            /// <summary>
            /// The underlying backup log stream to read blocks from.
            /// </summary>
            private Stream backupLogStream;

            /// <summary>
            /// The shared memory stream for reading blocks (chunks) of log records.
            /// </summary>
            private MemoryStream blockStream;

            /// <summary>
            /// The offset and size in the underlying backup log stream of the log record chunks.
            /// </summary>
            private BlockHandle logRecordHandle;

            /// <summary>
            /// Create a new <see cref="LogRecordBlockAsyncEnumerator"/> class.
            /// </summary>
            /// <param name="stream">Backup log stream.</param>
            /// <param name="logRecordHandle">Location in the backup log stream of the log record chunks.</param>
            public LogRecordBlockAsyncEnumerator(Stream stream, BlockHandle logRecordHandle)
            {
                stream.Position = logRecordHandle.Offset;
                this.backupLogStream = stream;
                this.logRecordHandle = logRecordHandle;
                this.blockStream = new MemoryStream();
            }

            /// <summary>
            /// Gets the current log record chunk stream.
            /// </summary>
            public Stream Current { get; private set; }

            public void Dispose()
            {
                if (this.backupLogStream != null)
                {
                    this.backupLogStream.Dispose();
                }

                if (this.blockStream != null)
                {
                    this.blockStream.Dispose();
                }
            }

            public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
            {
                var position = this.backupLogStream.Position;
                var endOffset = this.logRecordHandle.EndOffset;
                if (position < endOffset)
                {
                    // Read the size of the next block.
                    if (position + sizeof(int) > endOffset)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_BackupLogFileAsyncEnumerator_Corrupt, SR.Error_RecordBlock_MissingBlockSize));
                    }

                    await this.backupLogStream.ReadAsync(this.buffer, 0, sizeof(int)).ConfigureAwait(false);
                    var blockSize = BitConverter.ToInt32(this.buffer, 0);
                    var blockSizeWithChecksum = blockSize + sizeof(ulong);

                    // We need to do extra validation on the blockSize, because we haven't validated the bits
                    // against the checksum and we need the blockSize to locate the checksum.
                    if (blockSize < 0)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_BackupLogFileAsyncEnumerator_Corrupt, SR.Error_RecordBlock_NegativeBlockSize));
                    }

                    if (position + blockSize > endOffset)
                    {
                        throw new InvalidDataException(
                            string.Format(CultureInfo.CurrentCulture, SR.Error_BackupLogFileAsyncEnumerator_Corrupt, SR.Error_RecordBlock_ExtendsPastFileEnd));
                    }

                    if (position + blockSizeWithChecksum > endOffset)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_BackupLogFileAsyncEnumerator_Corrupt, SR.Error_RecordBlock_MissingChecksum));
                    }

                    // Read the next block into memory (including the size and checksum).
                    this.backupLogStream.Position = position;
                    this.blockStream.Position = 0;
                    this.blockStream.SetLength(blockSizeWithChecksum);
                    await this.backupLogStream.ReadAsync(this.blockStream.GetBuffer(), 0, blockSizeWithChecksum).ConfigureAwait(false);

                    // Compute the checksum.
                    var computedChecksum = CRC64.ToCRC64(this.blockStream.GetBuffer(), 0, blockSize);

                    // Read the checksum (checksum is after the block bytes).
                    var checksum = BitConverter.ToUInt64(this.blockStream.GetBuffer(), blockSize);
                    // Verify the checksum.
                    if (checksum != computedChecksum)
                    {
                        throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_BackupLogFileAsyncEnumerator_Corrupt, SR.Error_Checksum_Mismatch));
                    }

                    // Return the prepared block reader, skipping over the int32 'size' field at the start and dropping the checksum.
                    this.blockStream.SetLength(blockSize);
                    this.Current = this.blockStream;
                    this.Current.Position = sizeof(int);
                    return true;
                }

                this.Current = null;
                return false;
            }

            public void Reset()
            {
                this.Current = null;
                this.backupLogStream.Position = this.logRecordHandle.Offset;
            }
        }
    }
}