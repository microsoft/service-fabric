// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Common;
    using System.Fabric.Common.Tracing;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    internal class Checkpoint<T>
    {
        private readonly string traceType;

        internal const int FileVersion = 1;

        private int refCount;

        private IStateSerializer<T> valueSerializer;

        public string FileName { get; private set; }

        public string FilePath { get; private set; }

        public List<IListElement<T>> ListElements { get; private set; }

        public Checkpoint(string directory, string fileName, IStateSerializer<T> valueSerializer, string traceType)
        {
            this.traceType = traceType;
            this.ListElements = null;
            this.FileName = fileName;
            this.FilePath = Path.Combine(directory, fileName);
            this.refCount = 1;
            this.valueSerializer = valueSerializer;
        }

        public Checkpoint(string directory, string fileName, List<IListElement<T>> listElements, IStateSerializer<T> valueSerializer, string traceType)
        {
            this.traceType = traceType;
            this.ListElements = listElements;
            this.valueSerializer = valueSerializer;
            this.FileName = fileName;
            this.FilePath = Path.Combine(directory, fileName);
            this.refCount = 1;
        }

        public CopyStream<T> GetCopyStream()
        {
            if (this.refCount <= 0)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "Checkpoint.GetCopyStream",
                    "refCount > 0. refCount: {0}",
                    this.refCount);
            }
            return new CopyStream<T>(this, this.traceType);
        }

        public async Task VerifyAsync()
        {
            await this.ReadListElementsAsync().ConfigureAwait(false);
            // todo?: if ListElements is already populated, verify that it is the same after reading
        }

        private async Task<List<IListElement<T>>> ReadListElementsAsync()
        {
            try
            {
                using (var stream = FabricFile.Open(this.FilePath, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.SequentialScan))
                {
                    var intSegment = new ArraySegment<byte>(new byte[sizeof(int)]);

                    var metadataMarker = await SerializationHelper.ReadIntAsync(intSegment, stream).ConfigureAwait(false);
                    if (metadataMarker != CheckpointFileHelper.MetadataMarker)
                    {
                        throw new InvalidDataException(
                            string.Format(
                                "Stream does not begin with metadata marker.  Expected: {0} Actual: {1}",
                                CheckpointFileHelper.MetadataMarker,
                                metadataMarker));
                    }

                    var versionRead = await SerializationHelper.ReadIntAsync(intSegment, stream).ConfigureAwait(false);
                    if (versionRead != FileVersion)
                    {
                        throw new InvalidDataException(
                            string.Format("Unsupported ReliableConcurrentQueue checkpoint version.  Expected: {0}  Actual: {1}", FileVersion, versionRead));
                    }

                    var listElements = new List<IListElement<T>>();
                    while (true)
                    {
                        var marker = await SerializationHelper.ReadIntAsync(intSegment, stream).ConfigureAwait(false);

                        if (marker == CheckpointFileHelper.BeginFrameMarker)
                        {
                            var chunkListElements = await CheckpointFrame<T>.ReadAsync(stream, this.valueSerializer, this.traceType).ConfigureAwait(false);
                            listElements.AddRange(chunkListElements);
                        }
                        else if (marker == CheckpointFileHelper.EndFramesMarker)
                        {
                            return listElements;
                        }
                        else
                        {
                            throw new InvalidDataException(string.Format("marker: {0}", marker));
                        }
                    }
                }
            }
            catch (Exception ex)
            {
                FabricEvents.Events.ReliableConcurrentQueue_ReadCheckpointError("ReliableConcurrentQueueCheckpointFile.ReadCheckpointFileAsync@", ex.ToString());
                throw;
            }
        }

        // returns false if this.ListElements was not set (no checkpoint), true if this.ListElements was set.
        public async Task<bool> TryReadAsync()
        {
            if (this.ListElements != null)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "Checkpoint.TryReadAsync",
                    "this.ListElements == null.  <listElementsCount: {0}, filePath: {1}, refCount: {2}>",
                    this.ListElements.Count, this.FilePath, this.refCount);
            }

            if (!FabricFile.Exists(this.FilePath))
            {
                FabricEvents.Events.ReliableConcurrentQueue_NoCheckpoint("Checkpoint.TryReadAsync@", this.FilePath);
                return false;
            }

            await this.ReadAsync().ConfigureAwait(false);
            return true;
        }

        public async Task ReadAsync()
        {
            if (this.ListElements != null)
            {
                TestableAssertHelper.FailInvalidData(this.traceType, "Checkpoint.ReadAsync", "this.ListElements == null");
            }
            this.ListElements = await this.ReadListElementsAsync().ConfigureAwait(false);
        }

        public async Task WriteAsync()
        {
            if (this.ListElements == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "Checkpoint.WriteAsync", "this.ListElements == null");
            }
            if (this.valueSerializer == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "Checkpoint.WriteAsync", "this.valueSerializer == null");
            }

            using (var stream = FabricFile.Open(this.FilePath, FileMode.Create, FileAccess.Write, FileShare.None, 4096, FileOptions.SequentialScan))
            {
                await
                    stream.WriteAsync(
                        CheckpointFileHelper.MetadataMarkerSegment.Array,
                        CheckpointFileHelper.MetadataMarkerSegment.Offset,
                        CheckpointFileHelper.MetadataMarkerSegment.Count).ConfigureAwait(false);

                var versionBytes = BitConverter.GetBytes(FileVersion);
                await stream.WriteAsync(versionBytes, 0, versionBytes.Length).ConfigureAwait(false);

                var frame = new CheckpointFrame<T>(this.valueSerializer);
                foreach (var listElement in this.ListElements)
                {
                    if (frame.Size >= CheckpointFileHelper.ChunkWriteThreshold)
                    {
                        await
                            stream.WriteAsync(
                                CheckpointFileHelper.BeginChunkMarkerSegment.Array,
                                CheckpointFileHelper.BeginChunkMarkerSegment.Offset,
                                CheckpointFileHelper.BeginChunkMarkerSegment.Count).ConfigureAwait(false);
                        await frame.WriteAsync(stream).ConfigureAwait(false);
                        frame.Reset();
                    }

                    frame.AddListElement(listElement);
                }

                if (frame.ListElementsCount > 0)
                {
                    await
                        stream.WriteAsync(
                            CheckpointFileHelper.BeginChunkMarkerSegment.Array,
                            CheckpointFileHelper.BeginChunkMarkerSegment.Offset,
                            CheckpointFileHelper.BeginChunkMarkerSegment.Count).ConfigureAwait(false);
                    await frame.WriteAsync(stream).ConfigureAwait(false);
                    frame.Reset();
                }

                await
                    stream.WriteAsync(
                        CheckpointFileHelper.EndFramesMarkerSegment.Array,
                        CheckpointFileHelper.EndFramesMarkerSegment.Offset,
                        CheckpointFileHelper.EndFramesMarkerSegment.Count).ConfigureAwait(false);
            }
        }

        public void Copy(string targetDirectory, bool overwrite)
        {
            var targetPath = Path.Combine(targetDirectory, this.FileName);

            if (!FabricFile.Exists(this.FilePath))
            {
                throw new InvalidOperationException(string.Format("Checkpoint file not found at path {0}", this.FilePath));
            }

            FabricFile.Copy(this.FilePath, targetPath, overwrite);
        }

        public void AddRef()
        {
            if (this.refCount <= 0)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "Checkpoint.AddRef", "refCount > 0. refCount: {0}", this.refCount);
            }
            Interlocked.Increment(ref this.refCount);
        }

        public void ReleaseRef()
        {
            var newRefCount = Interlocked.Decrement(ref this.refCount);
            if (newRefCount < 0)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "Checkpoint.ReleaseRef", "newRefCount >= 0. newRefCount: {0}", newRefCount);
            }
            if (newRefCount == 0)
            {
                FabricFile.Delete(this.FilePath);
            }
        }

        public static string GenerateFileName()
        {
            return Guid.NewGuid() + CheckpointFileHelper.CheckpointFileExtension;
        }
    }
}