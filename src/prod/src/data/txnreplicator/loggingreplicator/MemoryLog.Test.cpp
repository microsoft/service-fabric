// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace ktl;
    using namespace Data::Log;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;
    using namespace Common;

#define TESTMEMORYLOG_TAG 'LmMT'
    StringLiteral const TraceComponent = "MemoryLogTest";

    class MemoryLogTests
    {
    protected:
        MemoryLogTests()
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
        }

        void EndTest();

        MemoryLog::SPtr CreateMemoryLog();
        KBuffer::SPtr CreateBuffer(__in ULONG size);

        Awaitable<void> GenerateLogContentWithRandomWriteSizesAsync(
            __in MemoryLog & log,
            __in Random & random,
            __in LONG64 totalSize,
            __in LONG64 maxSingleWriteSize);

        Awaitable<LONG64> GenerateLogContentWithFixedWriteSizesAsync(
            __in MemoryLog & log,
            __in LONG64 totalSize,
            __in LONG64 maxSingleWriteSize);

        void VerifyBufferContents(
            __in KBuffer const & buffer,
            __in LONG64 offset,
            __in LONG64 count);

        Awaitable<void> Write1ByteTest();
        Awaitable<void> Write1ChunkTest();
        Awaitable<void> WriteBiggerThan1ChunkTest();
        Awaitable<void> WriteSmallerThan1ChunkTest();
        Awaitable<void> WriteMultipleHalfChunksTest();
        Awaitable<void> WriteBiggerThanDoubleChunkTest();
        Awaitable<void> Write100MBWithBigWritesTest(Random & random);
        Awaitable<void> Write100MBWithSmallWritesTest(Random & random);

        Awaitable<void> Read1ChunkTest(Random & random);
        Awaitable<void> Read2ChunksTest(Random & random);
        Awaitable<void> ReadAcross2ChunksTest(Random & random);
        Awaitable<void> ReadMoreThanAvailableTest(Random & random);

        Awaitable<void> TruncateHeadAt1ChunkTest();
        Awaitable<void> TruncateHeadWithinAChunkTest();
        Awaitable<void> TruncateHeadAtEndOfChunk1Test();
        Awaitable<void> TruncateHeadAtEndOfChunk2Test();
        Awaitable<void> TruncateHeadRandomTest(Random & random);

        Awaitable<void> TruncateTailAt1ChunkTest();
        Awaitable<void> TruncateTailWithinAChunkTest();
        Awaitable<void> TruncateTailAtStartOfChunk1Test();
        Awaitable<void> TruncateTailAtStartOfChunk2Test();
        Awaitable<void> TruncateTailRandomTest(Random & random);

        Awaitable<void> ThroughputPerfTest(__in ULONG chunkSize, __in ULONG writeSize, __in ULONG64 totalSize);
        Awaitable<void> ReadPerfTest(__in LONG64 chunkSize, __in LONG64 readSize, __in LONG64 totalSize);

        Awaitable<KBuffer::SPtr> ReadTask(
            int const taskId,
            MemoryLog & memoryLog, 
            Random & random,
            ULONG64 const offset, 
            ULONG64 const count);
        Awaitable<void> ReadConcurrentTest(Random & random);

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void MemoryLogTests::EndTest()
    {
        prId_.Reset();
    }

    MemoryLog::SPtr MemoryLogTests::CreateMemoryLog()
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

        MemoryLog::SPtr logSPtr = nullptr;
        NTSTATUS status = MemoryLog::Create(*prId_, allocator, logSPtr);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return logSPtr;
    }

    KBuffer::SPtr MemoryLogTests::CreateBuffer(__in ULONG size)
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

        KBuffer::SPtr result = nullptr;
        NTSTATUS status = KBuffer::Create(size, result, allocator, TESTMEMORYLOG_TAG);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        return result;
    }

    Awaitable<void> MemoryLogTests::GenerateLogContentWithRandomWriteSizesAsync(
        __in MemoryLog & log,
        __in Random & random,
        __in LONG64 totalSize,
        __in LONG64 maxSingleWriteSize)
    {
        byte value = 0;

        LONG64 bytesWritten = 0;
        while (bytesWritten < totalSize)
        {
            LONG64 bytesRemaining = totalSize - bytesWritten;
            LONG64 requestSize = random.Next(1, static_cast<int>(maxSingleWriteSize));
            LONG64 writeSize = bytesRemaining < requestSize ? bytesRemaining : requestSize;

            CODING_ERROR_ASSERT(writeSize >= 0);

            KBuffer::SPtr buffer = CreateBuffer(static_cast<ULONG>(writeSize));

            byte* bytes = static_cast<byte *>(buffer->GetBuffer());
            for (ULONG32 i = 0; i < buffer->QuerySize(); i++)
            {
                bytes[i] = value++;
            }

            co_await log.AppendAsync(*buffer, 0, static_cast<ULONG>(writeSize));
            bytesWritten += writeSize;
        }
    }

    Awaitable<LONG64> MemoryLogTests::GenerateLogContentWithFixedWriteSizesAsync(
        __in MemoryLog & log,
        __in LONG64 totalSize,
        __in LONG64 maxSingleWriteSize)
    {
        byte value = 0;
        LONG64 bytesWritten = 0;

        Stopwatch writeTime;

        while (bytesWritten < totalSize)
        {
            LONG64 bytesRemaining = totalSize - bytesWritten;
            LONG64 writeSize = bytesRemaining < maxSingleWriteSize ? bytesRemaining : maxSingleWriteSize;

            CODING_ERROR_ASSERT(writeSize >= 0);

            KBuffer::SPtr buffer = CreateBuffer(static_cast<ULONG>(writeSize));

            byte* bytes = static_cast<byte *>(buffer->GetBuffer());
            for (ULONG32 i = 0; i < buffer->QuerySize(); i++)
            {
                bytes[i] = value++;
            }

            writeTime.Start();
            co_await log.AppendAsync(*buffer, 0, static_cast<ULONG>(writeSize));
            writeTime.Stop();

            bytesWritten += writeSize;
        }

        co_return writeTime.ElapsedTicks;
    }

    void MemoryLogTests::VerifyBufferContents(
        __in KBuffer const & buffer,
        __in LONG64 offset,
        __in LONG64 count)
    {
        const byte* bytes = static_cast<const byte *>(buffer.GetBuffer());

        byte value = 0;
        for (LONG64 i = offset; i < offset + count; i++)
        {
            byte actualValue = bytes[i];
            byte expectedValue = value++;
            VERIFY_ARE_EQUAL(expectedValue, actualValue);
        }
    }

    Awaitable<void> MemoryLogTests::ThroughputPerfTest(
        __in ULONG chunkSize,
        __in ULONG readWriteSize,
        __in ULONG64 totalSize)
    {
        MemoryLog::ChunkSize = chunkSize;
        MemoryLog::SPtr logSPtr = CreateMemoryLog();

        LONG64 ticks = co_await GenerateLogContentWithFixedWriteSizesAsync(*logSPtr, totalSize, readWriteSize);
        LONG64 writeTimeMS = Stopwatch::ConvertTicksToMilliseconds(ticks);

        Stopwatch readWatch;
        ILogicalLogReadStream::SPtr reader = nullptr;
        logSPtr->CreateReadStream(reader, 0);

        ULONG bytesRead = 0;
        do
        {
            KBuffer::SPtr readBuffer = CreateBuffer(static_cast<ULONG>(readWriteSize));

            readWatch.Start();
            co_await reader->ReadAsync(*readBuffer, bytesRead, 0, readBuffer->QuerySize());
            readWatch.Stop();
        } while (bytesRead != 0);

        LONG64 readTimeMS = Stopwatch::ConvertTicksToMilliseconds(readWatch.ElapsedTicks);

        Trace.WriteWarning(
            TraceComponent,
            "Throughput test. TotalSize={0} MB; WriteSize={1} bytes; ChunkSize={2} KB;\r\n\tNumChunks={3}\r\n\tNumWrites={4}\r\n\tWriteTime={5} ms\r\n\tWrite Throughput={6} MB/sec\r\n\tRead Time={7} ms\r\n\tRead Throughput={8} MB/s",
            totalSize >> 20, readWriteSize, chunkSize >> 10,
            logSPtr->Chunks->Count,
            totalSize / readWriteSize,
            writeTimeMS,
            ((1000 * totalSize) / writeTimeMS) >> 20,
            readTimeMS,
            ((1000 * totalSize) / readTimeMS) >> 20);
        
    }

    Awaitable<void> MemoryLogTests::Write1ByteTest()
    {
        auto memoryLog = CreateMemoryLog();

        auto buffer = CreateBuffer(1);

        co_await memoryLog->AppendAsync(*buffer, 0, 1);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(1, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::Write1ChunkTest()
    {
        auto memoryLog = CreateMemoryLog();
        auto buffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize));

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::WriteBiggerThan1ChunkTest()
    {
        auto memoryLog = CreateMemoryLog();
        auto buffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize + 1));

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize + 1, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::WriteSmallerThan1ChunkTest()
    {
        auto memoryLog = CreateMemoryLog();
        auto buffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize - 1));

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::WriteMultipleHalfChunksTest()
    {
        auto memoryLog = CreateMemoryLog();
        auto buffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize / 2));

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize / 2, memoryLog->Tail);

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize + MemoryLog::ChunkSize / 2, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::WriteBiggerThanDoubleChunkTest()
    {
        auto memoryLog = CreateMemoryLog();
        auto buffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize * 2 + 1));

        co_await memoryLog->AppendAsync(*buffer, 0, buffer->QuerySize());

        VERIFY_ARE_EQUAL(3, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize * 2 + 1, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::Write100MBWithBigWritesTest(Random & random)
    {
        ULONG64 targetLogSize = 100 * 1024 * 1024;
        ULONG maxWriteSize = 1024 * 1024;

        auto memoryLog = CreateMemoryLog();
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, maxWriteSize);

        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Tail);

        co_return;
    }

    Awaitable<void> MemoryLogTests::Write100MBWithSmallWritesTest(Random & random)
    {
        ULONG64 targetLogSize = 100 * 1024 * 1024;
        ULONG maxWriteSize = 1024;

        auto memoryLog = CreateMemoryLog();
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, maxWriteSize);

        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Tail);

        co_return;
    }

    Awaitable<void> MemoryLogTests::Read1ChunkTest(Random & random)
    {
        LONG64 targetLogSize = MemoryLog::ChunkSize;
        LONG64 maxWriteSize = 1024;

        auto memoryLog = CreateMemoryLog();
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, maxWriteSize);

        ULONG offset = 100;
        KBuffer::SPtr readBuffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize + offset));
        
        ILogicalLogReadStream::SPtr readStream = nullptr;
        memoryLog->CreateReadStream(readStream, 0);

        ULONG bytesRead = 0;
        NTSTATUS status = co_await readStream->ReadAsync(*readBuffer, bytesRead, offset, static_cast<ULONG>(MemoryLog::ChunkSize));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, bytesRead);
        VerifyBufferContents(*readBuffer, offset, MemoryLog::ChunkSize);
    }

    Awaitable<void> MemoryLogTests::Read2ChunksTest(Random & random)
    {
        LONG64 maxWriteSize = 1024;

        auto memoryLog = CreateMemoryLog();

        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize, maxWriteSize);
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize, maxWriteSize);

        KBuffer::SPtr readBuffer = CreateBuffer(static_cast<ULONG>(2 * MemoryLog::ChunkSize));
        
        ILogicalLogReadStream::SPtr readStream = nullptr;
        memoryLog->CreateReadStream(readStream, 0);

        ULONG bytesRead = 0;
        NTSTATUS status = co_await readStream->ReadAsync(*readBuffer, bytesRead, 0, static_cast<ULONG>(2 * MemoryLog::ChunkSize));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        VERIFY_ARE_EQUAL(2 * MemoryLog::ChunkSize, bytesRead);
        VerifyBufferContents(*readBuffer, 0, MemoryLog::ChunkSize);
        VerifyBufferContents(*readBuffer, MemoryLog::ChunkSize, MemoryLog::ChunkSize);
    }

    Awaitable<void> MemoryLogTests::ReadAcross2ChunksTest(Random & random)
    {
        LONG64 maxWriteSize = 1024;

        auto memoryLog = CreateMemoryLog();

        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize/2, maxWriteSize);
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize, maxWriteSize);
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize/2, maxWriteSize);

        KBuffer::SPtr readBuffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize));
        
        ILogicalLogReadStream::SPtr readStream = nullptr;
        memoryLog->CreateReadStream(readStream, 0);

        ULONG bytesRead = 0;
        readStream->Position = MemoryLog::ChunkSize / 2;
        NTSTATUS status = co_await readStream->ReadAsync(*readBuffer, bytesRead, 0, static_cast<ULONG>(MemoryLog::ChunkSize));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, bytesRead);
        VerifyBufferContents(*readBuffer, 0, MemoryLog::ChunkSize);
    }

    Awaitable<void> MemoryLogTests::ReadMoreThanAvailableTest(Random & random)
    {
        LONG64 maxWriteSize = 1024;

        auto memoryLog = CreateMemoryLog();

        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize, maxWriteSize);
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, MemoryLog::ChunkSize * 10, maxWriteSize);

        KBuffer::SPtr readBuffer = CreateBuffer(static_cast<ULONG>(MemoryLog::ChunkSize * 12));
        
        ILogicalLogReadStream::SPtr readStream = nullptr;
        memoryLog->CreateReadStream(readStream, 0);

        ULONG bytesRead = 0;
        readStream->Position = MemoryLog::ChunkSize / 2;
        NTSTATUS status = co_await readStream->ReadAsync(*readBuffer, bytesRead, 0, static_cast<ULONG>(MemoryLog::ChunkSize * 12));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        VERIFY_ARE_EQUAL(11 * MemoryLog::ChunkSize - MemoryLog::ChunkSize / 2, bytesRead);
        VerifyBufferContents(*readBuffer, 0, MemoryLog::ChunkSize/2);
        VerifyBufferContents(*readBuffer, MemoryLog::ChunkSize/2, MemoryLog::ChunkSize * 10);
    }

    Awaitable<KBuffer::SPtr> MemoryLogTests::ReadTask(
        int const taskId,
        MemoryLog & memoryLog, 
        Random & random,
        ULONG64 const startOffset, 
        ULONG64 const count)
    {
        co_await CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        KBuffer::SPtr buffer = CreateBuffer(static_cast<ULONG>(count));
        ULONG64 totalBytesRead = 0;

        ILogicalLogReadStream::SPtr readStream = nullptr;
        memoryLog.CreateReadStream(readStream, 0);

        readStream->Position = startOffset;
        VERIFY_IS_TRUE(readStream->Position >= 0);
        VERIFY_ARE_EQUAL(startOffset, static_cast<ULONGLONG const>(readStream->Position));
        
        Trace.WriteInfo(
            TraceComponent,
            "{0} taskId {1} startOffset {2} count {3}",
            prId_->TraceId,
            taskId,
            startOffset,
            count);

        while (totalBytesRead < count)
        {
            LONG64 readSize = random.Next(1, static_cast<int>(MemoryLog::ChunkSize));
            if (totalBytesRead + readSize > count)
            {
                readSize = count - totalBytesRead;
            }

            ULONG bytesRead;
            co_await readStream->ReadAsync(*buffer, bytesRead, static_cast<ULONG>(totalBytesRead), static_cast<ULONG>(readSize));

            VERIFY_ARE_EQUAL(readSize, bytesRead);
            totalBytesRead += bytesRead;
        }

        co_return buffer;
    }

    Awaitable<void> MemoryLogTests::ReadConcurrentTest(Random & random)
    {
        auto memoryLog = CreateMemoryLog();
        
        LONG64 targetLogSize = MemoryLog::ChunkSize * 1000;
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, MemoryLog::ChunkSize * 2);

        ULONG32 numTasks = 4;
        KArray<Awaitable<KBuffer::SPtr>> tasks = KArray<Awaitable<KBuffer::SPtr>>(underlyingSystem_->NonPagedAllocator(), numTasks);
        Trace.WriteWarning(TraceComponent, "{0} - targetLogSize: {1}, chunkCount: {2}, size: {3}", prId_->TraceId, targetLogSize, memoryLog->Chunks->Count, memoryLog->Size);

        for (ULONG32 i = 0; i < numTasks; i++)
        {
            tasks.Append(ReadTask(i, *memoryLog, random, i * MemoryLog::ChunkSize, targetLogSize - i * MemoryLog::ChunkSize));
        }

        for (ULONG32 i = 0; i < numTasks; i++)
        {
            auto buffer = co_await tasks[i];
            VerifyBufferContents(*buffer, 0, targetLogSize - i * MemoryLog::ChunkSize);
        }
    }

    Awaitable<void> MemoryLogTests::TruncateHeadAt1ChunkTest()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize;
        LONG64 writeSize = 1024;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);

        co_await memoryLog->TruncateHead(MemoryLog::ChunkSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateHeadWithinAChunkTest()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 100;
        LONG64 writeSize = 1024;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 100, memoryLog->Tail);

        co_await memoryLog->TruncateHead(100);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(100, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 100, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateHeadAtEndOfChunk1Test()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 1;
        LONG64 writeSize = 100;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        co_await memoryLog->TruncateHead(MemoryLog::ChunkSize - 2);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 2, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        KBuffer::SPtr data = CreateBuffer(1024);
        co_await memoryLog->AppendAsync(*data, 100, 1);

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 2, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);

        co_await memoryLog->TruncateHead(MemoryLog::ChunkSize);
        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateHeadAtEndOfChunk2Test()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 1;
        LONG64 writeSize = 100;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        co_await memoryLog->TruncateHead(MemoryLog::ChunkSize - 1);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        KBuffer::SPtr data = CreateBuffer(1024);
        co_await memoryLog->AppendAsync(*data, 100, 1);

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateHeadRandomTest(Random & random)
    {
        ULONG64 targetLogSize = 100LL * 1024 * 1024;
        auto maxWriteSize = 100;

        auto memoryLog = CreateMemoryLog();
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, maxWriteSize);

        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Tail);

        while (memoryLog->Head < memoryLog->Tail)
        {
            LONG64 truncateSize = random.Next(1, static_cast<int>(MemoryLog::ChunkSize * 15));
            if (truncateSize + memoryLog->Head > memoryLog->Tail)
            {
                truncateSize = memoryLog->Tail - memoryLog->Head;
            }

            co_await memoryLog->TruncateHead(memoryLog->Head + truncateSize);
        }

        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Head);
        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Tail);
        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
    }

    Awaitable<void> MemoryLogTests::TruncateTailAt1ChunkTest()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize;
        LONG64 writeSize = 1024;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(2, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize, memoryLog->Tail);

        co_await memoryLog->TruncateTail(0);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(0, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateTailWithinAChunkTest()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 100;
        LONG64 writeSize = 1024;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 100, memoryLog->Tail);

        co_await memoryLog->TruncateTail(memoryLog->Tail - 100);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 200, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateTailAtStartOfChunk1Test()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 1;
        LONG64 writeSize = 100;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        co_await memoryLog->TruncateTail(1);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(1, memoryLog->Tail);

        co_await memoryLog->TruncateTail(0);
        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(0, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateTailAtStartOfChunk2Test()
    {
        auto memoryLog = CreateMemoryLog();

        LONG64 targetLogSize = MemoryLog::ChunkSize - 1;
        LONG64 writeSize = 100;

        co_await GenerateLogContentWithFixedWriteSizesAsync(*memoryLog, targetLogSize, writeSize);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(MemoryLog::ChunkSize - 1, memoryLog->Tail);

        co_await memoryLog->TruncateTail(1);

        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(1, memoryLog->Tail);
    }

    Awaitable<void> MemoryLogTests::TruncateTailRandomTest(Random & random)
    {
        ULONG64 targetLogSize = 100LL * 1024 * 1024;
        auto maxWriteSize = 100;

        auto memoryLog = CreateMemoryLog();
        co_await GenerateLogContentWithRandomWriteSizesAsync(*memoryLog, random, targetLogSize, maxWriteSize);

        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(targetLogSize, memoryLog->Tail);

        while (memoryLog->Tail > memoryLog->Head)
        {
            ULONG64 maxTruncateSize = __min(MemoryLog::ChunkSize * 15, memoryLog->Tail - memoryLog->Head);

            LONGLONG truncateSize = random.Next(1, static_cast<int>(maxTruncateSize));

            co_await memoryLog->TruncateTail(memoryLog->Tail - truncateSize);
        }

        VERIFY_ARE_EQUAL(0, memoryLog->Head);
        VERIFY_ARE_EQUAL(0, memoryLog->Tail);
        VERIFY_ARE_EQUAL(1, memoryLog->Chunks->Count);
    }

    BOOST_FIXTURE_TEST_SUITE(MemoryLogTestsSuite, MemoryLogTests)

    BOOST_AUTO_TEST_CASE(Write1Byte)
    {
        TEST_TRACE_BEGIN("Write1Byte")
        {
            SyncAwait(Write1ByteTest());
        }
    }

    BOOST_AUTO_TEST_CASE(Write1Chunk)
    {
        TEST_TRACE_BEGIN("Write1Chunk")
        {
            SyncAwait(Write1ChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(WriteBiggerThan1Chunk)
    {
        TEST_TRACE_BEGIN("WriteBiggerThan1Chunk")
        {
            SyncAwait(WriteBiggerThan1ChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(WriteSmallerThan1Chunk)
    {
        TEST_TRACE_BEGIN("WriteSmallerThan1Chunk")
        {
            SyncAwait(WriteSmallerThan1ChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(WriteMultipleHalfChunks)
    {
        TEST_TRACE_BEGIN("WriteMultipleHalfChunks")
        {
            SyncAwait(WriteMultipleHalfChunksTest());
        }
    }

    BOOST_AUTO_TEST_CASE(WriteBiggerThanDoubleChunk)
    {
        TEST_TRACE_BEGIN("WriteBiggerThanDoubleChunk")
        {
            SyncAwait(WriteBiggerThanDoubleChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(Write100MBWithBigWrites)
    {
        TEST_TRACE_BEGIN("Write100MBWithBigWrites")
        {
            SyncAwait(Write100MBWithBigWritesTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(Write100MBWithSmallWrites)
    {
        TEST_TRACE_BEGIN("Write100MBWithSmallWrites")
        {
            SyncAwait(Write100MBWithSmallWritesTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(Read1Chunk)
    {
        TEST_TRACE_BEGIN("Read1Chunk")
        {
            SyncAwait(Read1ChunkTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(Read2Chunks)
    {
        TEST_TRACE_BEGIN("Read2Chunks")
        {
            SyncAwait(Read2ChunksTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(ReadAcross2Chunks)
    {
        TEST_TRACE_BEGIN("ReadAcross2Chunks")
        {
            SyncAwait(ReadAcross2ChunksTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(ReadMoreThanAvailable)
    {
        TEST_TRACE_BEGIN("ReadMoreThanAvailable")
        {
            SyncAwait(ReadMoreThanAvailableTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(ReadConcurrent)
    {
        TEST_TRACE_BEGIN("ReadConcurrent")
        {
            SyncAwait(ReadConcurrentTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadAt1Chunk)
    {
        TEST_TRACE_BEGIN("TruncateHeadAt1Chunk")
        {
            SyncAwait(TruncateHeadAt1ChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadWithinAChunk)
    {
        TEST_TRACE_BEGIN("TruncateHeadWithinAChunk")
        {
            SyncAwait(TruncateHeadWithinAChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadAtEndOfChunk1)
    {
        TEST_TRACE_BEGIN("TruncateHeadAtEndOfChunk1")
        {
            SyncAwait(TruncateHeadAtEndOfChunk1Test());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadAtEndOfChunk2)
    {
        TEST_TRACE_BEGIN("TruncateHeadAtEndOfChunk2")
        {
            SyncAwait(TruncateHeadAtEndOfChunk2Test());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateHeadRandom)
    {
        TEST_TRACE_BEGIN("TruncateHeadRandom")
        {
            SyncAwait(TruncateHeadRandomTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailAt1Chunk)
    {
        TEST_TRACE_BEGIN("TruncateTailAt1Chunk")
        {
            SyncAwait(TruncateTailAt1ChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailWithinAChunk)
    {
        TEST_TRACE_BEGIN("TruncateTailWithinAChunk")
        {
            SyncAwait(TruncateTailWithinAChunkTest());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailAtStartOfChunk1)
    {
        TEST_TRACE_BEGIN("TruncateTailAtStartOfChunk1")
        {
            SyncAwait(TruncateTailAtStartOfChunk1Test());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailAtStartOfChunk2)
    {
        TEST_TRACE_BEGIN("TruncateTailAtStartOfChunk2")
        {
            SyncAwait(TruncateTailAtStartOfChunk2Test());
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTailRandom)
    {
        TEST_TRACE_BEGIN("TruncateTailRandom")
        {
            SyncAwait(TruncateTailRandomTest(r));
        }
    }

    BOOST_AUTO_TEST_CASE(ReadWriteThroughputTest, *boost::unit_test_framework::disabled())
    {
        TEST_TRACE_BEGIN("ReadWriteThroughputTest")
        {
            SyncAwait(ThroughputPerfTest(
                64 * 1024, 
                1024, 
                4LL * 1024 * 1024 * 1024));

            SyncAwait(ThroughputPerfTest(
                64 * 1024, 
                1024 * 1024, 
                4LL * 1024 * 1024 * 1024));

            SyncAwait(ThroughputPerfTest(
                1024 * 1024, 
                1024, 
                4LL * 1024 * 1024 * 1024));

            SyncAwait(ThroughputPerfTest(
                1024 * 1024, 
                1024 * 1024, 
                4LL * 1024 * 1024 * 1024));
        }
    }


    BOOST_AUTO_TEST_SUITE_END()
}
