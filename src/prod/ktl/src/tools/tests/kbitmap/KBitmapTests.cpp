/*++

Copyright (c) Microsoft Corporation

Module Name:

    KBitmapTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KBitmapTests.h.
    2. Add an entry to the gs_KuTestCases table in KBitmapTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KBitmapTests.h"
#include <CommandLineParser.h>
#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

NTSTATUS
KBitmapTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    const ULONG TestSize = 0xF945;
    ULONG bufferSize;
    KBuffer::SPtr buffer;
    ULONG i;
    BOOLEAN bit;
    KBitmap bitmap;
    ULONG comprehensiveTestLengths[] = { 1, 3, 8, 31, 64, 199 };

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KBitmapTest test\n");

    //
    // Create a KBuffer to use for the backing buffer.
    //

    bufferSize = KBitmap::QueryRequiredBufferSize(TestSize);

    status = KBuffer::Create(bufferSize, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("failed to create KBuffer %x\n", status);
        goto Finish;
    }

    //
    // Initialize the bitmap via the constructor.
    //

    {
        KBitmap anotherBitmap(buffer->GetBuffer(), buffer->QuerySize(), TestSize);

        status = anotherBitmap.Status();

        if (!NT_SUCCESS(status)) {
            KTestPrintf("failed anotherBitmap constructor %x\n", status);
            goto Finish;
        }

        //
        // Check that everything is in place.
        //

        if (anotherBitmap.GetBuffer() != buffer->GetBuffer()) {
            KTestPrintf("anotherBitmap buffer is not correct.\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        if (anotherBitmap.QueryBufferSize() != bufferSize) {
            KTestPrintf("anotherBitmap size is not correct.\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        if (anotherBitmap.QueryNumberOfBits() != TestSize) {
            KTestPrintf("anotherBitmap number of bits is not correct.\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    if (bitmap.GetBuffer()) {
        KTestPrintf("default bitmap buffer is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryBufferSize()) {
        KTestPrintf("default bitmap buffer size is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryNumberOfBits()) {
        KTestPrintf("default bitmap buffer size is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Now invoke the reuse method and check.
    //

    status = bitmap.Reuse(buffer->GetBuffer(), buffer->QuerySize(), TestSize);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("reuse failed %x\n", status);
        goto Finish;
    }

    if (bitmap.GetBuffer() != buffer->GetBuffer()) {
        KTestPrintf("bitmap buffer is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryBufferSize() != bufferSize) {
        KTestPrintf("bitmap size is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryNumberOfBits() != TestSize) {
        KTestPrintf("bitmap number of bits is not correct.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check that 'ClearBits', 'SetAllBits', 'QueryRunLength' do what is expected.
    //

    bitmap.SetAllBits();
    bitmap.ClearBits(5, 10);

    if (bitmap.QueryRunLength(0, bit) != 5 || !bit || bitmap.QueryRunLength(5, bit) != 10 || bit ||
            bitmap.QueryRunLength(15, bit) != TestSize - 15 || !bit) {

        KTestPrintf("clear bits doesn't work \n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Same test, but the opposite, for 'SetBits', 'ClearAllBits'.
    //

    bitmap.ClearAllBits();
    bitmap.SetBits(5, 10);

    if (bitmap.QueryRunLength(0, bit) != 5 || bit || bitmap.QueryRunLength(5, bit) != 10 || !bit ||
            bitmap.QueryRunLength(15, bit) != TestSize - 15 || bit) {

        KTestPrintf("set bits doesn't work \n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Verify 'checkbit'.
    //

    for (i = 0; i < 5; i++) {
        if (bitmap.CheckBit(i)) {
            KTestPrintf("check bit doesn't work %x\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    for (; i < 15; i++) {
        if (!bitmap.CheckBit(i)) {
            KTestPrintf("check bit doesn't work %x\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    for (; i < TestSize; i++) {
        if (bitmap.CheckBit(i)) {
            KTestPrintf("check bit doesn't work %x\n", i);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Check 'QueryNumberOfSetBits'.
    //

    if (bitmap.QueryNumberOfSetBits() != 10) {
        KTestPrintf("query number of set bits doesn't work %x\n", bitmap.QueryNumberOfSetBits());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check 'AreBitsClear'.
    //

    if (!bitmap.AreBitsClear(0, 5)) {
        KTestPrintf("are bits clear not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (!bitmap.AreBitsClear(0, 4)) {
        KTestPrintf("are bits clear not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.AreBitsClear(0, 6)) {
        KTestPrintf("are bits clear not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (!bitmap.AreBitsClear(15, TestSize - 15)) {
        KTestPrintf("are bits clear not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check 'AreBitsSet'.
    //

    if (bitmap.AreBitsSet(0, 5)) {
        KTestPrintf("are bits set not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.AreBitsSet(0, 4)) {
        KTestPrintf("are bits set not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.AreBitsSet(0, 6)) {
        KTestPrintf("are bits set not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (!bitmap.AreBitsSet(5, 10)) {
        KTestPrintf("are bits set not working\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check 'query-preceding-run-length'.
    //

    if (bitmap.QueryPrecedingRunLength(TestSize, bit, 15) != 15) {
        KTestPrintf("query preceding run length doesn't work with max\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryPrecedingRunLength(TestSize, bit) != TestSize - 15 || bit) {
        KTestPrintf("query preceding run length doesn't work\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryPrecedingRunLength(15, bit) != 10 || !bit) {
        KTestPrintf("query preceding run length doesn't work\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (bitmap.QueryPrecedingRunLength(5, bit) != 5 || bit) {
        KTestPrintf("query preceding run length doesn't work\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Comprehensive test.
    //

    for (ULONG length : comprehensiveTestLengths) {
        ULONG lastIndex = TestSize - length;
        for (i = 0; i <= lastIndex; i++) {

            //
            // Test set bits.
            //

            bitmap.ClearAllBits();
            bitmap.SetBits(i, length);

            if (((i != 0) && (bitmap.QueryRunLength(0, bit) != i || bit)) ||
                bitmap.QueryRunLength(i, bit) != length || !bit ||
                (i != lastIndex) && (bitmap.QueryRunLength(i + length, bit) != lastIndex - i || bit)) {

                KTestPrintf("!!!SetBits doesn't work (1)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (((i == 0) == bitmap.AreBitsClear(0, i)) ||
                ((i == lastIndex) == bitmap.AreBitsClear(i + length, lastIndex - i))) {

                KTestPrintf("!!!AreBitsClear doesn't work (2)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (bitmap.AreBitsSet(i, length) == FALSE || 
                bitmap.AreBitsSet(i - 1, length) ||
                bitmap.AreBitsSet(i, length + 1)) {

                KTestPrintf("!!!AreBitsSet doesn't work (3)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (bitmap.QueryNumberOfSetBits() != length) {
                KTestPrintf("!!!QueryNumberOfSetBits doesn't work (6)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            //
            // Test clear bits.
            //

            bitmap.SetAllBits();
            bitmap.ClearBits(i, length);

            if (((i != 0) && (bitmap.QueryRunLength(0, bit) != i || !bit)) ||
                bitmap.QueryRunLength(i, bit) != length || bit ||
                (i != lastIndex) && (bitmap.QueryRunLength(i + length, bit) != lastIndex - i || !bit)) {

                KTestPrintf("!!!ClearBits doesn't work (4)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (((i == 0) == bitmap.AreBitsSet(0, i)) ||
                ((i == lastIndex) == bitmap.AreBitsSet(i + length, lastIndex - i))) {

                KTestPrintf("!!!AreBitsSet doesn't work (5)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (!bitmap.AreBitsClear(i, length)) {
                KTestPrintf("!!!AreBitsClear doesn't work (6)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

            if (bitmap.QueryNumberOfSetBits() != TestSize - length) {
                KTestPrintf("!!!QueryNumberOfSetBits doesn't work (6)!!!\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }
        }
    }

    //
    // Test edge cases where input equals zero.
    //

    {
        //
        // Clear the buffer.
        //

        bitmap.ClearAllBits();

        for (i = 0; i != TestSize; i++) {

            //
            // AreBitsClear should always return false at NumberOfBits = 0.
            //

            if (bitmap.AreBitsClear(i, 0)) {
                KTestPrintf("AreBitsClear doesn't work at NumberOfBits=0\n");
                status = STATUS_INVALID_PARAMETER;
            }

            //
            // SetBits should not affect the buffer.
            //

            bitmap.SetBits(i, 0);
        }

        //
        // Buffer should still be clear.
        //

        if (bitmap.AreBitsClear(0, TestSize) == FALSE) {
            KTestPrintf("SetBits doesn't work at NumberOfBits=0\n");
            status = STATUS_INVALID_PARAMETER;
        }

        //
        // Set the buffer.
        //

        bitmap.SetAllBits();

        for (i = 0; i != TestSize; i++) {

            //
            // AreBitsSet should always return false at NumberOfBits = 0.
            //

            if (bitmap.AreBitsSet(i, 0)) {
                KTestPrintf("AreBitsSet doesn't work at NumberOfBits=0\n");
                status = STATUS_INVALID_PARAMETER;
            }

            //
            // ClearBits should not affect the buffer.
            //

            bitmap.ClearBits(i, 0);
        }

        //
        // Buffer should still be set.
        //

        if (bitmap.AreBitsSet(0, TestSize) == FALSE) {
            KTestPrintf("SetBits doesn't work at NumberOfBits=0\n");
            status = STATUS_INVALID_PARAMETER;
        }
    }

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KBitmapTest(
    int argc, WCHAR* args[]
    )
{
    KTestPrintf("KBitmapTest: START\n");
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

    status = KBitmapTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KBitmapTest: COMPLETE\n");

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

    return RtlNtStatusToDosError(KBitmapTest(0, NULL));
}
#endif
