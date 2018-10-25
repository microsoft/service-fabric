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
    using System.Threading.Tasks;

    internal class CopyFileWriter : IDisposable
    {
        private readonly string traceType;

        private FileStream fileStream;

        private bool isDisposed;

        private bool isFirstChunk = true;

        public CopyFileWriter(string filePath, string traceType)
        {
            this.traceType = traceType;
            this.fileStream = FabricFile.Open(filePath, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.Asynchronous);
        }

        public async Task<bool> WriteNextAsync(OperationData operationData)
        {
            if (this.isDisposed)
            {
                throw new FabricObjectClosedException();
            }

            if (operationData == null)
            {
                throw new ArgumentNullException("operationData");
            }

            if (operationData.Count != 1)
            {
                throw new ArgumentException(string.Format("Unexpected operationData count: {0}", operationData.Count));
            }

            ArraySegment<byte> dataSegment;
            using (var enumerator = operationData.GetEnumerator())
            {
                bool res = enumerator.MoveNext();
                if (!res)
                {
                    throw new InvalidDataException("operationData enumeration is empty");
                }

                dataSegment = enumerator.Current;
            }

            if (dataSegment.Count <= 0)
            {
                throw new ArgumentException("Empty operationData data segment.");
            }

            // the first chunk should contain only metadata (the version), so at least one chunk will follow
            if (this.isFirstChunk)
            {
                this.isFirstChunk = false;
                await this.fileStream.WriteAsync(dataSegment.Array, dataSegment.Offset, dataSegment.Count).ConfigureAwait(false);
                return false;
            }

            // subsequent chunks should be checkpoint frames
            return await CheckpointFileHelper.WriteFrameAsync(dataSegment, this.fileStream, this.traceType).ConfigureAwait(false);

            // todo: trace
        }

        public void Dispose()
        {
            if (this.isDisposed)
            {
                return;
            }

            this.fileStream.Flush();
            this.fileStream.Dispose();
            this.fileStream = null;

            this.isDisposed = true;
        }
    }
}