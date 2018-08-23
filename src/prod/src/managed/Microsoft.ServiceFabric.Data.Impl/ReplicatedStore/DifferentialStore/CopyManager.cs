// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Globalization;
    using System.IO;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.Data;
    using Microsoft.ServiceFabric.ReplicatedStore;
    using Microsoft.ServiceFabric.Replicator;
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;

    /// <summary>
    /// Processes copy data and populates internal structures.<see cref="CopyManager"/> files
    /// and/or a <see cref="CopyManager"/> file.
    /// </summary>
    internal class CopyManager : IDisposable
    {
        public const int InvalidVersion = int.MinValue;
        public const int CopyProtocolVersion = 1;

        private const int SizeOfFileId = sizeof(int);

        /// <summary>
        /// Tracing type information.
        /// </summary>
        private readonly string traceType;

        /// <summary>
        /// Performance counter writer for disk write rate during copy.
        /// </summary>
        private readonly TStoreCopyRateCounterWriter perfCounterWriter;

        /// <summary>
        /// Copy operation support - the name of the file currently being copied from a Primary.
        /// Paired with <see cref="currentCopyFileName"/>.
        /// </summary>
        private string currentCopyFileName;

        /// <summary>
        /// Copy operation support - the file stream for the currently being copied from a Primary.
        /// Paired with <see cref="currentCopyFileStream"/>.
        /// </summary>
        private FileStream currentCopyFileStream;

        /// <summary>
        /// Copy operation support - the protocol number for the current copy operation.
        /// </summary>
        private int copyProtocolVersion = InvalidVersion;

        // Track files being sent to ensure that the count matches with the metadatatable.
        private int fileCount;

        /// <summary>
        /// Whether the copy has been completed. Starts at true in case of empty copy
        /// </summary>
        private bool copyCompleted = true;

        /// <summary>
        /// Protects against multiple dispose calls.
        /// </summary>
        private bool disposed = false;

        /// <summary>
        /// Helper class to build files.
        /// </summary>
        /// <param name="traceType">Tracing information.</param>
        /// <param name="perfCounters">Store performance counters instance.</param>
        public CopyManager(string traceType, FabricPerformanceCounterSetInstance perfCounters)
        {
            this.traceType = traceType;
            this.perfCounterWriter = new TStoreCopyRateCounterWriter(perfCounters);
        }

        public MetadataTable MetadataTable { get; set; }

        public bool CopyCompleted { get { return copyCompleted; } }

        /// <summary>
        /// Gets the current file stream where data is being copied into.
        /// </summary>
        /// <devnote>Exposed for testability only.</devnote>
        public FileStream CurrentCopyFileStream
        {
            get
            {
                return this.currentCopyFileStream;
            }
        }

        /// <summary>
        /// Add the given Copy operation.
        /// </summary>
        /// <param name="directory">Directory to create the copied files.</param>
        /// <param name="data">Copy operation data.</param>
        /// <param name="traceType">Trace information.</param>
        public async Task AddCopyDataAsync(string directory, OperationData data, string traceType)
        {
            // Process each of the copy operation data segments.
            var receivedBytes = 0;
            foreach (var dataSegment in data)
            {
                receivedBytes += await this.ProcessCopyOperationAsync(directory, dataSegment).ConfigureAwait(false);
            }

            FabricEvents.Events.Bytes_SetCurrentState(traceType, DifferentialStoreConstants.AddCopyData_Received, receivedBytes);
        }

        /// <summary>
        /// Process an operation data segment from a Copy operation.  The valid operation types
        /// are defined by the TStoreCopyOperation enum.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">A single copy operation data segment.</param>
        /// <returns>The number of bytes in the operation data segment.</returns>
        private async Task<int> ProcessCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            Diagnostics.Assert(data.Array != null, this.traceType, "OperationData segment is null.");
            Diagnostics.Assert(data.Count > 0, this.traceType, "OperationData segment is empty.");

            // The last byte indicates the operation type.
            var operation = (TStoreCopyOperation)data.Array[data.Offset + data.Count - 1];
            var operationData = new ArraySegment<byte>(data.Array, data.Offset, data.Count - 1);

            switch (operation)
            {
                case TStoreCopyOperation.Version:
                    this.ProcessVersionCopyOperation(directory, operationData);
                    break;

                case TStoreCopyOperation.MetadataTable:
                    await this.ProcessMetadataTableCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.StartKeyFile:
                    await this.ProcessStartKeyFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.WriteKeyFile:
                    await this.ProcessWriteKeyFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.EndKeyFile:
                    await this.ProcessEndKeyFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.StartValueFile:
                    await this.ProcessStartValueFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.WriteValueFile:
                    await this.ProcessWriteValueFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.EndValueFile:
                    await this.ProcessEndValueFileCopyOperationAsync(directory, operationData).ConfigureAwait(false);
                    break;

                case TStoreCopyOperation.Complete:
                    this.ProcessCompleteCopyOperation(directory, operationData);
                    break;

                default:
                    Diagnostics.Assert(false, traceType, "invalid TStore copy operation: {0}", operation.ToString());
                    break;
            }

            return data.Count;
        }

        /// <summary>
        /// Process a TStoreCopyOperation.Version copy operation.
        /// The operation data segment contains the copy protocol version number.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private void ProcessVersionCopyOperation(string directory, ArraySegment<byte> data)
        { 
            // Copy has begun because we received a version operation
            copyCompleted = false;

            FabricEvents.Events.ProcessVersionCopyOperationData(this.traceType, directory, data.Count);

            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion == InvalidVersion, this.traceType, "unexpected copy operation: Version received multiple times");
            Diagnostics.Assert(
                data.Count == sizeof(int),
                traceType,
                "unexpected copy operation: Version operation data has an unexpected size: {0}",
                data.Count);

            // Verify we are able to understand the copy protocol version.
            var copyVersion = BitConverter.ToInt32(data.Array, data.Offset);
            if (copyVersion != CopyProtocolVersion)
            {
                FabricEvents.Events.ProcessVersionCopyOperationMsg(this.traceType, copyVersion);
                throw new InvalidDataException(string.Format(CultureInfo.CurrentCulture, SR.Error_CopyManager_UnknownProtocol, copyVersion));
            }

            // Store the copy protocol version number.
            this.copyProtocolVersion = copyVersion;

            FabricEvents.Events.ProcessVersionCopyOperationProtocol(this.traceType, directory, this.copyProtocolVersion);
        }

        /// <summary>
        /// Process a TStoreCopyOperation.MetadataTable copy operation.
        /// The operation data segment contains the full master table bytes.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessMetadataTableCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            FabricEvents.Events.ProcessMetadataTableCopyOperation(this.traceType, directory, data.Count);

            // Consistency checks.
            Diagnostics.Assert(
                this.copyProtocolVersion != TableConstants.InvalidVersion,
                this.traceType,
                "unexpected copy operation: MetadataTable received before Version operation");
            Diagnostics.Assert(this.MetadataTable == null, this.traceType, "unexpected copy operation: MetadataTable received multiple times");
            Diagnostics.Assert(
                this.currentCopyFileStream == null,
                this.traceType,
                "unexpected copy operation: MetadataTable received when we are copying a checkpoint file");

            this.MetadataTable = new MetadataTable(this.traceType);

            // Deserialize the metadata table in memory.  Currently we just use it to read out properties and use it to validate when Copy completes.
            using (var memoryStream = new MemoryStream(data.Array, data.Offset, data.Count, writable: false))
            {
                await MetadataManager.OpenAsync(this.MetadataTable, memoryStream, this.traceType).ConfigureAwait(false);
            }
        }

        /// <summary>
        /// Process a TStoreCopyOperation.StartKeyFile copy operation.  Indicates a new key checkpoint file is being copied.
        /// The operation data segment contains the full or partial checkpoint file bytes.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessStartKeyFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion != InvalidVersion, this.traceType, "unexpected copy operation: StartKeyFile received before Version operation");
            Diagnostics.Assert(this.MetadataTable != null, this.traceType, "unexpected copy operation: key checkpoint file received before metadata table");
            Diagnostics.Assert(
                this.currentCopyFileStream == null,
                this.traceType,
                "unexpected copy operation: StartKeyFile received when we are already copying a checkpoint file");

            // Start writing a new file.
            var checkpointFileName = Guid.NewGuid().ToString();
            this.currentCopyFileName = checkpointFileName + KeyCheckpointFile.FileExtension;
            this.fileCount++;

            // Get the file id.
            var fileId = BitConverter.ToUInt32(data.Array, data.Offset + data.Count - sizeof(int));

            // Assert that it is present in metadatatable.
            Diagnostics.Assert(this.MetadataTable.Table.ContainsKey(fileId), this.traceType, "Invalid file id");

            // Update value
            this.MetadataTable.Table[fileId].FileName = checkpointFileName;

            FabricEvents.Events.ProcessStartKeyFileCopyOperation(this.traceType, directory, this.currentCopyFileName, data.Count);

            // Create the file with asynchronous flag and 4096 cache size (C# default).
            var fullCopyFileName = Path.Combine(directory, this.currentCopyFileName);

            // MCoskun: Default IoPriorityHint is used.
            // Reason: We do not know whether this copy might be used to build a replica that is or will be required for write quorum.
            // In future we can check write status at GetNextAsync to determine whether IO priority needs to be increased or decreased.
            this.currentCopyFileStream = FabricFile.Open(
                fullCopyFileName,
                FileMode.CreateNew,
                FileAccess.ReadWrite,
                FileShare.ReadWrite,
                4096,
                FileOptions.Asynchronous);

            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.WriteAsync(data.Array, data.Offset, data.Count - SizeOfFileId).ConfigureAwait(false);
            perfCounterWriter.StopMeasurement(data.Count - SizeOfFileId);
        }

        /// <summary>
        /// Process a TStoreCopyOperation.WriteKeyFile copy operation.  Indicates a sequential section of a key checkpoint file is being copied.
        /// The operation data segment contains the next sequential checkpoint file bytes.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessWriteKeyFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            FabricEvents.Events.ProcessWriteKeyFileCopyOperation(this.traceType, directory, this.currentCopyFileName, data.Count);
            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion != InvalidVersion, this.traceType, "unexpected copy operation: WriteKeyFile received before Version operation");
            Diagnostics.Assert(this.MetadataTable != null, this.traceType, "unexpected copy operation: WriteKeyFile received before metadata table");
            Diagnostics.Assert(this.currentCopyFileStream != null, this.traceType, "unexpected copy operation: WriteKeyFile received before StartKeyFile");

            // Append the data to the existing checkpoint file stream.
            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.WriteAsync(data.Array, data.Offset, data.Count).ConfigureAwait(false);
            perfCounterWriter.StopMeasurement(data.Count);
        }

        /// <summary>
        /// Process a TStoreCopyOperation.EndKeyFile copy operation.  Indicates the current key checkpoint file has finished being copied.
        /// The operation data segment is empty.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessEndKeyFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion != InvalidVersion, this.traceType, "unexpected copy operation: EndKeyFile received before Version operation");
            Diagnostics.Assert(
                this.currentCopyFileStream != null,
                this.traceType,
                "unexpected copy operation: EndKeyFile received when we aren't copying a checkpoint file");
            Diagnostics.Assert(
                !string.IsNullOrEmpty(this.currentCopyFileName),
                this.traceType,
                "unexpected copy operation: EndKeyFile received when we don't have a valid checkpoint file");

            FabricEvents.Events.ProcessEndKeyFileCopyOperation(this.traceType, directory, this.currentCopyFileName, this.currentCopyFileStream.Length);

            // Flush and close the current copied checkpoint file stream.
            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.FlushAsync().ConfigureAwait(false);
            perfCounterWriter.StopMeasurement();

            this.currentCopyFileStream.Dispose();
            this.currentCopyFileStream = null;
        }

        /// <summary>
        /// Process a TStoreCopyOperation.StartValueFile copy operation.  Indicates a new value checkpoint file is being copied.
        /// The operation data segment contains the full or partial checkpoint file bytes.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessStartValueFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            // Consistency checks.
            Diagnostics.Assert(
                this.copyProtocolVersion != InvalidVersion,
                this.traceType,
                "unexpected copy operation: StartValueFile received before Version operation");
            Diagnostics.Assert(this.MetadataTable != null, this.traceType, "unexpected copy operation: key checkpoint file received before metadata table");
            Diagnostics.Assert(
                this.currentCopyFileStream == null,
                this.traceType,
                "unexpected copy operation: StartValueFile received when we are already copying a checkpoint file");

            // Get the file id.
            var fileId = BitConverter.ToUInt32(data.Array, data.Offset + data.Count - sizeof(int));

            // Assert that it is present in metadatatable.
            Diagnostics.Assert(this.MetadataTable.Table.ContainsKey(fileId), this.traceType, "Invalid file id");

            // Update value
            this.currentCopyFileName = this.MetadataTable.Table[fileId].FileName + ValueCheckpointFile.FileExtension;

            FabricEvents.Events.ProcessStartValueFileCopyOperation(this.traceType, directory, this.currentCopyFileName, data.Count);

            // Create the file with asynchronous flag and 4096 cache size (C# default).
            var fullCopyFileName = Path.Combine(directory, this.currentCopyFileName);

            // MCoskun: Default IoPriorityHint is used.
            // Reason: We do not know whether this copy might be used to build a replica that is or will be required for write quorum.
            // In future we can check write status at GetNextAsync to determine whether IO priority needs to be increased or decreased.
            this.currentCopyFileStream = FabricFile.Open(
                fullCopyFileName,
                FileMode.CreateNew,
                FileAccess.ReadWrite,
                FileShare.ReadWrite,
                4096,
                FileOptions.Asynchronous);

            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.WriteAsync(data.Array, data.Offset, data.Count - SizeOfFileId).ConfigureAwait(false);
            perfCounterWriter.StopMeasurement(data.Count - SizeOfFileId);
        }

        /// <summary>
        /// Process a TStoreCopyOperation.WriteValueFile copy operation.  Indicates a sequential section of a value checkpoint file is being copied.
        /// The operation data segment contains the next sequential checkpoint file bytes.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessWriteValueFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            FabricEvents.Events.ProcessWriteValueFileCopyOperation(this.traceType, directory, this.currentCopyFileName, data.Count);
            // Consistency checks.
            Diagnostics.Assert(
                this.copyProtocolVersion != InvalidVersion,
                this.traceType,
                "unexpected copy operation: WriteValueFile received before Version operation");
            Diagnostics.Assert(this.MetadataTable != null, this.traceType, "unexpected copy operation: WriteValueFile received before MetadataTable");
            Diagnostics.Assert(this.currentCopyFileStream != null, this.traceType, "unexpected copy operation: WriteValueFile received before MetadataTable");

            // Append the data to the existing checkpoint file stream.
            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.WriteAsync(data.Array, data.Offset, data.Count).ConfigureAwait(false);
            perfCounterWriter.StopMeasurement(data.Count);
        }

        /// <summary>
        /// Process a TStoreCopyOperation.EndValueFile copy operation.  Indicates the current value checkpoint file has finished being copied.
        /// The operation data segment is empty.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private async Task ProcessEndValueFileCopyOperationAsync(string directory, ArraySegment<byte> data)
        {
            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion != InvalidVersion, this.traceType, "unexpected copy operation: EndValueFile received before Version operation");
            Diagnostics.Assert(
                this.currentCopyFileStream != null,
                this.traceType,
                "unexpected copy operation: EndValueFile received when we aren't copying a checkpoint file");
            Diagnostics.Assert(
                !string.IsNullOrEmpty(this.currentCopyFileName),
                this.traceType,
                "unexpected copy operation: EndValueFile received when we don't have a valid checkpoint file");

            FabricEvents.Events.ProcessEndValueFileCopyOperation(this.traceType, directory, this.currentCopyFileName, this.currentCopyFileStream.Length);

            // Flush and close the current copied checkpoint file stream.
            perfCounterWriter.StartMeasurement();
            await this.currentCopyFileStream.FlushAsync().ConfigureAwait(false);
            perfCounterWriter.StopMeasurement();

            this.currentCopyFileStream.Dispose();
            this.currentCopyFileStream = null;
        }

        /// <summary>
        /// Process a TStoreCopyOperation.Complete copy operation.  Indicates the entire metadata table and referenced checkpoint files have been copied.
        /// The operation data segment is empty.
        /// </summary>
        /// <param name="directory">Working directory for the copy operation.</param>
        /// <param name="data">Copy operation data segment.</param>
        private void ProcessCompleteCopyOperation(string directory, ArraySegment<byte> data)
        {
            perfCounterWriter.UpdatePerformanceCounters();
#if !DotNetCoreClr
            FabricEvents.Events.ProcessCompleteCopyOperation(this.traceType, directory, perfCounterWriter.AvgDiskTransferBytesPerSec);
#endif

            // Consistency checks.
            Diagnostics.Assert(this.copyProtocolVersion != InvalidVersion, this.traceType, "unexpected copy operation: Complete received before Version operation");
            Diagnostics.Assert(
                this.currentCopyFileStream == null,
                this.traceType,
                "unexpected copy operation: Complete received when we are still copying a table file");
            Diagnostics.Assert(this.MetadataTable != null, this.traceType, "unexpected copy operation: Complete received before MetadataTable");

            // Verify we copied the master table correctly.
            var expectedCount = this.MetadataTable.Table.Count;
            if (this.fileCount != expectedCount)
            {
                throw new InvalidDataException(
                    string.Format(CultureInfo.CurrentCulture, SR.Error_CopyManager_CopyFailed, this.fileCount, expectedCount));
            }

            copyCompleted = true;

            // Reset copy state.
            this.currentCopyFileName = null;
            this.currentCopyFileStream = null;
            this.copyProtocolVersion = InvalidVersion;
        }

        /// <summary>
        /// Dispose unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (this.currentCopyFileStream != null)
                    {
                        this.currentCopyFileStream.Dispose();
                        this.currentCopyFileStream = null;
                    }
                }

            }

            this.disposed = true;
        }
    }
}