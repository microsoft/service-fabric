// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace TStoreTests
{
    using namespace ktl;

    class MemoryBufferTest
    {
    public:
        MemoryBufferTest()
        {
            NTSTATUS status = KtlSystem::Initialize(FALSE, &ktlSystem_);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            ktlSystem_->SetStrictAllocationChecks(TRUE);
        }

        ~MemoryBufferTest()
        {
            ktlSystem_->Shutdown();
        }

        KAllocator& GetAllocator()
        {
            return ktlSystem_->NonPagedAllocator();
        }

        bool BufferEquals(
            __in KBuffer::SPtr const & one, 
            __in KBuffer::SPtr const & two,
            __in ULONG count,
            __in ULONG offsetOne = 0,
            __in ULONG offsetTwo = 0)
        {
            if (one == nullptr || two == nullptr)
            {
                return one == two;
            }

            auto oneBytes = (byte *)one->GetBuffer();
            auto twoBytes = (byte *)two->GetBuffer();

            for (ULONG32 i = 0; i < count; i++)
            {
                if (oneBytes[i + offsetOne] != twoBytes[i + offsetTwo])
                {
                    return false;
                }
            }
            return true;
        }
        
        void WriteAsyncPositiveTest(__in MemoryBuffer & stream, __in ULONG sourceSize, __in ULONG sourceOffset, __in ULONG sourceCount, __in ULONG expectedNumBytesWritten)
        {
            KBuffer::SPtr sourceBufferSPtr;
            auto status = CreateBufferAndWriteToStream(stream, sourceSize, sourceOffset, sourceCount, sourceBufferSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            CODING_ERROR_ASSERT(BufferEquals(stream.GetBuffer(), sourceBufferSPtr, expectedNumBytesWritten, 0, sourceOffset));
        }

        void WriteAsyncNegativeTest(__in MemoryBuffer & stream, __in ULONG sourceSize, __in ULONG sourceOffset, __in ULONG sourceCount)
        {
            KBuffer::SPtr sourceBufferSPtr;
            auto status = CreateBufferAndWriteToStream(stream, sourceSize, sourceOffset, sourceCount, sourceBufferSPtr);
            CODING_ERROR_ASSERT(!NT_SUCCESS(status));
        }

        void ReadAsyncPositiveTest(
            __in MemoryBuffer & stream,
            __in ULONG sourceSize,
            __in ULONG destinationSize,
            __in ULONG destinationOffset,
            __in ULONG numBytesToRead,
            __in ULONG expectedNumBytesRead)
        {
            KBuffer::SPtr srcBufferSPtr = nullptr;
            CreateBufferAndWriteToStream(stream, sourceSize, 0, 0, srcBufferSPtr);

            // Reset the position to the beginning of the stream
            stream.Position = 0;

            KBuffer::SPtr destBufferSPtr = nullptr;
            auto status = KBuffer::Create(destinationSize, destBufferSPtr, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            ULONG actualNumBytesRead;
            status = SyncAwait(stream.ReadAsync(*destBufferSPtr, actualNumBytesRead, destinationOffset, numBytesToRead));

            CODING_ERROR_ASSERT(actualNumBytesRead == expectedNumBytesRead);
            CODING_ERROR_ASSERT(BufferEquals(srcBufferSPtr, destBufferSPtr, actualNumBytesRead, 0, destinationOffset));
        }
    
        NTSTATUS CreateBufferAndWriteToStream(
            __in MemoryBuffer & stream, 
            __in ULONG sourceSize, 
            __in ULONG sourceOffset, 
            __in ULONG sourceCount,
            __out KBuffer::SPtr & sourceBufferSPtr)
        {
            sourceBufferSPtr = CreateBuffer(sourceSize);
            auto status = SyncAwait(stream.WriteAsync(*sourceBufferSPtr, sourceOffset, sourceCount));
            return status;
        }

        KBuffer::SPtr CreateBuffer(__in ULONG size)
        {
            KBuffer::SPtr result;
            auto status = KBuffer::Create(size, result, GetAllocator());
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            byte* buffer = (byte *)result->GetBuffer();
            for (ULONG32 i = 0; i < size; i++)
            {
                buffer[i] = static_cast<byte>(i);
            }
            return result;
        }

    private:
        KtlSystem* ktlSystem_;

#pragma region test functions
    public:
        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 4;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_WriteCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_WriteMoreThanCapacity_ShouldFail_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 32;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);
        
            WriteAsyncNegativeTest(*streamSPtr, sourceSize, sourceOffset, sourceCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_WriteMoreThanSourceCapacity_ShouldFail_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 32; // Copy more than the buffer holds

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);
        
            WriteAsyncNegativeTest(*streamSPtr, sourceSize, sourceOffset, sourceCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_NonZeroOffset_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 8;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize - sourceOffset;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_FixedCapacityStream_Chunked_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 1024;

            KBuffer::SPtr sourceBuffer = CreateBuffer(streamCapacity);

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);

            ULONG offset = 0;
            ULONG chunkSize = 16;

            while (NT_SUCCESS(co_await streamSPtr->WriteAsync(*sourceBuffer, offset, chunkSize)))
            {
                offset += chunkSize;
            }

            CODING_ERROR_ASSERT(offset == streamCapacity);
            CODING_ERROR_ASSERT(BufferEquals(streamSPtr->GetBuffer(), sourceBuffer, streamCapacity, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 8;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer
        
            ULONG expectedCount = destinationSize - destinationOffset;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);

            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_FixedCapacityStream_FullCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 16;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer
        
            ULONG expectedCount = destinationSize - destinationOffset;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);

            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_FixedCapacityStream_MoreThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 32;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer as much as possible
        
            ULONG expectedCount = streamCapacity;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);

            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_FixedCapacityStream_Chunked_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 1024;

            KBuffer::SPtr sourceBuffer = CreateBuffer(streamCapacity);

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(streamCapacity, GetAllocator(), streamSPtr);

            auto status = co_await streamSPtr->WriteAsync(*sourceBuffer); //Write the entire source buffer into the stream
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            streamSPtr->Position = 0;

            ULONG offset = 0;
            ULONG chunkSize = 16;

            KBuffer::SPtr destinationBuffer;
            KBuffer::Create(streamCapacity, destinationBuffer, GetAllocator());

            ULONG bytesRead = 0;
            status = co_await streamSPtr->ReadAsync(*destinationBuffer, bytesRead, offset, chunkSize);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            while (bytesRead > 0)
            {
                CODING_ERROR_ASSERT(bytesRead == chunkSize);
                offset += bytesRead;
                status = co_await streamSPtr->ReadAsync(*destinationBuffer, bytesRead, offset, chunkSize);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CODING_ERROR_ASSERT(offset == streamCapacity);
            CODING_ERROR_ASSERT(BufferEquals(sourceBuffer, destinationBuffer, streamCapacity, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 4;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_WriteCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_WriteMoreThanCapacity_ShouldFail_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 32;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            WriteAsyncNegativeTest(*streamSPtr, sourceSize, sourceOffset, sourceCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_WriteMoreThanSourceCapacity_ShouldFail_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 32; // Copy more than the buffer holds

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            WriteAsyncNegativeTest(*streamSPtr, sourceSize, sourceOffset, sourceCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_NonZeroOffset_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG sourceSize = 16;
            ULONG sourceOffset = 8;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize - sourceOffset;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_Chunked_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 1024;

            KBuffer::SPtr sourceBuffer = CreateBuffer(streamCapacity);

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            ULONG offset = 0;
            ULONG chunkSize = 16;

            while (NT_SUCCESS(co_await streamSPtr->WriteAsync(*sourceBuffer, offset, chunkSize)))
            {
                offset += chunkSize;
            }

            CODING_ERROR_ASSERT(offset == streamCapacity);
            CODING_ERROR_ASSERT(BufferEquals(streamSPtr->GetBuffer(), sourceBuffer, streamCapacity, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ExistingBuffer_ByByte_ShouldSucceed_Test()
        {
            ULONG bufferSize = 1024;

            KBuffer::SPtr expectedBuffer = CreateBuffer(bufferSize);
            byte* items = static_cast<byte *>(expectedBuffer->GetBuffer());
        
            KBuffer::SPtr streamBuffer;
            KBuffer::Create(bufferSize, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);

            for (ULONG32 i = 0; i < bufferSize; i++)
            {
                byte item = items[i];
                auto status = co_await streamSPtr->WriteAsync(item);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CODING_ERROR_ASSERT(BufferEquals(streamBuffer, expectedBuffer, bufferSize, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 8;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer
        
            ULONG expectedCount = destinationSize - destinationOffset;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_ExistingBuffer_FullCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 16;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer
        
            ULONG expectedCount = destinationSize - destinationOffset;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_ExistingBuffer_MoreThanCapacity_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 16;

            ULONG destinationSize = 32;
            ULONG destinationOffset = 0;
            ULONG destinationCount = 0; // Fill entire buffer as much as possible
        
            ULONG expectedCount = streamCapacity;

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            ReadAsyncPositiveTest(*streamSPtr, streamCapacity, destinationSize, destinationOffset, destinationCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> ReadAsync_ExistingBuffer_Chunked_ShouldSucceed_Test()
        {
            ULONG streamCapacity = 1024;

            KBuffer::SPtr sourceBuffer = CreateBuffer(streamCapacity);

            KBuffer::SPtr streamBuffer;
            KBuffer::Create(streamCapacity, streamBuffer, GetAllocator());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(*streamBuffer, GetAllocator(), streamSPtr);
        
            auto status = co_await streamSPtr->WriteAsync(*sourceBuffer); //Write the entire source buffer into the stream
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            streamSPtr->Position = 0;

            ULONG offset = 0;
            ULONG chunkSize = 16;

            KBuffer::SPtr destinationBuffer;
            KBuffer::Create(streamCapacity, destinationBuffer, GetAllocator());

            ULONG bytesRead = 0;
            status = co_await streamSPtr->ReadAsync(*destinationBuffer, bytesRead, offset, chunkSize);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            while (bytesRead > 0)
            {
                CODING_ERROR_ASSERT(bytesRead == chunkSize);
                offset += bytesRead;
                status = co_await streamSPtr->ReadAsync(*destinationBuffer, bytesRead, offset, chunkSize);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CODING_ERROR_ASSERT(offset == streamCapacity);
            CODING_ERROR_ASSERT(BufferEquals(sourceBuffer, destinationBuffer, streamCapacity, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_ShouldSucceed_Test()
        {
            ULONG sourceSize = 4;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_WriteMoreThanSourceCapacity_ShouldFail_Test()
        {
            ULONG sourceSize = 16;
            ULONG sourceOffset = 0;
            ULONG sourceCount = 32; // Copy more than the buffer holds

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);
        
            WriteAsyncNegativeTest(*streamSPtr, sourceSize, sourceOffset, sourceCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_NonZeroOffset_ShouldSucceed_Test()
        {
            ULONG sourceSize = 16;
            ULONG sourceOffset = 8;
            ULONG sourceCount = 0; // Copy entire buffer

            ULONG expectedCount = sourceSize - sourceOffset;

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);
        
            WriteAsyncPositiveTest(*streamSPtr, sourceSize, sourceOffset, sourceCount, expectedCount);
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_Chunked_ShouldSucceed_Test()
        {
            ULONG bufferSize = 1024;

            KBuffer::SPtr sourceBuffer = CreateBuffer(bufferSize);

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);

            ULONG offset = 0;
            ULONG chunkSize = 16;

            while (NT_SUCCESS(co_await streamSPtr->WriteAsync(*sourceBuffer, offset, chunkSize)))
            {
                offset += chunkSize;
            }

            CODING_ERROR_ASSERT(offset == bufferSize);
            CODING_ERROR_ASSERT(BufferEquals(streamSPtr->GetBuffer(), sourceBuffer, bufferSize, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_ByByte_ShouldSucceed_Test()
        {
            ULONG bufferSize = 1024;

            KBuffer::SPtr expectedBuffer = CreateBuffer(bufferSize);
            byte* items = static_cast<byte *>(expectedBuffer->GetBuffer());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);

            for (ULONG32 i = 0; i < bufferSize; i++)
            {
                byte item = items[i];
                auto status = co_await streamSPtr->WriteAsync(item);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CODING_ERROR_ASSERT(BufferEquals(streamSPtr->GetBuffer(), expectedBuffer, bufferSize, 0, 0));
            co_return;
        }

        ktl::Awaitable<void> WriteAsync_ResizableBuffer_ByULONG32_ShouldSucceed_Test()
        {
            ULONG numItems = 1024;
            ULONG bufferSize = numItems * sizeof(ULONG32);

            KBuffer::SPtr expectedBuffer = CreateBuffer(bufferSize);
            ULONG32* items = static_cast<ULONG32 *>(expectedBuffer->GetBuffer());

            MemoryBuffer::SPtr streamSPtr;
            MemoryBuffer::Create(GetAllocator(), streamSPtr);

            for (ULONG32 i = 0; i < numItems; i++)
            {
                ULONG32 item = items[i];
                auto status = co_await streamSPtr->WriteAsync(item);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            CODING_ERROR_ASSERT(BufferEquals(streamSPtr->GetBuffer(), expectedBuffer, bufferSize, 0, 0));
            co_return;
        }
    #pragma endregion
    };

    BOOST_FIXTURE_TEST_SUITE(MemoryBufferTestSuite, MemoryBufferTest)

#pragma region Fixed capacity stream tests
    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_WriteCapacity_ShouldSucceed)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_WriteCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_WriteMoreThanCapacity_ShouldFail)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_WriteMoreThanCapacity_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_WriteMoreThanSourceCapacity_ShouldFail)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_WriteMoreThanSourceCapacity_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_NonZeroOffset_ShouldSucceed)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_NonZeroOffset_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(WriteAsync_FixedCapacityStream_Chunked_ShouldSucceed)
    {
        SyncAwait(WriteAsync_FixedCapacityStream_Chunked_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_FixedCapacityStream_LessThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_FixedCapacityStream_FullCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_FixedCapacityStream_FullCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_FixedCapacityStream_MoreThanCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_FixedCapacityStream_MoreThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_FixedCapacityStream_Chunked_ShouldSucceed)
    {
        SyncAwait(ReadAsync_FixedCapacityStream_Chunked_ShouldSucceed_Test());
    }
#pragma endregion

#pragma region Existing buffer tests
    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_WriteCapacity_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ExistingBuffer_WriteCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_WriteMoreThanCapacity_ShouldFail)
    {
        SyncAwait(WriteAsync_ExistingBuffer_WriteMoreThanCapacity_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_WriteMoreThanSourceCapacity_ShouldFail)
    {
        SyncAwait(WriteAsync_ExistingBuffer_WriteMoreThanSourceCapacity_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_NonZeroOffset_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ExistingBuffer_NonZeroOffset_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_Chunked_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ExistingBuffer_Chunked_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ExistingBuffer_ByByte_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ExistingBuffer_ByByte_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_ExistingBuffer_LessThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_ExistingBuffer_FullCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_ExistingBuffer_FullCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_ExistingBuffer_MoreThanCapacity_ShouldSucceed)
    {
        SyncAwait(ReadAsync_ExistingBuffer_MoreThanCapacity_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(ReadAsync_ExistingBuffer_Chunked_ShouldSucceed)
    {
        SyncAwait(ReadAsync_ExistingBuffer_Chunked_ShouldSucceed_Test());
    }
#pragma endregion

#pragma region Resizable buffer tests
    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ResizableBuffer_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_WriteMoreThanSourceCapacity_ShouldFail)
    {
        SyncAwait(WriteAsync_ResizableBuffer_WriteMoreThanSourceCapacity_ShouldFail_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_NonZeroOffset_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ResizableBuffer_NonZeroOffset_ShouldSucceed_Test());
    }
    
    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_Chunked_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ResizableBuffer_Chunked_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_ByByte_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ResizableBuffer_ByByte_ShouldSucceed_Test());
    }

    BOOST_AUTO_TEST_CASE(WriteAsync_ResizableBuffer_ByULONG32_ShouldSucceed)
    {
        SyncAwait(WriteAsync_ResizableBuffer_ByULONG32_ShouldSucceed_Test());
    }
#pragma endregion
    BOOST_AUTO_TEST_SUITE_END()
}
