// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Fabric;
    using System.Fabric.Common;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    // todo: tracing
    internal sealed class CopyStream<T> : IOperationDataStream, IDisposable
    {
        private readonly IAsyncEnumerator<OperationData> checkpointFileEnumerator;

        private bool isDisposed;

        private readonly string traceType;

        private class CheckpointFileEnumerator : IAsyncEnumerator<OperationData>
        {
            private readonly string traceType;

            private FileStream fileStream;

            private MemoryStream frameStream;

            private bool readAllData;

            private bool isFirstChunk = true;

            private Checkpoint<T> checkpoint; 

            public CheckpointFileEnumerator(Checkpoint<T> checkpoint, string traceType)
            {
                this.traceType = traceType;
                this.checkpoint = checkpoint;
                this.checkpoint.AddRef();
                this.frameStream = new MemoryStream();
                this.fileStream = FabricFile.Open(this.checkpoint.FilePath, FileMode.Open, FileAccess.Read, FileShare.Read, 4096, FileOptions.Asynchronous);
            }

            private void TryCleanupFileStream()
            {
                var fs = this.fileStream;
                this.fileStream = null;
                if (fs != null)
                {
                    fs.Dispose();
                }
            }

            public void Dispose()
            {
                // don't guard against multiple dispose as this class is private and the containing class will not call dispose twice
                this.TryCleanupFileStream();
                this.frameStream.Dispose();
                this.frameStream = null;
                this.checkpoint.ReleaseRef();
                this.checkpoint = null;
            }

            // don't guard against this being called after the end of the stream, as this class is private and the containing class will not do so
            public OperationData Current { get; private set; }

            /// <summary>
            /// Get the next full frame from the checkpoint file.
            /// </summary>
            /// <returns>False if the end of the checkpoint data has been reached, true otherwise.</returns>
            public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
            {
                if (this.readAllData)
                {
                    this.TryCleanupFileStream(); // clean up file stream as soon as we can.
                    this.Current = null;
                    return false;
                }

                // metadata frame - read the version 
                if (this.isFirstChunk)
                {
                    this.isFirstChunk = false;
                    var segment = new ArraySegment<byte>(new byte[2 * sizeof(int)]); // marker and version
                    await SerializationHelper.ReadBytesAsync(segment, segment.Count, this.fileStream).ConfigureAwait(false);
                    this.Current = new OperationData(segment);
                    return true; // there will be at least one more frame
                }

                // one of the data frames
                this.frameStream.Position = 0;
                this.readAllData = await CheckpointFileHelper.CopyNextFrameAsync(this.fileStream, this.frameStream, this.traceType).ConfigureAwait(false);
                this.Current = new OperationData(new ArraySegment<byte>(this.frameStream.GetBuffer(), 0, (int)this.frameStream.Position));

                return true;
            }

            public void Reset()
            {
                throw new NotImplementedException();
            }
        }

        public CopyStream(Checkpoint<T> checkpoint, string traceType)
        {
            this.traceType = traceType;
            this.checkpointFileEnumerator = new CheckpointFileEnumerator(checkpoint, traceType);
        }

        public async Task<OperationData> GetNextAsync(CancellationToken cancellationToken)
        {
            if (this.isDisposed)
            {
                throw new FabricObjectClosedException();
            }

            if (await this.checkpointFileEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false))
            {
                return this.checkpointFileEnumerator.Current;
            }

            this.Dispose(); // dispose aggressively
            return null;
        }

        public void Dispose()
        {
            if (this.isDisposed)
            {
                return;
            }

            this.checkpointFileEnumerator.Dispose();

            this.isDisposed = true;
        }
    }
}