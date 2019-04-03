/*++

Copyright (c) Microsoft Corporation

Module Name:

    KBlockFileNoPrivTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KBlockFileNoPrivTests.h.
    2. Add an entry to the gs_KuTestCases table in KBlockFileNoPrivTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#define _CRT_RAND_S
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KBlockFileNoPrivTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

using namespace ktl::test::file;

const ULONGLONG g_TestSize = 1024*1024*1024;

class RandomAllocator : public KObject<RandomAllocator> {

    public:

        ~RandomAllocator(
            );

        FAILABLE
        RandomAllocator(
            __in ULONG NumberOfBits,
            __in ULONG MaxAllocNumberOfBits
            );

        BOOLEAN
        Allocate(
            __out ULONG& StartBit,
            __out ULONG& Length
            );

        VOID
        Clear(
            );

    private:

        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __in ULONG NumberOfBits,
            __in ULONG MaxAllocNumberOfBits
            );

        KBuffer::SPtr _Buffer;
        KBitmap _Bitmap;
        ULONG _MaxAllocNumberOfBits;

};

RandomAllocator::~RandomAllocator(
    )
{
    Cleanup();
}

FAILABLE
RandomAllocator::RandomAllocator(
    __in ULONG NumberOfBits,
    __in ULONG MaxAllocNumberOfBits
    )
{
    Zero();
    SetConstructorStatus(Initialize(NumberOfBits, MaxAllocNumberOfBits));
}

ULONGLONG
RandomNumber(
    )
{
#if KTL_USER_MODE
    unsigned int r;
#ifdef PLATFORM_UNIX
    rand_r(&r);
#else
    rand_s(&r);
#endif
    return r;
#else
    return rand();
#endif
}

BOOLEAN
RandomAllocator::Allocate(
    __out ULONG& StartBit,
    __out ULONG& Length
    )
{
    ULONGLONG formerRandomNumber;
    BOOLEAN bit;

    static ULONGLONG randomNumber = 0;

    Length = (ULONG) (RandomNumber()%_MaxAllocNumberOfBits) + 1;

    formerRandomNumber = InterlockedExchangeAdd64((LONGLONG*) &randomNumber, Length);

    StartBit = (ULONG) (formerRandomNumber%_Bitmap.QueryNumberOfBits());

    if (_Bitmap.CheckBit(StartBit)) {

        StartBit += _Bitmap.QueryRunLength(StartBit, bit);
        ASSERT(bit);

        if (StartBit == _Bitmap.QueryNumberOfBits()) {

            StartBit = 0;

            if (_Bitmap.CheckBit(StartBit)) {

                StartBit += _Bitmap.QueryRunLength(StartBit, bit);
                ASSERT(bit);

                if (StartBit == _Bitmap.QueryNumberOfBits()) {
                    return FALSE;
                }
            }
        }
    }

    Length = _Bitmap.QueryRunLength(StartBit, bit, Length);
    ASSERT(!bit);
    _Bitmap.SetBits(StartBit, Length);

    return TRUE;
}

VOID
RandomAllocator::Clear(
    )
{
    _Bitmap.ClearAllBits();
}

VOID
RandomAllocator::Zero(
    )
{
    _MaxAllocNumberOfBits = 0;
}

VOID
RandomAllocator::Cleanup(
    )
{
}

NTSTATUS
RandomAllocator::Initialize(
    __in ULONG NumberOfBits,
    __in ULONG MaxAllocNumberOfBits
    )
{
    ULONG bufferSize;
    NTSTATUS status;

    bufferSize = KBitmap::QueryRequiredBufferSize(NumberOfBits);

    status = KBuffer::Create(bufferSize, _Buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = _Bitmap.Reuse(_Buffer->GetBuffer(), _Buffer->QuerySize(), NumberOfBits);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    _Bitmap.ClearAllBits();

    _MaxAllocNumberOfBits = MaxAllocNumberOfBits;

    return STATUS_SUCCESS;
}

VOID
FillBufferWithPattern(
    __in_bcount(Size) VOID* Buffer,
    __in ULONG Size,
    __in ULONGLONG FileOffset
    )
{
    ULONGLONG blockNumber;
    UCHAR fillByte;
    ULONG numBlocks;
    ULONG i;
    ULONG j;
    UCHAR* p;

    blockNumber = FileOffset/KBlockFile::BlockSize;
    fillByte = (UCHAR) (blockNumber%0x100);
    numBlocks = Size/KBlockFile::BlockSize;

    for (i = 0; i < numBlocks; i++) {
        p = (UCHAR*) Buffer + i*KBlockFile::BlockSize;
        for (j = 0; j < KBlockFile::BlockSize; j++) {
            p[j] = fillByte;
        }
        fillByte++;
    }
}

BOOLEAN
HasCorrectFill(
    __in_bcount(Size) VOID* Buffer,
    __in ULONG Size,
    __in ULONGLONG FileOffset
    )
{
    ULONGLONG blockNumber;
    UCHAR fillByte;
    ULONG numBlocks;
    ULONG i;
    ULONG j;
    UCHAR* p;

    blockNumber = FileOffset/KBlockFile::BlockSize;
    fillByte = (UCHAR) (blockNumber%0x100);
    numBlocks = Size/KBlockFile::BlockSize;

    for (i = 0; i < numBlocks; i++) {
        p = (UCHAR*) Buffer + i*KBlockFile::BlockSize;
        for (j = 0; j < KBlockFile::BlockSize; j++) {
            if (p[j] != fillByte) {
                KTestPrintf("Incorrect fillByte is %x, should be %x\n", p[j], fillByte);
                ASSERT(FALSE);
                return FALSE;
            }
        }
        fillByte++;
    }

    return TRUE;
}

VOID
FillIoBufferWithPattern(
    __inout KIoBuffer& IoBuffer,
    __in ULONGLONG FileOffset
    )
{
    ULONGLONG offset = FileOffset;
    KIoBufferElement* elem;

    for (elem = IoBuffer.First(); elem; elem = IoBuffer.Next(*elem)) {
        FillBufferWithPattern((VOID*) elem->GetBuffer(), elem->QuerySize(), offset);
        offset += elem->QuerySize();
    }
}

BOOLEAN
HasCorrectFill(
    __inout KIoBuffer& IoBuffer,
    __in ULONGLONG FileOffset
    )
{
    ULONGLONG offset = FileOffset;
    KIoBufferElement* elem;
    BOOLEAN b;

    for (elem = IoBuffer.First(); elem; elem = IoBuffer.Next(*elem)) {
        b = HasCorrectFill((VOID*) elem->GetBuffer(), elem->QuerySize(), offset);
        if (!b) {
            return FALSE;
        }
        offset += elem->QuerySize();
    }

    return TRUE;
}

NTSTATUS
BuildRandomIoBuffer(
    __in ULONG BufferSize,
    __out KIoBuffer::SPtr& IoBuffer
    )
{
    NTSTATUS status;
    ULONG numBlocks;
    ULONG allocSize;
    KIoBufferElement::SPtr ioBufferElement;
    VOID* buffer;

    status = KIoBuffer::CreateEmpty(IoBuffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        return status;
    }

    numBlocks = BufferSize/KBlockFile::BlockSize;

    while (numBlocks) {

        allocSize = (ULONG) ((RandomNumber()%numBlocks) + 1);
        numBlocks -= allocSize;
        allocSize *= KBlockFile::BlockSize;

        status = KIoBufferElement::CreateNew(allocSize, ioBufferElement, buffer, KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            return status;
        }

        IoBuffer->AddIoBufferElement(*ioBufferElement);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
DeleteTestFile(
    __in KWString& TestDirName,
    __in KWString& TestFileName
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;

    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
#if KTL_USER_MODE
        printf("could not create test context %x\n", status);
#else
        KTestPrintf("could not create test context %x\n", status);
#endif
        goto Finish;
    }

    completion.Bind(context.RawPtr(), &TestContext::Completion);

    status = KVolumeNamespace::DeleteFileOrDirectory(TestFileName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
#if KTL_USER_MODE
        printf("could not delete file %x\n", status);
#else
        KTestPrintf("could not delete file %x\n", status);
#endif
        goto Finish;
    }

    status = KVolumeNamespace::DeleteFileOrDirectory(TestDirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        status = STATUS_SUCCESS;
        goto Finish;
    }


Finish:

    return status;
}

NTSTATUS
BasicTest(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;
    KIoBuffer::SPtr ioBuffer;
    VOID* buffer;
    ULONG i;
    UCHAR* p;
    ULONGLONG tickCountBefore;
    ULONGLONG tickCountAfter;
    ULONG elapsedSeconds;
    KIoBuffer::SPtr ioBuffer2;
    VOID* buffer2;

    //
    // Create the test context that will allow this test to just run on this thread.
    //

    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test context %x\n", status);
        goto Finish;
    }

    completion.Bind(context.RawPtr(), &TestContext::Completion);

    //
    // The file was created without write-through, the query should reflect this.
    //

    if (File.IsWriteThrough()) {
        KTestPrintf("file believes that it is write-through\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Ensure that the file report the size that we set it to.
    //

    if (File.QueryFileSize() != TestSize) {
        KTestPrintf("wrong size being reported %I64x\n", File.QueryFileSize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check that the first block is zero.
    //

    status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, ioBuffer, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("create io buffer failed %x\n", status);
        goto Finish;
    }

    status = File.Transfer(KBlockFile::eForeground, KBlockFile::SystemIoPriorityHint::eLow, KBlockFile::eRead, 0, *ioBuffer, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not read file %x\n", status);
        goto Finish;
    }

    p = (UCHAR*) buffer;
    for (i = 0; i < KBlockFile::BlockSize; i++) {
        if (p[i]) {
            KTestPrintf("first block of file is not zero\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Write to the last block of the range.  This should be fast because we already set the valid-data-length.
    //

    for (i = 0; i < KBlockFile::BlockSize; i++) {
        p[i] = 0xF8;
    }

    tickCountBefore = KNt::GetTickCount64();

    status = File.Transfer(KBlockFile::eBackground, KBlockFile::eWrite, TestSize - KBlockFile::BlockSize, p,
            KBlockFile::BlockSize, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not write the last file block %x", status);
        goto Finish;
    }

    tickCountAfter = KNt::GetTickCount64();

    elapsedSeconds = (ULONG) ((tickCountAfter - tickCountBefore)/1000);

    //
    // Check that the number of seconds for this single IO was less than, say, 3 seconds.
    //

    if (elapsedSeconds > 3) {
        KTestPrintf("valid data length didn't work.  The write to the last block of the file took %d seconds.\n", elapsedSeconds);
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Do a copy of the last block to block 5, then re-read block 5 and make sure that it has the right pattern.
    //

    status = File.Copy(KBlockFile::eForeground, TestSize - KBlockFile::BlockSize, 5*KBlockFile::BlockSize, p,
            KBlockFile::BlockSize, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("copy failed with %x\n", status);
        goto Finish;
    }

    status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, ioBuffer2, buffer2, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("create io buffer failed with %x\n", status);
        goto Finish;
    }

    status = File.Transfer(KBlockFile::eBackground, KBlockFile::eRead, 5*KBlockFile::BlockSize, *ioBuffer2, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("read failed with %x\n", status);
        goto Finish;
    }

    p = (UCHAR*) buffer2;
    for (i = 0; i < KBlockFile::BlockSize; i++) {
        if (p[i] != 0xF8) {
            KTestPrintf("copy of block to position 5 didn't work properly\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Do a flush.
    //

    status = File.Flush(completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("flush failed with %x\n", status);
        goto Finish;
    }

    //
    // Cancel all requests.
    //

    File.CancelAll();

Finish:

    return status;
}

#if 0

//
// Older synchronous version of Knarly Test.  Kept here for reference.
//

NTSTATUS
KnarlyTest(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;
    RandomAllocator allocator((ULONG) (TestSize/KBlockFile::BlockSize), 4096);
    BOOLEAN b;
    ULONG startBit;
    ULONG numBits;
    ULONGLONG offset;
    ULONG length;
    KIoBuffer::SPtr ioBuffer;

    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test context %x\n", status);
        goto Finish;
    }

    completion.Bind((TestContext*) context, &TestContext::Completion);

    for (;;) {

        b = allocator.Allocate(startBit, numBits);

        if (!b) {
            break;
        }

        offset = ((ULONGLONG) startBit)*KBlockFile::BlockSize;
        length = numBits*KBlockFile::BlockSize;

        status = BuildRandomIoBuffer(length, ioBuffer);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not build a random io buffer %x\n", status);
            ASSERT(FALSE);
            goto Finish;
        }

        FillIoBufferWithPattern(*ioBuffer, offset);

        status = File.Transfer(KBlockFile::eForeground, KBlockFile::eWrite, offset, *ioBuffer, completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("write failed at offset %x, with status %x\n", offset, status);
            ASSERT(FALSE);
            goto Finish;
        }
    }

    allocator.Clear();

    for (;;) {

        b = allocator.Allocate(startBit, numBits);

        if (!b) {
            break;
        }

        offset = ((ULONGLONG) startBit)*KBlockFile::BlockSize;
        length = numBits*KBlockFile::BlockSize;

        status = BuildRandomIoBuffer(length, ioBuffer);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("could not build a random io buffer %x\n", status);
            ASSERT(FALSE);
            goto Finish;
        }

        status = File.Transfer(KBlockFile::eForeground, KBlockFile::eRead, offset, *ioBuffer, completion);

        if (status == STATUS_PENDING) {
            status = context->Wait();
        }

        if (!NT_SUCCESS(status)) {
            KTestPrintf("read failed at offset %x, with status %x\n", offset, status);
            ASSERT(FALSE);
            goto Finish;
        }

        b = HasCorrectFill(*ioBuffer, offset);

        if (!b) {
            KTestPrintf("data is not correct\n");
            ASSERT(FALSE);
            goto Finish;
        }
    }

Finish:

    return status;
}

#endif

#if 1

//
// More better completely async and pipelined version of Knarly test.
//

NTSTATUS
DoKnarlyTest(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize,
    __in KBlockFile::TransferType Type,
    __in KAsyncContextBase::CompletionCallback Completion
    );

class KnarlyTestContext : public KAsyncContextBase {

    K_FORCE_SHARED(KnarlyTestContext);

    FAILABLE
    KnarlyTestContext(
        __inout KBlockFile& File,
        __in ULONGLONG TestSize,
        __in KBlockFile::TransferType Type
        );

    private:

        class IoContext {

            public:

                VOID
                Completion(
                    __in KAsyncContextBase* const ParentAsync,
                    __in KAsyncContextBase& Async
                    );

                KnarlyTestContext* Context;
                KIoBuffer::SPtr IoBuffer;
                LONGLONG Offset;

        };


        VOID
        Zero(
            );

        VOID
        Cleanup(
            );

        NTSTATUS
        Initialize(
            __inout KBlockFile& File,
            __in ULONGLONG TestSize,
            __in KBlockFile::TransferType Type
            );

        VOID
        OnStart(
            );

        friend
        NTSTATUS
        DoKnarlyTest(
            __inout KBlockFile& File,
            __in ULONGLONG TestSize,
            __in KBlockFile::TransferType Type,
            __in KAsyncContextBase::CompletionCallback Completion
            );

        KBlockFile* _File;
        ULONGLONG _TestSize;
        KBlockFile::TransferType _TransferType;
        ULONG _NumberOfIoContexts;
        IoContext* _ContextArray;
        RandomAllocator _Allocator;
        NTSTATUS _Status;
        LONG _RefCount;

};

KnarlyTestContext::~KnarlyTestContext(
    )
{
    Cleanup();
}

FAILABLE
KnarlyTestContext::KnarlyTestContext(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize,
    __in KBlockFile::TransferType Type
    ) : _Allocator((ULONG) (TestSize/KBlockFile::BlockSize), 4096)
{
    Zero();
    SetConstructorStatus(Initialize(File, TestSize, Type));
}

VOID
KnarlyTestContext::Zero(
    )
{
    _TestSize = 0;
    _TransferType = KBlockFile::eWrite;
    _NumberOfIoContexts = 0;
    _ContextArray = NULL;
    _Status = STATUS_SUCCESS;
    _RefCount = 0;
}

VOID
KnarlyTestContext::Cleanup(
    )
{
    _deleteArray(_ContextArray);
}

NTSTATUS
KnarlyTestContext::Initialize(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize,
    __in KBlockFile::TransferType Type
    )
{
    NTSTATUS status;

    _File = &File;
    _TestSize = TestSize;
    _TransferType = Type;
    _NumberOfIoContexts = 16;
    _ContextArray = _newArray<IoContext>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), _NumberOfIoContexts);

    if (!_ContextArray) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = _Allocator.Status();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
KnarlyTestContext::OnStart(
    )
{
    ULONG i;
    IoContext* context;
    BOOLEAN b;
    ULONG startBit;
    ULONG numBits;
    ULONG length;
    NTSTATUS status;
    KAsyncContextBase::CompletionCallback completion;

    for (i = 0; i < _NumberOfIoContexts; i++) {

        context = &(_ContextArray[i]);
        context->Context = this;

        b = _Allocator.Allocate(startBit, numBits);

        if (!b) {
            break;
        }

        context->Offset = ((ULONGLONG) startBit)*KBlockFile::BlockSize;
        length = numBits*KBlockFile::BlockSize;

        status = BuildRandomIoBuffer(length, context->IoBuffer);

        if (!NT_SUCCESS(status)) {
            _Status = status;
            KTestPrintf("could not build a random io buffer %x", status);
            ASSERT(FALSE);
            break;
        }

        if (_TransferType == KBlockFile::eWrite) {
            FillIoBufferWithPattern(*context->IoBuffer, context->Offset);
        }

        completion.Bind(context, &IoContext::Completion);

        _RefCount++;

        status = _File->Transfer(KBlockFile::eForeground, _TransferType, context->Offset, *context->IoBuffer, completion, this);

        if (status != STATUS_PENDING) {
            _RefCount--;
            _Status = status;
            break;
        }
    }

    if (!_RefCount) {
        Complete(_Status);
    }
}

VOID
KnarlyTestContext::IoContext::Completion(
    __in KAsyncContextBase* const ParentAsync,
    __in KAsyncContextBase& Async
    )
{
    UNREFERENCED_PARAMETER(ParentAsync);

    NTSTATUS status;
    BOOLEAN b;
    ULONG startBit;
    ULONG numBits;
    ULONG length;
    KAsyncContextBase::CompletionCallback completion;

    status = Async.Status();

    if (!NT_SUCCESS(status)) {
        Context->_Status = status;
        KTestPrintf("IO failed with %x\n", status);
        ASSERT(FALSE);
    }

    if (Context->_TransferType == KBlockFile::eRead) {
        b = HasCorrectFill(*IoBuffer, Offset);
        if (!b) {
            KTestPrintf("data is not correct\n");
            ASSERT(FALSE);
            Context->_Status = STATUS_DISK_CORRUPT_ERROR;
        }
    }

    b = Context->_Allocator.Allocate(startBit, numBits);

    if (!b) {
        Context->_RefCount--;
        if (!Context->_RefCount) {
            Context->Complete(Context->_Status);
        }
        return;
    }

    Offset = ((ULONGLONG) startBit)*KBlockFile::BlockSize;
    length = numBits*KBlockFile::BlockSize;

    status = BuildRandomIoBuffer(length, IoBuffer);

    if (!NT_SUCCESS(status)) {
        Context->_Status = status;
        KTestPrintf("could not build a random io buffer %x\n", status);
        ASSERT(FALSE);
        Context->_RefCount--;
        if (!Context->_RefCount) {
            Context->Complete(Context->_Status);
        }
        return;
    }

    if (Context->_TransferType == KBlockFile::eWrite) {
        FillIoBufferWithPattern(*IoBuffer, Offset);
    }

    completion.Bind(this, &IoContext::Completion);

    status = Context->_File->Transfer(KBlockFile::eForeground, Context->_TransferType, Offset, *IoBuffer, completion, Context);

    if (status != STATUS_PENDING) {
        Context->_Status = status;
        KTestPrintf("transfer failed to start %x\n", status);
        ASSERT(FALSE);
        Context->_RefCount--;
        if (!Context->_RefCount) {
            Context->Complete(Context->_Status);
        }
        return;
    }
}

NTSTATUS
DoKnarlyTest(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize,
    __in KBlockFile::TransferType Type,
    __in KAsyncContextBase::CompletionCallback Completion
    )

{
    KnarlyTestContext* context;
    NTSTATUS status;

    context = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KnarlyTestContext(File, TestSize, Type);

    if (!context) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = context->Status();

    if (!NT_SUCCESS(status)) {
        _delete(context);
        return status;
    }

    context->Start(NULL, Completion);

    return STATUS_PENDING;
}

NTSTATUS
KnarlyTest(
    __inout KBlockFile& File,
    __in ULONGLONG TestSize
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    TestContext::SPtr context;
    KAsyncContextBase::CompletionCallback completion;

    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test context %x\n", status);
        goto Finish;
    }

    completion.Bind(context.RawPtr(), &TestContext::Completion);

    status = DoKnarlyTest(File, TestSize, KBlockFile::eWrite, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("do knarly test failed with %x\n", status);
        goto Finish;
    }

    status = DoKnarlyTest(File, TestSize, KBlockFile::eRead, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("do knarly test failed with %x\n", status);
        goto Finish;
    }

Finish:

    return status;
}

#endif

#if KTL_USER_MODE
# define VERIFY_IS_TRUE(__condition, s) if (! (__condition)) {  KTestPrintf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", s, __FILE__, __LINE__); DebugBreak(); };
#else
# define VERIFY_IS_TRUE(__condition, s) if (! (__condition)) {  KDbgPrintf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", s, __FILE__, __LINE__); KInvariant(FALSE); };
#endif


NTSTATUS
RunTheTests(
    BOOLEAN SparseFile,
    KWString& dirName
           )
{
    NTSTATUS status = STATUS_SUCCESS;
    KWString fileName(KtlSystem::GlobalNonPagedAllocator());
    KBlockFile::SPtr file;
    KSynchronizer sync;
    ULONG count = 60;

    //
    // Create the non sparse test file.
    //
    status = CreateTestFile(SparseFile, FALSE, TRUE, g_TestSize, KBlockFile::DefaultQueueDepth, dirName, fileName, file);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test file %x\n", status);
        goto Finish;
    }

    //
    // Verify SystemIoPriorityHint is not specified
    //
    KBlockFile::SystemIoPriorityHint hint;
    hint = file->GetSystemIoPriorityHint();
    VERIFY_IS_TRUE( (hint == KBlockFile::eNotDefined), "SystemIoPriorityHint should be eNotDefined");

    LONGLONG endOfFile;
    LONGLONG newEndOfFile;

    //
    // Verify end of file at time of creation
    //
    file->GetEndOfFile(endOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "GetEndOfFile on creation failed");
    VERIFY_IS_TRUE((endOfFile == g_TestSize), "EndOfFile on creation size is not correct");

    //
    // Verify set end of file beyond VDL
    //
    newEndOfFile = g_TestSize * 2;
    file->SetEndOfFile(newEndOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "SetEndOfFile beyond VDL fail");

    file->GetEndOfFile(endOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "GetEndOfFile beyond VDL failed");
    VERIFY_IS_TRUE((endOfFile == newEndOfFile), "EndOfFile beyond VDL size is not correct");

    //
    // Verify set end of file before VDL
    //
    newEndOfFile = g_TestSize / 2;
    file->SetEndOfFile(newEndOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "SetEndOfFile before VDL fail");


    file->GetEndOfFile(endOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "GetEndOfFile before VDL failed");
    VERIFY_IS_TRUE((endOfFile == newEndOfFile), "EndOfFile before VDL size is not correct");


    //
    // Verify set end of file to VDL
    //
    newEndOfFile = g_TestSize;
    file->SetEndOfFile(newEndOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "SetEndOfFile at VDL fail");


    file->GetEndOfFile(endOfFile,
                       sync,
                       NULL,
                       NULL);
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "GetEndOfFile at VDL failed");
    VERIFY_IS_TRUE((endOfFile == newEndOfFile), "EndOfFile at VDL size is not correct");


    //
    // Restore file size
    //
    status = file->SetFileSize(g_TestSize, sync);
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "Reset file size");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE((NT_SUCCESS(status)), "Reset file size");
        
    //
    // Do some basic testing.
    //

    status = BasicTest(*file, g_TestSize);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("basic testing failed with %x\n", status);
        goto Finish;
    }

    //
    // Do some knarly testing.
    //

    status = KnarlyTest(*file, g_TestSize);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("knarly testing failed with %x\n", status);
        goto Finish;
    }

#if !defined(PLATFORM_UNIX) 
    //
    // Now do this with high IO priority
    //
    status = file->SetSystemIoPriorityHint(KBlockFile::SystemIoPriorityHint::eLow, sync);
    KInvariant(NT_SUCCESS(status));
    status = sync.WaitForCompletion();
    KInvariant(NT_SUCCESS(status));
    
    //
    // Verify SystemIoPriorityHint is high
    //
    hint = file->GetSystemIoPriorityHint();
    VERIFY_IS_TRUE( (hint == KBlockFile::eLow), "SystemIoPriorityHint should be eLow");
#endif
    
    status = KnarlyTest(*file, g_TestSize);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("knarly testing failed with %x\n", status);
        goto Finish;
    }

#if KTL_USER_MODE
#else
    if (! SparseFile)
    {
#if !defined(PLATFORM_UNIX) 
        //
        // Now do this with high IO priority
        //
        status = file->SetSystemIoPriorityHint(KBlockFile::SystemIoPriorityHint::eHigh, sync);
        KInvariant(NT_SUCCESS(status));
        status = sync.WaitForCompletion();

        KInvariant(NT_SUCCESS(status));
#endif
        status = KnarlyTest(*file, g_TestSize);

        if (!NT_SUCCESS(status)) {
            KTestPrintf("knarly testing failed with %x\n", status);
            goto Finish;
        }
    }
#endif


    //
    // Do delayed close test. This verifies that the file is not closed
    // while in the context of the IoCompletion routine
    //
    {
        static const ULONG NumberOutstandingIO = 1024;
        KSynchronizer* sync1[NumberOutstandingIO];
        KIoBuffer::SPtr ioBuffer;
        PVOID buffer;

        status = KIoBuffer::CreateSimple(KBlockFile::BlockSize, ioBuffer, buffer, KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            KTestPrintf("create io buffer failed %x\n", status);
            goto Finish;
        }

        for (ULONG i = 0; i < NumberOutstandingIO; i++)
        {
            sync1[i] = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KSynchronizer();
            status = file->Transfer(KBlockFile::eForeground, KBlockFile::eRead, 0, *ioBuffer, *sync1[i]);
        }

        BOOLEAN b = file.Reset();
        if (b)
        {
            KTestPrintf("KBlockFile was destructed in foreground. Consider increasing the number of outstanding IOs\n");
        }

        for (ULONG i = 0; i < NumberOutstandingIO; i++)
        {
            status = sync1[i]->WaitForCompletion();

            if(! NT_SUCCESS(status))
            {
                KTestPrintf("Transfer failed. Status: %x\n", status);
            }
        }
    }

    //
    // Clean up.
    //

    status = STATUS_SHARING_VIOLATION;

    while ((count > 0) && (status == STATUS_SHARING_VIOLATION))
    {
        KTestPrintf("Try delete test file: %d\n", count);
        KNt::Sleep(500L);  //  Stabilize test due to some race condition (delete failing with STATUS_SHARING_VIOLATION)
        status = DeleteTestFile(dirName, fileName);
        count--;
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not delete test file %x\n", status);
    }

Finish:

    return(status);
}


NTSTATUS
KBlockFileNoPrivTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    KWString dirName(KtlSystem::GlobalNonPagedAllocator());

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KBlockFileNoPrivTest Nonsparse test\n");
    status = RunTheTests(FALSE, dirName);
	VERIFY_IS_TRUE((status == STATUS_PRIVILEGE_NOT_HELD), "Expect nonsparse to fail");

    KTestPrintf("Starting KBlockFileNoPrivTest Sparse test\n", "Expect sparse to succeed");
    status = RunTheTests(TRUE, dirName);
	VERIFY_IS_TRUE(NT_SUCCESS(status), "Expect sparse to succeed");

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KBlockFileNoPrivTest(
    int argc, WCHAR* args[]
    )
{
    NTSTATUS status;

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KBlockFileNoPrivTestX(argc, args);

    KtlSystem::Shutdown();

    if (NT_SUCCESS(status)) {
        KTestPrintf("KBlockFileNoPrivTests completed successfully.\n");
    } else {
        KTestPrintf("KBlockFileNoPrivTests failed with status: %x\n", status);
    }

    return status;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs);
#endif
    return RtlNtStatusToDosError(KBlockFileNoPrivTest(argc, args));
}
#endif
