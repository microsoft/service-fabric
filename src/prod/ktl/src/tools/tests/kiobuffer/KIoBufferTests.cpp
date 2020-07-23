/*++

Copyright (c) Microsoft Corporation

Module Name:

    KIoBufferTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KIoBufferTests.h.
    2. Add an entry to the gs_KuTestCases table in KIoBufferTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KIoBufferTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


NTSTATUS
KIoBufferTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    const ULONG TestSize = 64*1024;
    NTSTATUS status = STATUS_SUCCESS;
    KIoBufferElement::SPtr ioBufferElement;
    VOID* buffer;
    UCHAR* p;
    ULONG i;
    KIoBufferElement::SPtr ioBufferElementReference;
    UCHAR* q;
    KIoBuffer::SPtr ioBuffer;

    EventRegisterMicrosoft_Windows_KTL();

    //
    // Try a strange size and make sure it works and that the result is correct.
    //

    status = KIoBufferElement::CreateNew(TestSize - 1, ioBufferElement, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create IO buffer element failed with %x\n", status);
        goto Finish;
    }

    //
    // Check that the strange size is preserved.
    //

    if (ioBufferElement->QuerySize() != TestSize - 1) {
        KTestPrintf("io-buffer-element returns incorrect size %d\n", ioBufferElement->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Allocate an IO buffer element.
    //

    status = KIoBufferElement::CreateNew(TestSize, ioBufferElement, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create IO buffer element failed with %x\n", status);
        goto Finish;
    }

    //
    // Fill the buffer with a pattern.  This ensures that the allocated memory is usable.
    //

    p = (UCHAR*) buffer;
    for (i = 0; i < TestSize; i++) {
        p[i] = 0xFA;
    }

    //
    // Make sure that 'GetBuffer' returns the same buffer that was returned by 'create-new'.
    //

    if (ioBufferElement->GetBuffer() != buffer) {
        KTestPrintf("io-buffer-element returns incorrect buffer pointer\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Make sure that the size returned for the io buffer element is the one passed in.
    //

    if (ioBufferElement->QuerySize() != TestSize) {
        KTestPrintf("io-buffer-element returns incorrect size %d\n", ioBufferElement->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Now create a reference to a part of the buffer.  Set a different pattern on this part.
    //

    for (i = 0x2000; i < 0x3000; i++) {
        p[i] = 0xFB;
    }

    status = KIoBufferElement::CreateReference(*ioBufferElement, 0x2000, 0x1000, ioBufferElementReference,
            KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create IO buffer element reference failed with %x\n", status);
        goto Finish;
    }

    //
    // Check the reference size, pointer, and pattern.
    //

    q = (UCHAR*) ioBufferElementReference->GetBuffer();

    if (q - p != 0x2000) {
        KTestPrintf("io-buffer-element-reference returns incorrect pointer\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (ioBufferElementReference->QuerySize() != 0x1000) {
        KTestPrintf("io-buffer-element-reference returns incorrect size %d\n", ioBufferElementReference->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    for (i = 0; i < 0x1000; i++) {
        if (q[i] != 0xFB) {
            KTestPrintf("bad value in io-buffer-element-reference %d\n", q[i]);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Create an empty IO buffer and then add the 2 iobuffer elements that we already have.
    //

    status = KIoBuffer::CreateEmpty(ioBuffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create empty IO buffer failed with %x\n", status);
        goto Finish;
    }

    ioBuffer->AddIoBufferElement(*ioBufferElement);
    ioBuffer->AddIoBufferElement(*ioBufferElementReference);

    //
    // Check that it has 2 elements.
    //

    if (ioBuffer->QueryNumberOfIoBufferElements() != 2) {
        KTestPrintf("Number of io buffer element is not 2 but %d\n", ioBuffer->QueryNumberOfIoBufferElements());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check the total size.
    //

    if (ioBuffer->QuerySize() != TestSize + 0x1000) {
        KTestPrintf("Total size isn't TestSize + 0x1000 but %x\n", ioBuffer->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }


    //
    // Iterate through the io buffer references and make sure that the list is correct.
    //

    {
        KIoBufferElement* elem;
        ULONG n;

        n = 0;
        for (elem = ioBuffer->First(); elem; elem = ioBuffer->Next(*elem)) {
            n++;
            if (n == 1) {
                if (elem != ioBufferElement.RawPtr()) {
                    KTestPrintf("incorrect first io buffer element\n");
                    status = STATUS_INVALID_PARAMETER;
                    goto Finish;
                }
            } else {
                if (elem != ioBufferElementReference.RawPtr()) {
                    KTestPrintf("incorrect second io buffer element\n");
                    status = STATUS_INVALID_PARAMETER;
                    goto Finish;
                }
            }
        }

        if (n != 2) {
            KTestPrintf("incorrect number of iterations %d\n", n);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Create a simple IO buffer with exactly one element.
    //

    status = KIoBuffer::CreateSimple(TestSize, ioBuffer, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Create simple IO buffer failed with %x\n", status);
        goto Finish;
    }

    //
    // Fill the buffer with a pattern.  This ensures that the allocated memory is usable.
    //

    p = (UCHAR*) buffer;
    for (i = 0; i < TestSize; i++) {
        p[i] = 0xFA;
    }

    //
    // Check that the io buffer has exactly one buffer.
    //

    if (ioBuffer->QueryNumberOfIoBufferElements() != 1) {
        KTestPrintf("incorrect number of io buffer elements on simple io buffer, should be 1, is %d\n",
                ioBuffer->QueryNumberOfIoBufferElements());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check that the size is correct.
    //

    if (ioBuffer->QuerySize() != TestSize) {
        KTestPrintf("incorrect test size not %x, but %x\n", TestSize, ioBuffer->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Make sure that the io-buffer-element pointer matches the one returned in create-simple.
    //

    KIoBufferElement* elem;

    {
        elem = ioBuffer->First();

        if (!elem) {
            KTestPrintf("iterator for simple io buffer reports no elements\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        if (elem->GetBuffer() != buffer) {
            KTestPrintf("element returns wrong buffer\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        if (elem->QuerySize() != TestSize) {
            KTestPrintf("element in simple io buffer is wrong size: %x\n", elem->QuerySize());
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        if (ioBuffer->Next(*elem)) {
            KTestPrintf("iterator for simple io buffer reports too many elements\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }
    }

    //
    // Add an io buffer reference to the simple io buffer.
    //

    status = ioBuffer->AddIoBufferElementReference(*elem, 0x2000, 0x1000);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("add io buffer element reference failed with %x\n", status);
        goto Finish;
    }

    //
    // Check the total size and the number of elements.
    //

    if (ioBuffer->QueryNumberOfIoBufferElements() != 2) {
        KTestPrintf("Number of io buffer elements is not 2 but %d\n", ioBuffer->QueryNumberOfIoBufferElements());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Check the total size.
    //

    if (ioBuffer->QuerySize() != TestSize + 0x1000) {
        KTestPrintf("Total size isn't correct but %x\n", ioBuffer->QuerySize());
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }


    //
    // Test GetContiguousBufferPointer
    //
    {
#define Value(index, c) ( ((index) % (c)) * (c) )
        KIoBuffer::SPtr iob;
        PVOID p1;
        PUCHAR puA, puB, puC;
        ULONG sizeA = 0x4000;
        ULONG sizeB = 0x2000;
        ULONG sizeC = 0x8000;
        KIoBufferElement::SPtr ioba;
        KIoBufferElement::SPtr iobb;
        KIoBufferElement::SPtr iobc;

        status = KIoBuffer::CreateEmpty(iob, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("CreateEmpty failed %x\n", status);
            goto Finish;
        }

        status = KIoBufferElement::CreateNew(sizeA, ioba, p1, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("CreateNew a failed %x\n", status);
            goto Finish;
        }
        puA = (PUCHAR)p1;
        for (ULONG j = 0; j < sizeA; j++)
        {
            puA[j] = Value(j, 'A');
        }

        status = KIoBufferElement::CreateNew(sizeB, iobb, p1, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("CreateNew b failed %x\n", status);
            goto Finish;
        }
        puB = (PUCHAR)p1;
        for (ULONG j = 0; j < sizeB; j++)
        {
            puB[j] = Value(j, 'B');
        }

        status = KIoBufferElement::CreateNew(sizeC, iobc, p1, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
        if (! NT_SUCCESS(status))
        {
            KTestPrintf("CreateNew c failed %x\n", status);
            goto Finish;
        }
        puC = (PUCHAR)p1;
        for (ULONG j = 0; j < sizeC; j++)
        {
            puC[j] = Value(j, 'C');
        }

        //
        // Test 0: pointer in empty
        //
        KIoBufferElement::SPtr elementHint;
        ULONG positionHint;

        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 50, 0x1000);
        if (p1 != NULL)
        {
            KTestPrintf("GetContiguousPointer bad %x\n", 50);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        iob->AddIoBufferElement(*ioba);
        iob->AddIoBufferElement(*iobb);
        iob->AddIoBufferElement(*iobc);

        //
        // Test 1: pointer in first element
        //
        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 50, 0x1000);
        if (p1 != (puA + 50))
        {
            KTestPrintf("GetContiguousPointer bad %x\n", 50);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Test 2: pointer in 3rd element
        //
        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 0x7124, 50);
        if (p1 != (puC + 0x1124))
        {
            KTestPrintf("GetContiguousPointer bad %x\n", 0x7124);
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Test 3: pointer crossing elements
        //
        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 0x6000 - 200, 500);
        if (p1 != NULL)
        {
            KTestPrintf("GetContiguousPointer bad cross 1\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Test 4: pointer beyond the end
        //
        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 0xE000 - 200, 500);
        if (p1 != (puC + 0x7f38))
        {
            KTestPrintf("GetContiguousPointer beyond end\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

        //
        // Test 5: pointer well beyond the end
        //
        elementHint = nullptr;
        positionHint = 0;
        p1 = iob->GetContiguousPointer(elementHint, positionHint, 0xE000 + 200, 500);
        if (p1 != NULL)
        {
            KTestPrintf("GetContiguousPointer well beyond end\n");
            status = STATUS_INVALID_PARAMETER;
            goto Finish;
        }

    }


Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
ViewTest(
    )
{
    NTSTATUS status;
    KIoBuffer::SPtr ioBuffer;
    ULONG offset;
    ULONG i;
    ULONG len;
    ULONG j;
    KIoBufferElement::SPtr ioBufferElement;
    VOID* buffer;
    KIoBufferElement::SPtr ioBufferElementReference;
    UCHAR patternByte;
    UCHAR* p;
    KIoBufferView::SPtr ioBufferView;

    //
    // Start the test by creating a random io buffer with a string of 10 io buffer elements of random lengths.
    //

    status = KIoBuffer::CreateEmpty(ioBuffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("create io buffer failed with %x\n", status);
        return status;
    }

    offset = 0;

    for (i = 0; i < 10; i++) {

        len = (rand()%10 + 1)*0x1000;

        status = KIoBufferElement::CreateNew(len, ioBufferElement, buffer, KtlSystem::GlobalNonPagedAllocator());

        if (!NT_SUCCESS(status)) {
            KTestPrintf("create io buffer element failed with %x\n", status);
            return status;
        }

        //
        // Fill with a known pattern relative to the start of the io buffer.
        //

        for (j = 0; j < len; j += 0x1000) {
            RtlFillMemory((UCHAR*) buffer + j, 0x1000, (UCHAR) ((ioBuffer->QuerySize() + j)/0x1000));
        }

        ioBuffer->AddIoBufferElement(*ioBufferElement);
    }

    //
    // Now create a random view of the IoBuffer.
    //

    status = KIoBufferView::Create(ioBufferView, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("create io buffer view failed with %x\n", status);
        return status;
    }

    len = ioBuffer->QuerySize();
    len /= 0x1000;
    len = rand()%len;
    patternByte = (UCHAR) len;
    len *= 0x1000;

    ioBufferView->SetView(*ioBuffer, len, ioBuffer->QuerySize() - len);

    if (ioBufferView->QuerySize() != ioBuffer->QuerySize() - len) {
        KTestPrintf("io buffer view has wrong size");
        return STATUS_INVALID_PARAMETER;
    }

    //
    // Check that the data on this random view is correct.
    //

    offset = 0;
    for (ioBufferElement = ioBufferView->First(); ioBufferElement; ioBufferElement = ioBufferView->Next(*ioBufferElement)) {

        patternByte = (UCHAR) ((offset + len)/0x1000);
        p = (UCHAR*) ioBufferElement->GetBuffer();

        for (i = 0; i < ioBufferElement->QuerySize(); i += 0x1000) {
            for (j = 0; j < 0x1000; j++) {
                if (p[i + j] != patternByte) {
                    KTestPrintf("io buffer element reference has wrong data");
                    return STATUS_INVALID_PARAMETER;
                }
            }
            patternByte++;
        }

        offset += ioBufferElement->QuerySize();
    }

    //
    // Clear the io buffer view.
    //

    ioBufferView->Clear();

    return STATUS_SUCCESS;
}

NTSTATUS
IoBufferStreamTests()
{
    KIoBufferElement::SPtr  bfr0;
    KIoBufferElement::SPtr  bfr1;
    UCHAR*                  ptr0;
    UCHAR*                  ptr1;
    NTSTATUS                status = STATUS_SUCCESS;
    const ULONG             sizeOfFirstBuffer = 0x2f000;
    const ULONG             sizeOfIoBuffer = 0x73e000;
    const ULONG             sizeOfSecondBuffer = sizeOfIoBuffer - sizeOfFirstBuffer;

    // Test KIoBufferStream fetching buffer ptr after skipping past the last location in an KIoBufferElement
    status = KIoBufferElement::CreateNew(sizeOfFirstBuffer, bfr0, (void*&)ptr0, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("IoBufferStreamTests: CreateNew failed: %i\n", __LINE__);
        return status;
    }

    status = KIoBufferElement::CreateNew(sizeOfSecondBuffer, bfr1, (void*&)ptr1, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("IoBufferStreamTests: CreateNew failed: %i\n", __LINE__);
        return status;
    }

    KIoBuffer::SPtr         ioBuffer;
    status = KIoBuffer::CreateEmpty(ioBuffer, KtlSystem::GlobalNonPagedAllocator(), KTL_TAG_TEST);
    if (!NT_SUCCESS(status))
    {
        KTestPrintf("IoBufferStreamTests: CreateEmpty failed: %i\n", __LINE__);
        return status;
    }

    // Prove element boundary cases
    {
        ioBuffer->AddIoBufferElement(*bfr0);
        ioBuffer->AddIoBufferElement(*bfr1);
        KIoBufferStream         stream(*ioBuffer, 0);
        UCHAR                   testSpace[10];
        auto                    ClearTestSpace = [&]()
        {
            for (UCHAR ix = 0; ix < sizeof(testSpace); ix++)
            {
                testSpace[ix] = ix;
            }
        };


        //** 1st byte of 2nd buffer
        KFatal(stream.Skip(sizeOfFirstBuffer));
        UCHAR*          edgePosition = (UCHAR*)stream.GetBufferPointer();

        // Prove memory access
        *edgePosition = 'X';
        KInvariant((*edgePosition) == 'X');
        KInvariant(stream.GetBufferPointerRange() == sizeOfSecondBuffer);

        // Prove Put
        KInvariant(stream.Put((PVOID)"Y", 1));
        KInvariant((*edgePosition) == 'Y');
        KInvariant(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));

        // Prove Pull
        KInvariant(stream.PositionTo(sizeOfFirstBuffer));
        KInvariant(stream.GetBufferPointerRange() == sizeOfSecondBuffer);
        ClearTestSpace();
        KInvariant(stream.Pull(testSpace, 1));
        KInvariant(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));
        KInvariant(testSpace[0] == 'Y');

        // Prove CopyTo
        KInvariant(KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer, 1, (PVOID)"A"));
        KInvariant((*edgePosition) == 'A');

        // Prove CopyFrom
        ClearTestSpace();
        KInvariant(KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer, 1, testSpace));
        KInvariant(testSpace[0] == 'A');

        //** Last byte of 1st buffer
        KInvariant(stream.PositionTo(sizeOfFirstBuffer - 1));
        edgePosition = (UCHAR*)stream.GetBufferPointer();
        *edgePosition = 'Z';
        KInvariant((*edgePosition) == 'Z');
        KInvariant(stream.GetBufferPointerRange() == 1);

        // Prove Put
        KInvariant(stream.Put((PVOID)"Q", 1));
        KInvariant((*edgePosition) == 'Q');
        KInvariant(stream.GetBufferPointerRange() == sizeOfSecondBuffer);

        // Prove Pull
        KInvariant(stream.PositionTo(sizeOfFirstBuffer - 1));
        KInvariant(stream.GetBufferPointerRange() == 1);
        ClearTestSpace();
        KInvariant(stream.Pull(testSpace, 1));
        KInvariant(stream.GetBufferPointerRange() == sizeOfSecondBuffer);
        KInvariant(testSpace[0] == 'Q');

        // Prove CopyTo - partial fast path
        KInvariant(KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer - 1, 1, (PVOID)"B"));
        KInvariant((*edgePosition) == 'B');

        // Prove CopyFrom - partial fast path
        ClearTestSpace();
        KInvariant(KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer - 1, 1, testSpace));
        KInvariant(testSpace[0] == 'B');

        //** Cross buffer boundary
        KInvariant(stream.PositionTo(sizeOfFirstBuffer - 1));
        KInvariant(stream.GetBufferPointerRange() == 1);

        // Prove Put
        KInvariant(stream.Put((PVOID)"CD", 2));
        KInvariant(*(ptr0 + sizeOfFirstBuffer - 1) == 'C');
        KInvariant(*ptr1 == 'D');
        KInvariant(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));

        // Prove Pull
        KInvariant(stream.PositionTo(sizeOfFirstBuffer - 1));
        KInvariant(stream.GetBufferPointerRange() == 1);
        ClearTestSpace();
        KInvariant(stream.Pull(testSpace, 2));
        KInvariant(stream.GetBufferPointerRange() == (sizeOfSecondBuffer - 1));
        KInvariant(testSpace[0] == 'C');
        KInvariant(testSpace[1] == 'D');

        // Prove CopyTo
        KInvariant(KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer - 1, 2, (PVOID)"EF"));
        KInvariant(*(ptr0 + sizeOfFirstBuffer - 1) == 'E');
        KInvariant(*ptr1 == 'F');

        // Prove CopyFrom
        ClearTestSpace();
        KInvariant(KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer - 1, 2, testSpace));
        KInvariant(testSpace[0] == 'E');
        KInvariant(testSpace[1] == 'F');

        //* Prove over-CopyTo and CopyFrom fails
        KInvariant(!KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer - 1, 2, (PVOID)"XX"));
        KInvariant(!KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer, 1, (PVOID)"XX"));
        KInvariant(!KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer - 1, 2, testSpace));
        KInvariant(!KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer + sizeOfSecondBuffer, 1, testSpace));

        //* Prove single buffer optimatzation paths for CopyTo/CopyFrom
        ioBuffer->Clear();
        ioBuffer->AddIoBufferElement(*bfr0);

        // Prove CopyTo - full fast path
        KInvariant(KIoBufferStream::CopyTo(*ioBuffer, sizeOfFirstBuffer - 2, 2, (PVOID)"ST"));
        KInvariant(*(ptr0 + sizeOfFirstBuffer - 2) == 'S');
        KInvariant(*(ptr0 + sizeOfFirstBuffer - 1) == 'T');

        // Prove CopyFrom - full fast path
        ClearTestSpace();
        KInvariant(KIoBufferStream::CopyFrom(*ioBuffer, sizeOfFirstBuffer - 2, 2, testSpace));
        KInvariant(testSpace[0] == 'S');
        KInvariant(testSpace[1] == 'T');
    }

    // Prove basic size limit behavior
    {
        ioBuffer->Clear();
        ioBuffer->AddIoBufferElement(*bfr0);

        KIoBufferStream         stream(*ioBuffer, 0, ioBuffer->QuerySize() - 10);
        KInvariant(stream.GetBufferPointerRange() == (bfr0->QuerySize() - 9));
        KInvariant(stream.PositionTo(ioBuffer->QuerySize() - 10));

        char		x = 1;
        KInvariant(stream.Put(x));				// prove putting at limit works
        KInvariant(!stream.Put(x));				// Prove putting past limit fails
        KInvariant(stream.PositionTo(ioBuffer->QuerySize() - 10));

        char		x1 = 0;
        KInvariant(stream.Pull(x1));				// prove getting at limit works
        KInvariant(x == x1);
        KInvariant(!stream.Pull(x1));				// Prove putting past limit fails

        ioBuffer->AddIoBufferElement(*bfr1);
        stream.Reuse(*ioBuffer, 0, bfr0->QuerySize() - 1);
        KInvariant(stream.GetBufferPointerRange() == bfr0->QuerySize());
    }
    return STATUS_SUCCESS;
}




NTSTATUS
KIoBufferTest(
    int argc, WCHAR* args[]
    )
{

    NTSTATUS status;

#if defined(PLATFORM_UNIX)
    status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
	
    KTestPrintf("KIoBufferTest: STARTED\n");
	
    status = KtlSystem::Initialize();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KIoBufferTestX(argc, args);
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    status = ViewTest();
    if (!NT_SUCCESS(status)) {
        goto Finish;
    }

    status = IoBufferStreamTests();

Finish:
    KtlSystem::Shutdown();

    KTestPrintf("KIoBufferTest: COMPLETED\n");

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

    return RtlNtStatusToDosError(KIoBufferTest(0, NULL));
}
#endif
