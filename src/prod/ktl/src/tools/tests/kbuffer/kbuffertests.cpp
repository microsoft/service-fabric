/*++

Copyright (c) Microsoft Corporation

Module Name:

    KBufferTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KBufferTests.h.
    2. Add an entry to the gs_KuTestCases table in KBufferTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KBufferTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


NTSTATUS
KBufferTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    const ULONG testSize = 100;
    NTSTATUS status = STATUS_SUCCESS;
    KBuffer::SPtr buffer1;
    KBuffer::SPtr buffer2;
    UCHAR* buf1;
    UCHAR* buf2;
    ULONG i;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KBufferTest test");

    // Prove Creation and dtor from a KUniquePtr works
    LONGLONG                allocsBeforeBuffer = KAllocatorSupport::gs_AllocsRemaining;
    UCHAR*                  rawBlockPtr = _new(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator()) UCHAR[151];
    KUniquePtr<UCHAR>       rawBlock(rawBlockPtr);
    KFatal(rawBlock);
    KFatal(KAllocatorSupport::gs_AllocsRemaining == (allocsBeforeBuffer + 1));

    
    KBuffer::SPtr           rawBlockBuffer;
    status = KBuffer::Create(rawBlock, rawBlockBuffer, KtlSystem::GlobalNonPagedAllocator());
    KFatal(NT_SUCCESS(status));
    KFatal(rawBlockBuffer->QuerySize() == 151);
    KFatal(rawBlockBuffer->GetBuffer() == rawBlockPtr);
    KFatal(!rawBlock);

    KFatal(allocsBeforeBuffer != KAllocatorSupport::gs_AllocsRemaining);
    rawBlockBuffer = nullptr;
    KFatal(allocsBeforeBuffer == KAllocatorSupport::gs_AllocsRemaining);

    //
    // Create 2 buffers of 'testSize' bytes.
    //

    status = KBuffer::Create(testSize, buffer1, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create buffer1 failed with %x\n", status);
        goto Finish;
    }

    buf1 = (UCHAR*) buffer1->GetBuffer();

    if (!buf1) {
        KTestPrintf("buffer1 has no valid buffer pointer\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    status = KBuffer::Create(testSize, buffer2, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create buffer2 failed with %x\n", status);
        goto Finish;
    }

    buf2 = (UCHAR*) buffer2->GetBuffer();

    if (!buf2) {
        KTestPrintf("buffer2 has no valid buffer pointer\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Make sure that we can look at all 'testSize' bytes without faulting.
    //

    for (i = 0; i < testSize; i++) {
        buf2[i] = buf1[i];
    }

    //
    // Make sure that 'query-size' returns the correct value.
    //

    if (buffer1->QuerySize() != testSize || buffer2->QuerySize() != testSize) {
        KTestPrintf("buffer1 or buffer2 has an incorrect size\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Fill buf1 with 0xFF.
    //

    for (i = 0; i < testSize; i++) {
        buf1[i] = 0xFF;
    }

    //
    // Zero the buffer and check.
    //

    buffer1->Zero();

    for (i = 0; i < testSize; i++) {
        if (buf1[i]) {
            KTestPrintf("offset %d of buf1 is not zero\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Fill buf1 with 0xFF.
    //

    for (i = 0; i < testSize; i++) {
        buf1[i] = 0xFF;
    }

    //
    // Zero only part of buf1.
    //

    buffer1->Zero(testSize - 1, 1);

    //
    // Check that everything is 0xFF except the last byte.
    //

    for (i = 0; i < testSize - 1; i++) {
        if (buf1[i] != 0xFF) {
            KTestPrintf("offset %d of buf1 is not FF\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Check that the last byte is 0.
    //

    if (buf1[testSize - 1]) {
        KTestPrintf("last byte is not zero\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Zero buffer 2.
    //

    buffer2->Zero();

    //
    // Fill buf1 with 0xFF.
    //

    for (i = 0; i < testSize; i++) {
        buf1[i] = 0xFF;
    }

    //
    // Copy part of buffer 2 to buffer 1.
    //

    buffer1->CopyFrom(5, *buffer2, 0, 5);

    //
    // Check that the copy affected only exactly those 5 bytes.
    //

    for (i = 0; i < 5; i++) {
        if (buf1[i] != 0xFF) {
            KTestPrintf("offset %d of buf1 is not FF\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    for (; i < 10; i++) {
        if (buf1[i]) {
            KTestPrintf("offset %d of buf1 is not 0\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    for (; i < testSize; i++) {
        if (buf1[i] != 0xFF) {
            KTestPrintf("offset %d of buf1 is not FF\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Now try Move
    //

    buffer1->Move(0, 5, 5);

    for (i = 0; i < 10; i++) {
        if (buf1[i]) {
            KTestPrintf("offset %d of buf1 is not 0\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    for (; i < testSize; i++) {
        if (buf1[i] != 0xFF) {
            KTestPrintf("offset %d of buf1 is not FF\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KBufferTest(
    int argc, WCHAR* args[]
    )
{
    KTestPrintf("KBufferTest: STARTED\n");
    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KBufferTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KBufferTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
	
    return status;
}

#if CONSOLE_TEST
int
main(int argc, char* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    return RtlNtStatusToDosError(KBufferTest(0, NULL));
}
#endif
