/*++

Copyright (c) Microsoft Corporation

Module Name:

    KAllocationBitmapTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KAllocationBitmapTests.h.
    2. Add an entry to the gs_KuTestCases table in KAllocationBitmapTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KAllocationBitmapTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

NTSTATUS
KAllocationBitmapTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    const ULONG TestSize = 0xF942;
    NTSTATUS status = STATUS_SUCCESS;
    KIoBufferElement::SPtr ioBufferElement;
    VOID* buffer;
    KAllocationBitmap::SPtr allocationBitmap;
    BOOLEAN isAllocated;
    ULONG i;
    ULONG start;
    ULONG length;

    EventRegisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("Starting KAllocationBitmapTest test");
	
    //
    // Create an allocation bitmap from an io buffer element.
    //

    status = KIoBufferElement::CreateNew(0x1000, ioBufferElement, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("failed to create io buffer element %x\n", status);
        goto Finish;
    }

    status = KAllocationBitmap::CreateFromIoBufferElement(0x1000*8, *ioBufferElement, allocationBitmap,
            KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("failed to create allocation bitmap from io buffer element\n", status);
        goto Finish;
    }

    //
    // Check the basic elements of the bitmap.
    //

    if (allocationBitmap->QueryNumberOfBits() != 0x1000*8) {
        KTestPrintf("allocation bitmap from io buffer element has wrong bit count\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Do a verify.
    //

    if (!allocationBitmap->Verify()) {
        KTestPrintf("allocation bitmap from io buffer element is corrupt\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Create an allocation bitmap that has its own storage.
    //

    status = KAllocationBitmap::Create(TestSize, allocationBitmap, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("failed to create allocation bitmap %x\n", status);
        goto Finish;
    }

    //
    // Do a verify.
    //

    if (!allocationBitmap->Verify()) {
        KTestPrintf("allocation bitmap is corrupt\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check the bit count.
    //

    if (allocationBitmap->QueryNumberOfBits() != TestSize) {
        KTestPrintf("allocation bitmap has wrong bit count\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // All bits should start out as free.
    //

    if (allocationBitmap->QueryNumFreeBits() != TestSize) {
        KTestPrintf("allocation bitmap has free bit count\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (!allocationBitmap->CheckIfFree(0, TestSize)) {
        KTestPrintf("allocation bitmap thinks it is not completely free\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (allocationBitmap->CheckIfAllocated(101, 1)) {
        KTestPrintf("allocation bitmap thinks it has an allocated bit\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (allocationBitmap->QueryRunLength(5, isAllocated) != TestSize - 5 || isAllocated) {
        KTestPrintf("allocation bitmap getting query run length wrong\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (allocationBitmap->QueryPrecedingRunLength(10, isAllocated) != 10 || isAllocated) {
        KTestPrintf("allocation bitmap getting query preceding run length wrong\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Do a verify.
    //

    if (!allocationBitmap->Verify()) {
        KTestPrintf("allocation bitmap is corrupt\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Now run through a terrible allocate/free loop, all the while making sure that the allocation bitmap is not
    // corrupt.
    //

    for (i = 0; i < 100000; i++) {

        //
        // Pick a random range of the bitmap.
        //

        start = rand()%TestSize;
        length = rand()%TestSize;
        if (TestSize - start < length) {
            length = TestSize - start;
        }

        if (!length) {
            continue;
        }

        if (i%3 == 0) {

            //
            // Do a plain Alloc for the length.
            //

            isAllocated = allocationBitmap->Allocate(length, start);

            if (isAllocated && !allocationBitmap->CheckIfAllocated(start, length)) {
                KTestPrintf("allocation is not properly recorded\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

        } else if (i%3 == 1) {

            //
            // Mark the range as allocated.
            //

            allocationBitmap->SetAsAllocated(start, length);

            if (!allocationBitmap->CheckIfAllocated(start, length)) {
                KTestPrintf("allocation is not properly recorded\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }

        } else {

            //
            // Free that region.
            //

            allocationBitmap->SetAsFree(start, length);

            if (!allocationBitmap->CheckIfFree(start, length)) {
                KTestPrintf("allocation is not properly recorded\n");
                status = STATUS_INVALID_PARAMETER;
                goto Finish;
            }
        }

        if (!allocationBitmap->Verify()) {
            KTestPrintf("allocation bitmap is corrupt\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

#if defined(PLATFORM_UNIX)
	KtlTraceUnregister();
#endif	
    return status;
}

NTSTATUS
KAllocationBitmapTest(
    int argc, WCHAR* args[]
    )
{
    KTestPrintf("KAllocationBitmapTest: STARTED\n");
    NTSTATUS status;

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KAllocationBitmapTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KAllocationBitmapTest: COMPLETED\n");
    return status;
}

#if CONSOLE_TEST
int
main(int argc, char* args[])
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    return RtlNtStatusToDosError(KAllocationBitmapTest(0, NULL));
}
#endif
