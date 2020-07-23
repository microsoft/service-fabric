/*++

Copyright (c) Microsoft Corporation

Module Name:

    KMemChannelTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KMemChannelTests.h.
    2. Add an entry to the gs_KuTestCases table in KMemChannelTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#if defined(PLATFORM_UNIX)
#include <vector>
#endif
#include <ktl.h>
#include <ktrace.h>
#include "KMemChannelTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif


#if KTL_USER_MODE
extern volatile LONGLONG gs_AllocsRemaining;
#endif

#define RETURN_ON_FAIL(x)  if (!NT_SUCCESS(x)) { return x; }

// Bugs: Fix seek + subsequent rewrite in middle
//

ULONG BoundedRandNz(ULONG Max)
{
#if !defined(PLATFORM_UNIX)
    ULONG r;
    while (1)
    {
        r = rand();
        if (r == 0 || r > Max)
            continue;
        break;
    }

    return r;
#else
    return (rand() % Max) + 1;
#endif
}

ULONG BoundedRandNz(ULONG Min, ULONG Max)
{
    ULONG Try = 0;
    while (1)
    {
        Try = BoundedRandNz(Max);
        if (Try >= Min)
        {
            break;
        }
    }

    return Try;
}

NTSTATUS RandomReadWrite()
{
    NTSTATUS Result;

    unsigned randBlockSize = BoundedRandNz(9000);

    KMemChannel Ch(KtlSystem::GlobalNonPagedAllocator(), randBlockSize);

    unsigned ArrayLength = BoundedRandNz(512);
    ULONG* Values = _newArray<ULONG>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), ArrayLength);
    KFinally([&](){ _deleteArray(Values); });

    for (unsigned i = 0; i < ArrayLength; i++)
    {
        Values[i] = i;
        Result = Ch.Write(sizeof(ULONG), &Values[i]);
        RETURN_ON_FAIL(Result);
    }

    Result = Ch.SetCursor(0, KMemChannel::eFromBegin);
    RETURN_ON_FAIL(Result);

    for (unsigned i = 0; i < ArrayLength; i++)
    {
        ULONG Val = 0;
        ULONG TotalRead = 0;
        Result = Ch.Read(sizeof(ULONG), &Val, &TotalRead);
        RETURN_ON_FAIL(Result);

        if (TotalRead != sizeof(ULONG))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (Val != i)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS Test1()
{
//    srand(time(NULL));

    for (int i = 0; i < 1000; i++)
    {
        NTSTATUS Res = RandomReadWrite();
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("FAIL: Test1() random iteration %d\n", i);
            return Res;
        }
    }

    return STATUS_SUCCESS;
}

//
// This tests concentratest on testing seek

NTSTATUS Test2()
{
    KMemChannel Ch(KtlSystem::GlobalNonPagedAllocator(), 28);  // 7 ULONGs, that is odd enough for our test.
    NTSTATUS Result;

    unsigned ArrayLength = 512;
    ULONG* Values = _newArray<ULONG>(KTL_TAG_TEST, KtlSystem::GlobalNonPagedAllocator(), ArrayLength);
    KFinally([&](){ _deleteArray(Values); });

    for (unsigned i = 0; i < ArrayLength; i++)
    {
        Values[i] = i;
        Result = Ch.Write(sizeof(ULONG), &Values[i]);
        RETURN_ON_FAIL(Result);
    }

    // Now we will seek the truth
    //
    for (unsigned i = 0; i < ArrayLength; i++)
    {
        Result = Ch.SetCursor(i*sizeof(ULONG), KMemChannel::eFromBegin);
        RETURN_ON_FAIL(Result);

        ULONG Val = 0;
        ULONG TotalRead = 0;
        Result = Ch.Read(sizeof(ULONG), &Val, &TotalRead);
        RETURN_ON_FAIL(Result);

        if (TotalRead != sizeof(ULONG))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (Val != i)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    // Seek end
    Result = Ch.SetCursor(0, KMemChannel::eBeforeEnd);
    RETURN_ON_FAIL(Result);

    for (unsigned i = ArrayLength-1; ; i--)
    {
        Result = Ch.SetCursor(4, KMemChannel::eBeforeCurrent);
        RETURN_ON_FAIL(Result);

        ULONG Val = 0;
        ULONG TotalRead = 0;
        Result = Ch.Read(sizeof(ULONG), &Val, &TotalRead);
        RETURN_ON_FAIL(Result);

        Result = Ch.SetCursor(4, KMemChannel::eBeforeCurrent);
        RETURN_ON_FAIL(Result);


        if (TotalRead != sizeof(ULONG))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (Val != i)
        {
            return STATUS_UNSUCCESSFUL;
        }

        // Do a seek after the current position

        if (i == 0)
        {
            break;
        }
    }



    return STATUS_SUCCESS;
}


// Test 3
//
// Write to one channel, read this and write into another channel
// read that back and make sure all the data is intact
//

NTSTATUS Test3_b(
    ULONG InitSize,
    ULONG MinGrowBy,
    ULONG MaxGrowBy,
    ULONG InitSize2,
    ULONG MinGrowBy2,
    ULONG MaxGrowBy2,
    ULONG Limit
    )
{
    KMemChannel Ch1(KtlSystem::GlobalNonPagedAllocator(), InitSize, MinGrowBy, MaxGrowBy);
    KMemChannel Ch2(KtlSystem::GlobalNonPagedAllocator(), InitSize2, MinGrowBy2, MaxGrowBy2);

    for (ULONG i = 0; i < Limit; i++)
    {
        Ch1.Write(sizeof(ULONG), &i);
    }

    Ch1.SetCursor(0, KMemChannel::eFromBegin);

    ULONG TotalSize = Ch1.Size();
    ULONG TotalRead = 0;
    while (1)
    {
        UCHAR Buffer[55];
        ULONG Total = 0;
        Ch1.Read(55, Buffer, &Total);
        if (Total == 0)
            break;
        TotalRead += Total;
        Ch2.Write(Total, Buffer);
    }

    if (TotalRead != TotalSize)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Ch2.ResetCursor();  // Back to the beginning

    for (ULONG i = 0; i < Limit; i++)
    {
        ULONG Check;
        ULONG Val;
        Ch2.Read(sizeof(ULONG), &Val, &Check);

        if (Check != sizeof(ULONG))
        {
            return STATUS_UNSUCCESSFUL;
        }

        if (Val != i)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }

    ULONG Sz = Ch2.Size();
    Ch2.Reset();
    Sz = Ch2.Size();

    return STATUS_SUCCESS;
}


NTSTATUS Test3()
{
    for (ULONG Limit = 32; Limit < 20000; Limit += 16)
    {
        ULONG InitSize   = BoundedRandNz(32, 255);
        ULONG MinGrowBy  = BoundedRandNz(32, 255);
        ULONG MaxGrowBy  = BoundedRandNz(256, 1024);
        ULONG InitSize2  = BoundedRandNz(55, 127);
        ULONG MinGrowBy2 = BoundedRandNz(55, 127);
        ULONG MaxGrowBy2 = BoundedRandNz(128, 1200);

        NTSTATUS Result = Test3_b(InitSize, MinGrowBy, MaxGrowBy, InitSize2, MinGrowBy2, MaxGrowBy2, Limit);
        if (!NT_SUCCESS(Result))
        {
            KTestPrintf("Failure in Test3()\n");
            return Result;
        }
    }

    return STATUS_SUCCESS;
}


NTSTATUS Test4_b(unsigned NUM_ELEMENTS, unsigned channelSize)
{
    //
    KMemChannel Ch1(KtlSystem::GlobalNonPagedAllocator(), channelSize);
    KMemChannel Ch2(KtlSystem::GlobalNonPagedAllocator(), channelSize);

    for (ULONG i = 0; i < NUM_ELEMENTS; i++)
    {
        Ch1.Write(sizeof(ULONG), &i);
    }

    Ch1.SetCursor(0, KMemChannel::eFromBegin);

    // Grab the blocks
    KArray<KMemRef> Ary(KtlSystem::GlobalNonPagedAllocator());

    ULONG NumBlocks = Ch1.GetBlockCount();
//    KTestPrintf("Ch1 has %d blocks\n", NumBlocks);

    for (ULONG i = 0; i < NumBlocks; i++)
    {
        KMemRef x;
        Ch1.GetBlock(i, x);
        Ary.Append(x);

//        KTestPrintf("Ch1 Block [%d] Address = %x  Size = %d  Param=%d\n", i, x._Address, x._Size, x._Param);
    }

    ULONG Ch1Size = Ch1.Size();

//    KTestPrintf("Channel 1 size is %d\n", Ch1Size);

    Ch1.DetachMemory();

    // Now we have everything in Ary

    Ch2.StartCapture();

    for (ULONG i = 0; i < Ary.Count(); i++)
    {
        Ch2.Capture(Ary[i]);
    }

    Ch2.EndCapture();
    Ch2.ResetCursor();

    NumBlocks = Ch2.GetBlockCount();
 //   KTestPrintf("Ch2 has %d blocks\n", NumBlocks);

    for (ULONG i = 0; i < NumBlocks; i++)
    {
        KMemRef x;
        Ch2.GetBlock(i, x);

   //     KTestPrintf("Ch2 Block [%d] Address = %x  Size = %d  Param=%d\n", i, x._Address, x._Size, x._Param);
    }


    for (ULONG i = 0; i < NUM_ELEMENTS; i++)
    {
        ULONG Check;
        ULONG Val;
        Ch2.Read(sizeof(ULONG), &Val, &Check);

        if (Check != sizeof(ULONG))
        {
                return STATUS_UNSUCCESSFUL;
        }

        if (Val != i)
        {
            return STATUS_UNSUCCESSFUL;
        }
    }


    // Check channel 2 blocks for total sizeness

    ULONG NumBlocksIn2 = Ch2.GetBlockCount();
    ULONG CheckSizeOn2 = 0;

    for (ULONG i = 0; i < NumBlocksIn2; i++)
    {
        KMemRef r;
        Ch2.GetBlock(i, r);
        CheckSizeOn2 += r._Param;
    }


    ULONG Sz = Ch2.Size();
    if (Sz != Ch1Size)
    {
        KTestPrintf("Channel2 size is not the same as Channel 1\n");
        return STATUS_UNSUCCESSFUL;
    }

    if (Sz != CheckSizeOn2)
    {
        KTestPrintf("Raw block get returns different size than Size()\n");
        return STATUS_UNSUCCESSFUL;
    }

    Ch2.Reset();
    Sz = Ch2.Size();

    return STATUS_SUCCESS;
}

NTSTATUS Test4()
{
    NTSTATUS Res;

    for (ULONG i = 1; i < 4000; i++)
    {
        ULONG ChSize = BoundedRandNz(256);

        Res = Test4_b(i, ChSize);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Failure\n");
            return Res;
        }
    }

    KTestPrintf("Test4() SUCCESS\n");
    return STATUS_SUCCESS;
}

// Seek to EOF and write test

NTSTATUS Test5()
{
    NTSTATUS Res;

    KTestPrintf("Begin Test5()\n");

    KMemChannel Ch(KtlSystem::GlobalNonPagedAllocator(), 64, 64, 64);

    for (ULONG i = 0; i < 16; i++)
    {
        Ch.Write(sizeof(ULONG), &i);
    }

    KTestPrintf("Size is %d\n", Ch.Size());

    // Seek back to beginning
    ULONG CursorPosition = 999999;

    ULONG Test = 1000;
    Res = Ch.SetCursor(0, KMemChannel::eFromBegin);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 0)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    Res = Ch.Read(sizeof(ULONG), &Test);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Test != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 4)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Now try to read the next element

    Test = 0;
    Res = Ch.Read(sizeof(ULONG), &Test);
    if (!NT_SUCCESS(Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Test != 1)
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 8)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    // Seek to end

    Res = Ch.SetCursor(0, KMemChannel::eBeforeEnd);
    if (!NT_SUCCESS(Res))
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 64)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    Test = 199;
    Res = Ch.Write(sizeof(ULONG), &Test);
    if (!NT_SUCCESS(Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 68)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    Res = Ch.SetCursor(4, KMemChannel::eBeforeEnd);
    if (!NT_SUCCESS(Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 64)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    Res = Ch.Read(sizeof(ULONG), &Test);
    if (!NT_SUCCESS(Test))
    {
        return STATUS_UNSUCCESSFUL;
    }

    if (Test != 199)
    {
        return STATUS_UNSUCCESSFUL;
    }

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 68)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    Ch.SetCursor(0, KMemChannel::eFromBegin);

    CursorPosition = Ch.GetCursor();

    if (CursorPosition != 0)
    {
        KTestPrintf("Cursor position is wrong\n");
        return STATUS_UNSUCCESSFUL;
    }

    for (ULONG i = 0; i < 17; i++)
    {
        ULONG Test1 = 0;
        Res = Ch.Read(sizeof(ULONG), &Test1);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Failed to read element %d\n", i);
        }
        else
        {
            KTestPrintf("Test = %d\n", Test1);
        }
    }

    // Try to read past end

    Test = 2000;
    Res = Ch.Read(sizeof(ULONG), &Test);
    if (Ch.Eof())
    {
        KTestPrintf("End of file\n");
    }

    return STATUS_SUCCESS;
}


// Comprehensive random seek & write and test
//
NTSTATUS Test6b(ULONG ChannelCfg, ULONG Limit)
{
    KTestPrintf("Start Test6b()\n");

    NTSTATUS Res;

    KArray<ULONG> Tracker(KtlSystem::GlobalNonPagedAllocator());

    KMemChannel Ch(KtlSystem::GlobalNonPagedAllocator(), ChannelCfg, ChannelCfg, ChannelCfg);

    for (ULONG i = 0; i < Limit; i++)
    {
        Ch.Write(sizeof(ULONG), &i);
        Tracker.Append(i);
    }

    // Now, randomly seek & populate

    ULONG Iterations = 50000;
    while (Iterations--)
    {
        ULONG ArrayPos = BoundedRandNz(Limit-1);

        ULONG SeekPos = ArrayPos * sizeof(ULONG);

        Res = Ch.SetCursor(SeekPos, KMemChannel::eFromBegin);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Failed to seek\n");
            return STATUS_UNSUCCESSFUL;
        }

        // Write a random value
        ULONG Val = BoundedRandNz(10000);

        Res = Ch.Write(sizeof(ULONG), &Val);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Failed to write\n");
            return STATUS_UNSUCCESSFUL;
        }

        // Record it
        Tracker[SeekPos/4] = Val;
        //KTestPrintf("Writing [array=%d/Seekpos=%d]  Value = %d\n", ArrayPos, SeekPos, Val);
    }

    KTestPrintf("Verifying...\n");

    // Now verify

    for (ULONG i = 0; i < Limit; i++)
    {
        ULONG SeekPos = i * sizeof(ULONG);

        Res = Ch.SetCursor(SeekPos, KMemChannel::eFromBegin);
        if (!NT_SUCCESS(Res))
        {
            KTestPrintf("Failed to seek\n");
            return STATUS_UNSUCCESSFUL;
        }

        ULONG Val = 0xFFFFFFFF;
        Ch.Read(sizeof(ULONG), &Val);

        //KTestPrintf("Value retrieved at [array=%d / seekpost=%d] is %d and tracker as %d\n", i, SeekPos, Val, Tracker[i]);

        if (Val != Tracker[i])
        {
            KTestPrintf("Failed to match at iteration %d\n", i);
            return STATUS_UNSUCCESSFUL;
        }
    }

    KTestPrintf("Test6b() completed successfully\n");

    return STATUS_SUCCESS;
}

NTSTATUS Test6()
{
    KTestPrintf("Test6()\n");

    for (ULONG Cfg = 32 ; Cfg < 4096; Cfg += 127)
    {
        for (ULONG Limit = 32; Limit < 10000; Limit += 992)
        {
            KTestPrintf("Iteration cfg = %d Limit = %d\n", Cfg, Limit);
            NTSTATUS Result = Test6b(Cfg, Limit);
            if (!NT_SUCCESS(Result))
            {
                return Result;
            }
        }
    }

    return STATUS_SUCCESS;
}


// This test introduces a leak on purpose.
//
NTSTATUS Test7()
{
    KMemChannel Ch(KtlSystem::GlobalNonPagedAllocator());

    ULONG Val1 = 33;

    Ch.Write(sizeof(ULONG), &Val1);

 //   Ch.DetachOnDestruct();
    return STATUS_SUCCESS;
}

NTSTATUS Test8()
{
    // Validate basic latching and error re-issue
    // Validate Reset() after latch
    // Validate FullRead required behavior
    {
        KMemChannel     ch(KtlSystem::GlobalNonPagedAllocator(), 100, 0, 0);
        char            data[100];

        NTSTATUS    status = ch.Write(sizeof(data), &data[0]);
        KInvariant(NT_SUCCESS(status));

        status = ch.Write(1, &data[0]);
        KInvariant(status == STATUS_INSUFFICIENT_RESOURCES);

        // Prove status is latched
        KInvariant(ch.Status() == STATUS_INSUFFICIENT_RESOURCES);
        status = ch.Write(1, &data[0]);
        KInvariant(status == STATUS_INSUFFICIENT_RESOURCES);
        status = ch.Read(1, &data[0]);
        KInvariant(status == STATUS_INSUFFICIENT_RESOURCES);

        // Prove Reset() unlatches
        ch.Reset();
        KInvariant(ch.Status() == STATUS_SUCCESS);

        // Prove out of data (non error) behavior
        status = ch.Read(1, &data[0]);
        KInvariant(status == STATUS_END_OF_MEDIA);

        // Prove fixed size read behavior - including latching on short read
        ch.SetReadFullRequired(TRUE);
        status = ch.Read(1, &data[0]);
        KInvariant(status == K_STATUS_COULD_NOT_READ_STREAM);
        KInvariant(ch.Status() == K_STATUS_COULD_NOT_READ_STREAM);
        status = ch.Read(1, &data[0]);
        KInvariant(ch.Status() == K_STATUS_COULD_NOT_READ_STREAM);
        status = ch.Write(1, &data[0]);
        KInvariant(ch.Status() == K_STATUS_COULD_NOT_READ_STREAM);

        // Cover partial read-path failure with ReadFullRequired 
        ch.Reset();
        KInvariant(ch.Status() == STATUS_SUCCESS);
        ch.SetReadFullRequired(TRUE);
        status = ch.Write(50, &data[0]);
        KInvariant(NT_SUCCESS(status));
        status = ch.Read(51, &data[0]);
        KInvariant(status == K_STATUS_COULD_NOT_READ_STREAM);
        status = ch.Read(1, &data[0]);
        KInvariant(status == K_STATUS_COULD_NOT_READ_STREAM);
        KInvariant(ch.Status() == K_STATUS_COULD_NOT_READ_STREAM);
        status = ch.Write(1, &data[0]);
        KInvariant(ch.Status() == K_STATUS_COULD_NOT_READ_STREAM);
    }

    return STATUS_SUCCESS;
}

NTSTATUS TestSequence()
{
    NTSTATUS        Result;
    LONGLONG        initialAllocations = KAllocatorSupport::gs_AllocsRemaining;

    KTestPrintf("Starting TestSequence...\n");

    KTestPrintf("Test1()\n");

    Result = Test1();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test1()\n");
        return Result;
    }

    KTestPrintf("Test2()\n");

    Result = Test2();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test2()\n");
        return Result;
    }
    KTestPrintf("Test3()\n");

    Result = Test3();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test3()\n");
        return Result;
    }

    KTestPrintf("Test4()\n");

    Result = Test4();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test4()\n");
        return Result;
    }

    KTestPrintf("Test5()\n");
    Result = Test5();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test5()\n");
        return Result;
    }

    KTestPrintf("Test6()\n");
    Result = Test6();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test6()\n");
        return Result;
    }

    KTestPrintf("Test7()\n");
    Result = Test7();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test7()\n");
        return Result;
    }

    KTestPrintf("Test8()\n");
    Result = Test8();
    if (!NT_SUCCESS(Result))
    {
        KTestPrintf("FAIL: Test8()\n");
        return Result;
    }

    KTestPrintf("Leak checks\n");

    if (KAllocatorSupport::gs_AllocsRemaining != initialAllocations)
    {
        KTestPrintf("Outstanding allocations\n");
        return STATUS_UNSUCCESSFUL;
    }
    else
    {
        KTestPrintf("No leaks.\n");
    }

    KTestPrintf("Ending TestSequence...\n");

    return STATUS_SUCCESS;
}


NTSTATUS
KMemChannelTest(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

#if defined(PLATFORM_UNIX)
    NTSTATUS status = KtlTraceRegister();
    if (! NT_SUCCESS(status))
    {
        KTestPrintf("Failed to KtlTraceRegister\n");
        return(status);
    }
#endif
    
    KTestPrintf("KMemChannelTest: STARTED\n");

    EventRegisterMicrosoft_Windows_KTL();
    KTestPrintf("Starting KMemChannelTest test");

    KFatal(NT_SUCCESS(KtlSystem::Initialize()));

    NTSTATUS Result = TestSequence();

    KtlSystem::Shutdown();

    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KMemChannelTest: COMPLETED\n");

#if defined(PLATFORM_UNIX)
    KtlTraceUnregister();
#endif  
    
    return Result;
}

#if CONSOLE_TEST
int
#if !defined(PLATFORM_UNIX)
main(int argc, WCHAR* args[])
{
#else
main(int argc, char* cargs[])
{
    CONVERT_TO_ARGS(argc, cargs)
#endif
    return KMemChannelTest(argc, args);
}
#endif
