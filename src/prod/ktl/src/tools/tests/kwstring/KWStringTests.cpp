/*++

Copyright (c) Microsoft Corporation

Module Name:

    KWStringTests.cpp

Abstract:

    This file contains test case implementations.

    To add a new unit test case:
    1. Declare your test case routine in KWStringTests.h.
    2. Add an entry to the gs_KuTestCases table in KWStringTestCases.cpp.
    3. Implement your test case routine. The implementation can be in
    this file or any other file.

--*/
#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <ktl.h>
#include <ktrace.h>
#include "KWStringTests.h"
#include <CommandLineParser.h>

#if KTL_USER_MODE
#include <ktlevents.um.h>
#else
#include <ktlevents.km.h>
#endif

#pragma warning(disable:4840)
NTSTATUS
KWStringTestX(
    int argc, WCHAR* args[]
    )
{
    UNREFERENCED_PARAMETER(argc);
    UNREFERENCED_PARAMETER(args);

    NTSTATUS status = STATUS_SUCCESS;
    static const GUID guid = { 0x732d51f2, 0x330c, 0x4e0c, { 0x9c, 0x89, 0xb5, 0x67, 0xc1, 0xa9, 0x6c, 0xc0 } };
    KWString string(KtlSystem::GlobalNonPagedAllocator());
    LONG r;
    KWString anotherString(KtlSystem::GlobalNonPagedAllocator());
    UNICODE_STRING unicodeString;
    GUID anotherGuid;

    KTestPrintf("Starting KWStringTest test\n");

    //
    // Verify that the default constructor creates a valid string.
    //

    r = string.CompareTo(L"");

    if (r) {
        KTestPrintf("Invalid initial string\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test the copy-constructor.
    //

    string = L"foo";

    {
        KWString testString(string);
        r = testString.CompareTo(L"foo");
        if (r) {
#if defined(PLATFORM_UNIX)
            KTestPrintf("Copy constructor yields wrong string: %s\n", Utf16To8((WCHAR*)testString).c_str());
#else
            KTestPrintf("Copy constructor yields wrong string: %ws\n", (WCHAR*) testString);
#endif
        }
    }

    if (r) {
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test the UNICODE_STRING constructor.
    //

    {
        UNICODE_STRING unicodeString1;
        RtlInitUnicodeString(&unicodeString1, L"foo");
        KWString testString(KtlSystem::GlobalNonPagedAllocator(), unicodeString1);
        r = testString.CompareTo(L"foo");
        if (r) {
#if defined(PLATFORM_UNIX)
            KTestPrintf("Unicode string constructor yields wrong string: %s\n", Utf16To8((WCHAR*)testString).c_str());
#else
            KTestPrintf("Unicode string constructor yields wrong string: %ws\n", (WCHAR*) testString);
#endif
        }
    }

    if (r) {
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test PWSTR constructor.
    //

    {
        KWString testString(KtlSystem::GlobalNonPagedAllocator(), L"foo");
        r = testString.CompareTo(L"foo");
        if (r) {
#if defined(PLATFORM_UNIX)
            KTestPrintf("pwsz constructor yields wrong string: %s\n", Utf16To8((WCHAR*)testString).c_str());
#else
            KTestPrintf("pwsz constructor yields wrong string: %ws\n", (WCHAR*) testString);
#endif
        }
    }

    if (r) {
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test GUID constructor.
    //

    string = L"{732D51F2-330C-4e0c-9C89-B567C1A96CC0}";


    {
        KWString testString(KtlSystem::GlobalNonPagedAllocator(), guid);
        r = testString.CompareTo(string, TRUE);
        if (r) {
#if defined(PLATFORM_UNIX)
            KTestPrintf("guid constructor yields wrong string: %s\n", Utf16To8((WCHAR*)testString).c_str());
#else
            KTestPrintf("guid constructor yields wrong string: %ws\n", (WCHAR*) testString);
#endif
        }
    }

    if (r) {
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operator= KWString.
    //

    anotherString = string;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator= KWString yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator= KWString yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation= UNICODE_STRING.
    //

    RtlInitUnicodeString(&unicodeString, string);
    anotherString = unicodeString;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator= UNICODE_STRING yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator= UNICODE_STRING yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation= WCHAR*.
    //

    anotherString = ((WCHAR*) string);
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator= WCHAR* yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator= WCHAR* yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation= GUID.
    //

    anotherString = guid;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator= GUID yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator= GUID yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation+= KWString.
    //

    string = L"";
    string += anotherString;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator+= KWString yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator+= KWString yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation+= UNICODE_STRING.
    //

    RtlInitUnicodeString(&unicodeString, anotherString);
    string = L"";
    string += unicodeString;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator+= UNICODE_STRING yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator+= UNICODE_STRING yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation+= WCHAR*.
    //

    string = L"";
    string += ((WCHAR*) anotherString);
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator+= WCHAR* yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator+= WCHAR* yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test operation+= GUID.
    //

    string = L"";
    string += guid;
    r = anotherString.CompareTo(string, TRUE);
    if (r) {
#if defined(PLATFORM_UNIX)
        KTestPrintf("operator+= GUID yields wrong string: %s\n", Utf16To8((WCHAR*)anotherString).c_str());
#else
        KTestPrintf("operator+= GUID yields wrong string: %ws\n", (WCHAR*) anotherString);
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Conversion operators are all checked above.  CompareTo KWString verified above.
    //

    //
    // Test CompareTo UNICODE_STRING.
    //

    r = string.CompareTo(unicodeString, TRUE);
    if (r) {
        KTestPrintf("CompareTo UNICODE_STRING does not work.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test CompareTo WCHAR*
    //

    r = string.CompareTo((WCHAR*) anotherString);
    if (r) {
        KTestPrintf("CompareTo WCHAR* does not work.\n");
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

    //
    // Test ExtractGuidSuffix.
    //

    string = guid;
    string += guid;
    string += guid;

    status = string.ExtractGuidSuffix(anotherGuid);

    if (!NT_SUCCESS(status)) {
        KTestPrintf("Extract Guid Suffix failed with %x\n", status);
        goto Finish;
    }

    if (anotherGuid != guid) {
#if !defined(PLATFORM_UNIX)
        KTestPrintf("Extract Guid Suffix returned the wrong guid: %ws\n", KWString(KtlSystem::GlobalNonPagedAllocator(), anotherGuid));
#else
        KTestPrintf("Extract Guid Suffix returned the wrong guid: %s\n", Utf16To8((WCHAR*)KWString(KtlSystem::GlobalNonPagedAllocator(), anotherGuid)).c_str());
#endif
        status = STATUS_INVALID_PARAMETER;
        goto Finish;
    }

Finish:

    return status;
}
#pragma warning(default:4840)


NTSTATUS
KWStringNewTests()
{
    KWString Str1(KtlSystem::GlobalNonPagedAllocator(), L"ABC");
    KWString Str2(KtlSystem::GlobalNonPagedAllocator(), L"ABC");
    KWString Str1_b(KtlSystem::GlobalNonPagedAllocator(), L"ABCD");
    KWString Str3(KtlSystem::GlobalNonPagedAllocator(), L"BCD");
    KWString Str4(KtlSystem::GlobalNonPagedAllocator(), L"");
    UNICODE_STRING StrX;
    KWString StrY(KtlSystem::GlobalNonPagedAllocator());

    RtlZeroMemory(&StrX, sizeof(UNICODE_STRING));

    LONG Res = Str1.Compare(Str2);

    if (Res != 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str1.Compare(Str1_b);

    if (Res >= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str1_b.Compare(Str1);

    if (Res <= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str1.Compare(Str3);
    if (Res >= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str3.Compare(Str1);
    if (Res <= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str3.Compare(Str4);
    if (Res <= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res = Str4.Compare(Str3);
    if (Res >= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res= Str4.Compare(StrX);
    if (Res <= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }

    Res= StrY.Compare(StrX);
    if (Res <= 0)
    {
        return STATUS_UNSUCCESSFUL;
    }


    return STATUS_SUCCESS;
}

NTSTATUS
TestKArray()
{
    ULONGLONG startingAllocs = KAllocatorSupport::gs_AllocsRemaining;
    {
        KWString                    testString1(KtlSystem::GlobalNonPagedAllocator(), L"Test String 1");
        KWString                    testString2(KtlSystem::GlobalNonPagedAllocator(), L"Test String 2");
        KArray<KWString>            testArray2(KtlSystem::GlobalNonPagedAllocator(), 1);
        KArray<KWString>            testArray1(KtlSystem::GlobalNonPagedAllocator(), 1);
        KArray<KArray<KWString>>    nestedArray1(KtlSystem::GlobalNonPagedAllocator(), 1);
        KArray<KArray<KWString>>    nestedArray2(KtlSystem::GlobalNonPagedAllocator(), 1);
        ULONG                       initialNestedArray1Count = nestedArray1.Max();
        ULONG                       initialNestedArray2Count = nestedArray2.Max();

        ULONGLONG startingAllocs1 = KAllocatorSupport::gs_AllocsRemaining;

        // Prove basic append works
        for (ULONG ix = 0; ix < 100; ix++)
        {
            testArray1.Append(testString1);
            if (!NT_SUCCESS(testArray1.Status()))
            {
                return testArray1.Status();
            }

            if (!NT_SUCCESS(testArray1[ix].Status()))
            {
                return testArray1[ix].Status();
            }
        }

        for (ULONG ix = 0; ix < testArray1.Count(); ix++)
        {
            KFatal(testArray1[ix].Compare(testString1) == 0);
        }

        // Prove basic cloning of KArray<KWString>
        KFatal(NT_SUCCESS(testArray2.Append(testString2)));

        testArray2 = testArray1;
        KFatal(NT_SUCCESS(testArray2.Status()));
        KFatal(testArray1.Count() == testArray2.Count());
        for (ULONG ix = 0; ix < testArray1.Count(); ix++)
        {
            KFatal(testArray1[ix].Compare(testString1) == 0);
        }
        for (ULONG ix = 0; ix < testArray2.Count(); ix++)
        {
            KFatal(testArray2[ix].Compare(testString1) == 0);
        }

        // Prove basic Clear() behavior
        testArray1.Clear();
        testArray2.Clear();

        // Prove basic nested (KArray<KArray<KWString>>) behavior
        for (ULONG x = 0; x < 100; x++)
        {
            KFatal(NT_SUCCESS(nestedArray1.Append(KArray<KWString>(KtlSystem::GlobalNonPagedAllocator()))));
            for (ULONG y = 0; y < 100; y++)
            {
                nestedArray1[x].Append(testString2);
                KFatal(NT_SUCCESS(nestedArray1[x][y].Status()));
            }
        }

        for (ULONG x = 0; x < 100; x++)
        {
            for (ULONG y = 0; y < 100; y++)
            {
                KFatal(nestedArray1[x][y].Compare(testString2) == 0);
            }
        }

        // Prove basic cloning of nested KArray<KWString>
        KFatal(NT_SUCCESS(nestedArray2.Append(KArray<KWString>(KtlSystem::GlobalNonPagedAllocator()))));
        KFatal(NT_SUCCESS(nestedArray2[0].Append(testString1)));

        nestedArray2 = nestedArray1;
        KFatal(NT_SUCCESS(nestedArray2.Status()));
        KFatal(nestedArray2.Count() == nestedArray2.Count());

        for (ULONG x = 0; x < 100; x++)
        {
            for (ULONG y = 0; y < 100; y++)
            {
                KFatal(nestedArray1[x][y].Compare(testString2) == 0);
                KFatal(nestedArray2[x][y].Compare(testString2) == 0);
            }
        }

        // Prove SetCount
        KFatal(nestedArray1.Count() == 100);
        KFatal(nestedArray1.SetCount(50));
        KFatal(nestedArray1.Count() == 50);

        // Prove basic Clear() and SetCount() behavior
        nestedArray1.Clear();
        nestedArray2.Clear();

        startingAllocs1 += nestedArray1.Max() - initialNestedArray1Count;
        startingAllocs1 += nestedArray2.Max() - initialNestedArray2Count;

        KFatal((KAllocatorSupport::gs_AllocsRemaining - startingAllocs1) == 0);

        // Prove Reserve
        startingAllocs1 = KAllocatorSupport::gs_AllocsRemaining;
        {
            KArray<KWString>            testArray3(KtlSystem::GlobalNonPagedAllocator(), 5);

            KFatal(testArray3.Max() == 5);
            KFatal(testArray3.Count() == 0);
            KFatal(NT_SUCCESS(testArray3.Append(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 3"))));
            KFatal(testArray3.Max() == 5);
            KFatal(testArray3.Count() == 1);
            KFatal(NT_SUCCESS(testArray3.Append(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 4"))));
            KFatal(testArray3.Max() == 5);
            KFatal(testArray3.Count() == 2);
            KFatal(NT_SUCCESS(testArray3.Append(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 5"))));
            KFatal(testArray3.Max() == 5);
            KFatal(testArray3.Count() == 3);

            KFatal(NT_SUCCESS(testArray3.Reserve(50)));
            KFatal(testArray3.Max() == 50);
            KFatal(testArray3.Count() == 3);

            KFatal(testArray3[0].Compare(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 3")) == 0);
            KFatal(testArray3[1].Compare(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 4")) == 0);
            KFatal(testArray3[2].Compare(KWString(KtlSystem::GlobalNonPagedAllocator(), L"Test String 5")) == 0);
        }
        KFatal((KAllocatorSupport::gs_AllocsRemaining - startingAllocs1) == 0);

    }
    KFatal((KAllocatorSupport::gs_AllocsRemaining - startingAllocs) == 0);

    return STATUS_SUCCESS;
}

NTSTATUS
TestKHashtable()
{
    ULONGLONG startingAllocs = KAllocatorSupport::gs_AllocsRemaining;
    {
        KHashTable<KWString, ULONG>    testTable1(20, K_DefaultHashFunction, KtlSystem::GlobalNonPagedAllocator());

        ULONGLONG startingAllocs1 = KAllocatorSupport::gs_AllocsRemaining;

        // Prove basic add works - NOTE: Compile will break if KHashTable<KWString, ...> is not supported
        ULONG       data = 1;
#if !defined(PLATFORM_UNIX)
        KFatal(NT_SUCCESS(testTable1.Put(KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr1"), data)));
        KFatal(NT_SUCCESS(testTable1.Put(KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr2"), data)));
        KFatal(NT_SUCCESS(testTable1.Put(KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr3"), data)));
#else
        {
            KWString testStr1 = KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr1");
            KWString testStr2 = KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr2");
            KWString testStr3 = KWString(KtlSystem::GlobalNonPagedAllocator(), L"TestStr3");

            KFatal(NT_SUCCESS(testTable1.Put(testStr1, data)));
            KFatal(NT_SUCCESS(testTable1.Put(testStr2, data)));
            KFatal(NT_SUCCESS(testTable1.Put(testStr3, data)));
        }
#endif

        testTable1.Clear();
        KFatal((KAllocatorSupport::gs_AllocsRemaining - startingAllocs1) == 0);
    }
    KFatal((KAllocatorSupport::gs_AllocsRemaining - startingAllocs) == 0);

    return STATUS_SUCCESS;
}

NTSTATUS
TestSequence(
    int argc, WCHAR* args[]
    )
{
    NTSTATUS Result;

    Result = KWStringNewTests();

    if (!NT_SUCCESS(Result)) {
        return Result;
    }

    Result = KWStringTestX(argc, args);

    if (!NT_SUCCESS(Result)) {
        return Result;
    }

    Result = TestKArray();
    if (!NT_SUCCESS(Result)) {
        return Result;
    }

    Result = TestKHashtable();
    if (!NT_SUCCESS(Result)) {
        return Result;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
KWStringTest(
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

    KTestPrintf("KWStringTest: STARTED\n");
    
    EventRegisterMicrosoft_Windows_KTL();

    status = KtlSystem::Initialize();

    status = TestSequence(argc, args);

    KtlSystem::Shutdown();
    EventUnregisterMicrosoft_Windows_KTL();

    KTestPrintf("KWStringTest: COMPLETED\n");

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

    return RtlNtStatusToDosError(KWStringTest(0, NULL));
}
#endif

