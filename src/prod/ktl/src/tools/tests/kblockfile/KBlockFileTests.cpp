/*++

Copyright (c) Microsoft Corporation

Module Name:

    KBlockFileTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KBlockFileTests.h.
    2. Add an entry to the gs_KuTestCases table in KBlockFileTestCases.cpp.
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
#include <unistd.h>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KBlockFileTests.h"
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
                KTestPrintf("Block 0x%llx at offset 0x%lx should be filled with 0x%x, found 0x%x\n", FileOffset + (i * KBlockFile::BlockSize), j, fillByte, p[j]);
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
        printf("Could not create test context %x\n", status);
#else
        KTestPrintf("Could not create test context %x\n", status);
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
        printf("Could not delete file %x\n", status);
#else
        KTestPrintf("Could not delete file %x\n", status);
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
        KTestPrintf("valid data length didn't work.  The write to the last block of the file took %d seconds. This may be ok if running in a non-elevated window.\n", elapsedSeconds);
#if KTL_USER_MODE
#else
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
#endif
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
        KTestPrintf("DoKnarly write test failed with %x\n", status);
        goto Finish;
    }

    status = DoKnarlyTest(File, TestSize, KBlockFile::eRead, completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("DoKnarly read test failed with %x\n", status);
        goto Finish;
    }

Finish:

    return status;
}

#endif

#if KTL_USER_MODE
# define VERIFY_IS_TRUE(__condition, s) if (! (__condition)) {  KTestPrintf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", s, __FILE__, __LINE__); KInvariant(FALSE); };
#else
# define VERIFY_IS_TRUE(__condition, s) if (! (__condition)) {  KDbgPrintf("VERIFY_IS_TRUE(%s) failed at %s line %d\n", s, __FILE__, __LINE__); KInvariant(FALSE); };
#endif

class WorkerAsync : public KAsyncContextBase
{
    K_FORCE_SHARED_WITH_INHERITANCE(WorkerAsync);

public:
    virtual VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) = 0;

protected:
    virtual VOID FSMContinue(
        __in NTSTATUS Status,
        __in KAsyncContextBase& Async
        )
    {
        UNREFERENCED_PARAMETER(Status);
        UNREFERENCED_PARAMETER(Async);

        Complete(STATUS_SUCCESS);
    }

    virtual VOID OnReuse() = 0;

    virtual VOID OnCompleted()
    {
    }

    virtual VOID OnCancel()
    {
    }

private:
    VOID OnStart()
    {
        _Completion.Bind(this, &WorkerAsync::OperationCompletion);
        FSMContinue(STATUS_SUCCESS, *this);
    }

    VOID OperationCompletion(
        __in_opt KAsyncContextBase* const ParentAsync,
        __in KAsyncContextBase& Async
        )
    {
        UNREFERENCED_PARAMETER(ParentAsync);
        FSMContinue(Async.Status(), Async);
    }

protected:
    KAsyncContextBase::CompletionCallback _Completion;

};

WorkerAsync::WorkerAsync()
{
}

WorkerAsync::~WorkerAsync()
{
}

class WriteThenReadAsync : public WorkerAsync
{
    K_FORCE_SHARED(WriteThenReadAsync);

public:
    static NTSTATUS
        Create(
        __in KAllocator& Allocator,
        __in ULONG AllocationTag,
        __out WriteThenReadAsync::SPtr& Context
        )
    {
        NTSTATUS status;
        WriteThenReadAsync::SPtr context;

        context = _new(AllocationTag, Allocator) WriteThenReadAsync();
        if (context == nullptr)
        {
            status = STATUS_INSUFFICIENT_RESOURCES;
            return(status);
        }

        status = context->Status();
        if (!NT_SUCCESS(status))
        {
            return(status);
        }

        Context = context.RawPtr();

        return(STATUS_SUCCESS);
    }

    VOID StartIt(
        __in PVOID Parameters,
        __in_opt KAsyncContextBase* const ParentAsyncContext,
        __in_opt KAsyncContextBase::CompletionCallback CallbackPtr) override
    {
        StartParameters* startParameters = (StartParameters*)Parameters;

        _State = Initial;

        _BlockDevice = startParameters->_BlockDevice;
        _BlockIndexPtr = startParameters->_BlockIndexPtr;
        _MaxBlockIndex = startParameters->_MaxBlockIndex;
        _ReadOnly = startParameters->_ReadOnly;

        Start(ParentAsyncContext, CallbackPtr);
    }

    struct StartParameters
    {
        KBlockDevice::SPtr _BlockDevice;
        ULONGLONG* volatile _BlockIndexPtr;
        ULONGLONG _MaxBlockIndex;
        BOOLEAN _ReadOnly;
    };

private:
    enum  FSMState { Initial = 0, StartNextWrite = 1, WriteRecord = 2, ReadRecord = 3, Completed = 4, Failed = 5 };
    FSMState _State;

protected:
    VOID FSMContinue(
        __in NTSTATUS Status,
        __in KAsyncContextBase& Async
        ) override
    {
        UNREFERENCED_PARAMETER(Async);

        if (!NT_SUCCESS(Status))
        {
            KTraceFailedAsyncRequest(Status, this, _State, 0);
            _State = Failed;
            VERIFY_IS_TRUE(false, "completion");
            Complete(Status);
            return;
        }

        switch (_State)
        {
            case Initial:
            {
                PVOID p;

                Status = KIoBuffer::CreateSimple(_MaxBufferLength, _WriteIoBuffer, p, KtlSystem::GlobalNonPagedAllocator());
                VERIFY_IS_TRUE(NT_SUCCESS(Status), "Alloc KioBuffer");

                Status = KIoBuffer::CreateSimple(_MaxBufferLength, _ReadIoBuffer, p, KtlSystem::GlobalNonPagedAllocator());
                VERIFY_IS_TRUE(NT_SUCCESS(Status), "Alloc KioBuffer");

                _State = StartNextWrite;
                // fall through
            }

            case StartNextWrite:
            {
StartNextWrite:
                ULONGLONG blockIndex;

                blockIndex = InterlockedIncrement64((volatile LONG64*)_BlockIndexPtr);

                if (blockIndex >= _MaxBlockIndex)
                {
                    //
                    // we are all done
                    //
                    _State = Completed;
                    goto CompletedState;
                }

                _Offset = blockIndex * 4096;
                _Length = 4096;

                PUCHAR writePtr;
                writePtr = (PUCHAR)_WriteIoBuffer->First()->GetBuffer();
                for (ULONG i = 0; i < _Length; i++)
                {
                    writePtr[i] = (i * _Offset) % 255;
                }

                _State = WriteRecord;
                if (!_ReadOnly)
                {
                    Status = _BlockDevice->Write(KBlockFile::IoPriority::eForeground, _Offset, *_WriteIoBuffer, _Completion);
                    VERIFY_IS_TRUE(NT_SUCCESS(Status), "write");
                    break;
                }

                //
                // Fall through
                //
            }

            case WriteRecord:
            {
                _State = ReadRecord;
                Status = _BlockDevice->Read(_Offset, _Length, TRUE, _ReadIoBuffer, _Completion);
                VERIFY_IS_TRUE(NT_SUCCESS(Status), "read");

                break;
            }

            case ReadRecord:
            {
                PUCHAR readPtr;
                PUCHAR writePtr;

                readPtr = (PUCHAR)_ReadIoBuffer->First()->GetBuffer();
                writePtr = (PUCHAR)_WriteIoBuffer->First()->GetBuffer();

                for (ULONG i = 0; i < _Length; i++)
                {
                    if (readPtr[i] != writePtr[i])
                    {
                        VERIFY_IS_TRUE(false, "data mismatch");
                        Status = STATUS_UNSUCCESSFUL;
                        break;
                    }
                }

                goto StartNextWrite;
            }

            case Completed:
            {
CompletedState:
              Complete(STATUS_SUCCESS);
              return;
            }

            default:
            {
               Status = STATUS_UNSUCCESSFUL;
               KTraceFailedAsyncRequest(Status, this, _State, 0);
               Complete(Status);
               return;
            }
        }

        return;
    }

    VOID OnReuse() override
    {
    }

    VOID OnCompleted() override
    {
        _BlockDevice = nullptr;
        _WriteIoBuffer = nullptr;
        _ReadIoBuffer = nullptr;
    }

    VOID OnCancel() override
    {
    }

    //
    // Parameters
    //
    KBlockDevice::SPtr _BlockDevice;
    volatile ULONGLONG* _BlockIndexPtr;
    ULONGLONG _MaxBlockIndex;
    BOOLEAN _ReadOnly;

    //
    // Internal
    //
    static const ULONG _MaxBufferLength = 4096;        // CONSIDER: use larger random sizes
    ULONGLONG _Offset;
    ULONG _Length;
    KIoBuffer::SPtr _WriteIoBuffer;
    KIoBuffer::SPtr _ReadIoBuffer;
};

WriteThenReadAsync::WriteThenReadAsync()
{
}

WriteThenReadAsync::~WriteThenReadAsync()
{
}


NTSTATUS WriteData(
    __inout KBlockDevice& File,
    __in ULONGLONG Offset,
    __in ULONG Length,
    __out KIoBuffer::SPtr& ioBuffer,
    __in KBlockFile::SystemIoPriorityHint IoPriorityHint = KBlockFile::SystemIoPriorityHint::eNotDefined
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    PVOID p;
    PUCHAR writePtr;

    status = KIoBuffer::CreateSimple(Length, ioBuffer, p, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Alloc KioBuffer");

    writePtr = (PUCHAR)p;
    for (ULONG i = 0; i < Length; i++)
    {
        writePtr[i] = (i * Offset) % 255;
    }

    status = File.Write(KBlockFile::IoPriority::eForeground, IoPriorityHint, 0, *ioBuffer, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "write");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "write");

    return(status);
}

NTSTATUS ReadAndVerifyData(
    __inout KBlockDevice& File,
    __in ULONGLONG Offset,
    __in ULONG Length,
    __in KIoBuffer& writeIoBuffer
    )
{
    NTSTATUS status;
    KIoBuffer::SPtr readIoBuffer;
    KSynchronizer sync;

    status = File.Read(Offset, Length, TRUE, readIoBuffer, sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "read");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "read");

    PUCHAR readPtr = (PUCHAR)readIoBuffer->First()->GetBuffer();
    PUCHAR writePtr = (PUCHAR)writeIoBuffer.First()->GetBuffer();

    for (ULONG i = 0; i < Length; i++)
    {
        if (readPtr[i] != writePtr[i])
        {
            VERIFY_IS_TRUE(false, "data mismatch");
            status = STATUS_UNSUCCESSFUL;
            break;
        }
    }
    return(status);
}

NTSTATUS ReadAheadCacheTestWorker(
    __inout KBlockDevice& BlockDevice,
    __in ULONGLONG TestSize,
    __in ULONG ReadAheadSize,
    __in BOOLEAN IsSparseFile
    )
{
    NTSTATUS status;
    KSynchronizer sync;
    const ULONG blockSize = 4096;
    const ULONGLONG maxBlockIndex = (TestSize / blockSize) / 4;


    if (IsSparseFile)
    {
        //
        // With Sparse file trim all allocations
        //
        status = BlockDevice.Trim(0, TestSize + 1, sync, nullptr, nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Trim");

        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Trim");
    }

    //
    // Test 1: Write less than the readahead size and then try to read it back. Verify no error and that we get correct data
    //
    ULONG bufferSize = 4 * 1024;
    KIoBuffer::SPtr writeIoBuffer;
    status = WriteData(BlockDevice, 0, bufferSize, writeIoBuffer, KBlockFile::SystemIoPriorityHint::eLow);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "WriteData");

    status = ReadAndVerifyData(BlockDevice, 0, bufferSize, *writeIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadData");

    //
    // Test 2: Write full readahead size and then read it back in pieces. Verify no error and that we get correct data
    //
    bufferSize = ReadAheadSize;
    status = WriteData(BlockDevice, 0, bufferSize, writeIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "WriteData");

    status = ReadAndVerifyData(BlockDevice, 0, bufferSize, *writeIoBuffer);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadData");

    {
        //
        // Test 3: Multithreaded Write followed by read for the entire file. Verify no error and we get the correct data
        //
        const ULONG MaxAsyncs = 32;

        KArray<WriteThenReadAsync::SPtr> asyncs(KtlSystem::GlobalNonPagedAllocator(), MaxAsyncs);
        asyncs.SetCount(MaxAsyncs);
        KArray<KSynchronizer*> syncs(KtlSystem::GlobalNonPagedAllocator(), MaxAsyncs);
        syncs.SetCount(MaxAsyncs);
        ULONGLONG blockIndex = 0;
        WriteThenReadAsync::StartParameters startParameters;

        asyncs.SetCount(MaxAsyncs);
        syncs.SetCount(MaxAsyncs);
        
        startParameters._BlockDevice = &BlockDevice;
        startParameters._BlockIndexPtr = &blockIndex;
        startParameters._MaxBlockIndex = maxBlockIndex;
        startParameters._ReadOnly = FALSE;
        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            status = WriteThenReadAsync::Create(KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST, asyncs[i]);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Create Async");
            syncs[i] = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) KSynchronizer();
        }

        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            asyncs[i]->StartIt(&startParameters, nullptr, *(syncs[i]));
        }

        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            status = syncs[i]->WaitForCompletion(10 * 1000);
            if (status == STATUS_IO_TIMEOUT)
            {
#if KTL_USER_MODE
                KTestPrintf("%d read/write blocks complete\n", (int)blockIndex);
#endif
                i--;
            }
            else
            {
                VERIFY_IS_TRUE(NT_SUCCESS(status), "Sync Status");
            }
        }

        //
        // Test 4: Multithreaded reads. Verify no error and we get the correct data
        //
        blockIndex = 0;
        startParameters._ReadOnly = TRUE;

        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            asyncs[i]->Reuse();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Create Async");
        }

        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            asyncs[i]->StartIt(&startParameters, nullptr, *(syncs[i]));
        }

        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            status = syncs[i]->WaitForCompletion(10 * 1000);
            if (status == STATUS_IO_TIMEOUT)
            {
#if KTL_USER_MODE
                KTestPrintf("%d read blocks complete\n", (int)blockIndex);
#endif
                i--;
            }
            else
            {
                VERIFY_IS_TRUE(NT_SUCCESS(status), "Sync Status");
            }
        }

#define RoundDownTo4K(x)  ( (x)&~0xfff )

        //
        // Test 5: NonContiguousRead tests - read without preallocation
        //         and even reads
        //
        {
            ULONG readLength = (ULONG)TestSize;
            ULONG bufferLength = RoundDownTo4K(readLength / 64);
            KIoBuffer::SPtr ioBuffer;

            status = BlockDevice.ReadNonContiguous(0, readLength, bufferLength, ioBuffer, sync, NULL, NULL, NULL);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 5: ReadNonContiguous");
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 5: ReadNonContiguous complete");
            VERIFY_IS_TRUE(ioBuffer->QuerySize() == readLength, "Test 5: ioBuffer->QuerySize");
            VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == 64, "Test 5: ioBuffer->QueryNumberElements");

            KIoBufferElement::SPtr e;
            for (e = ioBuffer->First(); e; e = ioBuffer->Next(*e))
            {
                VERIFY_IS_TRUE(e->QuerySize() == bufferLength, "Test 5: e->QuerySize() == bufferLength");
            }
        }

        //
        // Test 6: NonContiguousRead tests - read without preallocation
        //         and different last read
        //
        {
            ULONG readCount = 64;
            ULONG readLength = (readCount * (1024 * 1024)) + 4096;
            ULONG bufferLength = RoundDownTo4K(readLength / readCount);
            ULONG lastBufferLength = readLength - (bufferLength * readCount);
            KIoBuffer::SPtr ioBuffer;

            status = BlockDevice.ReadNonContiguous(0, readLength, bufferLength, ioBuffer, sync, NULL, NULL, NULL);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 6: ReadNonContiguous");
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 6: ReadNonContiguous complete");
            VERIFY_IS_TRUE(ioBuffer->QuerySize() == readLength, "Test 6: ioBuffer->QuerySize");
            VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == readCount+1, "Test 6: ioBuffer->QueryNumberElements");

            KIoBufferElement::SPtr e;
            e = ioBuffer->First();

            for (ULONG i = 0; i < readCount; i++)
            {
                VERIFY_IS_TRUE(e->QuerySize() == bufferLength, "Test 6: e->QuerySize() == bufferLength");
                e = ioBuffer->Next(*e);
            }
            VERIFY_IS_TRUE(e->QuerySize() == lastBufferLength, "Test 6: e->QuerySize() == lastBufferLength");
        }

        //
        // Test 7: NonContiguousRead tests - read with preallocation
        //
        {
            ULONG readCount = 256;
            ULONG readLength = (ULONG)TestSize;
            ULONG bufferLength = RoundDownTo4K(readLength / readCount);
            KInvariant( (readCount * bufferLength) == readLength );
            KIoBuffer::SPtr ioBuffer;
            KAsyncContextBase::SPtr readNonContiguousContext;
            KAsyncContextBase::SPtr async;
            KArray<KAsyncContextBase::SPtr> readOps(KtlSystem::GlobalNonPagedAllocator());

            status = BlockDevice.AllocateNonContiguousReadContext(readNonContiguousContext);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: AllocateNonContiguousReadContext");

            for (ULONG i = 0; i < readCount; i++)
            {
                status = BlockDevice.AllocateReadContext(async);
                VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: AllocateRead");
                readOps.Append(async);
            }

            status = BlockDevice.ReadNonContiguous(0, readLength, bufferLength, ioBuffer, sync, nullptr, readNonContiguousContext, &readOps);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: ReadNonContiguous");
            status = sync.WaitForCompletion();
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: ReadNonContiguous complete");
            VERIFY_IS_TRUE(ioBuffer->QuerySize() == readLength, "Test 7: ioBuffer->QuerySize");
            VERIFY_IS_TRUE(ioBuffer->QueryNumberOfIoBufferElements() == readCount, "Test 7: ioBuffer->QueryNumberElements");

            KIoBufferElement::SPtr e;
            e = ioBuffer->First();

            for (ULONG i = 0; i < readCount; i++)
            {
                VERIFY_IS_TRUE(e->QuerySize() == bufferLength, "Test 6: e->QuerySize() == bufferLength");
                e = ioBuffer->Next(*e);
            }
        }

        //
        // Test 8: NonContiguousRead tests - Bad parameter tests
        //
        {
            ULONG readCount = 256;
            ULONG readLength = (ULONG)TestSize - 4096;
            ULONG bufferLength = RoundDownTo4K(readLength / readCount);
            KIoBuffer::SPtr ioBuffer;
            KAsyncContextBase::SPtr readNonContiguousContext;
            KAsyncContextBase::SPtr async;
            KArray<KAsyncContextBase::SPtr> readOps(KtlSystem::GlobalNonPagedAllocator());

            status = BlockDevice.AllocateNonContiguousReadContext(readNonContiguousContext);
            VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: AllocateNonContiguousReadContext");

            for (ULONG i = 0; i < readCount; i++)
            {
                status = BlockDevice.AllocateReadContext(async);
                VERIFY_IS_TRUE(NT_SUCCESS(status), "Test 7: AllocateRead");
                readOps.Append(async);
            }

            //
            // Bad Offset + ReadLength
            //
            status = BlockDevice.ReadNonContiguous(8192, readLength, bufferLength, ioBuffer, sync, nullptr, readNonContiguousContext, &readOps);
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER, "Test 8: ReadNonContiguous Bad Offset");

            status = BlockDevice.ReadNonContiguous(0, readLength, 2*bufferLength, ioBuffer, sync, nullptr, readNonContiguousContext, &readOps);
            VERIFY_IS_TRUE(status == STATUS_INVALID_PARAMETER, "Test 8: ReadNonContiguous Bad readCount");

            //
            // ReadLength and BufferLength are zero
            //
            // TODO:
        }

        //
        // Test 9: Read using a single op
        //
        // TODO:
                               
        for (ULONG i = 0; i < MaxAsyncs; i++)
        {
            _delete(syncs[i]);
        }
    }

    return(STATUS_SUCCESS);
}

NTSTATUS ReadAheadCacheTests(
    __in ULONGLONG TestSize,
    __in KWString dirName
    )
{
    NTSTATUS status;
    KBlockDevice::SPtr blockDevice;
    KWString fileName(dirName);
    KGuid deviceId;
    KSynchronizer sync;
    const ULONG readAheadSize = 128 * 1024;

    TestContext::SPtr context;
    status = TestContext::Create(context);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test context %x\n", status);
        return(status);
    }

    KAsyncContextBase::CompletionCallback completion;
    completion.Bind(context.RawPtr(), &TestContext::Completion);

    //
    // First test against sparse file
    //
    deviceId.CreateNew();
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\SparseRH.test";
#else
    fileName += L"/SparseRH.test";
#endif

    KTestPrintf("Start Sparse ReadAhead test\n");
    status = DeleteTestFile(dirName, fileName);


    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    status = KBlockDevice::CreateDiskBlockDevice(deviceId,
                                                 TestSize,
                                                 TRUE,
                                                 TRUE,
                                                 fileName,
                                                 blockDevice,
                                                 sync,
                                                 nullptr,
                                                 KtlSystem::GlobalNonPagedAllocator(),
                                                 KTL_TAG_TEST, NULL, FALSE, TRUE, readAheadSize);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseRH");

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseRH");


#if !defined(PLATFORM_UNIX) 
    //
    // Test setting IoPriority to low and then back to normal
    //
    KAsyncContextBase::SPtr setSystemIoPriorityHint;
    status = blockDevice->AllocateSetSystemIoPriorityHintContext(setSystemIoPriorityHint);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "AllocateSetSystemIoPriorityHintContext1");
    status = blockDevice->SetSystemIoPriorityHint(KBlockFile::SystemIoPriorityHint::eLow, sync, NULL, setSystemIoPriorityHint);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSytemIoPriorityHint1");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSytemIoPriorityHint1");

    status = blockDevice->SetSystemIoPriorityHint(KBlockFile::SystemIoPriorityHint::eNormal, sync, NULL, setSystemIoPriorityHint);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSytemIoPriorityHint1");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSytemIoPriorityHint1");
#endif

    status = ReadAheadCacheTestWorker(*blockDevice, TestSize, readAheadSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadAheadTest Sparse");

    blockDevice = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);

    //
    // Next test against non sparse file RH
    //
    deviceId.CreateNew();
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\NonSparseRH.test";
#else
    fileName += L"/NonSparseRH.test";
#endif

    KTestPrintf("\n");
    KTestPrintf("Start NonSparse ReadAhead test\n");

    status = DeleteTestFile(dirName, fileName);

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    status = KBlockDevice::CreateDiskBlockDevice(deviceId,
        TestSize,
        TRUE,
        TRUE,
        fileName,
        blockDevice,
        sync,
        nullptr,
        KtlSystem::GlobalNonPagedAllocator(),
        KTL_TAG_TEST, NULL, FALSE, FALSE, readAheadSize);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparseRH");

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparseRH");

    status = ReadAheadCacheTestWorker(*blockDevice, TestSize, readAheadSize, FALSE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadAheadTest NonSparse");

    blockDevice = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);

    //
    // First test against sparse file
    //
    deviceId.CreateNew();
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\Sparse.test";
#else
    fileName += L"/Sparse.test";
#endif

    KTestPrintf("\n");
    KTestPrintf("Start Sparse test\n");

    status = DeleteTestFile(dirName, fileName);

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    status = KBlockDevice::CreateDiskBlockDevice(deviceId,
        TestSize,
        TRUE,
        TRUE,
        fileName,
        blockDevice,
        sync,
        nullptr,
        KtlSystem::GlobalNonPagedAllocator(),
        KTL_TAG_TEST, NULL, FALSE, TRUE, 0);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparse");

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparse");

    status = ReadAheadCacheTestWorker(*blockDevice, TestSize, readAheadSize, TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Test Sparse");

    blockDevice = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);

    //
    // Next test against non sparse file RH
    //
    deviceId.CreateNew();
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\NonSparse.test";
#else
    fileName += L"/NonSparse.test";
#endif

    KTestPrintf("\n");
    KTestPrintf("Start Nonsparse test\n");

    status = DeleteTestFile(dirName, fileName);

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), completion);

    if (status == STATUS_PENDING) {
        status = context->Wait();
    }

    status = KBlockDevice::CreateDiskBlockDevice(deviceId,
        TestSize,
        TRUE,
        TRUE,
        fileName,
        blockDevice,
        sync,
        nullptr,
        KtlSystem::GlobalNonPagedAllocator(),
        KTL_TAG_TEST, NULL, FALSE, FALSE, 0);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparse");

    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparse");

    status = ReadAheadCacheTestWorker(*blockDevice, TestSize, readAheadSize, FALSE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Test NonSparse");

    blockDevice = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);
    
    return(STATUS_SUCCESS);
}

NTSTATUS ReadVerifyAllZero(
    KBlockFile& BlockFile,
    ULONGLONG StartingOffset,
    ULONG Length
)
{
    NTSTATUS status;
    KSynchronizer sync;
    ULONG loops;
    ULONG twoFiftySixK = 256 * 1024;
    ULONG lastReadSize;
    KIoBuffer::SPtr io256KBuffer;
    PVOID pp;
    PUCHAR p;
    ULONGLONG offset;

    status = KIoBuffer::CreateSimple(twoFiftySixK, io256KBuffer, pp, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "##__LINE__");
    p = (PUCHAR)pp;

    loops = Length / twoFiftySixK;
    lastReadSize = Length - (loops * twoFiftySixK);

    offset = StartingOffset;
    for (ULONG i = 0; i < loops; i++)
    {
        status = BlockFile.Transfer(KBlockFile::eForeground,
                                    KBlockFile::eRead,
                                    offset,
                                    *io256KBuffer,
                                    sync,
                                    NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "##__LINE__");
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "##__LINE__");

        for (ULONG j = 0; j < twoFiftySixK; j++)
        {
            VERIFY_IS_TRUE(p[j] == 0, "##__LINE__");              
        }
        offset += twoFiftySixK;
    }

    if (lastReadSize != 0)
    {
        status = BlockFile.Transfer(KBlockFile::eForeground,
                                    KBlockFile::eRead,
                                    offset,
                                    p,
                                    lastReadSize,
                                    sync,
                                    NULL);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "##__LINE__");
        status = sync.WaitForCompletion();
        VERIFY_IS_TRUE(NT_SUCCESS(status), "##__LINE__");

        for (ULONG j = 0; j < lastReadSize; j++)
        {
            VERIFY_IS_TRUE(p[j] == 0, "##__LINE__");              
        }
    }
    
    return(STATUS_SUCCESS);
}

#if KTL_USER_MODE
NTSTATUS WriteAfterCloseTest(
    __in KWString dirName
    )
{
    NTSTATUS status;
    static const ULONG IoCount = 32;
    static const ULONG bufferSize = 0x1000;
    ULONG fileSize = IoCount * bufferSize;
    KBlockFile::SPtr blockFile;
    KIoBuffer::SPtr ioBuffer;
    PVOID p;    
    KWString fileName(dirName);
    KSynchronizer sync;
    KSynchronizer syncs[IoCount];

    status = KIoBuffer::CreateSimple(bufferSize, ioBuffer, p, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Alloc");
    
    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateDirectory");
    status = sync.WaitForCompletion();

    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\writeafterclose.test";
#else
    fileName += L"/writeafterclose.test";
#endif
    
    status = KBlockFile::Create(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eCreateAlways,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");

    status = blockFile->SetFileSize(fileSize, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == fileSize, "Verify Sparse File Size");

    ULONG i;
    for (i = 0; i < IoCount; i++)
    {
        status = blockFile->Transfer(KBlockFile::eForeground,
                                     KBlockFile::eWrite,
                                     i * bufferSize,
                                     *ioBuffer,
                                     syncs[i],
                                     nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");     
    }

    blockFile->Close();

    
    for (i = 0; i < IoCount; i++)
    {       
        status = syncs[i].WaitForCompletion();
        VERIFY_IS_TRUE((NT_SUCCESS(status) ||
                        (status == STATUS_INVALID_HANDLE) || (status == STATUS_FILE_CLOSED) || (status == STATUS_CANCELLED)), "Syncs");
    }

    i = 0;
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 i * bufferSize,
                                 *ioBuffer,
                                 syncs[i],
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");     
    status = syncs[i].WaitForCompletion();
    VERIFY_IS_TRUE(((status == STATUS_INVALID_HANDLE) || (status == STATUS_FILE_CLOSED) || (status == STATUS_CANCELLED)), "Syncs");

    status = KVolumeNamespace::DeleteFileOrDirectory(fileName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Delete file");
    status = sync.WaitForCompletion();

    //
    // Test 2: Try again but with a sparse file
    //
    status = KBlockFile::CreateSparseFile(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eCreateAlways,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");

    status = blockFile->SetFileSize(fileSize, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == fileSize, "Verify Sparse File Size");

    for (i = 0; i < IoCount; i++)
    {
        status = blockFile->Transfer(KBlockFile::eForeground,
                                     KBlockFile::eWrite,
                                     i * bufferSize,
                                     *ioBuffer,
                                     syncs[i],
                                     nullptr);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");     
    }

    blockFile->Close();

    
    for (i = 0; i < IoCount; i++)
    {       
        status = syncs[i].WaitForCompletion();
        VERIFY_IS_TRUE((NT_SUCCESS(status) ||
                        (status == STATUS_INVALID_HANDLE) || (status == STATUS_FILE_CLOSED) || (status == STATUS_CANCELLED)), "Syncs");
    }

    i = 0;
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 i * bufferSize,
                                 *ioBuffer,
                                 syncs[i],
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");     
    status = syncs[i].WaitForCompletion();
    VERIFY_IS_TRUE(((status == STATUS_INVALID_HANDLE) || (status == STATUS_FILE_CLOSED) || (status == STATUS_CANCELLED)), "Syncs");

    status = KVolumeNamespace::DeleteFileOrDirectory(fileName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Delete file");
    status = sync.WaitForCompletion();  
    
    return(STATUS_SUCCESS);
}
#endif

NTSTATUS MiscTest(
    __in ULONGLONG TestSize,
    __in KWString dirName
    )
{
    NTSTATUS status;
    KBlockFile::SPtr blockFile;
    KWString fileName(dirName);
    KSynchronizer sync;

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateDirectory");
    status = sync.WaitForCompletion();

    //
    // First test against sparse file
    //
    KArray<KBlockFile::AllocatedRange> allocations(KtlSystem::GlobalNonPagedAllocator());
    KIoBuffer::SPtr io128KBuffer;
    KIoBuffer::SPtr ioRead128KBuffer;
    PVOID pp;
    PUCHAR p;
    ULONG oneTwentyEightK = 128 * 1024;
    ULONGLONG filepos1, filepos2, filepos3;

    filepos1 = 0;
    filepos2 = (TestSize / 2);
    filepos3 = TestSize - oneTwentyEightK;
    
    status = KIoBuffer::CreateSimple(oneTwentyEightK, io128KBuffer, pp, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KIoBuffer::CreateSimple");
    p = (PUCHAR)pp;
    for (ULONG i = 0; i < oneTwentyEightK; i++)
    {
        p[i] = (UCHAR)(i % 255);
    }   

    status = KIoBuffer::CreateSimple(oneTwentyEightK, ioRead128KBuffer, pp, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "KIoBuffer::CreateSimple");
    
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\misc.test";
#else
    fileName += L"/misc.test";
#endif

    KTestPrintf("Start Sparse misc test\n");
    status = DeleteTestFile(dirName, fileName);

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseFile");
    status = sync.WaitForCompletion();
        
    status = KBlockFile::CreateSparseFile(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eCreateAlways,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseFile");


    //
    // Test 1: Verify file is a sparse file, size is 0, there are no
    //         allocations
    //
    VERIFY_IS_TRUE(blockFile->IsSparseFile() == TRUE, "Verify Sparse File");
    VERIFY_IS_TRUE(blockFile->QueryFileSize() == 0, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 0, "AllocationCount");

    
    //
    // Test 2: Set file size and then verify size and no allocations
    //
    status = blockFile->SetFileSize(TestSize, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 0, "AllocationCount");
    status = ReadVerifyAllZero(*blockFile, 0, (ULONG)TestSize);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");

    //
    // Test 3: Write at very end of file and verify size and allocations
    //
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 filepos3,
                                 *io128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 1, "AllocationCount");
    VERIFY_IS_TRUE(allocations[0].Offset == filepos3, "Offset == filepos3");
    VERIFY_IS_TRUE(allocations[0].Length == oneTwentyEightK, "Length == oneTwentyEightK");

    status = ReadVerifyAllZero(*blockFile, 0, (ULONG)filepos3);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");
    

    //
    // Test 4: Write at the front and middle of the file and then
    //         verify file size and allocations
    //
    
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 filepos1,
                                 *io128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 filepos2,
                                 *io128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 3, "AllocationCount");
    VERIFY_IS_TRUE(allocations[0].Offset == filepos1, "Offset == filepos1");
    VERIFY_IS_TRUE(allocations[0].Length == oneTwentyEightK, "Length == oneTwentyEightK");
    VERIFY_IS_TRUE(allocations[1].Offset == filepos2, "Offset == filepos2");
    VERIFY_IS_TRUE(allocations[1].Length == oneTwentyEightK, "Length == oneTwentyEightK");
    VERIFY_IS_TRUE(allocations[2].Offset == filepos3, "Offset == filepos3");
    VERIFY_IS_TRUE(allocations[2].Length == oneTwentyEightK, "Length == oneTwentyEightK");

    status = ReadVerifyAllZero(*blockFile, oneTwentyEightK, (ULONG)(filepos2 - oneTwentyEightK));
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");

    status = ReadVerifyAllZero(*blockFile, filepos2 + oneTwentyEightK, (ULONG)(filepos3 - (filepos2 + oneTwentyEightK)));
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");
    
    //
    // Test 5: Trim allocations and verify file size, allocations and
    //         read data
    //
    status = blockFile->Trim(filepos2, filepos2 + oneTwentyEightK, sync, NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Trim");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Trim");
    
    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 2, "AllocationCount");
    VERIFY_IS_TRUE(allocations[0].Offset == filepos1, "Offset == filepos1");
    VERIFY_IS_TRUE(allocations[0].Length == oneTwentyEightK, "Length == oneTwentyEightK");
    VERIFY_IS_TRUE(allocations[1].Offset == filepos3, "Offset == filepos3");
    VERIFY_IS_TRUE(allocations[1].Length == oneTwentyEightK, "Length == oneTwentyEightK");

    status = ReadVerifyAllZero(*blockFile, oneTwentyEightK, (ULONG)(filepos3 - oneTwentyEightK));
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");

    //
    // Test 5: Close and reopen sparse file and verify that it is
    //         sparse, file size, alocations and read
    //
    blockFile = nullptr;
    KNt::Sleep(500);

    status = KBlockFile::Create(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eOpenExisting,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateSparseFile");

    status = blockFile->SetSparseFile(TRUE);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSparseFile");
    
    VERIFY_IS_TRUE(blockFile->IsSparseFile() == TRUE, "Verify Sparse File");
    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");

    allocations.Clear();
    status = blockFile->QueryAllocations(0,
                              blockFile->QueryFileSize(),
                              allocations,
                              sync,
                              nullptr,
                              NULL);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "QueryAllocations");
    VERIFY_IS_TRUE(allocations.Count() == 2, "AllocationCount");
    VERIFY_IS_TRUE(allocations[0].Offset == filepos1, "Offset == filepos1");
    VERIFY_IS_TRUE(allocations[0].Length == oneTwentyEightK, "Length == oneTwentyEightK");
    VERIFY_IS_TRUE(allocations[1].Offset == filepos3, "##__LINE__");
    VERIFY_IS_TRUE(allocations[1].Length == oneTwentyEightK, "Length == oneTwentyEightK");

    status = ReadVerifyAllZero(*blockFile, oneTwentyEightK, (ULONG)(filepos3 - oneTwentyEightK));
    VERIFY_IS_TRUE(NT_SUCCESS(status), "ReadVerifyAllZero");


    //
    // Test 6: Perform an IO with maximum number of KIoBufferElements
    //
    static const ULONG maxElements = 384;
    KIoBuffer::SPtr manyElementsKIoBuffer;

    status = KIoBuffer::CreateEmpty(manyElementsKIoBuffer, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "manyElementsKIoBuffer");
    
    for (ULONG i = 0; i < maxElements; i++)
    {
        KIoBufferElement::SPtr ioElement;
        PVOID p1;
        
        status = KIoBufferElement::CreateNew(0x1000, ioElement, p1, KtlSystem::GlobalNonPagedAllocator());
        VERIFY_IS_TRUE(NT_SUCCESS(status), "manyElementsKIoBuffer");

        manyElementsKIoBuffer->AddIoBufferElement(*ioElement);
    }

    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 0,
                                 *manyElementsKIoBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    manyElementsKIoBuffer = nullptr;
    
    //
    // Cleanup sparse file
    //
    blockFile = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);


    //
    // Next test against nonsparse file
    //
    fileName = dirName;
#if !defined(PLATFORM_UNIX)
    fileName += L"\\miscns.test";
#else
    fileName += L"/miscns.test";
#endif

    KTestPrintf("Start Non Sparse misc test\n");
    status = DeleteTestFile(dirName, fileName);

    LONGLONG eof;
    ULONGLONG smallFileSize = TestSize / 2;
    LONGLONG eofSmall = (LONGLONG)smallFileSize - (oneTwentyEightK + 7);
    ULONGLONG fileSizeSmall = (LONGLONG)smallFileSize - oneTwentyEightK;

    status = KVolumeNamespace::CreateDirectory(dirName, KtlSystem::GlobalNonPagedAllocator(), sync);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateDirectory");
    status = sync.WaitForCompletion();
    
    status = KBlockFile::Create(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eCreateAlways,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparseFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateNonSparseFile");

    ULONG ii = 0;
    //
    // Test 1: Verify not a sparse file, file size and EOF
    //
    VERIFY_IS_TRUE(blockFile->IsSparseFile() == FALSE, "Verify Not Sparse File");
    VERIFY_IS_TRUE(blockFile->QueryFileSize() == 0, "Verify File Size");
    
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == 0, "EOF wrong");

    KTestPrintf("Test %d, FileSize %I64x, EOF %I64x\n", ++ii, blockFile->QueryFileSize(), eof);

    //
    // Test 2: Set file size and verify file size and EOF
    //
    status = blockFile->SetFileSize(TestSize, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify Sparse File Size");
    
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == (LONGLONG)TestSize, "EOF wrong");
        
    //
    // Test 3: Set file size smaller and verify file size and EOF. Also
    //         write stuff in the section that is not deallocated and
    //         then verify that stuff remains after a file resize
    //
    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 filepos1,
                                 *io128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eRead,
                                 filepos1,
                                 *ioRead128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    PUCHAR p1, p2;
    p1 = (PUCHAR)io128KBuffer->First()->GetBuffer();
    p2 = (PUCHAR)ioRead128KBuffer->First()->GetBuffer();
    for (ULONG i = 0; i < oneTwentyEightK; i++)
    {
        VERIFY_IS_TRUE(p1[i] == p2[i], "equal");
    }

    
    status = blockFile->SetFileSize(smallFileSize, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == smallFileSize, "Verify Sparse File Size");
    
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == (LONGLONG)smallFileSize, "EOF wrong");

    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eRead,
                                 filepos1,
                                 *ioRead128KBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    p1 = (PUCHAR)io128KBuffer->First()->GetBuffer();
    p2 = (PUCHAR)ioRead128KBuffer->First()->GetBuffer();
    for (ULONG i = 0; i < oneTwentyEightK; i++)
    {
        VERIFY_IS_TRUE(p1[i] == p2[i], "equal");
    }
    
    KTestPrintf("Test %d, FileSize %I64x, EOF %I64x\n", ++ii, blockFile->QueryFileSize(), eof);
    
    //
    // Test 4: Set EOF below file size and verify file size and EOF
    //
    status = blockFile->SetEndOfFile(eofSmall, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetEOF");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == smallFileSize, "Verify File Size");
    
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == eofSmall, "EOF wrong");
    
    KTestPrintf("Test %d, FileSize %I64x, EOF %I64x\n", ++ii, blockFile->QueryFileSize(), eof);
    
    //
    // Test 5: Set file size larger and reset EOF and verify filesize
    //         and EOF
    //
    status = blockFile->SetFileSize(TestSize, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetFileSize");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify File Size");
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == (LONGLONG)TestSize, "EOF wrong"); 
    
    status = blockFile->SetEndOfFile(eofSmall, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "SetEOF");

    VERIFY_IS_TRUE(blockFile->QueryFileSize() == TestSize, "Verify File Size");
    
    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == eofSmall, "EOF wrong");

    KTestPrintf("Test %d, FileSize %I64x, EOF %I64x\n", ++ii, blockFile->QueryFileSize(), eof);
    
    //
    // Tets 6: Reopen file and verify file size, EOF and not sparse
    //
    blockFile = nullptr;
    KNt::Sleep(500);

    status = KBlockFile::Create(fileName,
                                            TRUE,         // IsWriteThrough
                                            KBlockFile::eOpenExisting,
                                            blockFile,
                                            sync,
                                            nullptr,
                                            KtlSystem::GlobalNonPagedAllocator(),
                                            KTL_TAG_TEST);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "CreateFile");

    VERIFY_IS_TRUE(blockFile->IsSparseFile() == FALSE, "Verify Not Sparse File");
    VERIFY_IS_TRUE(blockFile->QueryFileSize() == fileSizeSmall, "Verify File Size");

    status = blockFile->GetEndOfFile(eof, sync, nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "GetEOF");
    VERIFY_IS_TRUE(eof == eofSmall, "EOF wrong");

    KTestPrintf("Test %d, FileSize %I64x, EOF %I64x\n", ++ii, blockFile->QueryFileSize(), eof);

    //
    // Test 6: Perform an IO with maximum number of KIoBufferElements
    //
    status = KIoBuffer::CreateEmpty(manyElementsKIoBuffer, KtlSystem::GlobalNonPagedAllocator());
    VERIFY_IS_TRUE(NT_SUCCESS(status), "manyElementsKIoBuffer");
    
    for (ULONG i = 0; i < maxElements; i++)
    {
        KIoBufferElement::SPtr ioElement;
        PVOID px;
        
        status = KIoBufferElement::CreateNew(0x1000, ioElement, px, KtlSystem::GlobalNonPagedAllocator());
        VERIFY_IS_TRUE(NT_SUCCESS(status), "manyElementsKIoBuffer");

        manyElementsKIoBuffer->AddIoBufferElement(*ioElement);
    }

    status = blockFile->Transfer(KBlockFile::eForeground,
                                 KBlockFile::eWrite,
                                 0,
                                 *manyElementsKIoBuffer,
                                 sync,
                                 nullptr);
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");
    status = sync.WaitForCompletion();
    VERIFY_IS_TRUE(NT_SUCCESS(status), "Transfer");

    manyElementsKIoBuffer = nullptr;
    
    //
    // Cleanup non sparse file
    //
    blockFile = nullptr;
    KNt::Sleep(500);
    status = DeleteTestFile(dirName, fileName);
    
    return(STATUS_SUCCESS);
}
//

NTSTATUS
RunTheTests(
    BOOLEAN SparseFile,
    KWString& dirName,
    ULONG QueueDepth
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
    status = CreateTestFile(SparseFile, FALSE, FALSE, g_TestSize, QueueDepth, dirName, fileName, file);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("could not create test file %x\n", status);
        goto Finish;
    }

    if (SparseFile)
    {
        status = file->SetSparseFile(TRUE);
        VERIFY_IS_TRUE(NT_SUCCESS(status), "SetSparseFile");        
    }
    
    //
    // Verify SystemIoPriorityHint is not specified
    //
    KBlockFile::SystemIoPriorityHint hint;
    hint = file->GetSystemIoPriorityHint();
    VERIFY_IS_TRUE( (hint == KBlockFile::eNotDefined), "SystemIoPriorityHint should be eNotDefined");

    LONGLONG endOfFile;

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

#if 0  // TODO: Check with Tyler about correctness of this. Linux has different semantics than windows
    LONGLONG newEndOfFile;
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
#endif

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
    // Now do this with low IO priority
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

    status = KnarlyTest(*file, g_TestSize);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("knarly testing failed with %x\n", status);
        goto Finish;
    }   
#endif
    

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

            _delete(sync1[i]);
            
            if(! NT_SUCCESS(status))
            {
                KTestPrintf("Transfer failed. Status: %x\n", status);
            }
        }
    }

    //
    // Clean up.
    //

    status = DeleteTestFile(dirName, fileName);

    //  Stabilize test due to some race condition (delete failing with STATUS_SHARING_VIOLATION)
    while ((count > 0) && (status == STATUS_SHARING_VIOLATION))
    {
        KTestPrintf("Try to delete the test file: %d\n", count);
        KNt::Sleep(500L);
        status = DeleteTestFile(dirName, fileName);
        count--;
    }

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Could not delete test file %x\n", status);
    }

Finish:

    return(status);
}

NTSTATUS
KBlockFileTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    KWString dirName(KtlSystem::GlobalNonPagedAllocator());
    ULONG t = rand() % 2;
    BOOLEAN useSparse = (t == 1);
    ULONG queueDepth = (rand() % 254) + 1;

    EventRegisterMicrosoft_Windows_KTL();


    KTestPrintf("Starting KBlockFileTest Nonsparse test\n");
    status = RunTheTests(FALSE, dirName, KBlockFile::DefaultQueueDepth);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    KTestPrintf("Starting KBlockFileTest Sparse test\n");
    status = RunTheTests(TRUE, dirName, KBlockFile::DefaultQueueDepth);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    srand((ULONG)KNt::GetTickCount64());
    
    KTestPrintf("Starting KBlockFileTest Nonsparse test queue depth\n");
    status = RunTheTests(useSparse, dirName, queueDepth);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

#if KTL_USER_MODE
    //
    // Misc tests
    //
    {
        status = WriteAfterCloseTest(dirName);
        if (! NT_SUCCESS(status)) {
            goto Finish;
        }
    }
#endif

    {
        status = MiscTest(g_TestSize, dirName);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("Misc tests failed. Status: %x\n", status);
        }

    }

    //
    // ReadAhead cache tests
    //
    {
        status = ReadAheadCacheTests(g_TestSize, dirName);
        if (!NT_SUCCESS(status))
        {
            KTestPrintf("Readahead cache tests failed. Status: %x\n", status);
        }

    }

Finish:
    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KBlockFileTest(
    int argc, WCHAR* args[]
    )
{
    KTestPrintf("KBlockFileTest: STARTED\n");

    NTSTATUS status;
    KtlSystem*  defaultSys;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    status = KtlSystem::Initialize(&defaultSys);
    defaultSys->SetStrictAllocationChecks(TRUE);    

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KBlockFileTestX(argc, args);

    KtlSystem::Shutdown();

    if (NT_SUCCESS(status)) {
        KTestPrintf("KBlockFileTests completed successfully.\n");
    } else {
        KTestPrintf("KBlockFileTests failed with status: %x\n", status);
    }

    KTestPrintf("KBlockFileTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
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
    return RtlNtStatusToDosError(KBlockFileTest(argc, args));
}
#endif
