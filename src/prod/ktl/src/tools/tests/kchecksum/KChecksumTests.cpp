/*++

Copyright (c) Microsoft Corporation

Module Name:

    KChecksumTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KChecksumTests.h.
    2. Add an entry to the gs_KuTestCases table in KChecksumTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ktl.h>
#include <ktrace.h>
#include "KChecksumTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


NTSTATUS
SlowCrc32(
    __in_bcount(Size) VOID* Buffer,
    __in ULONG Size,
    __out ULONG& Crc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONG* crcTable = NULL;
    ULONG polynomial;
    ULONG i;
    ULONG crc;
    ULONG j;
    UCHAR* p;

    crcTable = _newArray<ULONG>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), 256);
    if (!crcTable) {
        KTestPrintf("could not allocate crc table\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    //
    // CRC32C (Castagnoli)
    //

    polynomial = 0x82F63B78;
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            crc = (crc&1) ? (polynomial^(crc>>1)) : (crc>>1);
        }
        crcTable[i] = crc;
    }

    crc = 0xFFFFFFFF;
    p = (UCHAR*) Buffer;
    for (i = 0; i < Size; i++) {
        crc = crcTable[(crc^(*p++))&0xff]^(crc>>8);
    }
    crc = (crc^0xFFFFFFFF);

    Crc = crc;

Finish:

    _deleteArray(crcTable);

    return status;
}

NTSTATUS
SlowCrc64(
    __in_bcount(Size) VOID* Buffer,
    __in ULONG Size,
    __out ULONGLONG& Crc
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    ULONGLONG* crcTable = NULL;
    ULONGLONG polynomial;
    ULONG i;
    ULONGLONG crc;
    ULONG j;
    UCHAR* p;

    crcTable = _newArray<ULONGLONG>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), 256);
    if (!crcTable) {
        KTestPrintf("could not allocate crc table\n");
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto Finish;
    }

    //
    // CRC64
    //

    polynomial = 0x9a6c9329ac4bc9b5;
    for (i = 0; i < 256; i++) {
        crc = i;
        for (j = 0; j < 8; j++) {
            crc = (crc&1) ? (polynomial^(crc>>1)) : (crc>>1);
        }
        crcTable[i] = crc;
    }

    crc = 0xFFFFFFFFFFFFFFFF;
    p = (UCHAR*) Buffer;
    for (i = 0; i < Size; i++) {
        crc = crcTable[(crc^(*p++))&0xff]^(crc>>8);
    }
    crc = (crc^0xFFFFFFFFFFFFFFFF);

    Crc = crc;

Finish:

    _deleteArray(crcTable);

    return status;
}

NTSTATUS
KChecksumTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    const ULONG TestSize = 0x1000;
    NTSTATUS status = STATUS_SUCCESS;
    KBuffer::SPtr buffer;
    UCHAR* buf;
    ULONG i;
    ULONG splitPoint;
    ULONG crc32first;
    ULONG crc32;
    ULONGLONG crc64first;
    ULONGLONG crc64;
    ULONGLONG hash64first;
    ULONGLONG hash64;

    EventRegisterMicrosoft_Windows_KTL();

    KTestPrintf("Starting KChecksumTest test");

    //
    // Use KBuffer to create a buffer to checksum.
    //

    status = KBuffer::Create(TestSize, buffer, KtlSystem::GlobalNonPagedAllocator());

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Could not create a buffer %x\n", status);
        goto Finish;
    }

    buf = (UCHAR*) buffer->GetBuffer();

    //
    // Set the buffer to a known pattern.
    //

    for (i = 0; i < TestSize; i++) {
        buf[i] = (UCHAR) (i + 9);
    }

    //
    // Make sure that doing the checksum in 2 parts equals doing the checksum in one part.
    //

    splitPoint = TestSize/2;

    crc32first = KChecksum::Crc32(buf, splitPoint);
    crc32first = KChecksum::Crc32(buf + splitPoint, TestSize - splitPoint, crc32first);
    crc32 = KChecksum::Crc32(buf, TestSize);

    if (crc32 != crc32first) {
        KTestPrintf("Cummulative checksum property of CRC32 violated\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    crc64first = KChecksum::Crc64(buf, splitPoint);
    crc64first = KChecksum::Crc64(buf + splitPoint, TestSize - splitPoint, crc64first);
    crc64 = KChecksum::Crc64(buf, TestSize);

    if (crc64 != crc64first) {
        KTestPrintf("Cummulative checksum property of CRC64 violated\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    hash64first = KChecksum::Hash64(buf, splitPoint);
    hash64first = KChecksum::Hash64(buf + splitPoint, TestSize - splitPoint, hash64first);
    hash64 = KChecksum::Hash64(buf, TestSize);

    if (hash64 != hash64first) {
        KTestPrintf("Cummulative checksum property of HASH64 violated\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Compare the CRC32 implementation with the simpler non optimized version.
    //

    status = SlowCrc32(buf, TestSize, crc32);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Slow crc 32 failed with %x\n", status);
        goto Finish;
    }

    //
    // Compare the CRC64 implementation with the simpler non optimized version.
    //

    status = SlowCrc64(buf, TestSize, crc64);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Slow crc 64 failed with %x\n", status);
        goto Finish;
    }

    if (crc32 != crc32first) {
        KTestPrintf("CRC32 algorithms don't get the same result\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    if (crc64 != crc64first) {
        KTestPrintf("CRC64 algorithms don't get the same result\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

Finish:

    EventUnregisterMicrosoft_Windows_KTL();

    return status;
}

NTSTATUS
KChecksumTest(
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
    
    KTestPrintf("KChecksumTest: STARTED\n");

    status = KtlSystem::Initialize();

    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = KChecksumTestX(argc, args);

    KtlSystem::Shutdown();

    KTestPrintf("KChecksumTest: COMPLETED\n");

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

    return RtlNtStatusToDosError(KChecksumTest(0, NULL));
}
#endif
