// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Collections.Concurrent;
    using System.Fabric.Common;
    using System.Fabric.Data.Log;
    using System.Globalization;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Memory log implementation of the ILogicalLog interface
    /// Memory buffers are allocated in multiples of "CHUNK_SIZE", which is 64K by default- to avoid using the large object heap
    /// 
    /// This log is NOT thread safe for a general purpose usage.
    /// However, in the context of the replicator's usage of this log, it is safe to do multiple reads from the replicator, while performing writes.
    /// This is due to the guarantees that the replicator provides:
    ///     1. There is only 1 AppendAsync() or FlushAsync() or TruncateHead() invoked at any time (Write)
    ///     2. Any Read call is guaranteed to be at an offset greater than head of the log's position and offset lesser than tail of the log's position
    ///     3. Reads and Writes can happen concurrently with the above 2 limitations.
    /// 
    /// Due to the abvoe guarantee's this log can be lock free, but still requires a concurrent dictionary to keep track of all the buffers to be able to index into 
    /// a memory offset very quickly (in constant time)
    /// 
    /// Additionally, since truncatehead can happen while a read is going on, we need a thread safe data structure to keep track of the buffers.
    /// 
    /// Finally, memory is free'd up only in multiples of CHUNK's.
    /// 
    /// This also uses a buffer manager to reduce the amount of allocations+deallocations.
    /// </summary>
    internal class MemoryLogicalLog : ILogicalLog
    {
        internal const int ChunkSize = 64*1024;

        private readonly SynchronizedBufferPool<ByteArray> bufferPool;

        private readonly int maxChuckCount;

        private ConcurrentDictionary<long, ByteArray> bytes;

        private long head;

        private long tail;

        private bool isDisposed;

        private MemoryLogicalLog(long startingPos, int maxSizeInMB)
        {
            this.head = startingPos;
            this.tail = startingPos;
            this.bytes = null;
            this.isDisposed = false;

            var chunkSizeKB = ChunkSize/1024;
            this.maxChuckCount = maxSizeInMB*1024/chunkSizeKB;

            this.bufferPool = new SynchronizedBufferPool<ByteArray>(() => new ByteArray(ChunkSize), this.maxChuckCount);
            this.bytes = new ConcurrentDictionary<long, ByteArray>();
        }

        public long HeadTruncationPosition
        {
            get { throw new NotImplementedException(); }
        }

        public bool IsFunctional
        {
            get { return this.Bytes != null; }
        }

        public long Length
        {
            get { return Interlocked.Read(ref this.tail) - Interlocked.Read(ref this.head); }
        }

        public long MaximumBlockSize
        {
            get { throw new NotImplementedException(); }
        }

        public uint MetadataBlockHeaderSize
        {
            get { throw new NotImplementedException(); }
        }

        public long ReadPosition
        {
            get { throw new NotImplementedException(); }
        }

        public long WritePosition
        {
            get { return Interlocked.Read(ref this.tail); }
        }

        internal static int ChunkSizeBytes
        {
            get { return ChunkSize; }
        }

        internal int MaxChuckCount
        {
            get { return this.maxChuckCount; }
        }

        internal ConcurrentDictionary<long, ByteArray> Bytes
        {
            get { return this.bytes; }
        }

        internal long Head
        {
            get { return this.head; }
        }

        internal long Tail
        {
            get { return this.tail; }
        }

        public static Task<ILogicalLog> CreateMemoryLogicalLog(int maxSize)
        {
            return TestCreateMemoryLogicalLog(0, maxSize);
        }

        public void Abort()
        {
            this.Dispose();
        }

        public Task AppendAsync(byte[] buffer, int offset, int count, CancellationToken cancellationToken)
        {
            Utility.Assert(buffer != null && count > 0, "Buffer must be non null and count must be greater than 0");

            var offsetofCurrentArrayEnd = GetMultipleOfXGreaterThanY(x: ChunkSize, y: this.Tail);

            try
            {
                // If the tail is within the first array, insert it immediately
                if (count + this.Tail < offsetofCurrentArrayEnd)
                {
                    var offsetToCopyInto = checked((int) (this.Tail%ChunkSize));
                    var indexOfArray = this.Tail/ChunkSize;
                    Buffer.BlockCopy(buffer, offset, this.Bytes[indexOfArray].Array, offsetToCopyInto, count);
                    this.UpdateTail(count);
                }
                else
                {
                    // Append to the first buffer
                    var indexOfArray = this.Tail/ChunkSize;
                    var bytesAppended = 0;
                    var appendSize = checked((int) (offsetofCurrentArrayEnd - Interlocked.Read(ref this.tail)));

                    Buffer.BlockCopy(
                        buffer,
                        offset,
                        this.Bytes[indexOfArray].Array,
                        checked((int) (Interlocked.Read(ref this.tail)%ChunkSize)),
                        appendSize);
                    this.UpdateTail(appendSize);
                    bytesAppended += appendSize;

                    // Append to the remaining buffers
                    while (bytesAppended < count)
                    {
                        appendSize = (count - bytesAppended) > ChunkSize ? ChunkSize : count - bytesAppended;

                        indexOfArray = Interlocked.Read(ref this.tail)/ChunkSize;

                        Buffer.BlockCopy(buffer, offset + bytesAppended, this.Bytes[indexOfArray].Array, 0, appendSize);
                        bytesAppended += appendSize;
                        this.UpdateTail(appendSize);
                    }

                    Utility.Assert(
                        bytesAppended == count,
                        "BytesAppended = {0} is not equal to Count = {1}",
                        bytesAppended, count);
                }
            }
            catch (Exception e)
            {
                Utility.Assert(
                    false,
                    "Unexpected exception {0} {1} {2} in AppendAsync while adding bufer of size {3}. {4}",
                    e.Message, e.Message, e.StackTrace, count, this.GetState());
            }

            return Task.FromResult(0);
        }

        public Task CloseAsync(CancellationToken cancellationToken)
        {
            this.Dispose();
            return Task.FromResult(0);
        }

        public Task ConfigureWritesToOnlyDedicatedLogAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        public Task ConfigureWritesToSharedAndDedicatedLogAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        public Stream CreateReadStream(int sequentialAccessReadSize)
        {
            var newStream = new MemoryLogReaderStream(this);
            var readerStream = new BufferedStream(newStream);
            return readerStream;
        }

        public void Dispose()
        {
            if (this.isDisposed)
            {
                return;
            }

            this.bytes = null;

            if (this.bufferPool != null)
            {
                this.bufferPool.OnClear();
            }
            this.isDisposed = true;
        }

        public Task FlushAsync(CancellationToken cancellationToken)
        {
            return Task.FromResult(0);
        }

        public Task FlushWithMarkerAsync(CancellationToken cancellationToken)
        {
            return this.FlushAsync(cancellationToken);
        }

        public Task<uint> QueryLogUsageAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task<int> ReadAsync(
            byte[] buffer,
            int offset,
            int count,
            int bytesToRead,
            CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public long SeekForRead(long offset, SeekOrigin origin)
        {
            throw new NotImplementedException();
        }

        public void SetSequentialAccessReadSize(Stream logStream, int sequentialAccessReadSize)
        {
        }

        public void TruncateHead(long streamOffset)
        {
            Utility.Assert(
                streamOffset <= this.Tail && streamOffset >= this.Head,
                "Truncate request at {0} is beyond the tail of the log {1} or before the head {2}",
                streamOffset, this.Tail, this.Head);

            try
            {
                var startingIndexToTruncate = this.Head/ChunkSize;
                var endingIndexToTruncate = streamOffset/ChunkSize;

                Utility.Assert(
                    startingIndexToTruncate <= endingIndexToTruncate,
                    "Invalid StreamOffset during truncatehead {0}. Head = {1}. Tail = {2}",
                    streamOffset, this.Head, this.Tail);

                Interlocked.Exchange(ref this.head, streamOffset);

                if (startingIndexToTruncate == endingIndexToTruncate)
                {
                    // nothing to clean up as there are no byte arrays before the current head array
                    return;
                }

                do
                {
                    ByteArray byteArray;
                    Utility.Assert(
                        this.Bytes.TryRemove(startingIndexToTruncate, out byteArray),
                        "Failed to remove index {0} from concurrent dictionary",
                        startingIndexToTruncate);

                    this.bufferPool.Return(byteArray);

                    startingIndexToTruncate += 1;
                } while (startingIndexToTruncate < endingIndexToTruncate);
            }
            catch (Exception e)
            {
                Utility.Assert(
                    false,
                    "Unexpected exception {0} {1} {2} in TruncateHead while truncating to {3}. {4}",
                    e.Message, e.Message, e.StackTrace, streamOffset, this.GetState());
            }
        }

        public Task TruncateTail(long streamOffset, CancellationToken cancellationToken)
        {
            // TODO: Implement reverse logic of truncatehead
            // Good test exercise for new hire
            // since false progress never happens, we can ignore this case for now
            throw new NotImplementedException();
        }

        public Task WaitBufferFullNotificationAsync(CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        public Task WaitCapacityNotificationAsync(uint percentOfSpaceUsed, CancellationToken cancellationToken)
        {
            throw new NotImplementedException();
        }

        internal static Task<ILogicalLog> TestCreateMemoryLogicalLog(long startingPosition, int maxSize)
        {
            var created = new MemoryLogicalLog(startingPosition, maxSize);
            created.Open();

            return Task.FromResult(created as ILogicalLog);
        }

        private static long GetMultipleOfXGreaterThanY(long x, long y)
        {
            var quotient = y/x;
            return (quotient + 1)*x;
        }

        private string GetState()
        {
            return string.Format(CultureInfo.CurrentCulture, "Head = {0}, Tail = {1}, ArraySize = {2}", this.Head, this.Tail, this.Bytes.Count);
        }

        private int InternalRead(byte[] buffer, long readPosition, int offset, int count)
        {
            try
            {
                // Trying to read before the head
                if (readPosition < Interlocked.Read(ref this.head))
                {
                    return 0;
                }

                if (Interlocked.Read(ref this.head) + this.Length <= readPosition)
                {
                    return 0;
                }

                var readNumberOfBytes = count;

                // Stream issues larger reads than user's request. Handle the case where read is beyond end of stream
                if (Interlocked.Read(ref this.head) + this.Length < readPosition + count)
                {
                    var numberOfBytesToSkipRead = checked((int) (readPosition - this.Head));
                    readNumberOfBytes = checked((int) (this.Length - numberOfBytesToSkipRead));
                }

                // first find the index of the segment that we need to start reading from
                var indexToStartReadFrom = readPosition/ChunkSize;

                // Start reading from the current array and go upto the end depending on the count needed
                var offsetToReadFrom = checked((int) (readPosition%ChunkSize));

                if (GetMultipleOfXGreaterThanY(x: ChunkSize, y: offsetToReadFrom) - offsetToReadFrom
                    >= readNumberOfBytes)
                {
                    // Most common case where read is small and is serviced by 1 array
                    Buffer.BlockCopy(
                        this.Bytes[indexToStartReadFrom].Array,
                        offsetToReadFrom,
                        buffer,
                        offset,
                        readNumberOfBytes);
                    return readNumberOfBytes;
                }

                // This section of the code is for the more uncommon scenario where a single read spans multiple byte arrays.
                // Do the first read
                var bytesToRead =
                    checked((int) (GetMultipleOfXGreaterThanY(x: ChunkSize, y: offsetToReadFrom) - offsetToReadFrom));
                Buffer.BlockCopy(this.Bytes[indexToStartReadFrom].Array, offsetToReadFrom, buffer, offset, bytesToRead);
                var readFinishedCount = bytesToRead;
                indexToStartReadFrom += 1;

                // Perform the subsequent reads 
                do
                {
                    if (readNumberOfBytes - readFinishedCount > ChunkSize)
                    {
                        bytesToRead = ChunkSize;
                    }
                    else
                    {
                        bytesToRead = readNumberOfBytes - readFinishedCount;
                    }

                    Buffer.BlockCopy(
                        this.Bytes[indexToStartReadFrom].Array,
                        0,
                        buffer,
                        offset + readFinishedCount,
                        bytesToRead);
                    readFinishedCount += bytesToRead;
                    indexToStartReadFrom += 1;
                } while (readFinishedCount < readNumberOfBytes);

                Utility.Assert(
                    readFinishedCount == readNumberOfBytes,
                    "ReadFinished = {0} is not equal to Read Requested count = {1}",
                    readFinishedCount, readNumberOfBytes);

                return readNumberOfBytes;
            }
            catch (Exception e)
            {
                Utility.Assert(
                    false,
                    "Unexpected exception {0} {1} {2} in InternalRead at offset {3} for length {4}. {5}",
                    e.Message, e.Message, e.StackTrace, readPosition, count, this.GetState());

                throw;
            }
        }

        private void Open()
        {
            var array = this.bufferPool.Take();
            var isSuccess = this.Bytes.TryAdd(this.Tail/ChunkSize, array);
            Utility.Assert(isSuccess, "Failed to add");
        }

        private void UpdateTail(int appendSize)
        {
            Interlocked.Add(ref this.tail, appendSize);
            if (this.Tail%ChunkSize == 0)
            {
                var array = this.bufferPool.Take();
                Utility.Assert(
                    this.Bytes.TryAdd(this.Tail/ChunkSize, array),
                    "Try add failed while adding index {0} in UpdateTail",
                    this.Tail/ChunkSize);
            }
        }

        /// <summary>
        /// Wrapper for a raw ByteArray
        /// </summary>
        internal class ByteArray : IDisposable
        {
            public ByteArray(int size)
            {
                this.Array = new byte[size];
            }

            public void Dispose()
            {
                // Nothing to do here
            }

            public byte[] Array { get; private set; }
        }

        private class MemoryLogReaderStream : Stream
        {
            private readonly MemoryLogicalLog parent;

            private long readPosition;

            public MemoryLogReaderStream(MemoryLogicalLog parent)
            {
                this.parent = parent;
                this.readPosition = 0;
            }

            public override bool CanRead
            {
                get { return true; }
            }

            public override bool CanSeek
            {
                get { return true; }
            }

            public override bool CanWrite
            {
                get { return false; }
            }

            public override long Length
            {
                get { return this.parent.Length; }
            }

            public override long Position
            {
                get { return this.readPosition; }

                set { this.Seek(value, SeekOrigin.Begin); }
            }

            public override void Flush()
            {
                throw new NotImplementedException();
            }

            public override int Read(byte[] buffer, int offset, int count)
            {
                var readBytes = this.parent.InternalRead(buffer, this.readPosition, offset, count);
                this.readPosition += readBytes;
                return readBytes;
            }

            public override long Seek(long offset, SeekOrigin origin)
            {
                switch (origin)
                {
                    case SeekOrigin.Begin:
                        this.readPosition = offset;
                        break;
                    case SeekOrigin.Current:
                        this.readPosition += offset;
                        break;
                    case SeekOrigin.End:
                        this.readPosition += this.parent.WritePosition + offset;
                        break;
                }

                return this.readPosition;
            }

            public override void SetLength(long value)
            {
                throw new NotImplementedException();
            }

            public override void Write(byte[] buffer, int offset, int count)
            {
                throw new NotImplementedException();
            }
        }
    }
}