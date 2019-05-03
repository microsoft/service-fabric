//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------
#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KStreamTests.h"
#include <CommandLineParser.h>
#include <ktlevents.um.h>
#include <KTpl.h>

#if defined(K_UseResumable)

extern volatile LONGLONG gs_AllocsRemaining;

using namespace ktl;
using namespace ktl::kasync;
using namespace ktl::kservice;
using namespace ktl::io;
using namespace ktl::test::file;

const ULONGLONG TestFileSize = 1024 * 1024; // 1mb

#define VERIFY_STREAM_POSITION(stream, expected) \
    if (stream.GetPosition() != expected) \
    { \
        KDbgPrintfInformational("Unexpected stream position.  Expected: %lld Actual: %lld", expected, stream.GetPosition()); \
        KInvariant(FALSE); \
        return STATUS_INTERNAL_ERROR; \
    }

#define VERIFY_BUFFER_SUBSTRING(buffer, offset, str) \
    { \
        int cmp = strncmp(static_cast<PCCHAR>(buffer.GetBuffer()) + offset, str, strlen(str)); \
        if (0 != cmp) \
        { \
            KDbgPrintfInformational("Invalid data read.  Buffer offset: %lu expected: %s actual: %s", offset, str, static_cast<PCCHAR>(buffer.GetBuffer()) + offset); \
            KInvariant(FALSE); \
            return STATUS_INTERNAL_ERROR; \
        } \
    }

#define VERIFY_SUCCESS(status) \
    if (!NT_SUCCESS(status)) \
    { \
        KInvariant(FALSE); \
        return status; \
    }

#define VERIFY_EQUAL_ULONG(expected, actual) \
    if (expected != actual) \
    { \
        KDbgPrintfInformational("VERIFY_EQUAL failed.  Expected: %lu Actual: %lu", expected, actual); \
        KInvariant(FALSE); \
        return STATUS_INTERNAL_ERROR; \
    }

#define VERIFY_FLUSHED_EOF(file, stream, expected) \
    { \
        LONGLONG eof; \
        status = SyncAwait(file.GetEndOfFileAsync(eof)); \
        VERIFY_SUCCESS(status); \
        \
        if (eof != expected) \
        { \
            status = STATUS_INTERNAL_ERROR; \
            KDbgPrintfInformational("Unexpected EOF.  expected %lld actual %lld", expected, eof); \
            KInvariant(FALSE); \
            return STATUS_INTERNAL_ERROR; \
        } \
        \
        if (stream.GetLength() < expected) \
        { \
            status = STATUS_INTERNAL_ERROR; \
            KDbgPrintfInformational("Unexpected stream length (less than flushed length).  expected %lld actual %lld", expected, stream.GetLength()); \
            KInvariant(FALSE); \
            return status; \
        } \
    }

#define VERIFY_CACHED_EOF(stream, expected) \
    { \
        if (stream.GetLength() != expected) \
        { \
            status = STATUS_INTERNAL_ERROR; \
            KDbgPrintfInformational("Unexpected stream length.  expected %lld actual %lld", expected, stream.GetLength()); \
            KInvariant(FALSE); \
            return status; \
        } \
    }

#define VERIFY_BUFFER_CHAR_CONTENTS(buffer, offset, length, ch) \
    { \
        for(ULONG i = 0; i < length; i++) \
        { \
            char c = static_cast<PUCHAR>(buffer.GetBuffer())[offset + i]; \
            if (c != ch) \
            { \
                KDbgPrintfInformational("Unexpected character.  expected %c actual %c pos %lu offset %lu length %lu", ch, c, i, offset, length); \
                KInvariant(FALSE); \
                return STATUS_INTERNAL_ERROR; \
            } \
        } \
    }

#define FILL_BUFFER_CHAR_CONTENTS(buffer, offset, length, ch) \
    { \
        for(ULONG i = 0; i < length; i++) \
        { \
            static_cast<PUCHAR>(buffer.GetBuffer())[offset + i] = ch; \
        } \
    } 

NTSTATUS SimpleCreateTestFile(
    __in KAllocator& Allocator,
    __in BOOLEAN IsSparse,
    __in BOOLEAN IsWriteThrough,
    __in LPCWSTR partialFileName,
    __out KBlockFile::SPtr& File,
    __in_opt ULONGLONG fileSize = TestFileSize)
{
    NTSTATUS status = STATUS_SUCCESS;

    KWString dirName(Allocator);
    status = dirName.Status();
    VERIFY_SUCCESS(status);

    KWString fileName(Allocator);
    status = fileName.Status();
    VERIFY_SUCCESS(status);

    KBlockFile::SPtr file;
    status = CreateTestFile(
        IsSparse,
        IsWriteThrough,
        TRUE,            // Always use file system
        fileSize,
        KBlockFile::DefaultQueueDepth,
        dirName,
        fileName,
        file,
        partialFileName);
    VERIFY_SUCCESS(status);

    File = Ktl::Move(file);

    return STATUS_SUCCESS;
}

VOID TestLargeReadOffsets(KAllocator& allocator)
{
    KBlockFile::SPtr file;
    NTSTATUS status;
    KFileStream::SPtr stream;
    KBuffer::SPtr readBuffer;
    KBuffer::SPtr writeBuffer;
    ULONG bytesRead;

    status = KBuffer::Create(51204, readBuffer, allocator);
    KInvariant(NT_SUCCESS(status));

    status = SimpleCreateTestFile(
        allocator,
        TRUE,
        TRUE,
        L"test_readoffset",
        file,
        ULONG_MAX + static_cast<ULONGLONG>(1024));
    KInvariant(NT_SUCCESS(status));

    status = SyncAwait(file->SetFileSizeAsync(static_cast<ULONGLONG>(5) * 1024 * 1024 * 1024)); // 5 gibibytes
    KInvariant(NT_SUCCESS(status));

    KDbgPrintfInformational(" --- Replicate failing TStore test case");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        KInvariant(stream->Position == 0);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(4316762112);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 51204));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX - 1 without prior read");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX - 1);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX without prior read");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX + 1 without prior read");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX + static_cast<LONGLONG>(1));
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX - 1 with prior read at 0");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        KInvariant(stream->Position == 0);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX - 1);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX with prior read at 0");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        KInvariant(stream->Position == 0);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }

    KDbgPrintfInformational(" --- Read at ULONG_MAX + 1 with prior read at 0");
    {
        status = KFileStream::Create(stream, allocator);
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->OpenAsync(*file));
        KInvariant(NT_SUCCESS(status));

        KInvariant(stream->Position == 0);
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        stream->SetPosition(ULONG_MAX + static_cast<LONGLONG>(1));
        status = SyncAwait(stream->ReadAsync(*readBuffer, bytesRead, 0, 1024));
        KInvariant(NT_SUCCESS(status));

        status = SyncAwait(stream->CloseAsync());
        KInvariant(NT_SUCCESS(status));
    }
}

NTSTATUS
KFileStreamTest(
    __in KBlockFile& File,
    __in KStream& Stream,
    __in KBuffer& ReadBuffer,
    __in KBuffer& WriteBuffer, 
    __in KAllocator& Allocator)
{
    NTSTATUS status;
    ULONG bytesRead;

    // Try read of empty Stream.  should not read any bytes.
    KDbgPrintfInformational(" --- Read Empty Stream");
    {
        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, 10));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(0, bytesRead);
        VERIFY_STREAM_POSITION(Stream, 0);
        VERIFY_CACHED_EOF(Stream, 0);
    }

    LPCSTR text = "testtest";
    ULONG textLen = static_cast<ULONG>(strlen(text));
    KInvariant(textLen == 8);
    KMemCpySafe(WriteBuffer.GetBuffer(), WriteBuffer.QuerySize(), text, textLen);

    // Write to empty Stream, read back two portions
    KDbgPrintfInformational(" --- Simple Write/Read");
    {
        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, textLen));

        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, textLen);
        VERIFY_CACHED_EOF(Stream, textLen);

        Stream.SetPosition(4);
        VERIFY_STREAM_POSITION(Stream, 4);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, 4));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(4, bytesRead);
        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 0, "test");
        VERIFY_CACHED_EOF(Stream, textLen);

        Stream.SetPosition(1);
        VERIFY_STREAM_POSITION(Stream, 1);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 4, 6));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(bytesRead, 6);
        VERIFY_STREAM_POSITION(Stream, 7);
        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 4, "esttes");
        VERIFY_CACHED_EOF(Stream, textLen);

        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 0, "testesttes"); // note: does not match what is on disk
    }

    // todo: test passing default count (0) to read/write

    // Try reading at EOF.  should not read any bytes.
    KDbgPrintfInformational(" --- Read at EOF");
    {
        VERIFY_CACHED_EOF(Stream, textLen);

        Stream.SetPosition(textLen);
        VERIFY_STREAM_POSITION(Stream, textLen);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, textLen, 1));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(0, bytesRead);
        VERIFY_STREAM_POSITION(Stream, textLen);
        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 0, "testesttes");
    }

    // Confirm that EOF is incremented by the right amount
    KDbgPrintfInformational(" --- Verify EOF extension");
    {
        VERIFY_CACHED_EOF(Stream, textLen);

        Stream.SetPosition(5);
        VERIFY_STREAM_POSITION(Stream, 5);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, 4));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF(Stream, 9);
        VERIFY_STREAM_POSITION(Stream, 9);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, 9));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF(Stream, 9);
        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 0, "testttest");
    }

    // Try reading past EOF.  should read up to EOF.
    KDbgPrintfInformational(" --- Read past EOF");
    {
        VERIFY_CACHED_EOF(Stream, 9);

        Stream.SetPosition(4);
        VERIFY_STREAM_POSITION(Stream, 4);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, 10));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(5, bytesRead);
        VERIFY_STREAM_POSITION(Stream, 9);
        VERIFY_CACHED_EOF(Stream, 9);
        VERIFY_BUFFER_SUBSTRING(ReadBuffer, 0, "ttesttest");
    }

    const CHAR readChar = 'r';
    const CHAR writeChar = 'w';
    FILL_BUFFER_CHAR_CONTENTS(WriteBuffer, 0, WriteBuffer.QuerySize() - 1, writeChar);
    VERIFY_BUFFER_CHAR_CONTENTS(WriteBuffer, 0, WriteBuffer.QuerySize() - 1, writeChar);

    // Test an aligned block read/write
    KDbgPrintfInformational(" --- Aligned read/write");
    {
        FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_EQUAL_ULONG(KBlockFile::BlockSize, bytesRead);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);

        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, ReadBuffer.QuerySize() - KBlockFile::BlockSize - 1, readChar);
    }

    // Test writes and reads larger than a block
    KDbgPrintfInformational(" --- Multiblock read/write");
    {
        FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        // Initialize the first two blocks of the File with the contents of the "read buffer" to allow verifying the following write
        status = SyncAwait(Stream.WriteAsync(ReadBuffer, 0, KBlockFile::BlockSize * 2));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 2);
        VERIFY_CACHED_EOF(Stream, KBlockFile::BlockSize * 2);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize + 50));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize + 50);
        VERIFY_CACHED_EOF(Stream, KBlockFile::BlockSize * 2);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, KBlockFile::BlockSize + 100));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize + 100);

        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize + 50, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize + 50, 50, readChar);
    }

    // Test unaligned write which spans a block boundary
    KDbgPrintfInformational(" --- Unaligned write spanning block boundary");
    {
        FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        // Initialize the first two blocks of the File with the contents of the "read buffer" to allow verifying the following writes
        status = SyncAwait(Stream.WriteAsync(ReadBuffer, 10, KBlockFile::BlockSize * 2));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 2);
        VERIFY_CACHED_EOF(Stream, KBlockFile::BlockSize * 2);

        const ULONG writeStart = KBlockFile::BlockSize - 31;
        const ULONG writeLength = 52;

        Stream.SetPosition(writeStart);
        VERIFY_STREAM_POSITION(Stream, writeStart);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 10, writeLength));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, writeStart + writeLength);

        Stream.SetPosition(writeStart);
        VERIFY_STREAM_POSITION(Stream, writeStart);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 121, writeLength));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, writeStart + writeLength);

        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 121, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 121, writeLength, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 121 + writeLength, ReadBuffer.QuerySize() - writeLength - 1 - 121, readChar);
    }

    // Test seeking around for reading and writing
    KDbgPrintfInformational(" --- Write/seek/read");
    {
        FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        // Initialize the first four blocks of the File with the contents of the "read buffer" to allow verifying the following writes
        status = SyncAwait(Stream.WriteAsync(ReadBuffer, 10, KBlockFile::BlockSize * 4));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 4);
        VERIFY_CACHED_EOF(Stream, KBlockFile::BlockSize * 4);

        Stream.SetPosition(0);
        VERIFY_STREAM_POSITION(Stream, 0);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 100, 10));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, 10);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, 100));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, 110);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 10, 100, readChar);

        Stream.SetPosition(KBlockFile::BlockSize + 720);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize + 720);

        status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize * 2 + 30));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 3 + 750);

        Stream.SetPosition(100);
        VERIFY_STREAM_POSITION(Stream, 100);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 100, 100));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, 200);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 100, 100, readChar);

        Stream.SetPosition(KBlockFile::BlockSize + 1);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize + 1);

        status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, KBlockFile::BlockSize + 1, KBlockFile::BlockSize - 1));
        VERIFY_SUCCESS(status);
        VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 2);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, 720, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize + 720, KBlockFile::BlockSize - 720, writeChar);
    }

    // Test reading from a separate stream and confirm that a flush does not happen on every write
    KDbgPrintfInformational(" --- Write/Invalidate/Read/Flush/Read");
    {
        KFileStream::SPtr readStream;
        status = KFileStream::Create(readStream, Allocator);
        VERIFY_SUCCESS(status);

        // Initialize streams and buffers
        {
            FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            // Initialize the first four blocks of the File with the contents of the "read buffer" to allow verifying the following writes
            status = SyncAwait(Stream.WriteAsync(ReadBuffer, 0, KBlockFile::BlockSize * 4));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 4);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            status = SyncAwait(Stream.FlushAsync());
            VERIFY_SUCCESS(status);

            // Open a second stream for reading
            status = SyncAwait(readStream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
            VERIFY_SUCCESS(status);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);
        }

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

        // Invalidate and read, and confirm that it is the same since there has been no flush
        status = readStream->InvalidateForWrite(0, KBlockFile::BlockSize * 3);
        VERIFY_SUCCESS(status);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

        // Flush and read, and confirm it is the same since it has not been invalidated after the flush
        status = SyncAwait(Stream.FlushAsync());
        VERIFY_SUCCESS(status);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

        // Invalidate and read, and confirm that the flushed data is read
        status = readStream->InvalidateForWrite(0, 1);
        VERIFY_SUCCESS(status);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        status = SyncAwait(readStream->CloseAsync());
        VERIFY_SUCCESS(status);
    }

    // Test invalidating for writes
    KDbgPrintfInformational(" --- Write/Flush/Read/Invalidate/Read");
    {
        KFileStream::SPtr readStream;
        status = KFileStream::Create(readStream, Allocator);
        VERIFY_SUCCESS(status);

        // Initialize streams and buffers
        {
            FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            // Initialize the first four blocks of the File with the contents of the "read buffer" to allow verifying the following writes
            status = SyncAwait(Stream.WriteAsync(ReadBuffer, 0, KBlockFile::BlockSize * 4));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 4);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            status = SyncAwait(Stream.FlushAsync());
            VERIFY_SUCCESS(status);

            // Open a second stream for reading
            status = SyncAwait(readStream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
            VERIFY_SUCCESS(status);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, KBlockFile::BlockSize)); // implicitly flushes the write, todo: add explicit flush if I allow reading unflished writes
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, writeChar);
        }

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        // Verify that the readStream's read comes from its same buffer, without going back to disk
        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        // Test invalidating in several non-intersecting ranges and confirm that the reads stay the same
        status = readStream->InvalidateForWrite(KBlockFile::BlockSize, 1);
        VERIFY_SUCCESS(status);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        status = readStream->InvalidateForWrite(0, 0);
        if (NT_SUCCESS(status))
        {
            KDbgPrintfInformational("Invalidate with length 0 expected to fail.  Status: %ld", status);
            return STATUS_INTERNAL_ERROR;
        }

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        status = readStream->InvalidateForWrite(-1, 5);
        if (NT_SUCCESS(status))
        {
            KDbgPrintfInformational("Invalidate with negative file position expected to fail.  Status: %ld", status);
            return STATUS_INTERNAL_ERROR;
        }

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        status = readStream->InvalidateForWrite(0, -2);
        if (NT_SUCCESS(status))
        {
            KDbgPrintfInformational("Invalidate with negative length expected to fail.  Status: %ld", status);
            return STATUS_INTERNAL_ERROR;
        }

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        // Test invalidating the exact range of the read, and confirm that the next read returns the on-disk data
        status = readStream->InvalidateForWrite(1, KBlockFile::BlockSize - 2);
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, writeChar);

        // Reinitialize streams and buffers
        {
            FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            // Initialize the first four blocks of the File with the contents of the "read buffer" to allow verifying the following writes
            status = SyncAwait(Stream.WriteAsync(ReadBuffer, 0, KBlockFile::BlockSize * 4));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize * 4);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, ReadBuffer.QuerySize() - 1, readChar);
            status = SyncAwait(Stream.FlushAsync());
            VERIFY_SUCCESS(status);

            // Close and reopen the read stream
            status = SyncAwait(readStream->CloseAsync());
            VERIFY_SUCCESS(status);
            readStream->Reuse();
            status = SyncAwait(readStream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            VERIFY_STREAM_POSITION((*readStream), 0);
            status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
            VERIFY_SUCCESS(status);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, readChar);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            status = SyncAwait(Stream.WriteAsync(WriteBuffer, 0, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);

            Stream.SetPosition(0);
            VERIFY_STREAM_POSITION(Stream, 0);

            status = SyncAwait(Stream.ReadAsync(ReadBuffer, bytesRead, 0, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION(Stream, KBlockFile::BlockSize);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, writeChar);
        }

        // Test invalidating a small range inside the read
        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        status = readStream->InvalidateForWrite(2, 1);
        VERIFY_SUCCESS(status);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 1, KBlockFile::BlockSize - 2, readChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize - 1, 1, writeChar);
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, KBlockFile::BlockSize, KBlockFile::BlockSize, readChar);

        readStream->SetPosition(0);
        VERIFY_STREAM_POSITION((*readStream), 0);

        status = SyncAwait(readStream->ReadAsync(ReadBuffer, bytesRead, 1, KBlockFile::BlockSize - 2));
        VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, KBlockFile::BlockSize, writeChar);

        status = SyncAwait(readStream->CloseAsync());
        VERIFY_SUCCESS(status);
    }

    // Test initialized/flushed/invalid transitions
    KDbgPrintfInformational(" --- Flush/Invalidate/etc. transitions");
    {
        KDbgPrintfInformational(" --- --- Open/Flush/Close (success)");
        {
            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);
        }
        
        KDbgPrintfInformational(" --- --- Open/Invalidate/Close (success)");
        {
            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = stream->InvalidateForWrite(0, 100);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);
        }

        KDbgPrintfInformational(" --- --- Open/Invalidate/Flush (success, invalidate->flush should be no-op)");
        {
            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = stream->InvalidateForWrite(0, 100);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status); // should be no-op

            status = SyncAwait(stream->WriteAsync(WriteBuffer, 0, 10));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);
        }

        KDbgPrintfInformational(" --- --- Open/Write/Invalidate (failure, cannot invalidate un-flushed buffer)");
        {
            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->WriteAsync(WriteBuffer, 0, 10));
            VERIFY_SUCCESS(status);

            status = stream->InvalidateForWrite(0, 100);
            if (NT_SUCCESS(status))
            {
                KDbgPrintfInformational("Invalidate should fail for a buffer with unflushed writes.");
                return STATUS_UNSUCCESSFUL;
            }

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);

            status = stream->InvalidateForWrite(0, 100);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);
        }

        KDbgPrintfInformational(" --- --- Open/Write/Close, Open/Read (success, close without flush should be an implicit close");
        {
            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            // write 'readchar' to the file so we can verify the following write.
            FILL_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 10, readChar);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 10, readChar);

            status = SyncAwait(stream->WriteAsync(ReadBuffer, 0, 10));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            stream->Reuse();

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            // write 'writechar'
            VERIFY_BUFFER_CHAR_CONTENTS(WriteBuffer, 0, 10, writeChar);
            status = SyncAwait(stream->WriteAsync(WriteBuffer, 0, 10));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->CloseAsync());  // Should be an implicit flush
            VERIFY_SUCCESS(status);

            stream->Reuse();

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->ReadAsync(ReadBuffer, bytesRead, 0, 10));
            VERIFY_SUCCESS(status);
            VERIFY_EQUAL_ULONG(10, bytesRead);
            VERIFY_BUFFER_CHAR_CONTENTS(ReadBuffer, 0, 10, writeChar);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);
        }
    }

    KDbgPrintfInformational(" --- Aligned IO");
    {
        KBuffer::SPtr aBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, aBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

        KBuffer::SPtr bBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, bBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*bBuf), 0, KBlockFile::BlockSize, 'b');

        KDbgPrintfInformational(" --- --- Open/WriteAligned_1blockbuffer/Close");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 0);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 2 * KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 3 * KBlockFile::BlockSize);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 3 * KBlockFile::BlockSize);
        }

        KDbgPrintfInformational(" --- --- Open/WriteAligned_2blockbuffer/Close");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize * 2, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 2 * KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 2 * 2 * KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 3 * 2 * KBlockFile::BlockSize);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 3 * 2 * KBlockFile::BlockSize);
        }

        KDbgPrintfInformational(" --- --- Open/WriteAligned/Seek/Read/Write/Read/Close");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            // Disk: []
            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            // Disk: [a]
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);
            VERIFY_STREAM_POSITION((*stream), KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(bBuf->GetBuffer(), KBlockFile::BlockSize);

            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [b]
            // Disk: [a]
            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            // Disk: [ab]
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 2 * KBlockFile::BlockSize);
            VERIFY_STREAM_POSITION((*stream), 2 *KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            // Disk: [ab]
            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            // Disk: [aba]
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 3 * KBlockFile::BlockSize);
            VERIFY_STREAM_POSITION((*stream), 3 * KBlockFile::BlockSize);

            stream->SetPosition(0);
            VERIFY_STREAM_POSITION((*stream), 0);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [b]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), 2 * KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [b]
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'b');

            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [a]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), 3 * KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [a]
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

            stream->SetPosition(KBlockFile::BlockSize);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [a]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), 2 *KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'b');

            stream->SetPosition(0);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [b]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [a]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            // Memory: aBuf: [b] bBuf: [b] simpleBuf: [a]
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

            simpleBufStream.PositionTo(0);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]

            // Disk: [aba]
            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            // Disk: [aaa]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), 2 * KBlockFile::BlockSize);

            stream->SetPosition(KBlockFile::BlockSize);
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            // Memory: aBuf: [a] bBuf: [b] simpleBuf: [a]
            VERIFY_SUCCESS(status);
            VERIFY_STREAM_POSITION((*stream), 2 * KBlockFile::BlockSize);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
            VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 3 * KBlockFile::BlockSize);
        }


        KDbgPrintfInformational(" --- --- Aligned read or write at unaligned position (fails)");
        {
#if DBG
            KDbgPrintfInformational("Skipping test in debug build, as KBlockFile asserts");
#else

            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 0);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            stream->Position = 1;

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            if (NT_SUCCESS(status))
            {
                KDbgErrorWData(0, "aligned write at unaligned position should fail", status, stream->Position, stream->Length, simpleBuf->QuerySize(), 0);
                return STATUS_UNSUCCESSFUL;
            }
            VERIFY_CACHED_EOF((*stream), 0);
            //VERIFY_FLUSHED_EOF(File, (*stream), 0);  // We cannot guarantee the EOF wasn't changed, as filesize may have been extended
            VERIFY_STREAM_POSITION((*stream), 1);

            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            if (NT_SUCCESS(status))
            {
                KDbgErrorWData(0, "aligned read at unaligned position should fail", status, stream->Position, stream->Length, simpleBuf->QuerySize(), 0);
                return STATUS_UNSUCCESSFUL;
            }
            VERIFY_CACHED_EOF((*stream), 0);
            //VERIFY_FLUSHED_EOF(File, (*stream), 0);  // We cannot guarantee the EOF wasn't changed, as filesize may have been extended
            VERIFY_STREAM_POSITION((*stream), 1);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 0);
#endif
        }
    }

    KDbgPrintfInformational(" --- Change Back and forth between aligned and unaligned");
    {
        KBuffer::SPtr scratchBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, scratchBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*scratchBuf), 0, KBlockFile::BlockSize, 0);

        KBuffer::SPtr aBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, aBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

        KBuffer::SPtr bBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, bBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*bBuf), 0, KBlockFile::BlockSize, 'b');

        KDbgPrintfInformational(" --- --- Open/WriteUnaligned/ReadAligned/Close");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 0);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);

            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 10));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 10);

            status = SyncAwait(stream->WriteAsync(*bBuf, 0, 15));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 25);

            stream->Position = 100;
            VERIFY_CACHED_EOF((*stream), 25);
            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 100));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 200);

            stream->Position = 0;
            status = SyncAwait(stream->ReadAsync(*simpleBuf));
            VERIFY_SUCCESS(status);

            simpleBufStream.PositionTo(0);
            simpleBufStream.Pull(scratchBuf->GetBuffer(), 25);
            VERIFY_BUFFER_CHAR_CONTENTS((*scratchBuf), 0, 10, 'a');
            VERIFY_BUFFER_CHAR_CONTENTS((*scratchBuf), 10, 15, 'b');

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 200);
        }

        KDbgPrintfInformational(" --- --- Open/WriteAligned/WriteUnaligned/ReadUnaligned/Close");
        {
            FILL_BUFFER_CHAR_CONTENTS((*scratchBuf), 0, KBlockFile::BlockSize, 0);

            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 0);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);
            simpleBufStream.PositionTo(0);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

            stream->Position = 40;
            status = SyncAwait(stream->WriteAsync(*bBuf, 0, 13));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);


            stream->Position = 30;
            status = SyncAwait(stream->ReadAsync(*scratchBuf, bytesRead, 0, 20));
            VERIFY_SUCCESS(status);
            VERIFY_BUFFER_CHAR_CONTENTS((*scratchBuf), 0, 10, 'a');
            VERIFY_BUFFER_CHAR_CONTENTS((*scratchBuf), 10, 10, 'b');

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), KBlockFile::BlockSize);
        }
    }

    KDbgPrintfInformational(" --- Make sure flush sets the EOF on-disk correctly");
    {
        KBuffer::SPtr aBuf;
        status = KBuffer::Create(KBlockFile::BlockSize, aBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

        KDbgPrintfInformational(" --- --- Unaligned");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 0);

            stream->Position = 10;
            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 1));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 11);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 11);
            VERIFY_FLUSHED_EOF(File, (*stream), 11);

            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 100));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 111);

            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 100));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 211);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 211);
            VERIFY_FLUSHED_EOF(File, (*stream), 211);

            status = SyncAwait(stream->WriteAsync(*aBuf, 0, 1));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 212);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 212);
        }
        
        KDbgPrintfInformational(" --- --- Aligned");
        {
            status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
            VERIFY_SUCCESS(status);

            KFileStream::SPtr stream;
            status = KFileStream::Create(stream, Allocator);
            VERIFY_SUCCESS(status);

            status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 0);

            KIoBuffer::SPtr simpleBuf;
            VOID* buffer;
            status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
            VERIFY_SUCCESS(status);

            KIoBufferStream simpleBufStream(*simpleBuf);
            simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);
            VERIFY_FLUSHED_EOF(File, (*stream), KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 2 * KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 3 * KBlockFile::BlockSize);

            status = SyncAwait(stream->FlushAsync());
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 3 * KBlockFile::BlockSize);
            VERIFY_FLUSHED_EOF(File, (*stream), 3 * KBlockFile::BlockSize);

            status = SyncAwait(stream->WriteAsync(*simpleBuf));
            VERIFY_SUCCESS(status);
            VERIFY_CACHED_EOF((*stream), 4 * KBlockFile::BlockSize);

            status = SyncAwait(stream->CloseAsync());
            VERIFY_SUCCESS(status);

            VERIFY_FLUSHED_EOF(File, (*stream), 4 * KBlockFile::BlockSize);
        }
    }

    KDbgPrintfInformational(" --- Truncate");
    {
        KBuffer::SPtr aBuf;
        status = KBuffer::Create(KBlockFile::BlockSize * 10, aBuf, Allocator);
        VERIFY_SUCCESS(status);
        FILL_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize * 10, 'a');

        status = SyncAwait(File.SetEndOfFileAsync(0, nullptr, nullptr));
        VERIFY_SUCCESS(status);

        KFileStream::SPtr stream;
        status = KFileStream::Create(stream, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 0);

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, 10));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 10);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);

        stream->Reuse();

        status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 10);

        status = SyncAwait(File.SetFileSizeAsync(0, nullptr));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 10);  // note that we are out of sync at this point
        
        stream->Position = 0;

        stream->InvalidateForTruncate(0);
        VERIFY_CACHED_EOF((*stream), 0);

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, 4));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 4);

        // test nonzero truncate
        stream->Position = 1;

        status = SyncAwait(stream->ReadAsync(*aBuf, bytesRead));
        VERIFY_STREAM_POSITION((*stream), 4);
        VERIFY_CACHED_EOF((*stream), 4);
        KInvariant(bytesRead == 3);

        //status = SyncAwait(File.SetFileSizeAsync(3, nullptr)); 
        // setting filesize to an unaligned value fails for nonsparse files
        status = SyncAwait(File.SetEndOfFileAsync(3, nullptr, nullptr));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 4);  // note that we are out of sync at this point

        status = stream->InvalidateForTruncate(3);
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 3);

        stream->Position = 1;

        status = SyncAwait(stream->ReadAsync(*aBuf, bytesRead));
        VERIFY_STREAM_POSITION((*stream), 3);
        VERIFY_CACHED_EOF((*stream), 3);
        KInvariant(bytesRead == 2);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);

        // test truncate == eof

        stream->Reuse();

        status = SyncAwait(File.SetFileSizeAsync(0, nullptr));
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), 0);

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

        status = SyncAwait(stream->FlushAsync());
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);
        VERIFY_FLUSHED_EOF(File, (*stream), KBlockFile::BlockSize);

        stream->Position = 0;

        status = stream->InvalidateForTruncate(KBlockFile::BlockSize);
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);

        // test truncate > eof

        stream->Reuse();

        status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);

        stream->Position = 0;

        // Invalidate with unflushed data should fail
        status = stream->InvalidateForTruncate(2 * KBlockFile::BlockSize);
        KInvariant(!NT_SUCCESS(status));

        status = SyncAwait(stream->FlushAsync());
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize);
        VERIFY_FLUSHED_EOF(File, (*stream), KBlockFile::BlockSize);

        status = SyncAwait(File.SetFileSizeAsync(2 * KBlockFile::BlockSize, nullptr));
        VERIFY_SUCCESS(status);

        status = stream->InvalidateForTruncate(2 * KBlockFile::BlockSize);
        VERIFY_SUCCESS(status);
        VERIFY_CACHED_EOF((*stream), KBlockFile::BlockSize * 2);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);

        // test write / truncate / write

        stream->Reuse();

        status = SyncAwait(stream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);

        stream->Position = KBlockFile::BlockSize * 10;

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->FlushAsync());
        VERIFY_SUCCESS(status);

        status = SyncAwait(File.SetFileSizeAsync(KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);

        status = SyncAwait(File.SetEndOfFileAsync(1024));
        VERIFY_SUCCESS(status);

        status = stream->InvalidateForTruncate(1024);
        VERIFY_SUCCESS(status);

        stream->Position = 2048;
        status = SyncAwait(stream->ReadAsync(*aBuf, bytesRead, 0, 1024));
        VERIFY_SUCCESS(status);
        KInvariant(bytesRead == 0);

        status = SyncAwait(File.SetFileSizeAsync(KBlockFile::BlockSize * 5));
        VERIFY_SUCCESS(status);

        status = SyncAwait(File.SetEndOfFileAsync(KBlockFile::BlockSize * 5 - 130));
        VERIFY_SUCCESS(status);

        status = stream->InvalidateForTruncate(KBlockFile::BlockSize * 5 - 130);
        VERIFY_SUCCESS(status);

        stream->Position = KBlockFile::BlockSize * 5 - 130;

        status = SyncAwait(stream->WriteAsync(*aBuf, 0, KBlockFile::BlockSize * 2));
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);
    }

    // todo: test ALL off-by-ones w.r.t. buffer size, file position, offset, etc.
    // todo: test multiple streams concurrently reading
    // todo: test randomized workload with multiple readers/writers coordinating
    // todo: test "IsFaulted"

    return STATUS_SUCCESS;
}

NTSTATUS
KFileStreamTest(__in KBlockFile& File, __in KAllocator& Allocator)
{
    NTSTATUS status = STATUS_SUCCESS;

    KBuffer::SPtr readBuffer;
    status = KBuffer::Create(KBlockFile::BlockSize * 5, readBuffer, Allocator);
    RtlZeroMemory(readBuffer->GetBuffer(), readBuffer->QuerySize()); // treat the contents as a c-string, so never make the last char nonzero
    VERIFY_SUCCESS(status);

    KBuffer::SPtr writeBuffer;
    status = KBuffer::Create(KBlockFile::BlockSize * 5, writeBuffer, Allocator);
    VERIFY_SUCCESS(status);

    // Test with default internal buffer size
    {
        // Mark the file as empty
        status = SyncAwait(File.SetEndOfFileAsync(0));
        VERIFY_SUCCESS(status);
        
        KFileStream::SPtr fileStream;
        status = KFileStream::Create(fileStream, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(fileStream->OpenAsync(File));
        VERIFY_SUCCESS(status);

        KStream::SPtr stream(&*fileStream);
        VERIFY_STREAM_POSITION((*stream), 0);
        VERIFY_CACHED_EOF((*stream), 0);

        KDbgPrintfInformational("\n\nTesting with default buffer size: %lu", KFileStream::DefaultBufferSize);
        status = KFileStreamTest(File, *stream, *readBuffer, *writeBuffer, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);
    }

    // Test with one-block internal buffer size
    {
        // Mark the file as empty
        status = SyncAwait(File.SetEndOfFileAsync(0));
        VERIFY_SUCCESS(status);

        KFileStream::SPtr fileStream;
        status = KFileStream::Create(fileStream, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(fileStream->OpenAsync(File, KBlockFile::BlockSize));
        VERIFY_SUCCESS(status);

        KStream::SPtr stream(&*fileStream);
        VERIFY_STREAM_POSITION((*stream), 0);
        VERIFY_CACHED_EOF((*stream), 0);

        KDbgPrintfInformational("\n\nTesting with one-block buffer size: %lu", KBlockFile::BlockSize);
        status = KFileStreamTest(File, *stream, *readBuffer, *writeBuffer, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);
    }
    
    // Test with two-block internal buffer size
    {
        // Mark the file as empty
        status = SyncAwait(File.SetEndOfFileAsync(0));
        VERIFY_SUCCESS(status);

        KFileStream::SPtr fileStream;
        status = KFileStream::Create(fileStream, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(fileStream->OpenAsync(File, KBlockFile::BlockSize * 2));
        VERIFY_SUCCESS(status);

        KStream::SPtr stream(&*fileStream);
        VERIFY_STREAM_POSITION((*stream), 0);
        VERIFY_CACHED_EOF((*stream), 0);

        KDbgPrintfInformational("\n\nTesting with two-block buffer size: %lu", KBlockFile::BlockSize * 2);
        status = KFileStreamTest(File, *stream, *readBuffer, *writeBuffer, Allocator);
        VERIFY_SUCCESS(status);

        status = SyncAwait(stream->CloseAsync());
        VERIFY_SUCCESS(status);
    }

    return STATUS_SUCCESS;
}

NTSTATUS ProveKBlockFileEOFBehavior(KAllocator& Allocator)
{
    KDbgPrintfInformational("Proving EOF behavior");

    NTSTATUS status = STATUS_SUCCESS;

    KBlockFile::SPtr file;
    status = SimpleCreateTestFile(
        Allocator,
        FALSE,
        FALSE,
        L"testfileforeof",
        file);
    VERIFY_SUCCESS(status);

    KBuffer::SPtr aBuf;
    status = KBuffer::Create(KBlockFile::BlockSize, aBuf, Allocator);
    VERIFY_SUCCESS(status);
    FILL_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

    KBuffer::SPtr bBuf;
    status = KBuffer::Create(KBlockFile::BlockSize, bBuf, Allocator);
    VERIFY_SUCCESS(status);
    FILL_BUFFER_CHAR_CONTENTS((*bBuf), 0, KBlockFile::BlockSize, 'b');

    KIoBuffer::SPtr simpleBuf;
    VOID* buffer;
    status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, simpleBuf, buffer, Allocator);
    VERIFY_SUCCESS(status);

    KIoBufferStream simpleBufStream(*simpleBuf);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Put(aBuf->GetBuffer(), KBlockFile::BlockSize);

    status = SyncAwait(
        file->TransferAsync(
            KBlockFile::IoPriority::eForeground,
            KBlockFile::SystemIoPriorityHint::eNormal,
            KBlockFile::TransferType::eWrite,
            0,
            *simpleBuf,
            nullptr,
            nullptr));
    VERIFY_SUCCESS(status);

    status = SyncAwait(file->SetEndOfFileAsync(KBlockFile::BlockSize, nullptr, nullptr));
    VERIFY_SUCCESS(status);

    LONGLONG eof;
    status = SyncAwait(file->GetEndOfFileAsync(eof));
    VERIFY_SUCCESS(status);
    KInvariant(eof == KBlockFile::BlockSize);

    status = SyncAwait(
        file->TransferAsync(
            KBlockFile::IoPriority::eForeground,
            KBlockFile::SystemIoPriorityHint::eNormal,
            KBlockFile::TransferType::eRead,
            0,
            *simpleBuf,
            nullptr,
            nullptr));
    VERIFY_SUCCESS(status);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
    VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'a');

    simpleBufStream.PositionTo(0);
    simpleBufStream.Put(bBuf->GetBuffer(), KBlockFile::BlockSize);

    status = SyncAwait(file->SetEndOfFileAsync(500, nullptr, nullptr));
    VERIFY_SUCCESS(status);

    status = SyncAwait(file->GetEndOfFileAsync(eof));
    VERIFY_SUCCESS(status);
    KInvariant(eof == 500);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
    VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, KBlockFile::BlockSize, 'b');

    status = SyncAwait(
        file->TransferAsync(
            KBlockFile::IoPriority::eForeground,
            KBlockFile::SystemIoPriorityHint::eNormal,
            KBlockFile::TransferType::eRead,
            0,
            *simpleBuf,
            nullptr,
            nullptr));
    VERIFY_SUCCESS(status);

    status = SyncAwait(file->GetEndOfFileAsync(eof));
    VERIFY_SUCCESS(status);
    KInvariant(eof == 500);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
    VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, 500, 'a');
    // In a partial read scenario, no data in the buffer past EOF is valid
    //VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 500, KBlockFile::BlockSize - 500, 'b');

    // Try with a >1 block buffer
    simpleBufStream.Reset();
    status = KIoBuffer::CreateSimple(KBlockFile::BlockSize * 2, simpleBuf, buffer, Allocator);
    VERIFY_SUCCESS(status);
    simpleBufStream.Reuse(*simpleBuf);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Put(bBuf->GetBuffer(), KBlockFile::BlockSize);
    simpleBufStream.Put(bBuf->GetBuffer(), KBlockFile::BlockSize);

    status = SyncAwait(
        file->TransferAsync(
            KBlockFile::IoPriority::eForeground,
            KBlockFile::SystemIoPriorityHint::eNormal,
            KBlockFile::TransferType::eRead,
            0,
            *simpleBuf,
            nullptr,
            nullptr));
    VERIFY_SUCCESS(status);

    status = SyncAwait(file->GetEndOfFileAsync(eof));
    VERIFY_SUCCESS(status);
    KInvariant(eof == 500);

    simpleBufStream.PositionTo(0);
    simpleBufStream.Pull(aBuf->GetBuffer(), KBlockFile::BlockSize);
    VERIFY_BUFFER_CHAR_CONTENTS((*aBuf), 0, 500, 'a');


    file->Close();


    KWString fileName(Allocator, file->GetFileName());
    VERIFY_SUCCESS(fileName.Status());

    file = nullptr;
    // reopen file and verify that the EOF is the same
    status = SyncAwait(KBlockFile::CreateAsync(
        fileName,
        FALSE,
        KBlockFile::eOpenExisting,
        file,
        nullptr,
        Allocator));

    status = SyncAwait(file->GetEndOfFileAsync(eof));
    VERIFY_SUCCESS(status);
    KInvariant(eof == 500);

    KDbgPrintfInformational("EOF behavior (partial read) confirmed");

    return STATUS_SUCCESS;
}

NTSTATUS
KStreamTest(
    __in int argc,
    __in_ecount(argc) WCHAR* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status;
#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KStreamTest: STARTED\n");

    EventRegisterMicrosoft_Windows_KTL();

    KtlSystem* underlyingSystem;
    status = KtlSystem::Initialize(&underlyingSystem);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("%s: %i: KtlSystem::Initialize failed\n", __FUNCTION__, __LINE__);
        return status;
    }
    // Turn on strict allocation tracking
    underlyingSystem->SetStrictAllocationChecks(TRUE);
    
    KAllocator& allocator = underlyingSystem->NonPagedAllocator();
    
    TestLargeReadOffsets(allocator);

    status = ProveKBlockFileEOFBehavior(allocator);
    VERIFY_SUCCESS(status);

    KBlockFile::SPtr nonSparse;
    status = SimpleCreateTestFile(allocator, FALSE, FALSE, L"TestFileNonSparse_NoWritethrough", nonSparse);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a nonsparse file, no writethrough, initial size = %llu\n", TestFileSize);
    status = KFileStreamTest(*nonSparse, allocator);
    VERIFY_SUCCESS(status);
    nonSparse->Close();
    nonSparse = nullptr;

    status = SimpleCreateTestFile(allocator, FALSE, TRUE, L"TestFileNonSparse_Writethrough", nonSparse);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a nonsparse file, writethrough, initial size = %llu\n", TestFileSize);
    status = KFileStreamTest(*nonSparse, allocator);
    VERIFY_SUCCESS(status);
    nonSparse->Close();
    nonSparse = nullptr;

    KBlockFile::SPtr nonSparseEmpty;
    status = SimpleCreateTestFile(allocator, FALSE, FALSE, L"TestFileNonSparse_InitiallyEmpty_NoWritethrough", nonSparseEmpty, 0);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a nonsparse file, no writethrough, initial size = %llu\n", 0);
    status = KFileStreamTest(*nonSparseEmpty, allocator);
    VERIFY_SUCCESS(status);
    nonSparseEmpty->Close();
    nonSparseEmpty = nullptr;

    status = SimpleCreateTestFile(allocator, FALSE, TRUE, L"TestFileNonSparse_InitiallyEmpty_Writethrough", nonSparseEmpty, 0);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a nonsparse file, writethrough, initial size = %llu\n", 0);
    status = KFileStreamTest(*nonSparseEmpty, allocator);
    VERIFY_SUCCESS(status);
    nonSparseEmpty->Close();
    nonSparseEmpty = nullptr;

    KBlockFile::SPtr sparse;
    status = SimpleCreateTestFile(allocator, TRUE, FALSE, L"TestFileSparse_NoWritethrough", sparse);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a sparse file, no writethrough, initial size = %llu\n", TestFileSize);
    status = KFileStreamTest(*sparse, allocator);
    VERIFY_SUCCESS(status);
    sparse->Close();
    sparse = nullptr;

    status = SimpleCreateTestFile(allocator, TRUE, TRUE, L"TestFileSparse_Writethrough", sparse);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a sparse file, writethrough, initial size = %llu\n", TestFileSize);
    status = KFileStreamTest(*sparse, allocator);
    VERIFY_SUCCESS(status);
    sparse->Close();
    sparse = nullptr;

    KBlockFile::SPtr sparseEmpty;
    status = SimpleCreateTestFile(allocator, TRUE, FALSE, L"TestFileSparse_InitiallyEmpty_NoWritethrough", sparseEmpty, 0);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a sparse file, no writethrough, initial size = %llu\n", 0);
    status = KFileStreamTest(*sparseEmpty, allocator);
    VERIFY_SUCCESS(status);
    sparseEmpty->Close();
    sparseEmpty = nullptr;

    status = SimpleCreateTestFile(allocator, TRUE, TRUE, L"TestFileSparse_InitiallyEmpty_Writethrough", sparseEmpty, 0);
    VERIFY_SUCCESS(status);
    KTestPrintf("Testing with a sparse file, writethrough, initial size = %llu\n", 0);
    status = KFileStreamTest(*sparseEmpty, allocator);
    VERIFY_SUCCESS(status);
    sparseEmpty->Close();
    sparseEmpty = nullptr;

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

    if (NT_SUCCESS(status))
    {
        KTestPrintf("KStreamTests completed successfully.\n");
    }
    else
    {
        KTestPrintf("KStreamTests failed with status: %x\n", status);
    }

    KTestPrintf("KStreamTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return status;
}

#if CONSOLE_TEST

#if !defined(PLATFORM_UNIX)
int __cdecl
wmain(
    __in int argc,
    __in_ecount(argc) WCHAR* args[]
)
#else
int main(int argc, char* args[])
#endif
{
    NTSTATUS status;

    //
    // Adjust for the test name so CmdParseLine works.
    //
    if (argc > 0)
    {
        argc--;
        args++;
    }

    status = KStreamTest(0, nullptr);

    if (!NT_SUCCESS(status)) 
    {
        KTestPrintf("KStream tests failed Status = 0X%X\n", status);
        return RtlNtStatusToDosError(status);
    }

    return RtlNtStatusToDosError(status);
}
#endif
#endif
