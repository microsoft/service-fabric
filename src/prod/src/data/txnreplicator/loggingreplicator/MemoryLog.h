// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // Memory log implementation of the ILogicalLog interface
        // Memory buffers are allocated in multiple of "CHUNK_SIZE" which is 64K by default
        //
        // This log is NOT thread safe for a general purpose usage
        // However, in the context of the replicator's usage of this log, it is safe to do multiple reads from the replicators, while performing writes.
        //      1. There is only 1 AppendAsync() or FlushAsync() or TruncateHead() invoked at any time (Write)
        //      2. Any Read call is guaranteed to be at an offset greater than head of the log's position and offset lesser than tail of the log's position
        //      3. Reads and writes can happen concurrently with the above 2 limitations
        //
        // Due to the above guarantees, this log can be "lock free", but still requires a concurrent dictionary to keep track of all the buffers to be able to index into a
        // memory offset very quickly (in constant time). In other words, writes to the same chunk do not needed need to be protected, and the concurrent dictionary (which uses
        // locks internally) will protect adding and removing chunks (Append and Truncate)
        //
        // Additionally, since TruncateHead can happen while a read is going on, we need a thread safe data structure to keep track of the buffers
        // 
        // Finally, memory is freed up only in multiples of CHUNKs
        class MemoryLog
            : public KObject<MemoryLog>
            , public KShared<MemoryLog>
            , public Log::ILogicalLog
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(MemoryLog)
            K_SHARED_INTERFACE_IMP(ILogicalLog)
            K_SHARED_INTERFACE_IMP(IDisposable)

        private:
            class ReadStream;

        public:
            static ULONG ChunkSize; // Exposed for testability, defaults to 64K 

            static NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KAllocator& allocator,
                __out MemoryLog::SPtr & memoryLogicalLog);

            VOID Dispose() override;

            __declspec(property(get = get_Head)) ULONG64 Head;
            ULONG64 get_Head() const noexcept
            {
                return head_;
            }

            __declspec(property(get = get_Tail)) ULONG64 Tail;
            ULONG64 get_Tail() const noexcept
            {
                return tail_;
            }

            // Exposed for testability
            __declspec(property(get = get_Chunks)) ConcurrentHashTable<ULONG, KBuffer::SPtr>::SPtr Chunks;
            ConcurrentHashTable<ULONG, KBuffer::SPtr>::SPtr get_Chunks() const noexcept
            {
                return chunksSPtr_;
            }

        public:
#pragma region ILogicalLog
            ktl::Awaitable<NTSTATUS> CloseAsync(__in ktl::CancellationToken const & cancellationToken) override;

            void Abort() override;
            BOOLEAN GetIsFunctional() override;
            LONGLONG GetLength() const override;
            LONGLONG GetWritePosition() const override;
            LONGLONG GetReadPosition() const override;
            LONGLONG GetHeadTruncationPosition() const override;
            LONGLONG GetMaximumBlockSize() const override;
            ULONG GetMetadataBlockHeaderSize() const override;
            ULONGLONG GetSize() const override;
            ULONGLONG GetSpaceRemaining() const override;

            NTSTATUS CreateReadStream(__out Log::ILogicalLogReadStream::SPtr& Stream, __in LONG SequentialAccessReadSize) override;
            void SetSequentialAccessReadSize(__in Log::ILogicalLogReadStream& LogStream, __in LONG SequentialAccessReadSize) override;

            ktl::Awaitable<NTSTATUS> ReadAsync(
                __out LONG& BytesRead,
                __in KBuffer& Buffer,
                __in ULONG Offset,
                __in ULONG Count,
                __in ULONG BytesToRead,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            NTSTATUS SeekForRead(__in LONGLONG Offset, __in Common::SeekOrigin::Enum Origin) override;

            ktl::Awaitable<NTSTATUS> AppendAsync(
                __in KBuffer const & Buffer,
                __in LONG OffsetIntoBuffer,
                __in ULONG Count,
                __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            ktl::Awaitable<NTSTATUS> FlushWithMarkerAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;

            ktl::Awaitable<NTSTATUS> FlushAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> TruncateHead(__in LONGLONG StreamOffset) override;
            ktl::Awaitable<NTSTATUS> TruncateTail(__in LONGLONG StreamOffset, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitCapacityNotificationAsync(__in ULONG PercentOfSpaceUsed, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> WaitBufferFullNotificationAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToOnlyDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> ConfigureWritesToSharedAndDedicatedLogAsync(__in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
            ktl::Awaitable<NTSTATUS> QueryLogUsageAsync(__out ULONG& PercentUsage, __in ktl::CancellationToken const & cancellationToken = ktl::CancellationToken::None) override;
#pragma endregion

        private:

            MemoryLog(__in Data::Utilities::PartitionedReplicaId const & traceId);
            ULONG64 head_;
            ULONG64 tail_;

            ConcurrentHashTable<ULONG, KBuffer::SPtr>::SPtr chunksSPtr_;
        private:
            // Moves the tail forward by appendSize, allocating a new chunk if necessary. This is not thread-safe
            NTSTATUS UpdateTail(__in ULONG appendSize);

            // Copies data from the input buffer to the last chunk of the log. Assumes there are no other concurrent writes to the same chunk
            void CopyFromBufferToTailChunk(__in KBuffer const & buffer, __in ULONG offset, __in ULONG count);

            // Reads data from the log into the input buffer. Assumes there are no concurrent writes to the chunks being read
            ULONG InternalRead(__in KBuffer & buffer, __in ULONG64 readPosition, __in ULONG offset, __in ULONG count);

            // Copies data from a single chunk in the log to the input buffer. Assumes there are no concurrent writes to the same chunk
            void CopyToBufferFromChunk(
                __in KBuffer & buffer, 
                __in ULONG offset, 
                __in ULONG chunkIndex,
                __in ULONG chunkStartPosition,
                __in ULONG count);

            // Gets the end offset for the chunk the given offset is in
            static ULONG64 GetChunkEndOffset(ULONG64 offset);
        };
    }
}