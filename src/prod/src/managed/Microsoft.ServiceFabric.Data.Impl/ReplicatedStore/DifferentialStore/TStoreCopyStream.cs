// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Fabric.Common.Tracing;
    using Microsoft.ServiceFabric.ReplicatedStore.Diagnostics;

    /// <summary>
    /// Construct the copy stream for the store.
    /// </summary>
    internal sealed class TStoreCopyStream : IOperationDataStream, IDisposable
    {
        /// <summary>
        /// Fixed size (64 KB) chunks to stream table files during Copy.
        /// </summary>
        private const int CopyChunkSize = 64*1024;

        /// <summary>
        /// Provider to support table copy operations.
        /// </summary>
        private readonly IStoreCopyProvider copyProvider;

        /// <summary>
        /// Indicates the next stage of copy (set of data to be sent).
        /// </summary>
        private CopyStage copyStage = CopyStage.Version;

        /// <summary>
        /// Metadata table being copied.
        /// </summary>
        private MetadataTable copySnapshotOfMetadataTable;

        /// <summary>
        /// Metadata table being copied.
        /// </summary>
        private IEnumerator<KeyValuePair<uint, FileMetadata>> copySnapshotOfMetadataTableEnumerator;

        /// <summary>
        /// The current sorted table file stream being copied.
        /// </summary>
        private Stream currentFileStream;

        /// <summary>
        /// Reusable buffer for copying sorted table data (sizeof(int) for file id, +1 byte for the copy operation type).
        /// </summary>
        private byte[] copyDataBuffer = new byte[CopyChunkSize + sizeof(int) + 1];

        /// <summary>
        /// Used for tracing.
        /// </summary>
        private string traceType;

        private TStoreCopyRateCounterWriter perfCounterWriter;

        /// <summary>
        /// Protects against multiple dispose calls.
        /// </summary>
        private bool disposed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="TStoreCopyStream"/> class.
        /// </summary>
        /// <param name="copyProvider">The store copy provider.</param>
        /// <param name="traceType">Used for tracing.</param>
        /// <param name="perfCounters">Store performance counters instance.</param>
        public TStoreCopyStream(IStoreCopyProvider copyProvider, string traceType, FabricPerformanceCounterSetInstance perfCounters)
        {
            Diagnostics.Assert(copyProvider != null, traceType, "copyProvider CannotUnloadAppDomainException be null");

            this.copyProvider = copyProvider;
            this.traceType = traceType;
            this.perfCounterWriter = new TStoreCopyRateCounterWriter(perfCounters);
        }

        #region IOperationDataStream Members

        /// <summary>
        /// Copy operations are returned.
        /// </summary>
        /// <param name="cancellationToken"></param>
        /// <returns></returns>
        public async Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            var directory = this.copyProvider.WorkingDirectory;

            // Take a snapshot of the metadata table on first Copy operation.
            if (this.copySnapshotOfMetadataTable == null)
            {
                this.copySnapshotOfMetadataTable = await this.copyProvider.GetMetadataTableAsync().ConfigureAwait(false);
                Diagnostics.Assert(
                    this.copySnapshotOfMetadataTable != null,
                    this.traceType,
                    "IStoreCopyProvider.GetMetadataTableAsync() returned a null metadata table.");

                this.copySnapshotOfMetadataTableEnumerator = this.copySnapshotOfMetadataTable.Table.GetEnumerator();
            }

            // Send the copy protocol version first.
            if (this.copyStage == CopyStage.Version)
            {
                FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "starting. directory:{0}", directory));

                // Next copy stage.
                this.copyStage = CopyStage.MetadataTable;

                using (var memoryStream = new MemoryStream(sizeof(int) + sizeof(byte)))
                using (var writer = new BinaryWriter(memoryStream))
                {
                    // Write the copy protocol version number.
                    writer.Write(CopyManager.CopyProtocolVersion);

                    // Write a byte indicating the operation type is the copy protocol version.
                    writer.Write((byte) TStoreCopyOperation.Version);

                    // Send the version operation data.
                    FabricEvents.Events.CopyAsync(
                        this.traceType,
                        string.Format(System.Globalization.CultureInfo.InvariantCulture, "Version. version:{0} bytes:{1}", CopyManager.CopyProtocolVersion, memoryStream.Position));
                    return new OperationData(new ArraySegment<byte>(memoryStream.GetBuffer(), 0, checked((int) memoryStream.Position)));
                }
            }

            // Send the metadata table next.
            if (this.copyStage == CopyStage.MetadataTable)
            {
                // Consistency checks.
                Diagnostics.Assert(this.copySnapshotOfMetadataTable != null, this.traceType, "Unexpected copy error. Master table to be copied is null.");

                // Next copy stage.
                if (this.copySnapshotOfMetadataTableEnumerator.MoveNext())
                {
                    this.copyStage = CopyStage.KeyFile;
                }
                else
                {
                    this.copyStage = CopyStage.Complete;
                }

                using (var memoryStream = new MemoryStream())
                {
                    // Write the full metadata table (this will typically be small - even with 1000 tracked files, this will be under 64 KB).
                    await MetadataManager.WriteAsync(this.copySnapshotOfMetadataTable, memoryStream).ConfigureAwait(false);

                    using (var writer = new BinaryWriter(memoryStream))
                    {
                        // Write a byte indicating the operation type is the full metadata table.
                        writer.Write((byte) TStoreCopyOperation.MetadataTable);

                        // Send the metadata table operation data.
                        FabricEvents.Events.CopyAsync(
                            this.traceType,
                            string.Format(System.Globalization.CultureInfo.InvariantCulture, "Metadata table. directory:{0} bytes:{1} Total number of checkpoint files:{2}", directory, memoryStream.Position, this.copySnapshotOfMetadataTable.Table.Count));
                        return new OperationData(new ArraySegment<byte>(memoryStream.GetBuffer(), 0, checked((int) memoryStream.Position)));
                    }
                }
            }

            // Send the key file.
            if (this.copyStage == CopyStage.KeyFile)
            {
                var bytesRead = 0;

                var shortFileName = this.copySnapshotOfMetadataTableEnumerator.Current.Value.FileName;
                var dataFileName = Path.Combine(directory, shortFileName);
                var keyCheckpointFile = dataFileName + KeyCheckpointFile.FileExtension;

                // If we don't have the current file stream opened, this is the first table chunk.
                if (this.currentFileStream == null)
                {
                    Diagnostics.Assert(
                        FabricFile.Exists(keyCheckpointFile),
                        this.traceType,
                        "Unexpected copy error. Expected file does not exist: {0}",
                        keyCheckpointFile);

                    // Open the file with asynchronous flag and 4096 cache size (C# default).
                    FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "Opening key file. file:{0}", shortFileName));

                    // MCoskun: Default IoPriorityHint is used.
                    // Reason: We do not know whether this copy might be used to build a replica that is or will be required for write quorum.
                    this.currentFileStream = FabricFile.Open(keyCheckpointFile, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous);

                    // Send the start of file operation data.
                    this.perfCounterWriter.StartMeasurement();
                    bytesRead = await this.currentFileStream.ReadAsync(this.copyDataBuffer, 0, CopyChunkSize, cancellationToken).ConfigureAwait(false);
                    this.perfCounterWriter.StopMeasurement(bytesRead);

                    using (var stream = new MemoryStream(this.copyDataBuffer, writable: true))
                    using (var writer = new BinaryWriter(stream))
                    {
                        stream.Position = bytesRead;
                        writer.Write((int) this.copySnapshotOfMetadataTableEnumerator.Current.Value.FileId);
                    }

                    this.copyDataBuffer[bytesRead + sizeof(int)] = (byte) TStoreCopyOperation.StartKeyFile;
                    FabricEvents.Events.CopyAsync(
                        this.traceType,
                        string.Format(System.Globalization.CultureInfo.InvariantCulture, "StartKeyFile. file:{0} bytes:{1} totalFileSize:{2}", shortFileName, bytesRead + sizeof(int) + 1, this.currentFileStream.Length));
                    return new OperationData(new ArraySegment<byte>(this.copyDataBuffer, 0, bytesRead + sizeof(int) + 1));
                }

                // The start of the current file has been sent.  Check if there are more chunks to be sent (if the stream is at the end, this will return zero).
                this.perfCounterWriter.StartMeasurement();
                bytesRead = await this.currentFileStream.ReadAsync(this.copyDataBuffer, 0, CopyChunkSize, cancellationToken).ConfigureAwait(false);
                this.perfCounterWriter.StopMeasurement(bytesRead);
                if (bytesRead > 0)
                {
                    // Send the partial table file operation data.
                    this.copyDataBuffer[bytesRead] = (byte) TStoreCopyOperation.WriteKeyFile;
                    FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "WriteKeyFile. level:{0} bytes:{1} Position:{2}", shortFileName, bytesRead + 1, this.currentFileStream.Position));
                    return new OperationData(new ArraySegment<byte>(this.copyDataBuffer, 0, bytesRead + 1));
                }

                // There is no more data in the current file.  Send the end of file marker, and prepare for the next copy stage.
                this.currentFileStream.Dispose();
                this.currentFileStream = null;

                // Now move to the value file.
                this.copyStage = CopyStage.ValueFile;

                // Send the end of file operation data.
                var endFileData = new ArraySegment<byte>(new[] {(byte) TStoreCopyOperation.EndKeyFile});
                FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "EndKeyFile. file:{0}", shortFileName));
                return new OperationData(endFileData);
            }

            if (this.copyStage == CopyStage.ValueFile)
            {
                var bytesRead = 0;

                var shortFileName = this.copySnapshotOfMetadataTableEnumerator.Current.Value.FileName;
                var dataFileName = Path.Combine(directory, shortFileName);
                var valueCheckpointFile = dataFileName + ValueCheckpointFile.FileExtension;

                // If we don't have the current file opened, this is the first table chunk.
                if (this.currentFileStream == null)
                {
                    Diagnostics.Assert(
                        FabricFile.Exists(valueCheckpointFile),
                        this.traceType,
                        "Unexpected copy error. Expected file does not exist: {0}",
                        valueCheckpointFile);

                    // Open the file with asynchronous flag and 4096 cache size (C# default).
                    FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "Opening value file. directory:{0} file:{1}", directory, shortFileName));

                    // MCoskun: Default IoPriorityHint is used.
                    // Reason: We do not know whether this copy might be used to build a replica that is or will be required for write quorum.
                    this.currentFileStream = FabricFile.Open(
                        valueCheckpointFile,
                        FileMode.Open,
                        FileAccess.Read,
                        FileShare.Read,
                        4096,
                        FileOptions.Asynchronous);

                    // Send the start of file operation data.
                    this.perfCounterWriter.StartMeasurement();
                    bytesRead = await this.currentFileStream.ReadAsync(this.copyDataBuffer, 0, CopyChunkSize, cancellationToken).ConfigureAwait(false);
                    this.perfCounterWriter.StopMeasurement(bytesRead);

                    using (var stream = new MemoryStream(this.copyDataBuffer, writable: true))
                    using (var writer = new BinaryWriter(stream))
                    {
                        stream.Position = bytesRead;
                        writer.Write((int) this.copySnapshotOfMetadataTableEnumerator.Current.Value.FileId);
                    }

                    this.copyDataBuffer[bytesRead + sizeof(int)] = (byte) TStoreCopyOperation.StartValueFile;
                    FabricEvents.Events.CopyAsync(
                        this.traceType,
                        string.Format(System.Globalization.CultureInfo.InvariantCulture, "StartValueFile. file:{0} bytes:{1} totalFileSize:{2}", shortFileName, bytesRead + sizeof(int) + 1, this.currentFileStream.Length));
                    return new OperationData(new ArraySegment<byte>(this.copyDataBuffer, 0, bytesRead + sizeof(int) + 1));
                }

                // The start of the current file was sent.  Check if there are more chunks to be sent (if the stream is at the end, this will return zero).
                this.perfCounterWriter.StartMeasurement();
                bytesRead = await this.currentFileStream.ReadAsync(this.copyDataBuffer, 0, CopyChunkSize, cancellationToken);
                this.perfCounterWriter.StopMeasurement(bytesRead);

                if (bytesRead > 0)
                {
                    this.copyDataBuffer[bytesRead] = (byte) TStoreCopyOperation.WriteValueFile;
                    FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "WriteValueFile. file:{0} bytes:{1} Position:{2}", shortFileName, bytesRead + 1, this.currentFileStream.Position));
                    return new OperationData(new ArraySegment<byte>(this.copyDataBuffer, 0, bytesRead + 1));
                }

                // There is no more data in the current file.  Send the end of file marker, and prepare for the next copy stage.
                this.currentFileStream.Dispose();
                this.currentFileStream = null;

                // Check if there are more files.
                if (this.copySnapshotOfMetadataTableEnumerator.MoveNext())
                {
                    // More files.
                    this.copyStage = CopyStage.KeyFile;
                }
                else
                {
                    // No more files to be sent.
                    this.copyStage = CopyStage.Complete;
                }

                // Send the end of file operation data.
                var endFileData = new ArraySegment<byte>(new[] {(byte) TStoreCopyOperation.EndValueFile});
                FabricEvents.Events.CopyAsync(this.traceType, string.Format(System.Globalization.CultureInfo.InvariantCulture, "EndValueFile. file:{0}", shortFileName));
                return new OperationData(endFileData);
            }

            // Finally, send the "copy completed" marker.
            if (this.copyStage == CopyStage.Complete)
            {
                this.perfCounterWriter.UpdatePerformanceCounters();

                // Next copy stage.
                this.copyStage = CopyStage.None;

                // Indicate the copy operation is complete.
                FabricEvents.Events.CopyAsync(
                    this.traceType,
                    string.Format(
                        System.Globalization.CultureInfo.InvariantCulture,
                        "completed. directory:{0}, diskread:{1}bytes/sec",
                        directory,
                        this.perfCounterWriter.AvgDiskTransferBytesPerSec));
                var copyCompleteData = new ArraySegment<byte>(new[] {(byte) TStoreCopyOperation.Complete});
                return new OperationData(copyCompleteData);
            }

            // Finished copying.  Dispose immediately to release resources/locks.
            if (!this.disposed)
            {
                ((IDisposable) this).Dispose();
            }

            return null;
        }

        #endregion

        /// <summary>
        /// Dispose unmanaged resources.
        /// </summary>
        void IDisposable.Dispose()
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
                    // Trace that copy has completed if we started.
                    FabricEvents.Events.CopyAsync(this.traceType, "disposing");

                    if (this.currentFileStream != null)
                    {
                        this.currentFileStream.Dispose();
                        this.currentFileStream = null;
                    }

                    if (this.copySnapshotOfMetadataTableEnumerator != null)
                    {
                        this.copySnapshotOfMetadataTableEnumerator.Dispose();
                        this.copySnapshotOfMetadataTableEnumerator = null;
                    }

                    if (this.copySnapshotOfMetadataTable != null)
                    {
                        this.copySnapshotOfMetadataTable.ReleaseRef();
                        this.copySnapshotOfMetadataTable = null;
                    }
                }

                this.copyStage = CopyStage.None;
            }

            this.disposed = true;
        }


        /// <summary>
        /// Enumeration indicating the current copy stage (data to be sent).
        /// </summary>
        private enum CopyStage
        {
            /// <summary>
            /// Indicates the copy protocol version will be sent.
            /// </summary>
            Version,

            /// <summary>
            /// Indicates the metadata table bytes will be sent.
            /// </summary>
            MetadataTable,

            /// <summary>
            /// Indicates key file.
            /// </summary>
            KeyFile,

            /// <summary>
            /// Indicates value file.
            /// </summary>
            ValueFile,

            /// <summary>
            /// Indicates the copy "completed" marker will be sent.
            /// </summary>
            Complete,

            /// <summary>
            /// Indicates no more copy data needs to be sent.
            /// </summary>
            None,
        }
    }
}